#ifndef STEAM_API_H
#define STEAM_API_H

#include <stdint.h>

typedef uint64_t SteamID64;

class CSteamAPI
{
public:
    static CSteamAPI& GetInstance();

    bool initialize();
    void shutdown();
    void RunCallbacks();

    int GetSmallFriendAvatar(SteamID64 steamID);
    int GetMediumFriendAvatar(SteamID64 steamID);
    int GetLargeFriendAvatar(SteamID64 steamID);
    bool GetImageSize(int iImage, uint32_t *pnWidth, uint32_t *pnHeight);
    bool GetImageRGBA(int iImage, uint8_t *pubDest, int nDestBufferSize);
    SteamID64 GetLocalSteamID();

    void ActivateGameOverlayToUser(const char* pchDialog, SteamID64 steamID);

    bool IsInitialized() const { return m_initialized; }

    void* get_proc(const char* name);
private:
    CSteamAPI() = default;

    bool LoadSteamFunctions();

    void* m_library = nullptr;
    bool m_initialized = false;

    // API functions
    bool (*m_SteamAPI_Init)() = nullptr;
    void (*m_SteamAPI_Shutdown)() = nullptr;
    void (*m_SteamAPI_RunCallbacks)() = nullptr;

    // Interfaces
    void* (*m_SteamFriends)() = nullptr;
    void* (*m_SteamUtils)() = nullptr;
    void* (*m_SteamUser)() = nullptr;

    // Friends
    int (*m_GetSmallFriendAvatar)(void*, SteamID64) = nullptr;
    int (*m_GetMediumFriendAvatar)(void*, SteamID64) = nullptr;
    int (*m_GetLargeFriendAvatar)(void*, SteamID64) = nullptr;
    bool (*m_RequestUserInformation)(void*, SteamID64, bool) = nullptr;

    // Utils
    bool (*m_GetImageSize)(void*, int, uint32_t*, uint32_t*) = nullptr;
    bool (*m_GetImageRGBA)(void*, int, uint8_t*, int) = nullptr;

    // User
    SteamID64 (*m_GetSteamID)(void*) = nullptr;

    void (*m_ActivateGameOverlayToUser)(void*, const char*, SteamID64) = nullptr;
};

extern CSteamAPI& g_SteamAPI;

#endif