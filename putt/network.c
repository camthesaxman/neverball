/*
 *  Copyright (C) 2019  Neverball authors
 *
 *  This  program is  free software;  you can  redistribute  it and/or
 *  modify it  under the  terms of the  GNU General Public  License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program  is distributed in the  hope that it  will be useful,
 *  but  WITHOUT ANY WARRANTY;  without even  the implied  warranty of
 *  MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a  copy of the GNU General Public License
 *  along  with this  program;  if  not, write  to  the Free  Software
 *  Foundation,  Inc.,   59  Temple  Place,  Suite   330,  Boston,  MA
 *  02111-1307 USA
 */

#include <assert.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "network.h"

#define DEFAULT_CLIENT_PORT 54321
#define DEFAULT_SERVER_PORT 54322

/* This is negative because the protocol is in an experimental state. If this code ever gets merged
 * into mainline neverputt, we should change this to a positive number. */
#define PROTO_VERSION -1

const char *network_error = NULL;

static int net_sock = -1;

static char *strfmt(const char *fmt, ...)
{
    va_list args;
    static char buf[MAXSTR];
    
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return buf;
}

#ifdef _WIN32
static int ws_initialized = 0;

static int init_winsock(void)
{
    WSADATA wsadata;
    int result;

    if (ws_initialized)
        return 1;

    if ((result = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0)
    {
        network_error = strfmt("WSAStartup failed: %i\n", result);
        return 0;
    }

    ws_initialized = 1;
    return 1;
}

static char *get_socket_error(void)
{
    int error = WSAGetLastError();
    static char buffer[MAXSTR];
    
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, buffer, sizeof(buffer), NULL);
    //log_printf("socket error: %s\n", buffer);
    return buffer;
}
#endif

static int init_socket(int port)
{
    int sockfd;
    struct sockaddr_in sa = {0};

    /* Create socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        network_error = strfmt("error creating socket: %s", strerror(errno));
        return -1;
    }

    log_printf("created socket %i\n", sockfd);
    
    /* Bind socket to port */
    sa.sin_family = AF_INET;  /* TODO: support IPV6 */
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        network_error = "failed to bind to port";
        return -1;
    }

    log_printf("bound to port %i\n", port);

    return sockfd;
}

static void close_socket(int sockfd)
{
    if (sockfd > 0)
    {
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        printf("closed socket %i\n", sockfd);
        fflush(stdout);
    }
}

/* Determines if the socket has any data to receive */
static int socket_ready(int sockfd)
{
    fd_set read_fd_set = {0};
    struct timeval timeout = {0};

    FD_SET(sockfd, &read_fd_set);

    return (select(0, &read_fd_set, NULL, NULL, &timeout) > 0);
}

int network_send_cmd(union netcmd *cmd, uint32_t ip_addr, uint16_t port)
{
    int size;
    struct sockaddr_in sa = {0};

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip_addr;
    sa.sin_port = htons(port);

    assert(net_sock > 0);

    size = sendto(net_sock, (const char *)cmd, sizeof(*cmd), 0, (struct sockaddr *)&sa, sizeof(sa));
    if (size != sizeof(*cmd))
    {
        printf("sent %i of %i bytes\n", size, sizeof(*cmd));
        if (size < 0)
            network_error = strerror(errno);
        else
            network_error = "Send error";
        return 0;
    }
    return 1;
}

int network_recv_cmd(union netcmd *cmd, uint32_t *ip_addr, uint16_t *port)
{
    assert(net_sock > 0);

    if (socket_ready(net_sock))
    {
        int size;
        struct sockaddr_in sa = {0};
        int addr_len = sizeof(sa);

        log_printf("receiving...\n");
        size = recvfrom(net_sock, (char *)cmd, sizeof(*cmd), 0, (struct sockaddr *)&sa, &addr_len);
        log_printf("done\n");
        if (size == sizeof(*cmd))
        {
            *ip_addr = sa.sin_addr.s_addr;
            *port    = sa.sin_port;

            return 1;
        }

        if (size == -1)
            printf("recv error: %s\n", get_socket_error());
        else
            printf("invalid length %i, expected %u\n", size, sizeof(*cmd));
        fflush(stdout);
    }

    return 0;
}

static char *ip_to_str(uint32_t ip_addr)
{
    static char buf[16];
    
    sprintf(buf, "%u.%u.%u.%u", 
        (ip_addr >> 24) & 0xFF,
        (ip_addr >> 16) & 0xFF,
        (ip_addr >>  8) & 0xFF,
        (ip_addr >>  0) & 0xFF);
    return buf;
}

/*---------------------------------------------------------------------------*/
/* Client */

static uint32_t server_addr;
static uint16_t server_port;

int network_status = NETWORK_STATUS_IDLE;

int network_client_init(void)
{
    network_error = NULL;

#ifdef _WIN32
    if (!init_winsock())
        return 0;
#endif

    if ((net_sock = init_socket(DEFAULT_CLIENT_PORT)) == -1)
        return 0;

    network_status = NETWORK_STATUS_IDLE;
    
    return 1;
}

void network_client_close(void)
{
    close_socket(net_sock);
}

int network_client_join(const char *hostname)
{
    struct hostent *hostinfo;
    struct sockaddr_in sa;
    union netcmd cmd;
    int port = DEFAULT_SERVER_PORT;  /* TODO: allow connecting to other ports */

    network_error = NULL;

    hostinfo = gethostbyname(hostname);
    if (hostinfo == NULL)
    {
        network_error = strfmt("unknown host %s", hostname);
        return 0;
    }

    sa.sin_addr = *(struct in_addr *)hostinfo->h_addr;

    server_addr = sa.sin_addr.s_addr;
    server_port = port;

    cmd.knock.type = NETCMD_KNOCK;
    cmd.knock.version = PROTO_VERSION;
    strcpy(cmd.knock.name, "Mr. Client");
    network_send_cmd(&cmd, server_addr, server_port);

    network_status = NETWORK_STATUS_CONNECTING;

    return 1;
}

void network_client_process(void)
{
    uint32_t ip_addr;
    uint16_t port;
    union netcmd incmd = {0};
    int i;

    while (network_recv_cmd(&incmd, &ip_addr, &port))
    {
        union netcmd outcmd = {0};

        /* Only accept commands from server. nobody else. */
        if (ip_addr != server_addr || port != server_port)
            continue;

        switch (incmd.base.type)
        {
        case NETCMD_PING:
            outcmd.ack.type = NETCMD_ACK;
            outcmd.ack.seq_num = incmd.ping.seq_num;
            network_send_cmd(&outcmd, ip_addr, server_port);
            break;
        case NETCMD_WELCOME:
            log_printf("I joined the game as player %i\n", incmd.welcome.player_num);
            network_status = NETWORK_STATUS_CONNECTED;
            break;
        case NETCMD_PLAYERS:
            log_printf("List of players:\n");
            for (i = 0; i < 4; i++)
            {
                if (incmd.players.players[i].active)
                    log_printf("\tplayer %i: '%s'\n", i + 1, incmd.players.players[i].name);
            }
            break;
        case NETCMD_ERROR:
            log_printf("protocol error %i\n", incmd.error.errcode);
            break;
        }
    }
}

/*---------------------------------------------------------------------------*/
/* Server */

struct net_player
{
    int active;
    uint32_t ip_addr;  /* 0 here indicates the server */
    uint16_t port;
    uint32_t last_ping;  /* Timestamp of last ping */
    char name[MAXSTR];
};

static struct net_player net_players[4];

static int add_net_player(uint32_t ip_addr, uint16_t port, const char *name)
{
    int i;
    struct net_player *p;
    int index = -1;

    for (i = 0; i < 4; i++)
    {
        p = &net_players[i];

        if (p->active && p->ip_addr == ip_addr && p->port == port)
            return 0;  /* already exists */
        if (!p->active)
            index = i;
    }

    if (index != -1)
    {
        p = &net_players[index];

        p->active = 1;
        p->ip_addr = ip_addr;
        p->port = port;
        p->last_ping = 0;  /* TODO: implement ping */
        strncpy(p->name, name, sizeof(p->name));
        p->name[sizeof(p->name) - 1] = 0;
        return 1;
    }

    return 0;
}

static int remove_net_player(uint32_t ip_addr, uint16_t port)
{
    int i;
    struct net_player *p;

    for (i = 0; i < 4; i++)
    {
        p = &net_players[i];
        
        if (p->active && p->ip_addr == ip_addr && p->port == port)
        {
            p->active = 0;
            return 1;
        }
    }
    
    return 0;
}

static int is_player_port_addr(uint32_t ip_addr, uint16_t port)
{
    int i;
    struct net_player *p;
    
    for (i = 0; i < 4; i++)
    {
        p = &net_players[i];
        
        if (p->active && p->ip_addr == ip_addr && p->port == port)
            return 1;
    }
    return 0;
}

int network_server_init(void)
{
    network_error = NULL;

#ifdef _WIN32
    if (!init_winsock())
        return 0;
#endif

    if ((net_sock = init_socket(DEFAULT_SERVER_PORT)) != -1)
        return 0;

    add_net_player(0, 0, "Mr. Server");

    return 1;
}

void network_server_close(void)
{
    close_socket(net_sock);
}

void network_server_process(void)
{
    uint32_t ip_addr;
    uint16_t port;
    union netcmd incmd = {0};

    while (network_recv_cmd(&incmd, &ip_addr, &port))
    {
        union netcmd outcmd = {0};

        /* Only accept commands from players */
        if (!is_player_port_addr(ip_addr, port) && incmd.base.type != NETCMD_KNOCK)
            continue;

        switch (incmd.base.type)
        {
        case NETCMD_PING:
            outcmd.ack.type = NETCMD_ACK;
            outcmd.ack.seq_num = incmd.ping.seq_num;
            network_send_cmd(&outcmd, ip_addr, port);
            break;
        case NETCMD_KNOCK:
            if (add_net_player(ip_addr, port, incmd.knock.name))
            {
                int i;
                struct net_player *p;

                /* Notify player of successful join */
                
                log_printf("player '%s' (%s:%i) has joined\n", incmd.knock.name, ip_to_str(ip_addr), port);
                
                for (i = 0; i < 4; i++)
                {
                    p = &net_players[i];
                    
                    if (p->active && p->ip_addr != 0 && p->ip_addr == ip_addr && p->port == port)
                    {
                        outcmd.welcome.type = NETCMD_WELCOME;
                        outcmd.welcome.player_num = i + 1;
                        network_send_cmd(&outcmd, ip_addr, port);
                        break;
                    }
                }

                memset(&outcmd, 0, sizeof(outcmd));

                /* Notify all other players of join */
                
                outcmd.players.type = NETCMD_PLAYERS;
                
                for (i = 0; i < 4; i++)
                {
                    p = &net_players[i];
                    
                    if (p->active)
                    {
                        outcmd.players.players[i].active = 1;
                        SAFECPY(outcmd.players.players[i].name, p->name);
                    }
                }
                
                for (i = 0; i < 4; i++)
                {
                    p = &net_players[i];
                    
                    if (p->active && p->ip_addr != 0)
                        network_send_cmd(&outcmd, p->ip_addr, DEFAULT_CLIENT_PORT);
                }
            }
            else
            {
                log_printf("player '%s' (%s:%i) could not join\n", incmd.knock.name, ip_to_str(ip_addr), port);
            }
            break;
        /*
        case NETCMD_QUERY:
            outcmd.
        */
        default:  // Unknown command
            outcmd.error.type = NETCMD_ERROR;
            outcmd.error.errcode = ERR_INVALID;
            network_send_cmd(&outcmd, ip_addr, port);
            break;
        }
    }
}
