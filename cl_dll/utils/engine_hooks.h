#ifndef ENGINE_HOOKS_H
#define ENGINE_HOOKS_H

#include <stdint.h>
#include <string.h>

struct SvcHandler
{
    int opcode;
    const char *pszName;
    void (*pfnParse)(void);
};

typedef struct cl_enginemessages_s
{
    void (*pfnSvcBad)(void);
    void (*pfnSvcNop)(void);
    void (*pfnSvcDisconnect)(void);
    void (*pfnSvcEvent)(void);
    void (*pfnSvcVersion)(void);
    void (*pfnSvcSetView)(void);
    void (*pfnSvcSound)(void);
    void (*pfnSvcTime)(void);
    void (*pfnSvcPrint)(void);
    void (*pfnSvcStuffText)(void);
    void (*pfnSvcSetAngle)(void);
    void (*pfnSvcServerInfo)(void);
    void (*pfnSvcLightStyle)(void);
    void (*pfnSvcUpdateUserInfo)(void);
    void (*pfnSvcDeltaDescription)(void);
    void (*pfnSvcClientData)(void);
    void (*pfnSvcStopSound)(void);
    void (*pfnSvcPings)(void);
    void (*pfnSvcParticle)(void);
    void (*pfnSvcDamage)(void);
    void (*pfnSvcSpawnStatic)(void);
    void (*pfnSvcEventReliable)(void);
    void (*pfnSvcSpawnBaseline)(void);
    void (*pfnSvcTempEntity)(void);
    void (*pfnSvcSetPause)(void);
    void (*pfnSvcSignonNum)(void);
    void (*pfnSvcCenterPrint)(void);
    void (*pfnSvcKilledMonster)(void);
    void (*pfnSvcFoundSecret)(void);
    void (*pfnSvcSpawnStaticSound)(void);
    void (*pfnSvcIntermission)(void);
    void (*pfnSvcFinale)(void);
    void (*pfnSvcCdTrack)(void);
    void (*pfnSvcRestore)(void);
    void (*pfnSvcCutscene)(void);
    void (*pfnSvcWeaponAnim)(void);
    void (*pfnSvcDecalName)(void);
    void (*pfnSvcRoomType)(void);
    void (*pfnSvcAddAngle)(void);
    void (*pfnSvcNewUserMsg)(void);
    void (*pfnSvcPacketEntites)(void);
    void (*pfnSvcDeltaPacketEntites)(void);
    void (*pfnSvcChoke)(void);
    void (*pfnSvcResourceList)(void);
    void (*pfnSvcNewMoveVars)(void);
    void (*pfnSvcResourceRequest)(void);
    void (*pfnSvcCustomization)(void);
    void (*pfnSvcCrosshairAngle)(void);
    void (*pfnSvcSoundFade)(void);
    void (*pfnSvcFileTxferFailed)(void);
    void (*pfnSvcHltv)(void);
    void (*pfnSvcDirector)(void);
    void (*pfnSvcVoiceInit)(void);
    void (*pfnSvcVoiceData)(void);
    void (*pfnSvcSendExtraInfo)(void);
    void (*pfnSvcTimeScale)(void);
    void (*pfnSvcResourceLocation)(void);
    void (*pfnSvcSendCvarValue)(void);
    void (*pfnSvcSendCvarValue2)(void);
} cl_enginemessages_t;

#define SVC_MSG_COUNT (sizeof(cl_enginemessages_t) / sizeof(void *))

struct UserMessage
{
    int messageId;
    int messageLen;
    char messageName[16];
    UserMessage *nextMessage;
};

struct CommandLink
{
    CommandLink *nextCommand;
    char *commandName;
    void (*handler)(void);
    int addedByMod;
};

class EngineHooks
{
public:
    static void PatchEngine(void);
    static void UnpatchEngine(void);
    static void OnHudFrame(void);

    static void HookSvcMessages(cl_enginemessages_t *pEngineMessages);
    static void UnhookSvcMessages(cl_enginemessages_t *pEngineMessages);

    static void **EngineBuf(void)     { return s_engineBuf; }
    static int *EngineBufSize(void) { return s_engineBufSize; }
    static int *EngineReadPos(void) { return s_engineReadPos; }
    static UserMessage **UserMessages(void) { return s_userMessages; }
    static SvcHandler  *SvcArray(void)     { return s_svcArray; }

private:
    static void FindSvcArray(void);
    static void FindMsgBuf(void);
    static void FindUserMessageList(void);

    static size_t ConvertHexString(const char *src, unsigned char *out, size_t bufSize);
    static size_t MemoryFindForward(size_t start, size_t end, const unsigned char *pattern, const unsigned char *mask, size_t len);
    static size_t MemoryFindForward(size_t start, size_t end, const char *pattern, const char *mask);
    static size_t MemoryFindBackward(size_t start, size_t end, const unsigned char *pattern, const unsigned char *mask, size_t len);
    static uint32_t HookDWord(uintptr_t origAddr, uint32_t newDWord);

    static void PlatformInit(void);
    static void PlatformShutdown(void);

    static void **s_engineBuf;
    static int *s_engineBufSize;
    static int *s_engineReadPos;
    static UserMessage **s_userMessages;
    static SvcHandler *s_svcArray;

    static bool s_patchStatusPrinted;
    static char s_patchErrors[4096];

#ifdef _WIN32
    static size_t s_engineModuleBase;
    static size_t s_engineModuleEnd;
#else
    static void *s_engineModule;
    static void *ResolveSymbol(void *handle, const char *symbol);
#endif
};

#endif // ENGINE_HOOKS_H