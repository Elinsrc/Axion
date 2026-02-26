/***
*
*   Copyright © 2012, AGHL.RU, All rights reserved.
*
*   Purpose: Network communications.
*
****/


#include "net.h"

#if XASH_WIN32
#include <winsani_in.h>
#include <windows.h>
#include <winsani_out.h>
#include <time.h>

bool g_bInitialised = false;
bool g_bFailedInitialization = false;

void WinsockInit()
{
	g_bInitialised = true;

	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1, 1);
	if (WSAStartup(wVersionRequested, &wsaData))
	{
		g_bFailedInitialization = true;
		return;
	}

	// Confirm that Windows Sockets DLL supports 1.1
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		g_bFailedInitialization = true;
		return;
	}
}
#else
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <ctime>
#endif

#define SocketConvert(s) (s)

int NetSendReceiveUdp(const char *addr, int port, const char *sendbuf, int len, char *recvbuf, int size)
{
	unsigned long ulAddr = inet_addr(addr);
	return NetSendReceiveUdp(ulAddr, htons((unsigned short)port), sendbuf, len, recvbuf, size);
}

int NetSendReceiveUdp(unsigned long sin_addr, int sin_port, const char *sendbuf, int len, char *recvbuf, int size)
{
#if XASH_WIN32
	if (!g_bInitialised)
		WinsockInit();
	if (g_bFailedInitialization)
		return -1;
#endif

	// Create a socket for sending data
#if XASH_WIN32
	SOCKET SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
	int SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif

	struct sockaddr_in addr, fromaddr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = sin_addr;
	addr.sin_port = sin_port;

	// Send request
	sendto(SendSocket, sendbuf, len, 0, (struct sockaddr *)&addr, sizeof(addr));

	time_t timeout = time(NULL) + 3;

	// Receive response
	int res;
	unsigned long nonzero = 1;

#if XASH_WIN32
	ioctlsocket(SendSocket, FIONBIO, &nonzero);
	SOCKET maxfd;
#else
	ioctl(SendSocket, FIONBIO, &nonzero);
	int maxfd;
#endif

	fd_set rfd;
	fd_set efd;
	struct timeval tv;

	while (true)
	{
		FD_ZERO(&rfd);
		FD_ZERO(&efd);
		FD_SET(SendSocket, &rfd);
		FD_SET(SendSocket, &efd);

		maxfd = SendSocket;

		tv.tv_sec  = 1;
		tv.tv_usec = 0;

		res = select(maxfd + 1, &rfd, NULL, &efd, &tv);

		// Check for error on socket
#if XASH_WIN32
		if (res == SOCKET_ERROR || FD_ISSET(SendSocket, &efd))
			break;
#else
		if (res == -1 || FD_ISSET(SendSocket, &efd))
			break;
#endif

		time_t current = time(NULL);

		if (res == 0)
		{
			if (current >= timeout)
				break;
			continue;
		}

		socklen_t fromaddrlen = sizeof(fromaddr);

		res = recvfrom(SendSocket, recvbuf, size, 0,
					   (struct sockaddr *)&fromaddr, &fromaddrlen);

#if XASH_WIN32
		if (res == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
			break;
#else
		if (res == -1)
			break;
#endif

		// Check address from which data came
		if (res >= 0 &&
			addr.sin_addr.s_addr == fromaddr.sin_addr.s_addr &&
			addr.sin_port == fromaddr.sin_port)
		{
#if XASH_WIN32
			closesocket(SendSocket);
#else
			close(SendSocket);
#endif
			return res;
		}

		if (current >= timeout)
			break;
	}

#if XASH_WIN32
	closesocket(SendSocket);
#else
	close(SendSocket);
#endif

	return -1;
}

int NetSendUdp(unsigned long sin_addr, int sin_port, const char *sendbuf, int len, NetSocket *s)
{
#if _WIN32
	if (!g_bInitialised)
		WinsockInit();
	if (g_bFailedInitialization)
		return -1;
#endif

	// Create or get a socket for sending data
#if XASH_WIN32
	SOCKET s1;
#else
	int s1;
#endif

	if (s && *s)
		s1 = SocketConvert(*s);
	else
		s1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = sin_addr;
	addr.sin_port = sin_port;

	// Send buffer
	int res = sendto(s1, sendbuf, len, 0, (struct sockaddr *)&addr, sizeof(addr));

	// Return socket if send was successful
#if XASH_WIN32
	if (res != SOCKET_ERROR && s)
#else
	if (res != -1 && s)
#endif
		*s = SocketConvert(s1);
	else
	{
#if XASH_WIN32
		closesocket(s1);
#else
		close(s1);
#endif
	}

	return res;
}

int NetReceiveUdp(unsigned long sin_addr, int sin_port, char *recvbuf, int size, NetSocket ns)
{
#if XASH_WIN32
	if (!g_bInitialised)
		WinsockInit();
	if (g_bFailedInitialization)
		return -1;
#endif

	if (!ns)
		return -1;

	int s = SocketConvert(ns);

	struct sockaddr_in addr, fromaddr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = sin_addr;
	addr.sin_port = sin_port;

	int res;
	unsigned long nonzero = 1;

#if XASH_WIN32
	ioctlsocket(s, FIONBIO, &nonzero);
	SOCKET maxfd;
#else
	ioctl(s, FIONBIO, &nonzero);
	int maxfd;
#endif

	fd_set rfd;
	fd_set efd;
	struct timeval tv;

	FD_ZERO(&rfd);
	FD_ZERO(&efd);
	FD_SET(s, &rfd);
	FD_SET(s, &efd);

	maxfd = s;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	res = select(maxfd + 1, &rfd, NULL, &efd, &tv);

#if XASH_WIN32
	if (res == SOCKET_ERROR || FD_ISSET(s, &efd))
		return -1;
#else
	if (res == -1 || FD_ISSET(s, &efd))
		return -1;
#endif

	if (res == 0)
		return 0;

	socklen_t fromaddrlen = sizeof(fromaddr);

	res = recvfrom(s, recvbuf, size, 0, (struct sockaddr *)&fromaddr, &fromaddrlen);

#if XASH_WIN32
	if (res == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
		return -1;
#else
	if (res == -1)
		return -1;
#endif

	if (res >= 0 && ((addr.sin_addr.s_addr == 0 && addr.sin_port == 0) || (addr.sin_addr.s_addr == fromaddr.sin_addr.s_addr && addr.sin_port == fromaddr.sin_port)))
	{
		return res;
	}

	return 0;
}

void NetClearSocket(NetSocket s)
{
	if (!s)
		return;

	char buffer[2048];
	while (NetReceiveUdp(0, 0, buffer, sizeof(buffer), SocketConvert(s)) > 0);
}

void NetCloseSocket(NetSocket s)
{
	if (!s)
		return;

#if XASH_WIN32
	closesocket(SocketConvert(s));
#else
	close(SocketConvert(s));
#endif
}

char *NetGetRuleValueFromBuffer(const char *buffer, int len, const char *cvar)
{
	// Check response header
	if (len < 6 || (*(unsigned int *)buffer != 0xFFFFFFFF || buffer[4] != 'E'))
		return NULL;

	// Search for a cvar
	char *current = (char *)buffer + 4;
	char *end = (char *)buffer + len - strlen(cvar);

	while (current < end)
	{
		char *pcvar = (char *)cvar;

		while (*current == *pcvar && *pcvar != 0)
		{
			current++;
			pcvar++;
		}

		if (*pcvar == 0)
		{
			// Found
			return current + 1;
		}

		current++;
	}

	return NULL;
}
