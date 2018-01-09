/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include "freertos/semphr.h"
#include "lwip/mld6.h"
#include "lwip/sockets.h"

#include <assert.h>
#include "oc_buffer.h"
#include "port/oc_connectivity.h"

#include "freertos_mutex.h"
#include "exception_handling.h"
#include "debug_print.h"

// most of function declaration is under iotivity-constrained/port/oc_connectivity.h and oc_network_events_mutex.h

#define OCF_PORT_UNSECURED (5683)

static const uint8_t ALL_OCF_NODES_LL[] = {
    0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x58
};

static const uint8_t ALL_OCF_NODES_RL[] = {
    0xff, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x58
};

static const uint8_t ALL_OCF_NODES_SL[] = {
    0xff, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x58
};

#define ALL_COAP_NODES_V4 0xe00001bb

static TaskHandle_t event_thread = NULL;

static osi_mutex_t mutex;

static struct sockaddr_storage mcast, server, client;

static int server_sock = -1, mcast_sock = -1, terminate;

#ifdef OC_IPV4
static struct sockaddr_storage mcast4, server4;
static int server4_sock = -1, mcast4_sock = -1;
#endif

#ifdef OC_SECURITY
static struct sockaddr_storage secure;
static int secure_sock = -1;
#ifdef OC_IPV4
static struct sockaddr_storage secure4;
static int secure4_sock = -1;
#endif
static uint16_t dtls_port = 0;

uint16_t
oc_connectivity_get_dtls_port(void)
{
    return dtls_port;
}
#endif /* OC_SECURITY */

void oc_network_event_handler_mutex_init(void)
{
    if (mutex_new(&mutex) < 0) {
        OC_ERR("initializing network event handler mutex failed\n");
        task_fatal_error();
    }
}

void oc_network_event_handler_mutex_lock(void)
{
    if (mutex_lock(&mutex) < 0) {
        OC_ERR("mutex_lock network event handler mutex failed\n");
        task_fatal_error();
    }
}

void oc_network_event_handler_mutex_unlock(void)
{
    if (mutex_unlock(&mutex) < 0) {
        OC_ERR("mutex_unlock network event handler mutex failed\n");
        task_fatal_error();
    }
}

static void *network_event_thread(void *data)
{
    (void)data;
    struct sockaddr_in6 *c = (struct sockaddr_in6 *)&client;
    socklen_t len = sizeof(client);

#ifdef OC_IPV4
    struct sockaddr_in *c4 = (struct sockaddr_in *)&client;
#endif

    fd_set rfds, setfds;

    FD_ZERO(&rfds);
    FD_SET(server_sock, &rfds);
    FD_SET(mcast_sock, &rfds);

#ifdef OC_SECURITY
    FD_SET(secure_sock, &rfds);
#endif

#ifdef OC_IPV4
    FD_SET(server4_sock, &rfds);
    FD_SET(mcast4_sock, &rfds);
#ifdef OC_SECURITY
    FD_SET(secure4_sock, &rfds);
#endif
#endif

    int i = 0, n = 0;
    while (!terminate) {
        len = sizeof(client);
        setfds = rfds;

        int maxfd = (server_sock > mcast_sock) ? server_sock : mcast_sock;
        n = select(maxfd + 1, &setfds, NULL, NULL, NULL);
        for (i = 0; i < n; i++) {
            len = sizeof(client);
            oc_message_t *message = oc_allocate_message();

            if (!message) {
                break;
            }

            if (FD_ISSET(server_sock, &setfds)) {
                int count = recvfrom(server_sock, message->data, OC_PDU_SIZE, 0,
                                     (struct sockaddr *)&client, &len);
                if (count < 0) {
                    oc_message_unref(message);
                    continue;
                }
                message->length = count;
                message->endpoint.flags = IPV6;
                FD_CLR(server_sock, &setfds);
                goto common;
            }

            if (FD_ISSET(mcast_sock, &setfds)) {
                int count = recvfrom(mcast_sock, message->data, OC_PDU_SIZE, 0,
                                     (struct sockaddr *)&client, &len);
                if (count < 0) {
                    oc_message_unref(message);
                    continue;
                }
                message->length = count;
                message->endpoint.flags = IPV6;
                FD_CLR(mcast_sock, &setfds);
                goto common;
            }

#ifdef OC_IPV4
            if (FD_ISSET(server4_sock, &setfds)) {
                int count = recvfrom(server4_sock, message->data, OC_PDU_SIZE, 0,
                                     (struct sockaddr *)&client, &len);
                if (count < 0) {
                    oc_message_unref(message);
                    continue;
                }
                message->length = count;
                message->endpoint.flags = IPV4;
                FD_CLR(server4_sock, &setfds);
                goto common;
            }

            if (FD_ISSET(mcast4_sock, &setfds)) {
                int count = recvfrom(mcast4_sock, message->data, OC_PDU_SIZE, 0,
                                     (struct sockaddr *)&client, &len);
                if (count < 0) {
                    oc_message_unref(message);
                    continue;
                }
                message->length = count;
                message->endpoint.flags = IPV4;
                FD_CLR(mcast4_sock, &setfds);
                goto common;
            }
#endif

#ifdef OC_SECURITY
            if (FD_ISSET(secure_sock, &setfds)) {
                int count = recvfrom(secure_sock, message->data, OC_PDU_SIZE, 0,
                                     (struct sockaddr *)&client, &len);
                if (count < 0) {
                    oc_message_unref(message);
                    continue;
                }
                message->length = count;
                message->endpoint.flags = IPV6 | SECURED;
            }
#ifdef OC_IPV4
            if (FD_ISSET(secure4_sock, &setfds)) {
                int count = recvfrom(secure4_sock, message->data, OC_PDU_SIZE, 0,
                                     (struct sockaddr *)&client, &len);
                if (count < 0) {
                    oc_message_unref(message);
                    continue;
                }
                message->length = count;
                message->endpoint.flags = IPV4 | SECURED;
            }
#endif
#endif /* OC_SECURITY */

common:
#ifdef OC_IPV4
            if (message->endpoint.flags & IPV4) {
                memcpy(message->endpoint.addr.ipv4.address, &c4->sin_addr.s_addr,
                       sizeof(c4->sin_addr.s_addr));
                message->endpoint.addr.ipv4.port = ntohs(c4->sin_port);
            } else if (message->endpoint.flags & IPV6) {
#else
            if (message->endpoint.flags & IPV6) {
#endif
                memcpy(message->endpoint.addr.ipv6.address, c->sin6_addr.s6_addr,
                       sizeof(c->sin6_addr.s6_addr));
                message->endpoint.addr.ipv6.scope = c->sin6_scope_id;
                message->endpoint.addr.ipv6.port = ntohs(c->sin6_port);
            }

            OC_DBG("Incoming message from ");
            OC_LOGipaddr(message->endpoint);
            OC_DBG("\n");

            oc_network_event(message);
        }
    }

    vTaskDelete(NULL);
    return NULL;
}

void
oc_send_buffer(oc_message_t *message)
{
    print_message_info(message);
    OC_DBG("Outgoing message to ");
    OC_LOGipaddr(message->endpoint);
    OC_DBG("\n");

    struct sockaddr_storage receiver;
    memset(&receiver, 0, sizeof(struct sockaddr_storage));
#ifdef OC_IPV4
    if (message->endpoint.flags & IPV4) {
        struct sockaddr_in *r = (struct sockaddr_in *)&receiver;
        memcpy(&r->sin_addr.s_addr, message->endpoint.addr.ipv4.address,
               sizeof(r->sin_addr.s_addr));
        r->sin_family = AF_INET;
        r->sin_port = htons(message->endpoint.addr.ipv4.port);
    } else {
#else
    {
#endif
        struct sockaddr_in6 *r = (struct sockaddr_in6 *) &receiver;
        memcpy(r->sin6_addr.s6_addr, message->endpoint.addr.ipv6.address,
        sizeof(r->sin6_addr.s6_addr));
        r->sin6_family = AF_INET6;
        r->sin6_port = htons(message->endpoint.addr.ipv6.port);
        r->sin6_scope_id = message->endpoint.addr.ipv6.scope;
    }
    int send_sock = -1;

#ifdef OC_SECURITY
    if (message->endpoint.flags & SECURED) {
#ifdef OC_IPV4
        if (message->endpoint.flags & IPV4) {
            send_sock = secure4_sock;
        } else {
            send_sock = secure_sock;
        }
#else
        send_sock = secure_sock;
#endif
    } else
#endif /* OC_SECURITY */
#ifdef OC_IPV4
        if (message->endpoint.flags & IPV4) {
            send_sock = server4_sock;
        } else {
            send_sock = server_sock;
        }
#else  /* OC_IPV4 */
    {
        send_sock = server_sock;
    }
#endif /* !OC_IPV4 */

    int bytes_sent = 0, x;
    while (bytes_sent < (int)message->length) {
        x = sendto(send_sock, message->data + bytes_sent,
                   message->length - bytes_sent, 0, (struct sockaddr *)&receiver,
                   sizeof(receiver));
        if (x < 0) {
            OC_WRN("sendto() returned errno %d\n", errno);
            return;
        }
        bytes_sent += x;
    }
    OC_DBG("Sent %d bytes\n", bytes_sent);
}

#ifdef OC_CLIENT

void
oc_send_discovery_request(oc_message_t *message)
{
    oc_send_buffer(message);
}
#endif /* OC_CLIENT */

#ifdef OC_IPV4
static int
connectivity_ipv4_init(void)
{
    memset(&mcast4, 0, sizeof(struct sockaddr_storage));
    memset(&server4, 0, sizeof(struct sockaddr_storage));

    struct sockaddr_in *m = (struct sockaddr_in *)&mcast4;
    m->sin_family = AF_INET;
    m->sin_port = htons(OCF_PORT_UNSECURED);
    m->sin_addr.s_addr = INADDR_ANY;

    struct sockaddr_in *l = (struct sockaddr_in *)&server4;
    l->sin_family = AF_INET;
    l->sin_addr.s_addr = INADDR_ANY;
    l->sin_port = 0;

#ifdef OC_SECURITY
    memset(&secure4, 0, sizeof(struct sockaddr_storage));
    struct sockaddr_in *sm = (struct sockaddr_in *)&secure4;
    sm->sin_family = AF_INET;
    sm->sin_port = dtls_port;
    sm->sin_addr.s_addr = INADDR_ANY;

    secure4_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (secure4_sock < 0) {
        OC_ERR("creating secure IPv4 socket\n");
        return -1;
    }
#endif /* OC_SECURITY */

    server4_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    mcast4_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (server4_sock < 0 || mcast4_sock < 0) {
        OC_ERR("creating IPv4 server sockets\n");
        return -1;
    }

    if (bind(server4_sock, (struct sockaddr *)&server4, sizeof(server4)) == -1) {
        OC_ERR("binding server4 socket %d\n", errno);
        return -1;
    }

    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = htonl(ALL_COAP_NODES_V4);
    if (setsockopt(mcast4_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                   sizeof(mreq)) == -1) {
        OC_ERR("joining IPv4 multicast group %d\n", errno);
        return -1;
    }

    int reuse = 1;
    if (setsockopt(mcast4_sock, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) == -1) {
        OC_ERR("setting reuseaddr IPv4 option %d\n", errno);
        return -1;
    }
    if (bind(mcast4_sock, (struct sockaddr *)&mcast4, sizeof(mcast4)) == -1) {
        OC_ERR("binding mcast IPv4 socket %d\n", errno);
        return -1;
    }

#ifdef OC_SECURITY
    if (setsockopt(secure4_sock, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) == -1) {
        OC_ERR("setting reuseaddr IPv4 option %d\n", errno);
        return -1;
    }

    if (bind(secure4_sock, (struct sockaddr *)&secure4, sizeof(secure4)) == -1) {
        OC_ERR("binding IPv4 secure socket %d\n", errno);
        return -1;
    }
#endif /* OC_SECURITY */

    OC_DBG("Successfully initialized IPv4 connectivity\n");

    return 0;
}
#endif  /* OC_IPV4*/

static int add_mcast_sock_to_ipv6_multicast_group(const uint8_t *addr)
{
    if (mld6_joingroup(IP6_ADDR_ANY, addr) != ERR_OK) {
        OC_ERR("failed to joining IPv6 multicast group %d\n", errno);
        return -1;
    }

    return 0;
}

int oc_connectivity_init(void)
{
    memset(&mcast, 0, sizeof(struct sockaddr_storage));
    memset(&server, 0, sizeof(struct sockaddr_storage));

    struct sockaddr_in6 *m = (struct sockaddr_in6 *)&mcast;
    m->sin6_family = AF_INET6;
    m->sin6_port = htons(OCF_PORT_UNSECURED);
    m->sin6_addr = in6addr_any;

    struct sockaddr_in6 *l = (struct sockaddr_in6 *)&server;
    l->sin6_family = AF_INET6;
    l->sin6_addr = in6addr_any;
    l->sin6_port = 0;

#ifdef OC_SECURITY
    memset(&secure, 0, sizeof(struct sockaddr_storage));
    struct sockaddr_in6 *sm = (struct sockaddr_in6 *)&secure;
    sm->sin6_family = AF_INET6;
    sm->sin6_port = 0;
    sm->sin6_addr = in6addr_any;
#endif /* OC_SECURITY */

    server_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    mcast_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (server_sock < 0 || mcast_sock < 0) {
        OC_ERR("creating server sockets\n");
        return -1;
    }

#ifdef OC_SECURITY
    secure_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (secure_sock < 0) {
        OC_ERR("creating secure socket\n");
        return -1;
    }
#endif /* OC_SECURITY */

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) == -1) {
        OC_ERR("binding server socket %d\n", errno);
        return -1;
    }

    if (add_mcast_sock_to_ipv6_multicast_group(ALL_OCF_NODES_LL) < 0) {
        return -1;
    }
    if (add_mcast_sock_to_ipv6_multicast_group(ALL_OCF_NODES_RL) < 0) {
        return -1;
    }
    if (add_mcast_sock_to_ipv6_multicast_group(ALL_OCF_NODES_SL) < 0) {
        return -1;
    }

    int reuse = 1;
    if (setsockopt(mcast_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) ==
            -1) {

        OC_ERR("setting reuseaddr option %d\n", errno);
        return -1;
    }
    if (bind(mcast_sock, (struct sockaddr *)&mcast, sizeof(mcast)) == -1) {
        OC_ERR("binding mcast socket %d\n", errno);
        return -1;
    }

#ifdef OC_SECURITY
    if (setsockopt(secure_sock, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) == -1) {
        OC_ERR("setting reuseaddr option %d\n", errno);
        return -1;
    }
    if (bind(secure_sock, (struct sockaddr *)&secure, sizeof(secure)) == -1) {
        OC_ERR("binding IPv6 secure socket %d\n", errno);
        return -1;
    }

    socklen_t socklen = sizeof(secure);
    if (getsockname(secure_sock, (struct sockaddr *)&secure, &socklen) == -1) {
        OC_ERR("obtaining secure socket information %d\n", errno);
        return -1;
    }

    dtls_port = ntohs(sm->sin6_port);
#endif /* OC_SECURITY */

#ifdef OC_IPV4
    if (connectivity_ipv4_init() != 0) {
        PRINT("Could not initialize IPv4\n");
    }
#endif

    if (xTaskCreate(&network_event_thread, "network_event_thread", 8192, NULL, 5 , &event_thread) != pdPASS) {
        OC_ERR("creating network polling thread\n");
        return -1;
    }

    OC_DBG("Successfully initialized connectivity\n");
    return 0;
}

void
oc_connectivity_shutdown(void)
{
    terminate = 1;

    close(server_sock);
    close(mcast_sock);

#ifdef OC_IPV4
    close(server4_sock);
    close(mcast4_sock);
#endif /* OC_IPV4 */

#ifdef OC_SECURITY
    close(secure_sock);
#ifdef OC_IPV4
    close(secure4_sock);
#endif /* OC_IPV4 */
#endif /* OC_SECURITY */

    if ( event_thread != NULL ) {
        vTaskDelete( event_thread );
    }

    OC_DBG("oc_connectivity_shutdown\n");
}
