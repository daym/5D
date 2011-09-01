#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <string>
#include "Config/Config"

struct Config {
	char* fEnvironment;
	int fMainWindowWidth;
	int fMainWindowHeight;
	bool fBShowTips;
};
static std::wstring GetRegistryStringValueCXX(HKEY key, const std::wstring& valueKey) {
	DWORD size = 2048;
	DWORD typeID;
	WCHAR buffer[2049];
#if 0
	if(RegGetValue(key, subkey.length() > 0 ? subkey.c_str() : NULL, valueKey.c_str(), RRF_RT_REG_SZ, NULL, buffer, &size) != ERROR_SUCCESS)
		return(std::wstring());
#endif
	if(RegQueryValueEx(key, valueKey.c_str(), NULL, &typeID, (LPBYTE) buffer, &size) != ERROR_SUCCESS || typeID != REG_SZ)
		return(std::wstring());
	return(buffer);

}
static DWORD GetRegistryDWORDValueCXX(HKEY key, const std::wstring& valueKey, DWORD fallbackResult) {
	DWORD size = sizeof(DWORD);
	DWORD typeID;
	DWORD buffer;
#if 0
	if(RegGetValue(key, subkey.length() > 0 ? subkey.c_str() : NULL, valueKey.c_str(), RRF_RT_REG_SZ, NULL, buffer, &size) != ERROR_SUCCESS)
		return(std::wstring());
#endif
	if(RegQueryValueEx(key, valueKey.c_str(), NULL, &typeID, (LPBYTE) &buffer, &size) != ERROR_SUCCESS || typeID != REG_DWORD || size != sizeof(DWORD))
		return(fallbackResult);
	return(buffer);
}
static void SetRegistryStringValueCXX(HKEY key, const std::wstring& valueKey, const std::wstring& valueValue) {
	RegSetValueEx(key, valueKey.c_str(), 0, REG_SZ, (const BYTE*) valueValue.c_str(), sizeof(*valueValue.c_str()) * (valueValue.length() + 1));
}
static void SetRegistryDWORDValueCXX(HKEY key, const std::wstring& valueKey, DWORD valueValue) {
	RegSetValueEx(key, valueKey.c_str(), 0, REG_DWORD, (const BYTE*) &valueValue, sizeof(DWORD));
}
struct Config* load_Config(void) {
	struct Config* config;
	config = (struct Config*) calloc(1, sizeof(struct Config));
	config->fMainWindowWidth = 400;
	config->fMainWindowHeight = 400;
	HKEY nativeConfig;
	if(RegOpenKey(HKEY_CURRENT_USER, _T("SOFTWARE\\4D"), &nativeConfig) != ERROR_SUCCESS)
		if(RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\4D"), &nativeConfig) != ERROR_SUCCESS)
			return(config);
	config->fEnvironment = ToUTF8(GetRegistryStringValueCXX(nativeConfig, _T("Environment")));
	config->fMainWindowWidth = (int) GetRegistryDWORDValueCXX(nativeConfig, _T("MainWindowWidth"), 400);
	config->fMainWindowHeight = (int) GetRegistryDWORDValueCXX(nativeConfig, _T("MainWindowHeight"), 400);
	config->fBShowTips = GetRegistryDWORDValueCXX(nativeConfig, _T("ShowTips"), 1) != 0;
	RegCloseKey(nativeConfig);
	return(config);
}
bool Config_save(struct Config* config) {
	HKEY nativeConfig;
	if(RegOpenKey(HKEY_CURRENT_USER, _T("SOFTWARE\\4D"), &nativeConfig) != ERROR_SUCCESS)
		if(RegCreateKey(HKEY_CURRENT_USER, _T("SOFTWARE\\4D"), &nativeConfig) != ERROR_SUCCESS)
			return(false);
	SetRegistryStringValueCXX(nativeConfig, _T("Environment"), FromUTF8(config->fEnvironment));
	SetRegistryDWORDValueCXX(nativeConfig, _T("MainWindowWidth"), config->fMainWindowWidth);
	SetRegistryDWORDValueCXX(nativeConfig, _T("MainWindowHeight"), config->fMainWindowHeight);
	SetRegistryDWORDValueCXX(nativeConfig, _T("ShowTips"), config->fBShowTips ? 1 : 0);
	RegCloseKey(nativeConfig);
	return(true);
}
char* Config_get_environment_name(struct Config* config) {
	return(config->fEnvironment);
}
void Config_set_environment_name(struct Config* config, const char* value) {
	config->fEnvironment = strdup(value);
}
int Config_get_main_window_width(struct Config* config) {
	return(config->fMainWindowWidth);
}
int Config_get_main_window_height(struct Config* config) {
	return(config->fMainWindowHeight);
}
void Config_set_main_window_width(struct Config* config, int value) {
	config->fMainWindowWidth = value;
}
void Config_set_main_window_height(struct Config* config, int value) {
	config->fMainWindowHeight = value;
}
void Config_set_show_tips(struct Config* config, bool value) {
	// FIXME
}
bool Config_get_show_tips(struct Config* config) {
	// FIXME
	return(true);
}
