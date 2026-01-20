#include "ReflectorClientUdp.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sstream>  // for std::stringstream
#include <string>   // for std::string
#include <vector>   // for std::vector
#include <sys/socket.h> // for sendto
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h>  // for inet_addr
#include <unistd.h>     // for close
#include <cstring>      // for memset
#include <cstdio>       // for perror
#include <iostream>
#include <iomanip>
#include <cctype>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <algorithm>
#include <iostream>



ReflectorClientUdp::ReflectorClientUdp(
    const std::string& host,
    int udpPort,
    int tcpPort
)
    : m_host(host), m_udpPort(udpPort), m_tcpPort(tcpPort)
{
    // ---------- UDP SOCKET ----------
    m_udpSockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_udpSockfd < 0) {
        perror("UDP socket");
        throw std::runtime_error("Failed to create UDP socket");
    }

    memset(&m_udpAddr, 0, sizeof(m_udpAddr));
    m_udpAddr.sin_family = AF_INET;
    m_udpAddr.sin_port = htons(udpPort);

    if (inet_pton(AF_INET, host.c_str(), &m_udpAddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid UDP IP address");
    }

    // ---------- TCP SOCKET ----------
    m_tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_tcpSockfd < 0) {
        perror("TCP socket");
        throw std::runtime_error("Failed to create TCP socket");
    }

    memset(&m_tcpAddr, 0, sizeof(m_tcpAddr));
    m_tcpAddr.sin_family = AF_INET;
    m_tcpAddr.sin_port = htons(tcpPort);

    if (inet_pton(AF_INET, host.c_str(), &m_tcpAddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid TCP IP address");
    }
}

ReflectorClientUdp::~ReflectorClientUdp() {
    if (m_udpSockfd >= 0) close(m_udpSockfd);
    if (m_tcpSockfd >= 0) close(m_tcpSockfd);
}

//
// ---------------- UDP ----------------
//

void ReflectorClientUdp::sendControl(const std::string& msg) {
    ssize_t n = sendto(
        m_udpSockfd,
        msg.data(),
        msg.size(),
        0,
        (sockaddr*)&m_udpAddr,
        sizeof(m_udpAddr)
    );

    if (n < 0) {
        perror("sendControl");
    }
}

std::vector<unsigned char> ReflectorClientUdp::encryptAES(
    const void* data,
    size_t len,
    const std::string& key
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    unsigned char iv[16];
    RAND_bytes(iv, sizeof(iv));

    unsigned char aes_key[16] = { 0 };
    memcpy(aes_key, key.data(), std::min(key.size(), sizeof(aes_key)));

    std::vector<unsigned char> ciphertext(len + 16);
    int out_len1 = 0, out_len2 = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, aes_key, iv);
    EVP_EncryptUpdate(
        ctx,
        ciphertext.data(),
        &out_len1,
        static_cast<const unsigned char*>(data),
        len
    );
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len1, &out_len2);

    ciphertext.resize(out_len1 + out_len2);
    EVP_CIPHER_CTX_free(ctx);

    // Prepend IV
    std::vector<unsigned char> result;
    result.reserve(16 + ciphertext.size());
    result.insert(result.end(), iv, iv + 16);
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

void ReflectorClientUdp::sendUdp(const void *buf, size_t len)
{

    ssize_t n = sendto(
        m_udpSockfd,
        buf,
        len,
        0,
        (sockaddr*)&m_udpAddr,
        sizeof(m_udpAddr)
    );

    if (n < 0) {
        perror("sendUdp");
    }
    
}
void ReflectorClientUdp::sendUdp_crypt(const void* buf,size_t len,const std::string& key)
{
    auto encrypted = encryptAES(buf, len, key);

    ssize_t n = sendto(
        m_udpSockfd,
        encrypted.data(),
        encrypted.size(),
        0,
        (sockaddr*)&m_udpAddr,
        sizeof(m_udpAddr)
    );

    if (n < 0) {
        perror("sendUdp_crypt");
    }
}




ssize_t ReflectorClientUdp::receiveUdp(std::vector<uint8_t>& outBuffer) {
    outBuffer.resize(1500); // typical MTU

    ssize_t n = recvfrom(
        m_udpSockfd,
        outBuffer.data(),
        outBuffer.size(),
        0,
        nullptr,
        nullptr
    );

    if (n < 0) {
        perror("receiveUdp");
        return -1;
    }

    outBuffer.resize(n);
    return n;
}

//
// ---------------- TCP ----------------
//

void ReflectorClientUdp::connectTcp() {
    if (connect(
            m_tcpSockfd,
            (sockaddr*)&m_tcpAddr,
            sizeof(m_tcpAddr)
        ) < 0)
    {
        perror("connectTcp");
        throw std::runtime_error("TCP connect failed");
    }
}

ssize_t ReflectorClientUdp::sendTcp(const std::vector<uint8_t>& data) {
    ssize_t n = send(
        m_tcpSockfd,
        data.data(),
        data.size(),
        0
    );

    if (n < 0) {
        perror("sendTcp");
    }

    return n;
}

ssize_t ReflectorClientUdp::receiveTcp(std::vector<uint8_t>& outBuffer) {
    outBuffer.resize(4096);

    ssize_t n = recv(
        m_tcpSockfd,
        outBuffer.data(),
        outBuffer.size(),
        0
    );

    if (n < 0) {
        perror("receiveTcp");
        return -1;
    }

    outBuffer.resize(n);
    return n;
}

