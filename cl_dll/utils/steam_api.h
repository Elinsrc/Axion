#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>

typedef uint64_t SteamID64;

class CSteamAPI
{
public:
    static CSteamAPI& GetInstance();

    bool initialize();
    void shutdown();
    void RunCallbacks();
    bool IsInitialized() const { return m_initialized; }


private:
    CSteamAPI() = default;
    ~CSteamAPI() = default;
    CSteamAPI(const CSteamAPI&) = delete;
    CSteamAPI& operator=(const CSteamAPI&) = delete;

    void* get_proc(const char* name);
    bool LoadSteamFunctions();

private:
    void* m_library = nullptr;
    bool m_initialized = false;

    char m_playerSteamIDs[MAX_PLAYERS][64] = {};

    bool (*m_SteamAPI_Init)() = nullptr;
    void (*m_SteamAPI_Shutdown)() = nullptr;
    void  (*m_SteamAPI_RunCallbacks)() = nullptr;

    void* (*m_SteamFriends)() = nullptr;
    void* (*m_SteamUtils)() = nullptr;

    void* (*m_SteamUser)() = nullptr;
    SteamID64 (*m_GetSteamID)(void*) = nullptr;

    int (*m_GetSmallFriendAvatar)(void*, SteamID64) = nullptr;
    int (*m_GetMediumFriendAvatar)(void*, SteamID64) = nullptr;
    int (*m_GetLargeFriendAvatar)(void*, SteamID64) = nullptr;
    bool (*m_RequestUserInformation)(void*, SteamID64, bool) = nullptr;

    bool (*m_GetImageSize)(void*, int, uint32_t*, uint32_t*) = nullptr;
    bool (*m_GetImageRGBA)(void*, int, uint8_t*, int) = nullptr;
};

extern CSteamAPI& g_SteamAPI;