#ifndef DISCORD_H
#define DISCORD_H

int discord_init(const char *appid, const char *imagekey);
void discord_update_level(const char *set, const char *level);
void discord_quit(void);

#endif
