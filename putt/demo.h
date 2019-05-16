#ifndef DEMO_H
#define DEMO_H

#include "base_config.h"

struct demo
{
    char   path[MAXSTR];                /* Demo path                         */
    char   name[PATHMAX];               /* Demo basename                     */

//    char   player[MAXSTR];
    time_t date;

#if 0
    int    timer;
    int    coins;
    int    status;
    int    mode;
#endif

    char   shot[PATHMAX];               /* Image filename                    */
    char   file[PATHMAX];               /* Level filename                    */

    int score;
#if 0
    int    time;                        /* Time limit                        */
    int    goal;                        /* Coin limit                        */
    int    score;                       /* Total coins                       */
    int    balls;                       /* Number of balls                   */
    int    times;                       /* Total time                        */
#endif
};

int demo_recording;
int demo_playing;

int demo_play_init(const char *name);
void demo_play_stop(int d);

int demo_replay_init(const char *path);

#endif