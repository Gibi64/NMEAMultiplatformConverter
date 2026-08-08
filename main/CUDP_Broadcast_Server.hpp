#pragma once
#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#endif
class CUDP_Broadcast_Server
{
private:
    struct sockaddr_in addr;
    int m_sock;
public:
    CUDP_Broadcast_Server(int port) : addr{}
    {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        m_sock = create_udp_broadcast_socket(port);
    }
    int create_udp_broadcast_socket(int port)
    {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            return -1;

        int yes = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
#if defined(_WIN32) || defined(__linux__)
        // For local desktop testing (e.g., OpenCPN), send to loopback.
        InetPton(AF_INET, L"127.0.0.1", &addr.sin_addr);
#elif defined(_ESP32)
        // On target, keep LAN broadcast behavior.
        addr.sin_addr.s_addr = inet_addr("255.255.255.255");
#endif
        return sock;
    }
    void send(const std::string& msg)
    {
        if (m_sock >= 0)
        {
            sendto(m_sock, msg.c_str(), msg.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
        }
	}
};
