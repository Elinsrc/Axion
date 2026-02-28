/***
*
*	Copyright (c) 2012, AGHL.RU. All rights reserved.
*
****/
//
// Svc_messages.cpp
//
// Engine messages handlers
//

#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>

#include "build.h"

#include "hud.h"
#include "engine_hooks.h"
#include "parsemsg.h"
#include "cl_util.h"
#include "svc_messages.h"
#include "imgui_viewport.h"
#include "ui_ScorePanel.h"

#if XASH_LINUX
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

#define MAX_CMD_LINE 1024

cl_enginemessages_t pEngineMessages;
cvar_t *m_pCvarClLogMessages = 0;
cvar_t *m_pCvarClProtectLog = 0;
cvar_t *m_pCvarClProtectBlock = 0;
cvar_t *m_pCvarClProtectAllow = 0;
cvar_t *m_pCvarClProtectBlockCvar = 0;
char com_token[1024];

enum MatchType
{
	MATCH_EXACT,       
	MATCH_PREFIX, 
	MATCH_PREFIX_MIN1
};

struct BlockEntry
{
	const char *name;
	MatchType type;
};

static const BlockEntry s_blockedCommands[] =
{
	{ "exit",       MATCH_EXACT },
	{ "quit",       MATCH_EXACT },
	{ "bind",       MATCH_EXACT },
	{ "unbind",     MATCH_EXACT },
	{ "unbindall",  MATCH_EXACT },
	{ "kill",       MATCH_EXACT },
	{ "exec",       MATCH_EXACT },
	{ "alias",      MATCH_EXACT },
	{ "clear",      MATCH_EXACT },
	{ "motdfile",   MATCH_EXACT },
	{ "motd_write", MATCH_EXACT },
	{ "writecfg",   MATCH_EXACT },
	{ "cd",         MATCH_EXACT },
	{ "developer",  MATCH_EXACT },
	{ "fps",        MATCH_PREFIX_MIN1 },
	{ "rcon",       MATCH_PREFIX },     
};
static const int s_blockedCommandsCount = sizeof(s_blockedCommands) / sizeof(s_blockedCommands[0]);

static const BlockEntry s_blockedCvars[] =
{
	{ "rcon", MATCH_PREFIX },
};
static const int s_blockedCvarsCount = sizeof(s_blockedCvars) / sizeof(s_blockedCvars[0]);

static const char *s_blockListDesc = "exit, quit, bind, unbind, unbindall, kill, exec, alias, clear, motdfile, motd_write, writecfg, cd, developer, fps*, rcon*";
static const char *s_blockListCvarDesc = "rcon*";

static bool MatchesEntry(const char *str, const BlockEntry &entry)
{
	int nameLen = strlen(entry.name);
	switch (entry.type)
	{
	case MATCH_EXACT:
		return (_stricmp(str, entry.name) == 0);
	case MATCH_PREFIX:
		return (_strnicmp(str, entry.name, nameLen) == 0);
	case MATCH_PREFIX_MIN1:
		return (_strnicmp(str, entry.name, nameLen) == 0 && (int)strlen(str) > nameLen);
	}
	return false;
}

static bool IsInBlockList(const char *str, const BlockEntry *list, int count)
{
	for (int i = 0; i < count; i++)
	{
		if (MatchesEntry(str, list[i]))
			return true;
	}
	return false;
}

static bool IsInCvarList(const char *str, const char *cvarValue)
{
	if (!cvarValue || cvarValue[0] == 0)
		return false;

	char buf[1024];
	strncpy(buf, cvarValue, sizeof(buf));
	buf[sizeof(buf) - 1] = 0;

	char *token = strtok(buf, ",; \t");
	while (token)
	{
		int len = strlen(token);
		if (len > 0)
		{
			MatchType type = MATCH_EXACT;
			if (token[len - 1] == '*')
			{
				type = MATCH_PREFIX;
				token[len - 1] = 0;
				len--;
			}
			else if (token[len - 1] == '+')
			{
				type = MATCH_PREFIX_MIN1;
				token[len - 1] = 0;
				len--;
			}

			if (len > 0)
			{
				BlockEntry entry = { token, type };
				if (MatchesEntry(str, entry))
					return true;
			}
		}
		token = strtok(NULL, ",; \t");
	}
	return false;
}

bool IsCommandGood(const char *str)
{
	// Parse command into token
	char *ret = gEngfuncs.COM_ParseFile((char *)str, com_token);
	if (ret == NULL || com_token[0] == 0)
		return true; // no tokens

	// Block our filter from hacking
	if (!_stricmp(com_token, m_pCvarClProtectLog->name)) return false;
	if (!_stricmp(com_token, m_pCvarClProtectBlock->name)) return false;
	if (!_stricmp(com_token, m_pCvarClProtectAllow->name)) return false;
	if (!_stricmp(com_token, m_pCvarClProtectBlockCvar->name)) return false;

	// Check command name against block lists and whole command line against allow list
	bool blocked = IsInBlockList(com_token, s_blockedCommands, s_blockedCommandsCount) || IsInCvarList(com_token, m_pCvarClProtectBlock->string);
	bool allowed = IsInCvarList(str, m_pCvarClProtectAllow->string);

	if (blocked && !allowed)
		return false;

	return true;
}

bool IsCvarGood(const char *str)
{
	if (str[0] == 0)
		return true; // no cvar

	// Block our filter from getting
	if (!_stricmp(str, m_pCvarClProtectLog->name)) return false;
	if (!_stricmp(str, m_pCvarClProtectBlock->name)) return false;
	if (!_stricmp(str, m_pCvarClProtectAllow->name)) return false;
	if (!_stricmp(str, m_pCvarClProtectBlockCvar->name)) return false;

	// Check cvar name against block lists
	if (IsInBlockList(str, s_blockedCvars, s_blockedCvarsCount) || IsInCvarList(str, m_pCvarClProtectBlockCvar->string))
		return false;

	return true;
}

bool SanitizeCommands(char *str)
{
	bool changed = false;
	char *text = str;
	char command[MAX_CMD_LINE];
	int i, quotes;
	int len = strlen(str);

	// Split string into commands and check them separately
	while (text[0] != 0)
	{
		// Find \n or ; splitter
		quotes = 0;
		for (i = 0; i < len; i++)
		{
			if (text[i] == '"') quotes++;
			if (!(quotes & 1) && text[i] == ';')
				break;
			if (text[i] == '\n')
				break;
		}
		if (i >= MAX_CMD_LINE)
			i = MAX_CMD_LINE; // game engine behaviour
		strncpy(command, text, i);
		command[i] = 0;

		// Check command
		bool isGood = IsCommandGood(command);

		// Log command
		int log = (int)m_pCvarClProtectLog->value;
		if (log > 0)
		{
			/*
			0  - log (1) or not (0) to console
			1  - log to common (1) or to developer (0) console
			2  - log all (1) or only bad (0) to console
			8  - log (1) or not (0) to file
			9  - log all (1) or only bad (0) to file
			15 - log full command (1) or only name (0)
			*/

			// Full command or only command name
			char *c = (log & (1 << 15)) ? command : com_token;

			// Log destination
			if (log & (1 << 0)) // console
			{
				// Log only bad or all
				if (!isGood || (log & (1 << 2)))
				{
					// Action
					const char *a = isGood ? "Server executed command: %s\n" : "Server tried to execute bad command: %s\n";
					// Common or developer console
					void (*m)(const char *, ...) = (log & (1 << 1)) ? gEngfuncs.Con_Printf : gEngfuncs.Con_DPrintf;
					// Log
					m(a, c);
				}
			}
			if (log & (1 << 8)) // file
			{
				// Log only bad or all
				if (!isGood || (log & (1 << 9)))
				{
					FILE *f = fopen("svc_protect.log", "a+");
					if (f != NULL)
					{
						// The time
						time_t now;
						time(&now);
						struct tm *current = localtime(&now);
						if (current != NULL)
							fprintf(f, "[%04i-%02i-%02i %02i:%02i:%02i] ", current->tm_year + 1900, current->tm_mon + 1, current->tm_mday, current->tm_hour, current->tm_min, current->tm_sec);
						// Action
						const char *a = isGood ? "[allowed] " : "[blocked] ";
						fputs(a, f);
						// Command
						fputs(c, f);
						fputs("\n", f);
						fclose(f);
					}
				}
			}
		}

		len -= i;
		if (!isGood)
		{
			// Trash command, but leave the splitter
			strncpy(text, text + i, len);
			text[len] = 0;
			text++;
			changed = true;
		}
		else
		{
			text += i + 1;
		}
	}
	return changed;
}

static bool InfoValueForKey(const char *infostring, const char *key, char *out, size_t outSize)
{
	out[0] = 0;

	if (!infostring || !key || !key[0])
		return false;

	const char *s = infostring;

	while (s && *s)
	{
		if (*s == '\\') s++;

		char currentKey[256];
		int i = 0;
		while (*s && *s != '\\' && i < 255)
			currentKey[i++] = *s++;
		currentKey[i] = 0;

		if (*s == '\\') s++;

		char currentValue[256];
		i = 0;
		while (*s && *s != '\\' && i < 255)
			currentValue[i++] = *s++;
		currentValue[i] = 0;

		if (!_stricmp(currentKey, key))
		{
			strncpy(out, currentValue, outSize - 1);
			out[outSize - 1] = 0;
			return true;
		}
	}

	return false;
}

static bool ConvertSteam64ToSteamId(const char *steam64str, char *out, size_t outSize)
{
	if (!steam64str || !steam64str[0])
		return false;

	for (const char *p = steam64str; *p; p++)
	{
		if (*p < '0' || *p > '9')
			return false;
	}

	unsigned long long steam64 = 0;
	for (const char *p = steam64str; *p; p++)
		steam64 = steam64 * 10 + (*p - '0');

	const unsigned long long BASE = 76561197960265728ULL;

	if (steam64 < BASE)
		return false;

	unsigned long long diff = steam64 - BASE;
	unsigned int x = diff % 2;
	unsigned int y = (unsigned int)(diff / 2);

	snprintf(out, outSize, "0:%u:%u", x, y);
	return true;
}

void SvcPrint(void)
{
	void **engineBuf = EngineHooks::EngineBuf();
	int *engineBufSize = EngineHooks::EngineBufSize();
	int *engineReadPos = EngineHooks::EngineReadPos();

	if (engineBuf && engineBufSize && engineReadPos)
	{
		BEGIN_READ(*engineBuf, *engineBufSize, *engineReadPos);
		char *str = READ_STRING();

		// Timer resync on cvar changes
		if (!strncmp(str, "\"mp_timelimit\" changed to \"", 27) || !strncmp(str, "\"amx_nextmap\" changed to \"", 26))
		{
			gHUD.m_Timer.DoResync();
		}

		// Clear cached steam id for disconnected player
		int len = strlen(str);
		if (len >= 9 && !strcmp(str + len - 9, " dropped\n"))
		{
			str[len - 9] = 0;
			g_ImGuiViewport.GetAllPlayersInfo();
			for (int i = 1; i <= MAX_PLAYERS; i++)
			{
				if (g_PlayerInfoList[i].name != NULL && !strcmp(g_PlayerInfoList[i].name, str))
				{
					g_PlayerSteamId[i][0] = 0;
					break;
				}
			}
		}
	}

	pEngineMessages.pfnSvcPrint();
}

void SvcUpdateUserInfo(void)
{
	void **engineBuf = EngineHooks::EngineBuf();
	int *engineBufSize = EngineHooks::EngineBufSize();
	int *engineReadPos = EngineHooks::EngineReadPos();

	if (engineBuf && engineBufSize && engineReadPos)
	{
		int savedReadPos = *engineReadPos;

		BEGIN_READ(*engineBuf, *engineBufSize, *engineReadPos);
		int slot = READ_BYTE();
		long userid = READ_LONG();
		char *infostring = READ_STRING();

		if (slot >= 0 && slot < MAX_PLAYERS && infostring[0])
		{
			int playerIndex = slot + 1;

			char sid[256];
			InfoValueForKey(infostring, "*sid", sid, sizeof(sid));

			if (sid[0])
			{
				if (!strncmp(sid, "STEAM_", 6) || !strncmp(sid, "VALVE_", 6))
				{
					strncpy(g_PlayerSteamId[playerIndex], sid + 6, MAX_STEAMID);
				}
				else if (sid[0] >= '0' && sid[0] <= '9' && strlen(sid) > 10)
				{
					char converted[64];
					if (ConvertSteam64ToSteamId(sid, converted, sizeof(converted)))
						strncpy(g_PlayerSteamId[playerIndex], converted, MAX_STEAMID);
					else
						strncpy(g_PlayerSteamId[playerIndex], sid, MAX_STEAMID);
				}
				else
				{
					strncpy(g_PlayerSteamId[playerIndex], sid, MAX_STEAMID);
				}
				g_PlayerSteamId[playerIndex][MAX_STEAMID] = 0;
			}
		}

		*engineReadPos = savedReadPos;
	}

	pEngineMessages.pfnSvcUpdateUserInfo();
}

void SvcNewUserMsg(void)
{
	void **engineBuf = EngineHooks::EngineBuf();
	int *engineBufSize = EngineHooks::EngineBufSize();
	int *engineReadPos = EngineHooks::EngineReadPos();
	UserMessage **userMessages = EngineHooks::UserMessages();

	if (engineBuf && engineBufSize && engineReadPos)
	{
		BEGIN_READ(*engineBuf, *engineBufSize, *engineReadPos);
		int id = READ_BYTE();
		int len = READ_BYTE();
		char name[16];
		uint32_t value = READ_LONG();
		memcpy(name, &value, 4);
		value = READ_LONG();
		memcpy(name + 4, &value, 4);
		value = READ_LONG();
		memcpy(name + 8, &value, 4);
		value = READ_LONG();
		memcpy(name + 12, &value, 4);
		name[15] = 0;

		pEngineMessages.pfnSvcNewUserMsg();

		// Log user message to console
		if (m_pCvarClLogMessages && m_pCvarClLogMessages->value)
			gEngfuncs.Con_Printf("User Message: %d, %s, %d\n", id, name, len == 255 ? -1 : len);

		// Fix engine bug that leads to duplicate user message ids in user messages chain
		if (userMessages)
		{
			UserMessage *current = *userMessages;
			while (current != 0)
			{
				if (current->messageId == id && strcmp(current->messageName, name))
					current->messageId = 0;
				current = current->nextMessage;
			}
		}
	}
	else
	{
		pEngineMessages.pfnSvcNewUserMsg();
	}
}

void SvcStuffText(void)
{
	void **engineBuf = EngineHooks::EngineBuf();
	int *engineBufSize = EngineHooks::EngineBufSize();
	int *engineReadPos = EngineHooks::EngineReadPos();

	if (engineBuf && engineBufSize && engineReadPos)
	{
		BEGIN_READ(*engineBuf, *engineBufSize, *engineReadPos);
		char *commands = READ_STRING();

		char str[1024];
		strncpy(str, commands, sizeof(str));
		str[sizeof(str) - 1] = 0;

		if (SanitizeCommands(str))
		{
			// Some commands were removed, put cleaned command line back to stream
			int l1 = strlen(commands);
			int l2 = strlen(str);
			if (l2 == 0)
			{
				// Suppress commands if they are all removed
				*engineReadPos += l1 + 1;
				return;
			}
			int diff = l1 - l2;
			strncpy(commands + diff, str, l2);
			*engineReadPos += diff;
		}
	}

	pEngineMessages.pfnSvcStuffText();
}

void SvcSendCvarValue(void)
{
	void **engineBuf = EngineHooks::EngineBuf();
	int *engineBufSize = EngineHooks::EngineBufSize();
	int *engineReadPos = EngineHooks::EngineReadPos();

	if (engineBuf && engineBufSize && engineReadPos)
	{
		BEGIN_READ(*engineBuf, *engineBufSize, *engineReadPos);
		char *cvar = READ_STRING();

		// Check cvar
		if (!IsCvarGood(cvar))
		{
			gEngfuncs.Con_DPrintf("Server tried to request blocked cvar: %s\n", cvar);
			*engineReadPos += strlen(cvar) + 1;
			// Silently block the request
			return;
		}
	}

	pEngineMessages.pfnSvcSendCvarValue();
}

void SvcSendCvarValue2(void)
{
	void **engineBuf = EngineHooks::EngineBuf();
	int *engineBufSize = EngineHooks::EngineBufSize();
	int *engineReadPos = EngineHooks::EngineReadPos();

	if (engineBuf && engineBufSize && engineReadPos)
	{
		BEGIN_READ(*engineBuf, *engineBufSize, *engineReadPos);
		long l = READ_LONG();
		char *cvar = READ_STRING();

		// Check cvar
		if (!IsCvarGood(cvar))
		{
			gEngfuncs.Con_DPrintf("Server tried to request blocked cvar: %s\n", cvar);
			*engineReadPos += 4 + strlen(cvar) + 1;
			// Silently block the request
			return;
		}
	}

	pEngineMessages.pfnSvcSendCvarValue2();
}

void HookSvcMessages(void)
{
	memset(&pEngineMessages, 0, sizeof(cl_enginemessages_t));
	pEngineMessages.pfnSvcPrint = SvcPrint;
	pEngineMessages.pfnSvcUpdateUserInfo = SvcUpdateUserInfo;
	pEngineMessages.pfnSvcNewUserMsg = SvcNewUserMsg;
	pEngineMessages.pfnSvcStuffText = SvcStuffText;
	pEngineMessages.pfnSvcSendCvarValue = SvcSendCvarValue;
	pEngineMessages.pfnSvcSendCvarValue2 = SvcSendCvarValue2;
	EngineHooks::HookSvcMessages(&pEngineMessages);
}

void UnHookSvcMessages(void)
{
	EngineHooks::UnhookSvcMessages(&pEngineMessages);
}

void DumpUserMessages(void)
{
	UserMessage **msgs = EngineHooks::UserMessages();
	if (msgs)
	{
		UserMessage *current = *msgs;
		while (current != 0)
		{
			gEngfuncs.Con_Printf("User Message: %d, %s, %d\n", current->messageId, current->messageName, current->messageLen);
			current = current->nextMessage;
		}
	}
	else
	{
		gEngfuncs.Con_Printf("Can't dump user messages: engine wasn't hooked properly.\n");
	}
}

void ProtectHelp(void)
{
	gEngfuncs.Con_Printf("cl_protect_* cvars are used to protect from slowhacking.\n");
	gEngfuncs.Con_Printf("By default following commands are blocked:\n");
	gEngfuncs.Con_Printf("  %s\n", s_blockListDesc);
	gEngfuncs.Con_Printf("By default following cvar requests are blocked:\n");
	gEngfuncs.Con_Printf("  %s\n", s_blockListCvarDesc);
	gEngfuncs.Con_Printf("\nUser cvar format: \"word1,word2,prefix*,prefix+\"\n");
	gEngfuncs.Con_Printf("  word  - exact match\n");
	gEngfuncs.Con_Printf("  word* - prefix match (0+ chars after)\n");
	gEngfuncs.Con_Printf("  word+ - prefix match (1+ chars after)\n");
}

void SvcMessagesInit(void)
{
	m_pCvarClLogMessages = gEngfuncs.pfnRegisterVariable("cl_messages_log", "0", FCVAR_ARCHIVE);
	gEngfuncs.pfnAddCommand("cl_messages_dump", DumpUserMessages);

	m_pCvarClProtectLog = gEngfuncs.pfnRegisterVariable("cl_protect_log", "1", FCVAR_ARCHIVE);
	m_pCvarClProtectBlock = gEngfuncs.pfnRegisterVariable("cl_protect_block", "", FCVAR_ARCHIVE);
	m_pCvarClProtectAllow = gEngfuncs.pfnRegisterVariable("cl_protect_allow", "", FCVAR_ARCHIVE);
	m_pCvarClProtectBlockCvar = gEngfuncs.pfnRegisterVariable("cl_protect_block_cvar", "", FCVAR_ARCHIVE);
	gEngfuncs.pfnAddCommand("cl_protect", ProtectHelp);
}