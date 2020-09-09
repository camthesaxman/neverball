/*
 * Copyright (C) 2003 Robert Kooima
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

/*---------------------------------------------------------------------------*/

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#include "version.h"
#include "glext.h"
#include "config.h"
#include "video.h"
#include "image.h"
#include "audio.h"
#include "demo.h"
#include "progress.h"
#include "gui.h"
#include "set.h"
#include "fs.h"
#include "common.h"
#include "text.h"
#include "mtrl.h"
#include "geom.h"
#include "capture.h"

#include "st_conf.h"
#include "st_title.h"
#include "st_demo.h"
#include "st_level.h"
#include "st_pause.h"

#define CAPTURE_FPS 30

const char TITLE[] = "Neverball Recorder " VERSION;
const char ICON[] = "icon/neverball.png";

/*---------------------------------------------------------------------------*/

static int loop(void)
{
    SDL_Event e;

    /* Process SDL events. */

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return 0;
    }

    return 1;
}

/*---------------------------------------------------------------------------*/

static char *opt_data;
static char *opt_replay;
static char *opt_level;
static char *opt_video;
static unsigned int opt_fps = 30;
static unsigned int opt_width = 854;
static unsigned int opt_height = 480;

#define opt_usage                                                     \
    "Usage: %s [options ...]\n"                                       \
    "Options:\n"                                                      \
    "  -h, --help                show this usage message.\n"          \
    "  -v, --version             show version.\n"                     \
    "  -f, --framerate <n>       record video at 'n' frames per second (default: 30).\n"  \
    "  -r, --resolution <WxH>    set video resolution to 'W' x 'H' (default: 854x480).\n"

static void usage(const char *program, int exit_status)
{
    printf(opt_usage, program);
    exit(exit_status);
}

static void opt_error(const char *option)
{
    fprintf(stderr, "Option '%s' requires an argument.\n", option);
    exit(EXIT_FAILURE);
}

static void opt_parse(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (arg[0] == '-')
        {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
                usage(argv[0], EXIT_SUCCESS);
            else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
            {
                puts(VERSION);
                exit(EXIT_SUCCESS);
            }
            else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--framerate")    == 0)
            {
                if (i + 1 == argc)
                {
                    opt_error(argv[i]);
                    exit(EXIT_FAILURE);
                }

                if (sscanf(argv[i + 1], "%u", &opt_fps) != 1)
                    usage(argv[0], EXIT_FAILURE);

                i++;
            }
            else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--resolution") == 0)
            {
                if (i + 1 == argc)
                {
                    opt_error(argv[i]);
                    exit(EXIT_FAILURE);
                }

                if (sscanf(argv[i + 1], "%ux%u", &opt_width, &opt_height) != 2)
                    usage(argv[0], EXIT_FAILURE);

                i++;
            }
        }
        else
        {
            if (opt_replay == NULL)
                opt_replay = arg;
            else if (opt_video == NULL)
                opt_video = arg;
            else
                usage(argv[0], EXIT_FAILURE);
        }
    }

    if (opt_replay == NULL || opt_video == NULL)
        usage(argv[0], EXIT_FAILURE);
}

#undef opt_usage

/*---------------------------------------------------------------------------*/

static int is_replay(struct dir_item *item)
{
    return str_ends_with(item->path, ".nbr");
}

static int is_score_file(struct dir_item *item)
{
    return str_starts_with(item->path, "neverballhs-");
}

/*---------------------------------------------------------------------------*/

struct main_loop
{
    Uint32 now;
    unsigned int done:1;
};

static void step(void *data)
{
    struct main_loop *mainloop = (struct main_loop *) data;

    int running = loop();

    if (running)
    {
        Uint32 now = mainloop->now + 1000 / opt_fps;
        Uint32 dt = (now - mainloop->now);

        if (0 < dt && dt < 1000)
        {
            /* Step the game state. */

            st_timer(0.001f * dt);

            /* Render. */

            st_paint(0.001f * now);
            capture_put_frame();
            video_swap();
        }

        mainloop->now = now;
    }

    mainloop->done = !running;
}

int main(int argc, char *argv[])
{
    struct main_loop mainloop = { 0 };

    if (!fs_init(argc > 0 ? argv[0] : NULL))
    {
        fprintf(stderr, "Failure to initialize virtual file system (%s)\n",
                fs_error());
        return 1;
    }

    opt_parse(argc, argv);

    printf("replay = %s\nvideo out = %s\nfps = %u\nres = %ux%u\n", opt_replay, opt_video, opt_fps, opt_width, opt_height);

    config_paths(opt_data);
    log_init("Neverball", "neverball.log");

    /* Initialize SDL. */

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1)
    {
        log_printf("Failure to initialize SDL (%s)\n", SDL_GetError());
        return 1;
    }

    /* Intitialize configuration. */

    config_init();
    config_load();

    config_set_d(CONFIG_WIDTH, opt_width);
    config_set_d(CONFIG_HEIGHT, opt_height);

    /* Initialize localization. */

    lang_init();

    /* Initialize audio. */

    audio_init();

    /* Initialize video. */

    if (!video_init())
        return 1;

    capture_init(opt_video, opt_fps);

    /* Material system. */

    mtrl_init();

    /* Screen states. */

    init_state(&st_null);

    /* Initialize demo playback or load the level. */

    if (opt_replay &&
        fs_add_path(dir_name(opt_replay)) &&
        progress_replay(base_name(opt_replay)))
    {
        demo_play_goto(1);
        goto_state(&st_demo_play);
    }
    else
        goto_state(&st_title);

    /* Run the main game loop. */

    mainloop.now = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    /*
     * The Emscripten main loop is asynchronous. The third parameter
     * basically just determines what happens with main() beyond this point:
     *
     *   0 = execution continues to the end of the function.
     *   1 = execution stops here, the rest of the code is never reached.
     *
     * In either scenario, the shutdown code below is in a bad place. TODO.
     */
    emscripten_set_main_loop_arg(step, (void *) &mainloop, 0, 1);
#else
    while (!mainloop.done)
        step(&mainloop);
#endif

    //config_save();

    capture_quit();

    mtrl_quit();

    SDL_Quit();

    return 0;
}

/*---------------------------------------------------------------------------*/

