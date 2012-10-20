/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include <string.h>
#include <string>
#include <assert.h>
#include <sstream>
#include "Scanners/Scanner"
#include "Values/Values"
#include "Values/Symbols"
#include "Evaluators/Builtins"
#include "FFIs/Allocators"

// we assume that if fgetc() returns EOF, we can call it again and it will return EOF again. Hence FGETC.

/* parse_token() is the main function */

namespace Scanners {
using namespace Values;
using namespace Evaluators;

static inline void UNGETC(int input, FILE* input_file) {
	if(input != EOF)
		ungetc(input, input_file);
}
static inline int FGETC(FILE* input_file) {
	return feof(input_file) ? EOF : fgetc(input_file);
}
ParseException::ParseException(const char* s) throw() {
	message = GCx_strdup(s);
}
const char* ParseException::what() const throw() {
	return message; //message.c_str();
};

Scanner::Scanner(void) {
	input_file = NULL;
	position = 0;
	previous_position = 0;
	line_number = 0;
	backtracking_column_numbers[0] = 0;
	backtracking_column_numbers[1] = 0;
	backtracking_column_numbers[2] = 0;
	backtracking_column_numbers[3] = 0;
	B_beginning_of_line = true;
	brace_level = 0;
}
void Scanner::push(FILE* input_file, int line_number, const char* input_name) {
	this->input_file = input_file;
	this->line_number = line_number;
	this->input_name = GCx_strdup(input_name);
	this->position = 0;
	this->previous_position = 0;
	this->column_number = 0;
	this->B_beginning_of_line = true;
	this->open_indentations.clear();
	this->open_indentations.push_front(std::make_pair(0, 0));
}

void Scanner::pop(void) {
}

void Scanner::raise_error(const std::string& expected_text, std::string got_text) {
	std::stringstream s;
	s << "expected " << expected_text << " but got " << got_text << " near position " << position << " in line " << line_number + 1;
	if(input_name)
		s << " in file \"" << input_name << "\"";
	//std::cerr << s.str() << std::endl;
	throw ParseException(s.str().c_str());
}

void Scanner::raise_error(const std::string& expected_text, int got_text) {
	std::string s;
	if(got_text == -1)
		s = "<EOF>";
	else if(got_text > 127)
		s = "<junk>";
	else
		s = (char) got_text;
	raise_error(expected_text, s);
}

bool Scanner::EOFP(void) const {
	return(input_value == Symbols::SlessEOFgreater);
}
void Scanner::ensure_end(void) {
	if(!EOFP())
		raise_error("<EOF>", str(input_value));
}
/* returns the PREVIOUS value */
NodeT Scanner::consume(NodeT expected_value) {
	NodeT previous_value;
	previous_value = input_value;
	if(expected_value && expected_value != input_value)
		raise_error(str(expected_value), str(input_value));
	previous_position = position;
	parse_token();
	//std::string v = str(input_value);
	//printf("ready: %s\n", v.c_str());
	return(previous_value);
}
void Scanner::parse_token(void) {
	// TODO make this simpler for S expressions.
	if(!injected_input_values.empty()) {
		input_value = injected_input_values.front();
		injected_input_values.pop_front();
		return;
	}
	parse_optional_whitespace();
	previous_position = position; /* make sure it's not ON the whitespace */
	if(!injected_input_values.empty()) {
		input_value = injected_input_values.front();
		injected_input_values.pop_front();
		return;
	}
	input_value = NULL;
	int input;
	input = increment_position(FGETC(input_file)); // this will possibly inject stuff
	if(!injected_input_values.empty()) {
		UNGETC(decrement_position(input), input_file); // keep char for later.
		input_value = injected_input_values.front();
		injected_input_values.pop_front();
		return;
	}
	switch(input) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		parse_number(input);
		break;
	case 0xE2: /* part of "≠", "∂", "∈" */
	case 0xC2:
		parse_unicode(input);
		break;
	case '\'':
		input_value = Symbols::Squote;
		break;
	case '\\':
		input_value = Symbols::Sbackslash;
		break;
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
		parse_structural(input);
		break;
	case '"':
		parse_string(input);
		break;
	case EOF:
		if(!deinject())
			input_value = Symbols::SlessEOFgreater;
		break;
	case '@': // TODO remove this for S expressions?
		parse_keyword(input);
		break;
	case '#':
		parse_special_coding(input);
		break;
	case '_':
		input_value = Symbols::Sunderline;
		break;
	default:
		parse_operator(input);
		break;
	}
}
void Scanner::parse_optional_whitespace(void) {
	int input;
	// skip whitespace...
	while(input = increment_position(FGETC(input_file)), input == ' ' || input == '\t' || input == '\n' || input == '\r' || input == '\f') {
	}
	UNGETC(decrement_position(input), input_file);
}
void Scanner::parse_number(int input) {
	std::stringstream matchtext;
	char oldInput;
	bool hadDot = false;
	while((input >= '0' && input <= '9') || input == '.') {
		if(input == '.') {
			if(hadDot)
				break;
			hadDot = true;
		}
		matchtext << ((char) input);
		oldInput = input;
		input = increment_position(FGETC(input_file));
		if(oldInput == '.') {
			if(input < '0' || input > '9') { // this wasn't part of the number it seems, so maybe it's supposed to be an operator: 2.size
				break;
			}
		}
			
	}
	if(input == 'e' || input == 'E') {
		matchtext << ((char) input);
		input = increment_position(FGETC(input_file));
		if(input == '+' || input == '-') {
			matchtext << ((char) input);                   
			input = increment_position(FGETC(input_file));
		}
		while((input >= '0' && input <= '9') /* || input == '.'*/) {
			matchtext << ((char) input);                   
			input = increment_position(FGETC(input_file));
		}
	}
	if(input != ' ' && input != '\t')
		UNGETC(decrement_position(input), input_file);
	input_value = symbolFromStr(matchtext.str().c_str());
	/* actual value will be provided by provide_dynamic_builtins */
}
/*
static Symbol* compose_unicode(int a, int b, int c) {
	std::stringstream sst;
	sst << (char) a << (char) b << (char) c;
	std::string v = sst.str();
	return symbolFromStr(v.c_str());
}
*/
static unsigned int UTF8_decode_first(unsigned char value) {
	if((value & 0xe0) == 0xc0)
		return 2;
	else if((value & 0xf0) == 0xe0)
		return 3;
	else if((value & 0xf8) == 0xf0)
		return 4;
	else if((value & 0xfc) == 0xf8)
		return 5;
	else
		return 1;
}
/*
!! U+2000..U+206F 	General Punctuation 	112 	0 BMP 	Common, Inherited
!! U+2070..U+209F 	Superscripts and Subscripts 	48 	0 BMP 	Latin, Common
!! U+20A0..U+20CF 	Currency Symbols 	48 	0 BMP 	Common
?? U+20D0..U+20FF 	Combining Diacritical Marks for Symbols 	48 	0 BMP 	Inherited
!! U+2100..U+214F 	Letterlike Symbols 	80 	0 BMP 	Latin, Greek, Common
?? U+2150..U+218F 	Number Forms 	64 	0 BMP 	Latin, Common
!! U+2190..U+21FF 	Arrows 	112 	0 BMP 	Common
!! U+2200..U+22FF 	Mathematical Operators 	256 	0 BMP 	Common
!! U+2300..U+23FF 	Miscellaneous Technical 	256 	0 BMP 	Common
!! U+2400..U+243F 	Control Pictures 	64 	0 BMP 	Common
!! U+2440..U+245F 	Optical Character Recognition 	32 	0 BMP 	Common
!! U+2460..U+24FF 	Enclosed Alphanumerics 	160 	0 BMP 	Common
!! U+2500..U+257F 	Box Drawing 	128 	0 BMP 	Common
!! U+2580..U+259F 	Block Elements 	32 	0 BMP 	Common
!! U+25A0..U+25FF 	Geometric Shapes 	96 	0 BMP 	Common
!! U+2600..U+26FF 	Miscellaneous Symbols 	256 	0 BMP 	Common
!! U+2700..U+27BF 	Dingbats 	192 	0 BMP 	Common
!! U+27C0..U+27EF 	Miscellaneous Mathematical Symbols-A 	48 	0 BMP 	Common
!! U+27F0..U+27FF 	Supplemental Arrows-A 	16 	0 BMP 	Common
?  U+2800..U+28FF 	Braille Patterns 	256 	0 BMP 	Braille
!! U+2900..U+297F 	Supplemental Arrows-B 	128 	0 BMP 	Common
!! U+2980..U+29FF 	Miscellaneous Mathematical Symbols-B 	128 	0 BMP 	Common
!! U+2A00..U+2AFF 	Supplemental Mathematical Operators 	256 	0 BMP 	Common
!! U+2B00..U+2BFF 	Miscellaneous Symbols and Arrows 	256 	0 BMP 	Common
NO U+2C00..U+2C5F 	Glagolitic 	96 	0 BMP 	Glagolitic
NO U+2C60..U+2C7F 	Latin Extended-C 	32 	0 BMP 	Latin
NO U+2C80..U+2CFF 	Coptic 	128 	0 BMP 	Coptic
NO U+2D00..U+2D2F 	Georgian Supplement 	48 	0 BMP 	Georgian
NO U+2D30..U+2D7F 	Tifinagh 	80 	0 BMP 	Tifinagh
NO U+2D80..U+2DDF 	Ethiopic Extended 	96 	0 BMP 	Ethiopic
NO U+2DE0..U+2DFF 	Cyrillic Extended-A 	32 	0 BMP 	Cyrillic
!! U+2E00..U+2E7F 	Supplemental Punctuation 	128 	0 BMP 	Common
!! U+2E80..U+2EFF 	CJK Radicals Supplement 	128 	0 BMP 	Han
!! U+2F00..U+2FDF 	Kangxi Radicals 	224 	0 BMP 	Han
!! U+2FF0..U+2FFF 	Ideographic Description Characters 	16 	0 BMP 	Common

So everything in U+2xxx except the following are standalone:
U+20D0..U+20FF which is 0xE2 0x83 0x90 .. 0xE2 0x83 0xBF (e)
U+2150..U+218F which is 0xE2 0x85 0x90 .. 0xE2 0x86 0x8F
U+2C00..U+2DFF which is 0xE2 0xB0 0x80 .. 0xE2 0xB7 0xBF (e)
*/
void Scanner::parse_unicode(int input) {
	/* What this does is identify several special unicode characters which are supposed to stand on their own, i.e. mathematical operators etc.
	   All others will be handled as normal (potentially) multi-character symbols. */
	int size;
	unsigned char buf[4] = {0};
	// 0xC2 U+0080:U+00C0 generic (0xAC ¬)
	// 0xC3 U+00C0:U+0100 generic
	// 0xE2 0x86 U+2180:U+21C0 maths
	// 0xE2 0x88 U+2200:U+223F maths (0xAB ∫, 0x9A √, 0x9E ∞, 0x88 ∈, 0xA9 ∩, 0xAA ∪, 0x82 ∂
	// 0xE2 0x89 U+2240:U+2280 maths (0xA0 /=, 0xA4 ≤, 0xA5 ≥)
	// 0xE2 0x8A U+2280:U+22C0 maths
	// 0xE2 0x8B U+22C0:U+2300 maths (0x85 ⋅)
	// 0xE2 0x9F U+27C0:U+2800 brackets (0xA8 ⟨, 0xA9 ⟩)
	// 0xE2 0xA8 U+2A00:U+2B00 maths (0xAF ⨯)
	// others normal => parse_symbol
	if(input <= 0)
		raise_error("<operator>", input);
	size = UTF8_decode_first(input);
	if(size <= 3) {
		buf[0] = input;
		switch(size) {
		case 1:
			parse_symbol(input);
			break;
		case 2:
			input = increment_position(FGETC(input_file));
			buf[1] = input;
			if(buf[0] == 0xC2 && buf[1] == 0xAC) {
				input_value = Symbols::Snot;
				return;
			}
			parse_symbol(buf[1], buf[0]);
			break;
		case 3:
			input = increment_position(FGETC(input_file));
			buf[1] = input;
			input = increment_position(FGETC(input_file));
			buf[2] = input;
			if(buf[0] == 0xE2) { // U+2xxx
				switch(buf[1]) {
				case 0x83: /* U+20D0..U+20FF which is 0xE2 0x83 0x90 .. 0xE2 0x83 0xBF (e) */
					if(buf[2] < 0x90) {
						input_value = symbolFromStr((const char*) buf);
						return;
					}
					break;
				case 0x85: /* U+2150..U+218F which is 0xE2 0x85 0x90 .. 0xE2 0x86 0x8F */
					if(buf[2] < 0x90) {
						input_value = symbolFromStr((const char*) buf);
						return;
					}
					break;
				case 0x86: /* U+2150..U+218F which is 0xE2 0x85 0x90 .. 0xE2 0x86 0x8F */
					if(buf[2] > 0x8F) {
						input_value = symbolFromStr((const char*) buf);
						return;
					}
					break;
				case 0xB0:
				case 0xB1:
				case 0xB2:
				case 0xB3:
				case 0xB4:
				case 0xB5:
				case 0xB6:
				case 0xB7: /* U+2C00..U+2DFF which is 0xE2 0xB0 0x80 .. 0xE2 0xB7 0xBF (e) */
					break;
				default: /* normal */
					input_value = symbolFromStr((const char*) buf);
					return;
				}
			}
			parse_symbol(buf[2], buf[0], buf[1]);
			break;
		default:
			abort();
		}
	} else
		parse_symbol(input);
}
static bool structural_P(int input) {
	return(input == '(' || input == ')' || input == '[' || input == ']' || input == '{' || input == '}');
}
void Scanner::parse_structural(int input) {
	switch(input) {
	case '(':
		increase_brace_level();
		input_value = Symbols::Sleftparen;
		return;
	case ')':
		decrease_brace_level();
		input_value = Symbols::Srightparen;
		return;
	case '[':
		increase_brace_level();
		input_value = Symbols::Sleftbracket;
		return;
	case ']':
		decrease_brace_level();
		input_value = Symbols::Srightbracket;
		return;
	case '{':
		increase_brace_level();
		input_value = Symbols::Sleftcurly;
		return;
	case '}':
		decrease_brace_level();
		input_value = Symbols::Srightcurly;
		return;
	/* TODO other kind of braces? */
	default:
		raise_error("<expression>", input);	
	}
}
void Scanner::parse_string(int input) {
	std::stringstream matchtext;
	// assert input == '"'
	// TODO S-Expressions probably shouldn't use this.
	bool B_escaped = false;
	for(input = increment_position(FGETC(input_file)); input != EOF && (input != '"' || B_escaped); input = increment_position(FGETC(input_file))) {
		if(!B_escaped) {
			if(input == '\\') {
				B_escaped = true;
				continue;
			}
			matchtext << (char) input;
		} else { /* escaped */
			B_escaped = false;
			switch(input) {
			case 'a':
				matchtext << '\a';
				break;
			case 'b':
				matchtext << '\b';
				break;
			case 't':
				matchtext << '\t';
				break;
			case 'n':
				matchtext << '\n';
				break;
			case 'v':
				matchtext << '\v';
				break;
			case 'f':
				matchtext << '\f';
				break;
			case 'r':
				matchtext << '\r';
				break;
			case 'e':
				matchtext << (char) 27;
				break;
			case 'x':
				{
					int digit1;
					int digit2;
					input = increment_position(FGETC(input_file));
					// TODO handle invalid escapes.
					digit1 = (input >= '0' && input <= '9') ? (input - '0') : (input >= 'a' && input <= 'f') ? (10 + (input - 'a')) : (input >= 'A' && input <= 'F') ? (10 + (input - 'A')) : 0;
					input = increment_position(FGETC(input_file));
					digit2 = (input >= '0' && input <= '9') ? (input - '0') : (input >= 'a' && input <= 'f') ? (10 + (input - 'a')) : (input >= 'A' && input <= 'F') ? (10 + (input - 'A')) : 0;
					matchtext << (char) (digit1 * 16 + digit2);
				}
				break;
			case '\\':
			default:
				matchtext << (char) input;
			}
		}
	}
	if(input != '"')
		raise_error("<quote>", input);
	std::string value = matchtext.str();
	input_value = makeStrCXX(value);
}
static bool symbol1_char_P(int input) {
	return (input >= 'A' && input <= 'Z')
	    || (input >= 'a' && input <= 'z')
	    || input == '#'
	    || (input >= 128 && input != 0xE2 /* operators */);
}
bool symbol_char_P(int input) {
	return symbol1_char_P(input) 
	    || (input >= '0' && input <= '9') 
	    || /*input == '_' || */input == '?' || input == '!';
	  /*  || input == '^' not really part of the symbol name any more. */
}

void Scanner::parse_keyword(int input) {
	std::stringstream matchtext;
	input = increment_position(FGETC(input_file));
	if(!symbol1_char_P(input)) {
		raise_error("<symbol>", input);
		return;
	}
	matchtext << (char) input;
	input = increment_position(FGETC(input_file));
	while(symbol_char_P(input)) {
		matchtext << (char) input;
		input = increment_position(FGETC(input_file));
	}
	if(input != ':') {
		raise_error(":", input);
		return;
	}
	matchtext << (char) input;
	std::string v = matchtext.str();
	input_value = keywordFromStr(v.c_str());
}
void Scanner::parse_number_with_base(int input, int base) {
	int value = 0; /* TODO make this more general? */
	while(true) {
		int digit = (input >= '0' && input <= '9') ? (input - '0') :
		            (input >= 'A' && input <= 'Z') ? 10 + (input - 'A') :
		            (input >= 'a' && input <= 'z') ? 10 + (input - 'a') :
		            -1;
		if(digit == -1) {
			if(input != ' ' && input != '\t')
				UNGETC(decrement_position(input), input_file);
			break;
		}
		if(digit < 0 || digit >= base)
			raise_error("<number>", input);
		value = value * base + digit;
		input = increment_position(FGETC(input_file));
	}
	std::stringstream sst;
	sst << value; /* decimal */
	input_value = symbolFromStr(sst.str().c_str());
}
void Scanner::parse_shebang(int input) {
	std::stringstream matchtext;
	while(input != EOF && input != '\n') {
		matchtext << (char) input;
		input = increment_position(FGETC(input_file));
	}
	std::string value = matchtext.str();
	input_value = makeApplication(Symbols::Shashexclam, makeStrCXX(value)); // TODO maybe this is overkill
}
void Scanner::parse_special_coding(int input) {
	assert(input == '#');
	input = increment_position(FGETC(input_file));
	switch(input) {
	case '\\':
		/* TODO other names. ... */
		input = increment_position(FGETC(input_file));
		if(input != EOF) {
			if(input == '\\')
				input_value = symbolFromStr("\\");
			else if(symbol1_char_P(input))
				parse_symbol(input);
			else {
				char buf[2] = {0};
				buf[0] = input;
				input_value = symbolFromStr(buf);
			}
			// allow these to be overridden input_value = Numbers::internNative((Numbers::NativeInt) input);
			std::stringstream sst;
			const char* n;
			if((n = get_symbol1_name(input_value)) != NULL) {
				if(n[0] && !n[1])
					sst << (unsigned int) (unsigned char) n[0];
				else { /* more complicated character, i.e. control character... */
					if(input_value == Symbols::Stab)
						sst << 9;
					else if(input_value == Symbols::Snewline)
						sst << (unsigned) '\n';
					else if(input_value == symbolFromStr("space"))
						sst << (unsigned) ' ';
					else if(input_value == Symbols::Sbackspace)
						sst << (unsigned) '\b';
					else if(input_value == Symbols::Sescape)
						sst << 27;
					else if(input_value == symbolFromStr("equal"))
						sst << (unsigned) '=';
					else
						raise_error("<character>", str(input_value));
				}
			}
			input_value = symbolFromStr(sst.str().c_str());
		} else
			raise_error("<character>", "<EOF>");
		break;
	case 'o':
	case 'x':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': /* & "r" */
		{
			int base = input == 'o' ? 8 : input == 'x' ? 16 : 0;
			while(input >= '0' && input <= '9') {
				base = base * 10 + (input - '0');
				input = increment_position(FGETC(input_file));
			}
			/* TODO is #or1 valid? */
			if(input == 'r' || input == 'o' || input == 'x')
				input = increment_position(FGETC(input_file));
			parse_number_with_base(input, base);
		}
		break;
	//case '|': /* block comment */ TODO
	//	parse_block_comment();
	//	break;
	case '!':
		parse_shebang(input);
		break;
	default:
		parse_symbol(input, '#');
		break;
	}
}
static bool operatorCharP(int input) {
	// without '#' for now (not sure whether that's good. TODO find out)
	// without '@' for now (keywords).
	// without braces 40 41 91 93
	return(input == '!' || (input >= 36 && input < 48 && input != '(' && input != ')') || (input >= 58 && input < 64) || (input == '^') || (input == '|') || (input == '~')) || input == '[' || input == ']';
}
void Scanner::parse_operator(int input) {
	std::stringstream sst;
	// TODO UTF-8 math operators.
	if(!operatorCharP(input))
		return(parse_symbol(input));
	while(operatorCharP(input)) {
		sst << (char) input;
		input = increment_position(FGETC(input_file));
		if(input == '\'' || structural_P(input))
			break;
	}
	if(input != ' ' && input != '\t')
		UNGETC(decrement_position(input), input_file);
	std::string v = sst.str();
	input_value = symbolFromStr(v.c_str());
}
void Scanner::parse_symbol(int input, int special_prefix, int special_prefix_2) {
	std::stringstream matchtext;
	if(special_prefix) {
		matchtext << (char) special_prefix;
	}
	if(special_prefix_2) {
		matchtext << (char) special_prefix_2;
	}
	if(!symbol1_char_P(input)) {
		raise_error("<expression>", input);
		return;
	}
	while(symbol_char_P(input)) {
		matchtext << (char) input;
		input = increment_position(FGETC(input_file));
	}
	if(input == 0xE2) {
		input = increment_position(FGETC(input_file));
		if(input == 0x83) { // vector arrow etc.
			input = increment_position(FGETC(input_file));
			matchtext << (char) 0xE2 << (char) 0x83 << (char) input; // usually 0x97
		} else {
			UNGETC(decrement_position(input), input_file);
			UNGETC(decrement_position(0xE2), input_file); // FIXME it is actually unsupported to unget more than 1 character :-(
			//raise_error("<unicode_operator>", "<unknown>");
		}
	} else if(input != ' ' && input != '\t') /* ignore whitespace */
		UNGETC(decrement_position(input), input_file);
	std::string v = matchtext.str();
	input_value = symbolFromStr(v.c_str());
}

void Scanner::update_indentation() {
	std::pair<int, int> new_entry = std::make_pair(line_number, column_number);
	assert(!open_indentations.empty());
	if(B_honor_indentation && open_indentations.front().second != column_number) { // this NEEDS to be neutral on same indentation. Reason: parse_token() backtracks when it notices that someone injected something.
		int previous_indentation = open_indentations.front().second;
		while(!open_indentations.empty() && column_number < previous_indentation) {
			//printf("should close %d\n", previous_indentation);
			inject(Symbols::Sautorightparen);
			open_indentations.pop_front();
			previous_indentation = open_indentations.front().second;
		}
		if(column_number > previous_indentation) {
			inject(Symbols::Sautoleftparen);
			//printf("opening indentation at %d: %d after %d\n", line_number, column_number, open_indentations.front().second);
			open_indentations.push_front(new_entry);
		}
	}
}
void Scanner::inject(NodeT value) {
	/* it is assumed that this is called while scanning whitespace - or right afterwards */
	//input_value = &pending;
	//std::string v = str(value);
	//printf("injecting: %s\n", v.c_str());
	injected_input_values.push_front(value);
}
bool Scanner::deinject() { /* on EOF, makes sure that injected parens are closed, if need be. */
	if(!B_honor_indentation)
		return(false);
	increment_position('\n');
	update_indentation();
	if(!injected_input_values.empty()) {
		input_value = injected_input_values.front();
		injected_input_values.pop_front();
		return(true);
	} else
		return(false);
}

REGISTER_STR(Scanner, return("Scanner");)
};
