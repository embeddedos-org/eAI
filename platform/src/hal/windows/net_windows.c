// SPDX-License-Identifier: MIT
// HAL Network implementation for Windows (Winsock2)

#include "eai/platform.h"
#include <string.h>
#include <stdio.h>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

static int winsock_initialized = 0;

static void ensure_winsock(void)
{
    if (!winsock_initialized) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        winsock_initialized = 1;
    }
}

static eai_status_t win_socket_create(eai_socket_t *sock, eai_socket_type_t type)
{
    ensure_winsock();
    int stype = (type == EAI_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    SOCKET s = socket(AF_INET, stype, 0);
    if (s == INVALID_SOCKET) return EAI_ERR_IO;
    *sock = (eai_socket_t)(intptr_t)(s + 1);
    return EAI_OK;
}

static SOCKET sock_s(eai_socket_t sock)
{
    return (SOCKET)((intptr_t)sock - 1);
}

static eai_status_t win_connect(eai_socket_t sock, const char *host, uint16_t port)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0)
        return EAI_ERR_CONNECT;

    int rc = connect(sock_s(sock), res->ai_addr, (int)res->ai_addrlen);
    freeaddrinfo(res);
    return (rc == 0) ? EAI_OK : EAI_ERR_CONNECT;
}

static eai_status_t win_send(eai_socket_t sock, const void *data, size_t len, size_t *sent)
{
    int n = send(sock_s(sock), (const char *)data, (int)len, 0);
    if (n == SOCKET_ERROR) return EAI_ERR_IO;
    if (sent) *sent = (size_t)n;
    return EAI_OK;
}

static eai_status_t win_recv(eai_socket_t sock, void *buf, size_t buf_size, size_t *received)
{
    int n = recv(sock_s(sock), (char *)buf, (int)buf_size, 0);
    if (n == SOCKET_ERROR) return EAI_ERR_IO;
    if (received) *received = (size_t)n;
    return EAI_OK;
}

static void win_close(eai_socket_t sock)
{
    closesocket(sock_s(sock));
}

static eai_status_t win_dns_resolve(const char *hostname, char *ip_buf, size_t ip_buf_size)
{
    ensure_winsock();
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0)
        return EAI_ERR_NOT_FOUND;

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip_buf, ip_buf_size);
    freeaddrinfo(res);
    return EAI_OK;
}

const eai_hal_net_ops_t eai_hal_windows_net_ops = {
    .socket_create = win_socket_create,
    .connect       = win_connect,
    .send          = win_send,
    .recv          = win_recv,
    .close         = win_close,
    .dns_resolve   = win_dns_resolve,
};

#endif /* _WIN32 */
