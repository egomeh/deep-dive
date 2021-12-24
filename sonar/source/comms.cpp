#include "comms.h"

#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>


bool WinsockInitialized()
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET && WSAGetLastError() == WSANOTINITIALISED) {
        return false;
    }

    closesocket(s);
    return true;
}

bool InitWSA()
{
    WSADATA wsaData;
    DWORD iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (!iResult)
        return false;

    return true;
}

