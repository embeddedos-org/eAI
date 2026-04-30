// SPDX-License-Identifier: MIT
// HAL Network implementation for POSIX (Linux, macOS, Android)

#include "eai/platform.h"
#include <string.h>

#ifndef _WIN32

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

static eai_status_t posix_socket_create(eai_socket_t *sock, eai_socket_type_t type)
{
    int stype = (type == EAI_SOCK_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int fd = socket(AF_INET, stype, 0);
    if (fd < 0) return EAI_ERR_IO;
    /* Store fd as pointer-sized value */
    *sock = (eai_socket_t)(intptr_t)(fd + 1); /* +1 so 0 is invalid */
    return EAI_OK;
}

static int sock_fd(eai_socket_t sock)
{
    return (int)((intptr_t)sock - 1);
}

static eai_status_t posix_connect(eai_socket_t sock, const char *host, uint16_t port)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0)
        return EAI_ERR_CONNECT;

    int rc = connect(sock_fd(sock), res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    return (rc == 0) ? EAI_OK : EAI_ERR_CONNECT;
}

static eai_status_t posix_send(eai_socket_t sock, const void *data, size_t len, size_t *sent)
{
    ssize_t n = send(sock_fd(sock), data, len, 0);
    if (n < 0) return EAI_ERR_IO;
    if (sent) *sent = (size_t)n;
    return EAI_OK;
}

static eai_status_t posix_recv(eai_socket_t sock, void *buf, size_t buf_size, size_t *received)
{
    ssize_t n = recv(sock_fd(sock), buf, buf_size, 0);
    if (n < 0) return EAI_ERR_IO;
    if (received) *received = (size_t)n;
    return EAI_OK;
}

static void posix_close(eai_socket_t sock)
{
    close(sock_fd(sock));
}

static eai_status_t posix_dns_resolve(const char *hostname, char *ip_buf, size_t ip_buf_size)
{
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

const eai_hal_net_ops_t eai_hal_posix_net_ops = {
    .socket_create = posix_socket_create,
    .connect       = posix_connect,
    .send          = posix_send,
    .recv          = posix_recv,
    .close         = posix_close,
    .dns_resolve   = posix_dns_resolve,
};

#endif /* !_WIN32 */
