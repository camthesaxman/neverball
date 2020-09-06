#if ENABLE_DISCORD

#include <stdio.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <time.h>
#endif

#include "discord.h"
#include "log.h"
#include "config.h"

struct DiscordRichPresence
{
    const char *state;   /* max 128 bytes */
    const char *details; /* max 128 bytes */
    int64_t startTimestamp;
    int64_t endTimestamp;
    const char *largeImageKey;  /* max 32 bytes */
    const char *largeImageText; /* max 128 bytes */
    const char *smallImageKey;  /* max 32 bytes */
    const char *smallImageText; /* max 128 bytes */
    const char *partyId;        /* max 128 bytes */
    int partySize;
    int partyMax;
    const char *matchSecret;    /* max 128 bytes */
    const char *joinSecret;     /* max 128 bytes */
    const char *spectateSecret; /* max 128 bytes */
    int8_t instance;
};

struct DiscordUser
{
    const char *userId;
    const char *username;
    const char *discriminator;
    const char *avatar;
};

struct DiscordEventHandlers
{
    void (*ready)(const struct DiscordUser *request);
    void (*disconnected)(int errorCode, const char *message);
    void (*errored)(int errorCode, const char *message);
    void (*joinGame)(const char *joinSecret);
    void (*spectateGame)(const char *spectateSecret);
    void (*joinRequest)(const struct DiscordUser *request);
};

static int initialized = 0;
static void *lib = NULL;

static void (*Discord_Initialize)(const char *, struct DiscordEventHandlers *, int, const char *);
static void (*Discord_Shutdown)(void);
static void (*Discord_UpdatePresence)(struct DiscordRichPresence *);

static struct DiscordRichPresence presence = {0};

static uint64_t ms_since_epoch(void)
{
#ifdef _WIN32
    FILETIME time = {0};
    uint64_t result;

    GetSystemTimeAsFileTime(&time);
    result = ((uint64_t)time.dwHighDateTime << 32) | time.dwLowDateTime;
    result /= 10000;  // Convert to milliseconds
    // Windows epoch is Jan 1, 1601, but we need the time since Jan 1, 1970
    return result - 11644473600000;
#else
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
#endif
}

static int load_discord_library(void)
{
#ifdef _WIN32
    lib = LoadLibrary("discord-rpc.dll");
    if (lib == NULL)
    {
        char *msg;
        DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM
                    | FORMAT_MESSAGE_IGNORE_INSERTS;
        FormatMessageA(flags, NULL, GetLastError(), 0, (char *)&msg, 0, NULL);
        log_printf("Failed to open Discord library: %s\n", msg);
        LocalFree(msg);
        return 0;
    }

    Discord_Initialize     = GetProcAddress(lib, "Discord_Initialize");
    Discord_Shutdown       = GetProcAddress(lib, "Discord_Shutdown");
    Discord_UpdatePresence = GetProcAddress(lib, "Discord_UpdatePresence");
#else
    lib = dlopen("./libdiscord-rpc.so", RTLD_LAZY);
    if (lib == NULL)
    {
        log_printf("Failed to open Discord library: %s\n", dlerror());
        return 0;
    }

    Discord_Initialize     = dlsym(lib, "Discord_Initialize");
    Discord_Shutdown       = dlsym(lib, "Discord_Shutdown");
    Discord_UpdatePresence = dlsym(lib, "Discord_UpdatePresence");
#endif

    if (Discord_Initialize != NULL
     && Discord_Shutdown != NULL
     && Discord_UpdatePresence != NULL)
        return 1;

    log_printf("Unable to use Discord library\n");
    return 0;
}

static void on_ready(const struct DiscordUser *user)
{
    log_printf("on_ready: %s, %s\n", user->username, user->userId);
}

static void on_errored(int errorCode, const char *message)
{
    log_printf("on_errored: %i, %s\n", errorCode, message);
}

static void on_disconnected(int errorCode, const char *message)
{
    log_printf("on_disconnected: %i, %s\n", errorCode, message);
}

void discord_init(const char *appid, const char *imagekey)
{
    printf("discord: %i\n", config_get_d(CONFIG_DISCORD));

    if (config_get_d(CONFIG_DISCORD))
    {
        if (load_discord_library())
        {
            struct DiscordEventHandlers handlers = {0};

            handlers.ready = on_ready;
            handlers.errored = on_errored;
            handlers.disconnected = on_disconnected;
            Discord_Initialize(appid, &handlers, 0, NULL);

            presence.details = "https://neverball.org";
            presence.largeImageKey = imagekey;
            presence.startTimestamp = ms_since_epoch();
            Discord_UpdatePresence(&presence);

            log_printf("Discord Rich Presence enabled\n");
            initialized = 1;
        }
    }
}

void discord_update_level(const char *set, const char *level)
{
    if (initialized)
    {
        static char buffer[MAXSTR];

        if (set == NULL)
            presence.state = NULL;
        else
        {
            snprintf(buffer, sizeof(buffer), "%s - %s", set, level);
            presence.state = buffer;
        }

        Discord_UpdatePresence(&presence);
    }
}

void discord_quit(void)
{
    if (initialized)
    {
        Discord_Shutdown();
#ifdef _WIN32
        FreeLibrary(lib);
#else
        dlclose(lib);
#endif
        initialized = 0;
    }
}

#endif
