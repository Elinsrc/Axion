#pragma once

#include <stdint.h>
#include <stddef.h>
#include <mutex>
#include <vector>
#include <string>
#include <atomic>
#include <sstream>
#include <filesystem>

#include "imgui.h"
#include "imgui_utils.h"
#include "custom_utils.h"
#include "web_worker.h"
#include "web_client.h"

static constexpr int MAX_AVATAR_PLAYERS = 33;
static constexpr int AVATAR_TARGET_SIZE = 64;
static constexpr int FRAMES_PER_TICK = 1;

static constexpr const char* AVATAR_CACHE_DIRNAME = "avatar_cache";
static constexpr const char* AVATAR_CACHE_MAGIC = "AVC1";
static constexpr uint32_t AVATAR_CACHE_VERSION = 1u;
static constexpr int AVATAR_CACHE_MAX_UNUSED_DAYS = 7;

static constexpr const char* CUSTOM_AVATAR_DIRNAME = "avatar";
static constexpr const char* CUSTOM_AVATAR_HOST = "https://files.catbox.moe/";
static constexpr const char* CUSTOM_AVATAR_UPLOAD_URL = "https://catbox.moe/user/api.php";
static constexpr size_t CUSTOM_AVATAR_MAX_BYTES = 2 * 1024 * 1024;
static constexpr size_t CUSTOM_AVC_MAX_BYTES = 4 * 1024 * 1024;
static constexpr int CUSTOM_AVATAR_MAX_SIZE = 512;

typedef uint64_t SteamID64;

struct AvatarFrame
{
    ImTextureID texture = 0;
    float delayMs = 0.0f;
};

struct AvatarEntry
{
    ImTextureID texture = 0;
    SteamID64 steamId = 0;
    bool loaded = false;
    bool requested = false;
    bool failed = false;
    bool isAnimated = false;

    bool diskAttempted = false;
    std::string diskAvatarHash;

    std::string customName;
    bool customActive = false;
    bool customRequested = false;

    bool localActive = false;
    int localVersion = 0;

    std::vector<AvatarFrame> frames;
    float totalDurationMs = 0.0f;
    int animFrame = 0;
    float animAccumMs = 0.0f;
    double animLastTime = 0.0;

    std::vector<std::vector<uint8_t>> pendingFrameData;
    std::vector<float> pendingDelays;
    int pendingWidth = 0;
    int pendingHeight = 0;
    int pendingFrameCount = 0;
    int pendingFrameIndex = 0;
    bool isPending = false;
};

struct DownloadedAvatar
{
    int playerIndex = 0;
    SteamID64 steam64 = 0;
    bool isAnimated = false;
    bool failed = false;

    bool custom = false;
    std::string customName;

    std::vector<uint8_t> imageData;

    std::vector<std::vector<uint8_t>> gifFrames;
    std::vector<float> gifDelays;
    int gifWidth = 0;
    int gifHeight = 0;
};

struct AvatarTask
{
    int playerIndex = 0;
    SteamID64 steam64 = 0;
    std::string knownHash;
};

#pragma pack(push, 1)
struct AvcFileHeader
{
    char magic[4];
    uint32_t version;
    uint8_t isAnimated;
    uint16_t hashLen;
};
#pragma pack(pop)

class CAvatarCache
{
public:
    void Initialize();
    void VidInitialize();
    void Shutdown();

    void Update();
    void PrintCacheInfo();

    void SyncCustomAvatar();

    ImTextureID GetAvatar(int playerIndex);
    void ClearAvatar(int playerIndex);
    void ClearAll();

private:
    enum class GifFetch
    {
        Failed,
        NoAnimation,
        Ok
    };

    static std::string ProfileUrl(SteamID64 id);
    static std::string ExtractXmlTag(const std::string& xml, const std::string& tag);
    static std::string ExtractAnimatedAvatarUrl(const std::string& html);
    static std::string ExtractAvatarHash(const std::string& url);
    static HttpRequest MakeRequest(const std::string& url, long timeoutSec, const std::atomic<bool>& cancel);

    GifFetch FetchAnimatedGif(SteamID64 id, const std::atomic<bool>& cancel, std::vector<uint8_t>& out);

    static bool WriteAvcStream(std::ostream& f, bool isAnimated, const std::string& hash, const DownloadedAvatar& data);
    static bool ReadAvcStream(std::istream& f, DownloadedAvatar& out, std::string& hashOut, bool& isAnimatedOut);
    static bool ValidateCustomAvatar(const DownloadedAvatar& d);

    bool WriteAvcFile(const std::filesystem::path& path, bool isAnimated, const std::string& hash, const DownloadedAvatar& data);
    bool ReadAvcFile(const std::filesystem::path& path, DownloadedAvatar& out, std::string& hashOut, bool& isAnimatedOut);

    static void AvatarCacheInfo_f();
    static void AvatarUpload_f();

    void ProcessDownloadedAvatars();
    void ApplyStatic(AvatarEntry& entry, DownloadedAvatar& data);
    void ApplyAnimated(AvatarEntry& entry, DownloadedAvatar& data);
    void ApplyCustom(AvatarEntry& entry, DownloadedAvatar& data);
    void ProcessPendingTextures();
    void ResetVisuals(AvatarEntry& entry);

    void QueueStatic(const AvatarTask& task);
    void DownloadStatic(const AvatarTask& task);
    void DownloadAnimated(const AvatarTask& task, const std::string& staticHash);
    void QueueCustom(int playerIndex, const std::string& name);
    void DownloadCustom(int playerIndex, const std::string& name);
    void PushCompleted(DownloadedAvatar&& data);

    void UpdateCustomState(int playerIndex, AvatarEntry& entry, SteamID64 steam64, bool allowCustom, int localIndex);
    void UploadCustomAvatar();
    void QueuePublish(const std::string& name);
    void ApplyPublish();

    bool DecodeCustom(const std::vector<uint8_t>& data, bool isGif, DownloadedAvatar& out);
    void QueueLocal(DownloadedAvatar&& data, bool has);
    void ApplyLocal(AvatarEntry& entry, bool allowCustom);

    static bool IsValidCustomName(const std::string& s);
    static std::filesystem::path CustomAvatarDir();
    static std::filesystem::path AvatarsCacheDir();
    std::filesystem::path GetAvcPath(SteamID64 steam64) const;
    std::filesystem::path GetCustomAvcPath(const std::string& name) const;

    void InitDiskCache();
    void RunStartupCleanup();
    bool TryLoadFromDisk(int playerIndex, SteamID64 steam64);
    void TouchAvcFile(SteamID64 steam64);

    static bool IsValidPlayerIndex(int playerIndex)
    {
        return playerIndex >= 1 && playerIndex < MAX_AVATAR_PLAYERS;
    }

private:
    CustomUtils m_CustomUtils;
    AvatarEntry m_avatars[MAX_AVATAR_PLAYERS];

    WebWorker m_staticWorker;
    WebWorker m_animatedWorker;
    WebWorker m_customWorker;
    WebWorker m_uploadWorker;

    std::vector<DownloadedAvatar> m_completed;
    std::mutex m_completedMutex;

    std::mutex m_diskMutex;

    std::mutex m_publishMutex;
    bool m_publishPending = false;
    std::string m_publishName;

    std::mutex m_localMutex;
    DownloadedAvatar m_localData;
    bool m_hasLocal = false;
    int m_localVersion = 0;
    std::string m_localHash;

    std::atomic<bool> m_uploading{false};
    std::atomic<int> m_uploadSeq{0};
};

extern CAvatarCache g_AvatarCache;