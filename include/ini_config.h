#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct IniConfig IniConfig;

IniConfig* IniConfig_Create(const char* iniFilePath);
void IniConfig_Destroy(IniConfig* config);
void IniConfig_Load(IniConfig* config);
void IniConfig_Save(IniConfig* config);
const char* IniConfig_GetValue(IniConfig* config, const char* section, const char* key, const char* defaultValue);
void IniConfig_SetValue(IniConfig* config, const char* section, const char* key, const char* value);

uint32_t IniConfig_GetAppID(IniConfig* config);
void IniConfig_SetAppID(IniConfig* config, uint32_t appId);
const char* IniConfig_GetSteamAppIdFile(IniConfig* config);
void IniConfig_SetSteamAppIdFile(IniConfig* config, const char* filePath);
const char* IniConfig_GetOriginalDllPath(IniConfig* config);
void IniConfig_SetOriginalDllPath(IniConfig* config, const char* dllPath);
bool IniConfig_GetEnableLogging(IniConfig* config);
void IniConfig_SetEnableLogging(IniConfig* config, bool enable);
const char* IniConfig_GetLogFile(IniConfig* config);
void IniConfig_SetLogFile(IniConfig* config, const char* logFile);
