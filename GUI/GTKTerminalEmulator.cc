#include "GUI/TerminalEmulator"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pty.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "GUI/TerminalEmulator"

namespace GUI {

int start_child_shell(void) {
	pid_t child_PID;
	int master_FD;

	child_PID = forkpty(&master_FD, NULL, NULL, NULL); /* FIXME name, termios, winsize */
	if(child_PID == 0) { /* we are the child. */
		execlp("/bin/bash", "/bin/bash", "-l", NULL);
		_exit(1);
	}
	return master_FD; /* TODO return child_PID? */
}

struct TerminalEmulator {
	struct REPL* REPL;
	/* design limitation: no escape sequence can be longer than that. */
	unsigned char raw_buffer[1024]; /* usually empty except when there is an escape sequence. */
	unsigned int raw_buffer_fill_count;

	/*char escape_buffer[1024];
	unsigned int escape_buffer_fill_count;*/
	int B_bold;
	int master_FD;
	int B_shifted_in;
};

static inline unsigned int min(unsigned int a, unsigned int b) {
	return (a < b) ? a : b;
}

void TerminalEmulator_handle_escape_left(struct TerminalEmulator* emulator, const unsigned char* escape, unsigned int count) {
	char command;
	char values[200];
	int value;
	char* x_values;
	char* terminator;
	assert(count > 0);
	command = escape[count - 1];

	switch(command) {
	case 'K': /* CLREOL, ignore. */
		break;
	case 'H': /* gotoxy, ignore. */
	case 'f':
		break;
	case 'J': /* clear screen, ignore. */
		break;
	case 'm':
		memcpy(values, escape, min(count, 199));
		values[min(count, 199)] = 0;
		values[strlen(values) - 1] = ';';
		x_values = values;
		emulator->B_bold = 0;
		/*buffer_set_next_attribute(buffer, ATTRIBUTE_FOREGROUND_COLOR, "0");*/
		while((terminator = strchr(x_values, ';'))) {
			*terminator = 0;
			if(sscanf(x_values, "%d", &value) == 1) {
				/* TODO handle boldness */
				/*printf("M VALUE %d\n", value);*/
				if(value >= 30 && value < 38) {
					/*buffer_set_next_attribute(buffer, ATTRIBUTE_FOREGROUND_COLOR, 

							(value == 30) ? (emulator->B_bold ? "8" : "0") : 
							(value == 31) ? (emulator->B_bold ? "9" : "1") : 
							(value == 32) ? (emulator->B_bold ? "10" : "2") : 
							(value == 33) ? (emulator->B_bold ? "11" : "3") : 
							(value == 34) ? (emulator->B_bold ? "12" : "4") : 
							(value == 35) ? (emulator->B_bold ? "13" : "5") : 
							(value == 36) ? (emulator->B_bold ? "14" : "6") : 
							(value == 37) ? (emulator->B_bold ? "15" : "7") : "0");*/
				} else if(value == 1) {
					emulator->B_bold = 1;
				} /* FIXME
					02 faint
					03 standout
					04 underline
					05 blink
					07 reverse video
					08 invisible
					22 normal
					23 no-standout
					24 no-underline
					25 no-blink
					27 no-reverse
				*/
			}
			x_values = terminator + 1;
		}

		break;
	case 'A':
	case 'B':
	case 'C':
	case 'D':
		/* ignore app cursor control. */
		break;
	case 's': /* save current position */
		break;
	case 'u': /* restore current position */
		break;
#if 1
	case 'h': /* ESC[?7h wrap at end of line. */
		break;
	case 'r': /* ??? */
		break;
	case 'l': /* ESC[4l REPLACE */
		break;
	case 'd': /* ??? */
		break;
	case 'G': /* ??? */
		break;
	case 'X': /* ??? */
		break;
#endif
	default:
		fprintf(stderr, "warning: ignored unknown escape sequence '[%c'\n", command);
	}
}

/* ... except ESC and '\n' and SI */
static void TerminalEmulator_handle_control_character(struct TerminalEmulator* emulator, unsigned char item) {
	/*unsigned int caret_position = buffer_end(buffer);*/
	int raw[2] = {0};
	if(item == '\a') { /* beep */
		gdk_beep(); /* TODO on/off */
	} else if(item == 7) { /* CLREOL? */
	} else if(item == 13) { /* back to beginning of line and initiate overtype. */
	} else if(item == 9) { /* TAB */
		/* FIXME */
		raw[0] = 9;
		/*buffer_insert(emulator->buffer, buffer_end(emulator->buffer), raw, 1);*/
	} else if(item == 8) {
#if 0
			if(caret_position > 0) {
				int count = 0;
				while(caret_position > 0 && count < 6) {
					--caret_position;
					++count;
					if(UTF8_is_first(buffer_item_at(buffer, caret_position)))
						break;
				}
				/* FIXME also delete composites */
				buffer_delete(buffer, caret_position, count);
			}
#endif
	} else if(item == 14) {
		/* ??? */
		printf("SHIFT OUT\n");
	} else if(item == 15) {
		printf("SHIFT IN\n");
		emulator->B_shifted_in = TRUE;
	} else
		fprintf(stderr, "warning: ignored control character with code %d.\n", item);
}

unsigned int TerminalEmulator_maybe_handle_escape(struct TerminalEmulator* emulator, const unsigned char* raw_buffer, unsigned int count) {
	int i;
	unsigned char item;
	unsigned char* end;
	assert(count > 0);
	if(raw_buffer[0] != 27) {
		TerminalEmulator_handle_control_character(emulator, raw_buffer[0]);
		return 1;
	}
	if(count < 2) {
		return 0;
	}
	if(!(raw_buffer[1] == '[' || raw_buffer[1] == ']'))
		return 2;

	if(raw_buffer[1] == '[') {
		for(i = 2; i < count; ++i) {
			item = raw_buffer[i];
			if((item >= 'A' && item <= 'Z') || (item >= 'a' && item <= 'z')) {
				/* done. */
				TerminalEmulator_handle_escape_left(emulator, &raw_buffer[2], i - 2 + 1);
				return i + 1;
			}
		}
	} else if(raw_buffer[1] == ']') {
		end = (unsigned char*) memchr(raw_buffer, 7, count);
		if(end) {
			return end - raw_buffer + 1;
		}
	}
	return 0;
}

void TerminalEmulator_read_data(struct TerminalEmulator* emulator, int FD) {
	int count = read(FD, emulator->raw_buffer + emulator->raw_buffer_fill_count, sizeof(emulator->raw_buffer) - emulator->raw_buffer_fill_count);
	if(count == -1) {
		if(errno == EINTR) {
			/* FIXME */
			return;
		} else
			exit(0); /* done, Ctrl-D. FIXME add option to keep terminal open. */
	}
	/* FIXME if count == -1? */
	unsigned int raw_count;
	unsigned int text_count;
	unsigned int escape_count;
	unsigned int i;
	unsigned char* raw;
	unsigned char* escape;
	if(count > 0) {
		emulator->raw_buffer_fill_count += count;

		raw = emulator->raw_buffer;
		raw_count = emulator->raw_buffer_fill_count;

		do {
			/* FIXME other control characters, too. */
			escape = raw;
			for(i = 0; i < raw_count; ++i, ++escape)
				if(raw[i] == 27 || raw[i] == 8 || (raw[i] < 32 && raw[i] != '\n'))
					break;
			if(i >= raw_count)
				escape = NULL;
			/* = escape = memchr(raw, 27, raw_count); || ..8 */
			if(escape) {
				/*raw = escape;*/ /*raw += (escape - raw);*/
				text_count = (escape - raw);
			} else {
				text_count = raw_count;
			}

			if(emulator->B_shifted_in) {
				/* FIXME */
				memset(raw, '?', text_count);
				/*buffer_insert(emulator->buffer, buffer_end(emulator->buffer), raw, text_count);*/
			} else {
				/*buffer_insert(emulator->buffer, buffer_end(emulator->buffer), raw, text_count);*/
				/* FIXME */
			}
			raw_count -= text_count;
			/*raw += text_count;*/
			memcpy(raw, raw + text_count, raw_count);
			if(escape) {
				escape_count = TerminalEmulator_maybe_handle_escape(emulator, raw, raw_count);
				if(escape_count) {
					assert(raw_count >= escape_count);
					raw_count -= escape_count;
					memcpy(raw, raw + escape_count, raw_count);
				} else { /* incomplete escape sequence: keep it */
					break;
				}
			}
		} while(escape != NULL && raw_count > 0);
		emulator->raw_buffer_fill_count = raw_count;

	} /* else ??? */
}

static gboolean handle_terminal_IO(GIOChannel *source, GIOCondition condition, gpointer user_data) {
	struct TerminalEmulator* emulator = (struct TerminalEmulator*) user_data;
	int FD = g_io_channel_unix_get_fd(source);
	TerminalEmulator_read_data(emulator, FD);
	return TRUE;
}

void TerminalEmulator_send(struct TerminalEmulator* emulator, const unsigned char* text, unsigned int count) {
	int written_count;
	while(count > 0) {
		written_count = write(emulator->master_FD, text, count);
		if(written_count == -1) {
			/* FIXME error handling */
			return;
		}
		count -= written_count;
		text += written_count;
	}
}

void TerminalEmulator_init(struct TerminalEmulator* emulator, struct REPL* REPL) {
	emulator->REPL = REPL;
	int master_FD = start_child_shell();
	emulator->master_FD = master_FD;
	GIOChannel* master_channel = g_io_channel_unix_new(master_FD);
	g_io_add_watch(master_channel, (GIOCondition) (G_IO_IN|G_IO_HUP), handle_terminal_IO, emulator);
}

struct TerminalEmulator* TerminalEmulator_new(struct REPL* REPL) {
	struct TerminalEmulator* result;
	result = (struct TerminalEmulator*) calloc(1, sizeof(struct TerminalEmulator));
	TerminalEmulator_init(result, REPL);
	return result;
}

}; /* end terminalemulator */
