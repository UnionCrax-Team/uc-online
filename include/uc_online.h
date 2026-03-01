#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <windows.h>

typedef struct UCOnline UCOnline;

typedef bool (*SteamAPI_Init_t)(char*);
typedef bool (*SteamAPI_InitFlat_t)(char*);
typedef void (*SteamAPI_Shutdown_t)();
typedef void (*SteamAPI_RunCallbacks_t)();
typedef bool (*SteamAPI_RestartAppIfNecessary_t)(uint32_t);
typedef void* (*SteamClient_t)();
typedef void* (*SteamApps_t)();
typedef void* (*GetHSteamPipe_t)();

UCOnline* UCOnline_Create(const char* iniFilePath, const char* dllDirectory);
void UCOnline_Destroy(UCOnline* uconline);
bool UCOnline_Initialize(UCOnline* uconline);
void UCOnline_Shutdown(UCOnline* uconline);
void UCOnline_RunCallbacks(UCOnline* uconline);
void UCOnline_SetCustomAppID(UCOnline* uconline, uint32_t appID);
uint32_t UCOnline_GetCurrentAppID(UCOnline* uconline);
bool UCOnline_IsSteamInitialized(UCOnline* uconline);
bool UCOnline_LaunchGame(UCOnline* uconline);
void UCOnline_ClearLog(UCOnline* uconline);
void UCOnline_SetLoggingEnabled(UCOnline* uconline, bool enabled);
bool UCOnline_IsLoggingEnabled(UCOnline* uconline);
