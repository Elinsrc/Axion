#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "steam_api.h"
#include "build.h"

#if XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#ifdef PlaySound
#undef PlaySound
#endif
#else
#include <dlfcn.h>
#endif

#include <cstring>
#include <cstdio>
#include <cstdlib>

CSteamAPI& g_SteamAPI = CSteamAPI::GetInstance();

CSteamAPI& CSteamAPI::GetInstance()
{
    static CSteamAPI instance;
    return instance;
}

void* CSteamAPI::get_proc(const char* name)
{
#if XASH_WIN32
    return (void*)GetProcAddress((HMODULE)m_library, name);
#else
    return dlsym(m_library, name);
#endif
}

bool CSteamAPI::LoadSteamFunctions()
{
    m_SteamFriends = (void*(*)())get_proc("SteamAPI_SteamFriends_v017");
    m_SteamUtils = (void*(*)())get_proc("SteamAPI_SteamUtils_v010");
    m_SteamUser = (void*(*)())get_proc("SteamAPI_SteamUser_v023");

    if (!m_SteamFriends || !m_SteamUtils || !m_SteamUser)
    {
        return false;
    }

    m_GetSmallFriendAvatar = (int(*)(void*, SteamID64))get_proc("SteamAPI_ISteamFriends_GetSmallFriendAvatar");
    m_GetMediumFriendAvatar = (int(*)(void*, SteamID64))get_proc("SteamAPI_ISteamFriends_GetMediumFriendAvatar");
    m_GetLargeFriendAvatar = (int(*)(void*, SteamID64))get_proc("SteamAPI_ISteamFriends_GetLargeFriendAvatar");
    
    m_GetImageSize = (bool(*)(void*, int, uint32_t*, uint32_t*))get_proc("SteamAPI_ISteamUtils_GetImageSize");
    m_GetImageRGBA = (bool(*)(void*, int, uint8_t*, int))get_proc("SteamAPI_ISteamUtils_GetImageRGBA");
    
    m_GetSteamID = (SteamID64(*)(void*))get_proc("SteamAPI_ISteamUser_GetSteamID");

    if (!m_GetSmallFriendAvatar || !m_GetImageSize || !m_GetImageRGBA || !m_GetSteamID)
    {
        return false;
    }

    return true;
}

bool CSteamAPI::initialize()
{
    if (m_initialized)
        return true;

#if XASH_WIN32
    m_library = LoadLibraryA("steam_api.dll");
#else
    m_library = dlopen("libsteam_api.so", RTLD_NOW);
#endif

    if (!m_library)
    {
        gEngfuncs.Con_DPrintf("[SteamAPI] Failed to load steam_api library\n");
        return false;
    }

    gEngfuncs.Con_DPrintf("[SteamAPI] Library loaded: %p\n", m_library);

    m_SteamAPI_Init = (bool(*)())get_proc("SteamAPI_Init");
    m_SteamAPI_Shutdown = (void(*)())get_proc("SteamAPI_Shutdown");
    m_SteamAPI_RunCallbacks = (void(*)())get_proc("SteamAPI_RunCallbacks");

    if (!m_SteamAPI_Init || !m_SteamAPI_Shutdown)
    {
        gEngfuncs.Con_DPrintf("[SteamAPI] Failed to get basic API functions\n");
        shutdown();
        return false;
    }

    if (!m_SteamAPI_Init())
    {
        gEngfuncs.Con_Printf("[SteamAPI] SteamAPI_Init() Failed — Steam client not running?\n");
    }
    else
    {
        gEngfuncs.Con_Printf("[SteamAPI] SteamAPI_Init() OK\n");
    }

    LoadSteamFunctions();

    m_initialized = true;
    return true;
}

void CSteamAPI::shutdown()
{
    if (m_library)
    {
#if XASH_WIN32
        FreeLibrary((HMODULE)m_library);
#else
        dlclose(m_library);
#endif
    }

    m_library = nullptr;
    m_initialized = false;

    m_SteamAPI_Init = nullptr;
    m_SteamAPI_Shutdown = nullptr;
    m_SteamAPI_RunCallbacks = nullptr;
    m_SteamFriends = nullptr;
    m_SteamUtils = nullptr;
    m_GetSmallFriendAvatar = nullptr;
    m_GetMediumFriendAvatar = nullptr;
    m_GetLargeFriendAvatar = nullptr;
    m_RequestUserInformation = nullptr;
    m_GetImageSize = nullptr;
    m_GetImageRGBA = nullptr;
}

void CSteamAPI::RunCallbacks()
{
    if (m_initialized && m_SteamAPI_RunCallbacks)
        m_SteamAPI_RunCallbacks();
}