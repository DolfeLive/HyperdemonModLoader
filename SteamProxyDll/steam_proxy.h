#pragma once
#include <windows.h>

class SteamProxy {
public:
    static void SetOriginalDll(HMODULE hDll);
    static HMODULE GetOriginalDll();

    static bool SteamAPI_Init();
    static bool SteamAPI_IsSteamRunning();
    static int SteamAPI_GetHSteamUser();
    static int SteamAPI_GetHSteamPipe();
    static void* SteamInternal_ContextInit(void* ctx);
    static void* SteamInternal_CreateInterface(const char* version);
    static void SteamAPI_Shutdown();
    static void SteamAPI_RunCallbacks();

private:
    static HMODULE s_hOriginalDll;
};