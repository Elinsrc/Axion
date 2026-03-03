#include "build.h"

#include "hud.h"
#include "engine_hooks.h"
#include "parsemsg.h"
#include "cl_util.h"

#if XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <link.h>
#include <sys/stat.h>
#include <elf.h>
#endif

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#if XASH_LINUX
#define _stricmp strcasecmp
#endif

#define MAX_PATTERN 128

void **EngineHooks::s_engineBuf = 0;
int *EngineHooks::s_engineBufSize = 0;
int *EngineHooks::s_engineReadPos = 0;
UserMessage **EngineHooks::s_userMessages = 0;
SvcHandler *EngineHooks::s_svcArray = 0;
bool EngineHooks::s_patchStatusPrinted = false;
char EngineHooks::s_patchErrors[4096] = {};

#if XASH_WIN32
size_t EngineHooks::s_engineModuleBase = 0;
size_t EngineHooks::s_engineModuleEnd = 0;
#else
void *EngineHooks::s_engineModule = nullptr;
#endif

size_t EngineHooks::ConvertHexString(const char *src, unsigned char *out, size_t bufSize)
{
    unsigned char *in = (unsigned char *)src;
    unsigned char *o = out;
    unsigned char *end = out + bufSize;
    bool low = false;
    uint8_t byte = 0;
    while (*in && o < end)
    {
        if (*in >= '0' && *in <= '9')      
            byte |= *in - '0';
        else if (*in >= 'A' && *in <= 'F') 
            byte |= *in - 'A' + 10;
        else if (*in >= 'a' && *in <= 'f') 
            byte |= *in - 'a' + 10;
        else if (*in == ' ') 
        { 
            in++; 
            continue; 
        }
        
        if (!low) 
        { 
            byte <<= 4; 
            in++; 
            low = true; 
            continue; 
        }
         
        low = false;
        *o++ = byte;
        byte = 0;
        in++;
    }
    return o - out;
}

size_t EngineHooks::MemoryFindForward(size_t start, size_t end, const unsigned char *pattern, const unsigned char *mask, size_t pattern_len)
{
    if (start > end) { size_t t = end; end = start; start = t; }
    unsigned char *cend = (unsigned char *)(end - pattern_len + 1);
    unsigned char *cur = (unsigned char *)start;
    while (cur < cend)
    {
        size_t i;
        for (i = 0; i < pattern_len; i++)
        {
            if (mask)
            {
                if ((cur[i] & mask[i]) != (pattern[i] & mask[i])) break;
            }
            else
            {
                if (cur[i] != pattern[i]) break;
            }
        }
        if (i == pattern_len) return (size_t)cur;
        cur++;
    }
    return 0;
}

size_t EngineHooks::MemoryFindForward(size_t start, size_t end, const char *pattern, const char *mask)
{
    unsigned char p[MAX_PATTERN], m[MAX_PATTERN];
    size_t pl = ConvertHexString(pattern, p, sizeof(p));
    size_t ml = mask ? ConvertHexString(mask, m, sizeof(m)) : 0;
    return MemoryFindForward(start, end, p, mask ? m : NULL, pl >= ml ? pl : ml);
}

size_t EngineHooks::MemoryFindBackward(size_t start, size_t end,
    const unsigned char *pattern, const unsigned char *mask, size_t pattern_len)
{
    if (start < end) { size_t t = end; end = start; start = t; }
    unsigned char *cend = (unsigned char *)end;
    unsigned char *cur = (unsigned char *)(start - pattern_len);
    while (cur >= cend)
    {
        size_t i;
        for (i = 0; i < pattern_len; i++)
        {
            if (mask)
            {
                if ((cur[i] & mask[i]) != (pattern[i] & mask[i])) break;
            }
            else
            {
                if (cur[i] != pattern[i]) break;
            }
        }
        if (i == pattern_len) return (size_t)cur;
        cur--;
    }
    return 0;
}

uint32_t EngineHooks::HookDWord(uintptr_t origAddr, uint32_t newDWord)
{
    uint32_t origDWord = *(uint32_t *)origAddr;
#if XASH_WIN32
    DWORD oldProtect;
    VirtualProtect((void *)origAddr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint32_t *)origAddr = newDWord;
    VirtualProtect((void *)origAddr, 4, oldProtect, &oldProtect);
#else
    long ps = sysconf(_SC_PAGESIZE);
    void *aligned = (void *)((uintptr_t)origAddr & ~(ps - 1));
    mprotect(aligned, ps, PROT_READ | PROT_WRITE | PROT_EXEC);
    *(uint32_t *)origAddr = newDWord;
    mprotect(aligned, ps, PROT_READ | PROT_EXEC);
#endif
    return origDWord;
}

#if XASH_LINUX

struct sizebuf_t
{
    const char *buffername;
    uint16_t flags;
    uint8_t *data;
    int maxsize;
    int cursize;
};

void *EngineHooks::ResolveSymbol(void *handle, const char *symbol)
{
    void *addr = dlsym(handle, symbol);
    if (addr) return addr;

    struct link_map *dlmap = (struct link_map *)handle;

    int dlfile = open(dlmap->l_name, O_RDONLY);
    if (dlfile == -1) return nullptr;

    struct stat dlstat;
    if (fstat(dlfile, &dlstat) == -1)
    {
        close(dlfile);
        return nullptr;
    }

    Elf32_Ehdr *file_hdr = (Elf32_Ehdr *)mmap(nullptr, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
    close(dlfile);

    if (file_hdr == MAP_FAILED) return nullptr;

    uintptr_t map_base = (uintptr_t)file_hdr;

    if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
    {
        munmap(file_hdr, dlstat.st_size);
        return nullptr;
    }

    Elf32_Shdr *sections = (Elf32_Shdr *)(map_base + file_hdr->e_shoff);
    uint16_t section_count = file_hdr->e_shnum;

    Elf32_Shdr *shstrtab_hdr = &sections[file_hdr->e_shstrndx];
    const char *shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

    Elf32_Shdr *symtab_hdr = nullptr;
    Elf32_Shdr *strtab_hdr = nullptr;

    for (uint16_t i = 0; i < section_count; i++)
    {
        const char *name = shstrtab + sections[i].sh_name;
        if (strcmp(name, ".symtab") == 0)
            symtab_hdr = &sections[i];
        else if (strcmp(name, ".strtab") == 0)
            strtab_hdr = &sections[i];
    }

    if (!symtab_hdr || !strtab_hdr)
    {
        munmap(file_hdr, dlstat.st_size);
        return nullptr;
    }

    Elf32_Sym *symtab = (Elf32_Sym *)(map_base + symtab_hdr->sh_offset);
    const char *strtab = (const char *)(map_base + strtab_hdr->sh_offset);
    uint32_t symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

    void *result = nullptr;

    for (uint32_t i = 0; i < symbol_count; i++)
    {
        Elf32_Sym &sym = symtab[i];
        unsigned char sym_type = ELF32_ST_TYPE(sym.st_info);

        if (sym.st_shndx == SHN_UNDEF)
            continue;
        if (sym_type != STT_FUNC && sym_type != STT_OBJECT)
            continue;

        const char *sym_name = strtab + sym.st_name;
        if (strcmp(symbol, sym_name) == 0)
        {
            result = (void *)(dlmap->l_addr + sym.st_value);
            break;
        }
    }

    munmap(file_hdr, dlstat.st_size);
    return result;
}

void EngineHooks::PlatformInit(void)
{
    s_engineModule = dlopen("hw.so", RTLD_NOW | RTLD_NOLOAD);

    if (!s_engineModule)
    {
        snprintf(s_patchErrors + strlen(s_patchErrors), sizeof(s_patchErrors) - strlen(s_patchErrors), "EngineHooks: failed to load module: %s\n", dlerror());
        return;
    }

    dlclose(s_engineModule);
}

void EngineHooks::PlatformShutdown(void)
{
    s_engineModule = nullptr;
}

void EngineHooks::FindMsgBuf(void)
{
    if (!s_engineModule) 
        return;

    sizebuf_t *buf = (sizebuf_t *)ResolveSymbol(s_engineModule, "net_message");
    if (!buf)
    {
        strncat(s_patchErrors, "EngineHooks: net_message not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
        return;
    }

    s_engineBuf = (void **)&buf->data;
    s_engineBufSize = &buf->cursize;

    s_engineReadPos = (int *)ResolveSymbol(s_engineModule, "msg_readcount");
    if (!s_engineReadPos)
    {
        strncat(s_patchErrors, "EngineHooks: msg_readcount not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
        s_engineBuf = 0;
        s_engineBufSize = 0;
    }
}

void EngineHooks::FindSvcArray(void)
{
    if (!s_engineModule) return;

    s_svcArray = (SvcHandler *)ResolveSymbol(s_engineModule, "cl_parsefuncs");
    if (!s_svcArray)
    {
        strncat(s_patchErrors, "EngineHooks: cl_parsefuncs not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
        return;
    }

    if (strcmp(s_svcArray[0].pszName, "svc_bad") != 0 ||
        strcmp(s_svcArray[1].pszName, "svc_nop") != 0 ||
        strcmp(s_svcArray[2].pszName, "svc_disconnect") != 0)
    {
        strncat(s_patchErrors, "EngineHooks: SvcArray verification failed.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
        s_svcArray = nullptr;
    }
}

void EngineHooks::FindUserMessageList(void)
{
    if (!s_engineModule) 
        return;

    s_userMessages = (UserMessage **)ResolveSymbol(s_engineModule, "gClientUserMsgs");
    if (!s_userMessages)
    {
        strncat(s_patchErrors, "EngineHooks: gClientUserMsgs not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
    }
}

#else

void EngineHooks::PlatformInit(void)
{
    auto tryModule = [](const char *name, size_t &base, size_t &end) -> bool {
        HANDLE hProcess = GetCurrentProcess();
        HMODULE hModule = GetModuleHandle(name);
        if (!hProcess || !hModule) return false;
        MODULEINFO mi;
        GetModuleInformation(hProcess, hModule, &mi, sizeof(mi));
        base = (size_t)mi.lpBaseOfDll;
        end = (size_t)mi.lpBaseOfDll + (size_t)mi.SizeOfImage - 1;
        return true;
    };

    if (tryModule("hw.dll", s_engineModuleBase, s_engineModuleEnd) ||
        tryModule("sw.dll", s_engineModuleBase, s_engineModuleEnd) ||
        tryModule("hl.exe", s_engineModuleBase, s_engineModuleEnd))
        return;

    char moduleName[256];
    if (!GetModuleFileName(NULL, moduleName, sizeof(moduleName)))
        return;
    char *baseName = strrchr(moduleName, '\\');
    if (!baseName) return;
    baseName++;
    tryModule(baseName, s_engineModuleBase, s_engineModuleEnd);
}

void EngineHooks::PlatformShutdown(void)
{
    s_engineModuleBase = 0;
    s_engineModuleEnd = 0;
}

void EngineHooks::FindMsgBuf(void)
{
    if (!s_engineModuleBase) 
        return;

    const char pat1[] = "8B0DC45E2311 8D5101 3B1530F52211 7E0E C705C05E231101000000 83C8FF C3 A128F52211 0FBE0408 8915C45E2311 C3";
    const char msk1[] = "FFFF00000000 FFFFFF FFFF00000000 FFFF FFFF00000000FFFFFFFF FFFFFF FF FF00000000 FFFFFFFF FFFF00000000 FF";

    size_t addr = MemoryFindForward(s_engineModuleBase, s_engineModuleEnd, pat1, msk1);
    if (addr)
    {
        if ((int *)*(size_t *)(addr + 2) != (int *)*(size_t *)(addr + 42))
        {
            strncat(s_patchErrors, "EngineHooks: ReadPos mismatch (anniversary).\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
            return;
        }
        s_engineReadPos = (int *)*(size_t *)(addr + 2);
        s_engineBuf = (void **)*(size_t *)(addr + 32);
        s_engineBufSize = (int *)(*(size_t *)(addr + 32) + 8);
        return;
    }

    const char pat2[] = "A1283DCD02 8B1530E67602 8D4801 3BCA 7E0E C7052C3DCD0201000000 83C8FFC3 8B1528E67602 890D283DCD02";
    const char msk2[] = "FF00000000 FFFF00000000 FFFFFF FFFF FF00 FFFF0000000000000000 FFFFFFFF FFFF00000000 FFFF00000000";

    addr = MemoryFindForward(s_engineModuleBase, s_engineModuleEnd, pat2, msk2);
    if (addr)
    {
        if ((int *)*(size_t *)(addr + 1) != (int *)*(size_t *)(addr + 40))
        {
            strncat(s_patchErrors, "EngineHooks: ReadPos mismatch (old).\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
            return;
        }
        s_engineReadPos = (int *)*(size_t *)(addr + 1);
        s_engineBuf = (void **)*(size_t *)(addr + 34);
        s_engineBufSize = (int *)*(size_t *)(addr + 7);
        return;
    }

    strncat(s_patchErrors, "EngineHooks: buffer variables not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
}

void EngineHooks::FindSvcArray(void)
{
    if (!s_engineModuleBase) 
        return;

    const unsigned char d1[] = "svc_bad";
    size_t svc_bad = MemoryFindForward(s_engineModuleBase, s_engineModuleEnd, d1, NULL, 7);
    if (!svc_bad) 
    { 
        strncat(s_patchErrors, "EngineHooks: svc_bad not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1); 
        return; 
    }

    const unsigned char d2[] = "svc_nop";
    size_t svc_nop = MemoryFindForward(svc_bad, s_engineModuleEnd, d2, NULL, 7);
    if (!svc_nop) 
    { 
        strncat(s_patchErrors, "EngineHooks: svc_nop not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1); 
        return; 
    }

    const unsigned char d3[] = "svc_disconnect";
    size_t svc_disconnect = MemoryFindForward(svc_nop, s_engineModuleEnd, d3, NULL, 14);
    if (!svc_disconnect) 
    { 
        strncat(s_patchErrors, "EngineHooks: svc_disconnect not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1); 
        return; 
    }

    unsigned char data4[12 * 3 + 4];
    memset(data4, 0, sizeof(data4));
    *((uint32_t *)data4 + 0) = 0;
    *((uint32_t *)data4 + 1) = (uint32_t)svc_bad;
    *((uint32_t *)data4 + 3) = 1;
    *((uint32_t *)data4 + 4) = (uint32_t)svc_nop;
    *((uint32_t *)data4 + 6) = 2;
    *((uint32_t *)data4 + 7) = (uint32_t)svc_disconnect;
    *((uint32_t *)data4 + 9) = 3;

    const char mask4[] = "FFFFFFFFFFFFFFFF00000000 FFFFFFFFFFFFFFFF00000000 FFFFFFFFFFFFFFFF00000000 FFFFFFFF";
    unsigned char m[MAX_PATTERN];
    ConvertHexString(mask4, m, sizeof(m));

    size_t tableAddr = MemoryFindBackward(svc_bad, s_engineModuleBase, data4, m, sizeof(data4) - 1);
    if (!tableAddr)
        tableAddr = MemoryFindForward(svc_bad, s_engineModuleEnd, data4, m, sizeof(data4) - 1);

    if (!tableAddr)
    {
        strncat(s_patchErrors, "EngineHooks: SvcMessagesTable not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
        return;
    }

    s_svcArray = (SvcHandler *)tableAddr;

    if (strcmp(s_svcArray[0].pszName, "svc_bad") != 0 ||
        strcmp(s_svcArray[1].pszName, "svc_nop") != 0 ||
        strcmp(s_svcArray[2].pszName, "svc_disconnect") != 0)
    {
        s_svcArray = nullptr;
        strncat(s_patchErrors, "EngineHooks: SvcArray verification failed.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
    }
}

void EngineHooks::FindUserMessageList(void)
{
    if (!s_engineModuleBase)
        return;

    const char pat1[] = "81FF00010000 0F8D32010000 8B35B8DF3210 85F6 740C 90 393E 7412";
    const char msk1[] = "FFFFFFFFFFFF FFFF0000FFFF FFFF00000000 FFFF FF00 FF FFFF FF00";

    size_t addr = MemoryFindForward(s_engineModuleBase, s_engineModuleEnd, pat1, msk1);
    if (addr)
    {
        s_userMessages = (UserMessage **)*(size_t *)(addr + 14);
        return;
    }

    const char pat2[] = "81FB00010000 0F8D1B010000 8B3574FF6C03 85F6740B";
    const char msk2[] = "FFFFFFFFFFFF FFFF0000FFFF FFFF00000000 FFFFFF00";

    addr = MemoryFindForward(s_engineModuleBase, s_engineModuleEnd, pat2, msk2);
    if (addr)
    {
        s_userMessages = (UserMessage **)*(size_t *)(addr + 14);
        return;
    }

    strncat(s_patchErrors, "EngineHooks: UserMessages not found.\n", sizeof(s_patchErrors) - strlen(s_patchErrors) - 1);
}

#endif

void EngineHooks::HookSvcMessages(cl_enginemessages_t *pEngineMessages)
{
    if (!s_svcArray)
        return;

    int count = SVC_MSG_COUNT;
    void (**handlers)(void) = (void (**)(void))pEngineMessages;

    for (int i = 0; i < count; i++)
    {
        if (!handlers[i])
            continue;

    
        uintptr_t addr = (uintptr_t)&s_svcArray[i].pfnParse;
        uint32_t oldVal = HookDWord(addr, (uint32_t)(uintptr_t)handlers[i]);
        handlers[i] = (void (*)(void))(uintptr_t)oldVal;
    }
}

void EngineHooks::UnhookSvcMessages(cl_enginemessages_t *pEngineMessages)
{
    HookSvcMessages(pEngineMessages);
}

void EngineHooks::PatchEngine(void)
{
    PlatformInit();

    FindSvcArray();
    FindMsgBuf();
    FindUserMessageList();
}

void EngineHooks::UnpatchEngine(void)
{
    s_svcArray = 0;
    s_userMessages = 0;
    s_engineReadPos = 0;
    s_engineBufSize = 0;
    s_engineBuf = 0;

    PlatformShutdown();
}

void EngineHooks::OnHudFrame(void)
{
    if (s_patchStatusPrinted) 
        return;
    
    s_patchStatusPrinted = true;

    if (s_patchErrors[0] != 0)
    {
        gEngfuncs.pfnConsolePrint(s_patchErrors);
    }

    char status[256];
    snprintf(status, sizeof(status),
        "SvcArray: %s | Buffer: %s | UserMsg: %s\n",
        s_svcArray ? "OK" : "FAIL",
        (s_engineBuf && s_engineBufSize && s_engineReadPos) ? "OK" : "FAIL",
        s_userMessages ? "OK" : "FAIL");
    gEngfuncs.pfnConsolePrint(status);
}