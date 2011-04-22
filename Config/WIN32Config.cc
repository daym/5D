#include <stdlib.h>
#include "Config/Config"

struct Config;
struct Config* load_Config(void) {
	// FIXME
	return(NULL);
}

bool Config_save(struct Config* config) {
	// FIXME
	return(true);
}
char* Config_get_environment_name(struct Config* config) {
	// FIXME
	return(NULL);
}
void Config_set_environment_name(struct Config* config, const char* value) {
	// FIXME
}
int Config_get_main_window_width(struct Config* config) {
	// FIXME
	return(400);
}
int Config_get_main_window_height(struct Config* config) {
	// FIXME
	return(400);
}
void Config_set_main_window_width(struct Config* config, int value) {
	// FIXME
}
void Config_set_main_window_height(struct Config* config, int value) {
	// FIXME
}
