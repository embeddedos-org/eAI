// SPDX-License-Identifier: MIT
// Copyright (c) 2026 EoS Project
// HAL Network sub-vtable — nullable (MCUs without networking omit this)

#ifndef EAI_HAL_NET_H
#define EAI_HAL_NET_H

#include "eai/types.h"

typedef void *eai_socket_t;

typedef enum {
    EAI_SOCK_TCP = 0,
    EAI_SOCK_UDP = 1,
} eai_socket_type_t;

typedef struct {
    eai_status_t (*socket_create)(eai_socket_t *sock, eai_socket_type_t type);
    eai_status_t (*connect)(eai_socket_t sock, const char *host, uint16_t port);
    eai_status_t (*send)(eai_socket_t sock, const void *data, size_t len, size_t *sent);
    eai_status_t (*recv)(eai_socket_t sock, void *buf, size_t buf_size, size_t *received);
    void         (*close)(eai_socket_t sock);
    eai_status_t (*dns_resolve)(const char *hostname, char *ip_buf, size_t ip_buf_size);
} eai_hal_net_ops_t;

#endif /* EAI_HAL_NET_H */
