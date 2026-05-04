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

template<typename T>
bool LoadSteamFunction(T& out, const char* name)
{
    out = reinterpret_cast<T>(g_SteamAPI.get_proc(name));

    if (!out)
    {
        gEngfuncs.Con_Printf("\n[SteamAPI] Missing: %s\n", name);
        return false;
    }

    return true;
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
    if (!LoadSteamFunction(m_SteamFriends, "SteamAPI_SteamFriends_v017"))
        return false;
    if (!LoadSteamFunction(m_SteamUtils, "SteamAPI_SteamUtils_v010"))
        return false;
#if XASH_WIN32
    if (!LoadSteamFunction(m_SteamUser, "SteamAPI_SteamUser_v021"))
        return false;
#else
    if (!LoadSteamFunction(m_SteamUser, "SteamAPI_SteamUser_v023"))
        return false;
#endif
    if (!LoadSteamFunction(m_GetSmallFriendAvatar, "SteamAPI_ISteamFriends_GetSmallFriendAvatar"))
        return false;
    if (!LoadSteamFunction(m_GetMediumFriendAvatar, "SteamAPI_ISteamFriends_GetMediumFriendAvatar"))
        return false;
    if (!LoadSteamFunction(m_GetLargeFriendAvatar, "SteamAPI_ISteamFriends_GetLargeFriendAvatar"))
        return false;
    if (!LoadSteamFunction(m_GetImageSize, "SteamAPI_ISteamUtils_GetImageSize"))
        return false;
    if (!LoadSteamFunction(m_GetImageRGBA, "SteamAPI_ISteamUtils_GetImageRGBA"))
        return false;
    if (!LoadSteamFunction(m_GetSteamID, "SteamAPI_ISteamUser_GetSteamID"))
        return false;
    if (!LoadSteamFunction(m_ActivateGameOverlayToUser, "SteamAPI_ISteamFriends_ActivateGameOverlayToUser"))
        return false;

    return true;
}

bool CSteamAPI::initialize()
{
    if (m_initialized)
    {
        return true;
    }

#if XASH_WIN32
    m_library = LoadLibraryA("steam_api.dll");
#else
    m_library = dlopen("libsteam_api.so", RTLD_NOW);
#endif

    if (!m_library)
    {
        gEngfuncs.Con_Printf("\n[SteamAPI] Failed to load steam_api library\n");
        return false;
    }

    gEngfuncs.Con_Printf("\n[SteamAPI] Library loaded: %p\n", m_library);

    m_SteamAPI_Init = (bool(*)())get_proc("SteamAPI_Init");
    m_SteamAPI_Shutdown = (void(*)())get_proc("SteamAPI_Shutdown");
    m_SteamAPI_RunCallbacks = (void(*)())get_proc("SteamAPI_RunCallbacks");

    if (!m_SteamAPI_Init())
    {
        gEngfuncs.Con_Printf("\n[SteamAPI] SteamAPI_Init failed (Steam not running?)\n");
        shutdown();
        return false;
    }

    gEngfuncs.Con_Printf("\n[SteamAPI] Steam initialized successfully!\n");

    if (!LoadSteamFunctions())
    {
        gEngfuncs.Con_Printf("\n[SteamAPI] Failed to load Steam interfaces!\n");
        shutdown();
        return false;
    }

    m_initialized = true;

    gEngfuncs.Con_Printf("\n[SteamAPI] Fully initialized!\n");

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

    gEngfuncs.Con_Printf("\n[SteamAPI] Shutdown!\n");
}

void CSteamAPI::RunCallbacks()
{
    if (m_initialized && m_SteamAPI_RunCallbacks)
        m_SteamAPI_RunCallbacks();
}

int CSteamAPI::GetSmallFriendAvatar(SteamID64 steamID)
{
    return m_GetSmallFriendAvatar(m_SteamFriends(), steamID);
}

int CSteamAPI::GetMediumFriendAvatar(SteamID64 steamID)
{
    return m_GetMediumFriendAvatar(m_SteamFriends(), steamID);
}

int CSteamAPI::GetLargeFriendAvatar(SteamID64 steamID)
{
    return m_GetLargeFriendAvatar(m_SteamFriends(), steamID);
}

bool CSteamAPI::GetImageSize(int iImage, uint32_t *pnWidth, uint32_t *pnHeight)
{
    return m_GetImageSize(m_SteamUtils(), iImage, pnWidth, pnHeight);
}

bool CSteamAPI::GetImageRGBA(int iImage, uint8_t *pubDest, int nDestBufferSize)
{
    return m_GetImageRGBA(m_SteamUtils(), iImage, pubDest, nDestBufferSize);
}

SteamID64 CSteamAPI::GetLocalSteamID()
{
    return m_GetSteamID(m_SteamUser());
}

void CSteamAPI::ActivateGameOverlayToUser(const char* pchDialog, SteamID64 steamID)
{
    return m_ActivateGameOverlayToUser(m_SteamFriends(), pchDialog, steamID);
} 
