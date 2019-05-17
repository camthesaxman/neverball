#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

#include "common.h"

enum
{
    NETCMD_QUERY,  /* get game status */
    NETCMD_GAMESTATUS,  /* send game status */
    NETCMD_PING,
    NETCMD_ACK,
    NETCMD_KNOCK,  /* ask to join */
    NETCMD_WELCOME,  /* sent to player on successful join */
    NETCMD_PLAYERS,  /* server sends player list. Sent whenever someone joins or leaves */
    NETCMD_BYE,  /* client leaves */
    NETCMD_ERROR,  /* sent to client if there is an error */
};

enum
{
    ERR_INVALID,  /* Invalid command */
    ERR_TOO_MANY_PLAYERS,
};

struct netcmd_base
{
    uint8_t type;
};

struct netcmd_query
{
    uint8_t type;
};

struct netcmd_gamestatus
{
    uint8_t type;
    uint8_t num_players;
    uint8_t hole_num;
    char course[MAXSTR];
};

struct netcmd_ping
{
    uint8_t type;
    uint32_t seq_num;
};

struct netcmd_ack
{
    uint8_t type;
    uint32_t seq_num;
};

/* 
 * client to server
 * Asks to join the game. The server may respond with a NETCMD_WELCOME to allow
 * the player to join, or a NETCMD_ERROR to deny the player.
 */
struct netcmd_knock
{
    uint8_t type;
    int16_t version;
    char name[MAXSTR];
};

/*
 * server to client
 * Sent to player, allowing to allow them to join the game and assigning them
 * a player number.
 */
struct netcmd_welcome
{
    uint8_t type;
    uint8_t player_num;  /* assigned player number (1-4) */
};

/*
 * server to client
 * Lists all of the players currently in the game. Sent to all clients when a
 * player enters or exits the game.
 */
struct netcmd_players
{
    uint8_t type;
    struct
    {
        uint8_t active;
        char name[MAXSTR];
    } players[4];
};

struct netcmd_error
{
    uint8_t type;
    int16_t errcode;
};

union netcmd
{
    struct netcmd_base base;

    struct netcmd_ping    ping;
    struct netcmd_ack     ack;
    struct netcmd_knock   knock;
    struct netcmd_welcome welcome;
    struct netcmd_players players;
    struct netcmd_error   error;
};

enum
{
    NETWORK_STATUS_IDLE,
    NETWORK_STATUS_CONNECTING,  /* Waiting for server to approve join */
    NETWORK_STATUS_CONNECTED,
};

extern int network_status;

extern const char *network_error;

int network_send_cmd(union netcmd *cmd, uint32_t ip_addr, uint16_t port);
int network_recv_cmd(union netcmd *cmd, uint32_t *ip_addr, uint16_t *port);

int  network_client_init(void);
void network_client_close(void);
int  network_client_join(const char *hostname);
void network_client_process(void);

int  network_server_init(void);
void network_server_close(void);
void network_server_process(void);

#endif
