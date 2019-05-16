#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

enum
{
    NETCMD_PING,
    NETCMD_ACK,
    NETCMD_JOIN,
    NETCMD_LEAVE,
    NETCMD_ERROR,
};

struct netcmd_ping
{
    uint8_t type;
    uint32_t seq_nr;
};

struct netcmd_ack
{
    uint8_t type;
    uint32_t seq_nr;
};

struct netcmd_join
{
    uint8_t type;
    uint16_t version;
};

union netcmd
{
    struct netcmd_ping ping;
    struct netcmd_ack ack;
    struct netcmd_join join;
};

extern const char *network_error;

void network_client_init(void);
void network_client_close(void);
int network_client_join(const char *hostname);

void network_server_init(void);
void network_server_close(void);
int network_recv_command(union netcmd *cmd);

int network_listen(void);

#endif
