#include "comms.h"

#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

namespace
{
SOCKET client_socket;


bool winsock_initialized()
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET && WSAGetLastError() == WSANOTINITIALISED) {
        return false;
    }

    closesocket(s);
    return true;
}

bool init_wsa()
{
    WSADATA wsaData;
    DWORD iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (!iResult)
        return false;

    return true;
}
}

bool connect_to_buoy()
{
    constexpr const char* port = "30010";

    SOCKET connect_socket = INVALID_SOCKET;
    struct addrinfo* result = NULL;
    struct addrinfo* ptr = NULL;
    struct addrinfo hints;
    const char* sendbuf = "This is a message from inside Anno 1800";
    int iResult;
    int recvbuflen = 1024;

    if (!winsock_initialized() && init_wsa())
        return false;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", port, &hints, &result);
    if (iResult != 0)
        return false;

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        connect_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (connect_socket == INVALID_SOCKET)
            return false;

        // Connect to server.
        iResult = connect(connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen);

        if (iResult == SOCKET_ERROR)
        {
            closesocket(connect_socket);
            client_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);
    result = NULL;

    if (connect_socket == INVALID_SOCKET)
        return false;

    client_socket = connect_socket;

    return true;
}

bool read_from_buoy(std::vector<uint8_t>& data)
{
    char buffer[1024];

    data.clear();

    while (true)
    {
        DWORD bytesRead = recv(client_socket, buffer, 1024, 0);

        if (bytesRead == SOCKET_ERROR)
            return false;

        data.resize(data.size() + bytesRead);
        memcpy(data.data() + data.size() - bytesRead, buffer, bytesRead);

        if (bytesRead < 1024)
            break;
    }

    return true;
}

