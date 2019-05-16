#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "network.h"

#define DEFAULT_PORT 54321
#define PROTO_VERSION 1

const char *network_error = NULL;

#ifdef _WIN32
static int ws_initialized = 0;

static void init_winsock(void)
{
    if (!ws_initialized)
    {
        WSADATA wsadata;
        int result;

        if ((result = WSAStartup(MAKEWORD(2, 2), &wsadata)) != 0)
        {
            printf("WSAStartup failed: %i\n", result);
            fflush(stdout);
            return;
        }
        
        ws_initialized = 1;
    }
}
#endif

/*---------------------------------------------------------------------------*/
/* Client */

static int cl_sock = -1;

static struct sockaddr_in server_sa;  /* addr of server */

void network_client_init(void)
{
#ifdef _WIN32
    init_winsock();
#endif

    /* Create socket */
    cl_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (cl_sock == -1)
    {
        puts("client: error creating socket");
        fflush(stdout);
        return;
    }
    else
    {
        printf("client: created socket %i\n", cl_sock);
        fflush(stdout);
    }
}

void network_client_close(void)
{
    if (cl_sock > 0)
    {
#ifdef _WIN32
        closesocket(cl_sock);
#else
        close(cl_sock);
#endif
        printf("client: closed socket %i\n", cl_sock);
        fflush(stdout);
        cl_sock = -1;
    }
}

int network_client_join(const char *hostname)
{
    struct hostent *hostinfo;
    union netcmd cmd;
    int size;
    
    hostinfo = gethostbyname(hostname);
    if (hostinfo == NULL)
    {
        network_error = "Unknown host";
        return 0;
    }

    server_sa.sin_family = AF_INET;
    server_sa.sin_port = htons(DEFAULT_PORT);
    server_sa.sin_addr = *(struct in_addr *)hostinfo->h_addr;

    cmd.join.type = NETCMD_JOIN;
    cmd.join.version = PROTO_VERSION;

    size = sendto(cl_sock, (const char *)&cmd, sizeof(cmd), 0, (struct sockaddr *)&server_sa, 0);
    if (size != sizeof(cmd))
    {
        printf("client: sent %i of %i bytes\n", size, sizeof(cmd));
        if (size < 0)
            network_error = strerror(errno);
        else
            network_error = "Send error";
        return 0;
    }

    return 1;
}

void network_send_command(union netcmd *cmd)
{
    
}

/*---------------------------------------------------------------------------*/
/* Server */

static int server_sock = -1;

static struct sockaddr_in client_sa;  /* addr of client that sent command */

void network_server_init(void)
{
    struct sockaddr_in sa = {0};

#ifdef _WIN32
    init_winsock();
#endif

    /* Create socket */
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock == -1)
    {
        puts("server: error creating socket");
        fflush(stdout);
        return;
    }
    else
    {
        printf("server: created socket %i\n", server_sock);
        fflush(stdout);
    }

    /* Bind to port */
    sa.sin_family = AF_INET;  /* TODO: support IPV6 */
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(DEFAULT_PORT);
    if (bind(server_sock, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        puts("server: failed to bind to port");
        fflush(stdout);
        return;
    }
    else
    {
        printf("server: bound to port %i\n", DEFAULT_PORT);
        fflush(stdout);
    }
}

void network_server_close(void)
{
    if (server_sock > 0)
    {
#ifdef _WIN32
        closesocket(server_sock);
#else
        close(server_sock);
#endif
        printf("server: closed socket %i\n", server_sock);
        fflush(stdout);
        server_sock = -1;
    }
}

static int socket_ready(int sockfd)
{
    fd_set read_fd_set = {0};
    struct timeval timeout = {0};

    FD_SET(sockfd, &read_fd_set);

    return (select(0, &read_fd_set, NULL, NULL, &timeout) > 0);
}

int network_recv_command(union netcmd *cmd)
{
    if (socket_ready(server_sock))
    {
        size_t length;
        int addr_len;

        length = recvfrom(server_sock, (char *)cmd, sizeof(*cmd), MSG_WAITALL, (struct sockaddr *)&client_sa, &addr_len);
        /*
        if (length != -1)
        {
            puts("got data");
            getch();
        }
        */
        
        if (length == sizeof(*cmd))
        {
            puts("got cmd");
            fflush(stdout);
            return 1;
        }
        
        if (length == -1)
            ;//printf("server: recv error: %s\n", strerror(errno));
        else
            printf("server: invalid length %i, expected %u\n", length, sizeof(*cmd));
        fflush(stdout);
    }

    return 0;
}

/* Returns 1 if a player joined or quit */
int network_listen(void)
{
    union netcmd cmd;
    
    network_recv_command(&cmd);
    
    return 0;
}

void network_get_players(void)
{
    
}
