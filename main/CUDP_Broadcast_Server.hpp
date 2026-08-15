#pragma once
#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <mutex>
#endif
class CUDP_Broadcast_Server
{
private:
    struct sockaddr_in addr;
    unsigned long long m_sock;
	std::mutex m_mtxSend;
public:
    CUDP_Broadcast_Server(int port) : addr{}
    {
#ifdef _WIN32
        WSADATA wsa;
        auto ret =WSAStartup(MAKEWORD(2, 2), &wsa);
        if (ret) return;

#endif
        m_sock = create_udp_broadcast_socket(port);
    }
    unsigned long long create_udp_broadcast_socket(int port)
    {
#ifdef _WIN32
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCKET)
            return -1;
#else
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            return -1;
#endif

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
        std::lock_guard<std::mutex> lock(m_mtxSend);   // ? LA LIGNE QUI MANQUE
        if (m_sock >= 0)
        {
            sendto(m_sock, msg.c_str(), (int) msg.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
        }
    }
};
