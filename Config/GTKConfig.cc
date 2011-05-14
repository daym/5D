#include <glib.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include "Config/Config"

#define CONFIG_NAME "4D/config.INI"

struct Config {
	GKeyFile* key_file;
	char* environment_name;
	int main_window_width;
	int main_window_height;
	bool show_tips;
	int current_tip;
};
struct Config* load_Config(void) {
	const char** system_config_dirs;
	int system_config_dir_count;
	const char** config_dirs;
	struct Config* config;
	gchar* environment_name;
	char* full_name;
	const char* config_dir_name;
	GError* error = NULL;
	config_dir_name = g_get_user_config_dir();
	system_config_dirs = (const char**) g_get_system_config_dirs();
	config = new Config;
	config->show_tips = true;
	config->current_tip = 0;
	config->main_window_width = 400;
	config->main_window_height = 400;
	config->key_file = g_key_file_new();
	g_key_file_set_list_separator(config->key_file, ':');
	system_config_dir_count = g_strv_length((gchar**) system_config_dirs);
	config_dirs = (const char**) calloc(system_config_dir_count + 2, sizeof(char*));
	for(int i = 0; i < system_config_dir_count; ++i)
		config_dirs[i] = system_config_dirs[i];
	config_dirs[system_config_dir_count] = config_dir_name;
	config_dirs[system_config_dir_count + 1] = NULL;
	if(!g_key_file_load_from_dirs(config->key_file, CONFIG_NAME, config_dirs, &full_name, (GKeyFileFlags) (G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS), &error)) {
		//g_warning("could not load configuration %s", fname);
		g_free((gchar**) config_dirs);
		g_error_free(error);
		error = NULL;
		return(config);
	}
	g_free((gchar**) config_dirs);
	g_free(full_name);
	environment_name = g_key_file_get_string(config->key_file, "Global", "Environment", &error);
	config->environment_name = environment_name && environment_name[0] ? strdup(environment_name) : NULL;
	config->main_window_width = g_key_file_get_integer(config->key_file, "MainWindow", "Width", NULL);
	config->main_window_height = g_key_file_get_integer(config->key_file, "MainWindow", "Height", NULL);
	{
		GError* error = NULL;
		config->show_tips = g_key_file_get_boolean(config->key_file, "Global", "ShowTips", &error);
		if(error) {
			config->show_tips = true;
			g_error_free(error);
		}
	}
	config->current_tip = g_key_file_get_integer(config->key_file, "Global", "CurrentTip", NULL);
	if(!config->main_window_width || !config->main_window_height)
		config->main_window_height = config->main_window_width = 400;
	g_free(environment_name);
	g_key_file_free(config->key_file);
	return(config);
}
bool Config_save(struct Config* config) {
	const char* config_dir_name;
	char* full_name;
	GError* error = NULL;
	gsize size;
	gchar* key_file_contents;
	config->key_file = g_key_file_new();
	config_dir_name = g_get_user_config_dir();
	if(!g_str_has_suffix(config_dir_name, "/"))
		config_dir_name = g_strdup_printf("%s/", config_dir_name);
	g_mkdir_with_parents(g_strdup_printf("%s%s", config_dir_name, "4D"), 0775);
	full_name = g_strdup_printf("%s%s", config_dir_name, CONFIG_NAME);
	g_key_file_set_string(config->key_file, "Global", "Environment", config->environment_name ? config->environment_name : "");
	g_key_file_set_integer(config->key_file, "MainWindow", "Width", config->main_window_width);
	g_key_file_set_integer(config->key_file, "MainWindow", "Height", config->main_window_height);
	g_key_file_set_boolean(config->key_file, "Global", "ShowTips", config->show_tips);
	g_key_file_set_integer(config->key_file, "Global", "CurrentTip", config->current_tip);
	key_file_contents = g_key_file_to_data(config->key_file, &size, &error);
	if(!key_file_contents || !size || !g_file_set_contents(full_name, key_file_contents, size, &error)) {
		g_warning("could not save config \"%s\": %s", full_name, error ? error->message : "unknown error");
		if(error)
			g_error_free(error);
		g_free(full_name);
		return(false);
	}
	g_free(full_name);
	return(true);
}
char* Config_get_environment_name(struct Config* config) {
	return(config->environment_name ? strdup(config->environment_name) : NULL);
}
void Config_set_environment_name(struct Config* config, const char* value) {
	config->environment_name = value ? strdup(value) : NULL;
}
int Config_get_main_window_width(struct Config* config) {
	return(config->main_window_width);
}
int Config_get_main_window_height(struct Config* config) {
	return(config->main_window_height);
}
void Config_set_main_window_width(struct Config* config, int value) {
	config->main_window_width = value;
}
void Config_set_main_window_height(struct Config* config, int value) {
	config->main_window_height = value;
}
void Config_set_show_tips(struct Config* config, bool value) {
	config->show_tips = value;
}
bool Config_get_show_tips(struct Config* config) {
	return(config->show_tips);
}
int Config_get_current_tip(struct Config* config) {
	return(config->current_tip);
}
void Config_set_current_tip(struct Config* config, int index) {
	config->current_tip = index;
}
