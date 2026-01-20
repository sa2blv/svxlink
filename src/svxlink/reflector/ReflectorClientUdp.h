#pragma once

#include <string>
#include <vector>
#include <netinet/in.h>
#include "ReflectorMsg.h"
#include <AsyncUdpSocket.h>
#include <AsyncIpAddress.h>


class ReflectorClientUdp {
public:
    ReflectorClientUdp(const std::string& host, int udpPort, int tcpPort);
    ~ReflectorClientUdp();

    // UDP
    void sendControl(const std::string& msg);
    void sendUdp(const void *buf, size_t len);
    ssize_t receiveUdp(std::vector<uint8_t>& outBuffer);

    // TCP
    void connectTcp();
    ssize_t sendTcp(const std::vector<uint8_t>& data);
    ssize_t receiveTcp(std::vector<uint8_t>& outBuffer);

    void sendUdp_crypt(const void* buf, size_t len, const std::string& key);

private:
    std::string m_host;
    int m_udpPort;
    int m_tcpPort;

    int m_udpSockfd{-1};
    int m_tcpSockfd{-1};

    sockaddr_in m_udpAddr{};
    sockaddr_in m_tcpAddr{};

    std::vector<unsigned char> encryptAES(
        const void* data,
        size_t len,
        const std::string& key
    );




};
