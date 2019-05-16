#include "binary.h"
#include "demo.h"
#include "common.h"
#include "course.h"
#include "hole.h"

#include "st_all.h"

#define DEMO_MAGIC (0xAF | 'N' << 8 | 'P' << 16 | 'R' << 24)
#define DEMO_VERSION 9

#define DATELEN sizeof ("YYYY-MM-DDTHH:MM:SS")

int demo_recording = 0;
int demo_playing = 0;

fs_file demo_fp;

/*---------------------------------------------------------------------------*/

static const char *demo_path(const char *name)
{
    static char path[MAXSTR];
    sprintf(path, "Replays/%s.npr", name);
    return path;
}

static const char *demo_name(const char *path)
{
    static char name[MAXSTR];
    SAFECPY(name, base_name_sans(path, ".npr"));
    return name;
}

/*---------------------------------------------------------------------------*/

static int demo_header_read(fs_file fp, struct demo *d)
{
    int magic;
    int version;
    int t;

    struct tm date;
    char datestr[DATELEN];

    magic   = get_index(fp);
    version = get_index(fp);

    //t = get_index(fp);
    t = 1;

    if (magic == DEMO_MAGIC && version == DEMO_VERSION && t)
    {
        /*
        d->timer = t;

        d->coins  = get_index(fp);
        d->status = get_index(fp);
        d->mode   = get_index(fp);
        */

        //get_string(fp, d->player, sizeof (d->player));
        get_string(fp, datestr, sizeof (datestr));
        
        printf("date: '%s'\n", datestr);
        fflush(stdout);

        sscanf(datestr,
               "%d-%d-%dT%d:%d:%d",
               &date.tm_year,
               &date.tm_mon,
               &date.tm_mday,
               &date.tm_hour,
               &date.tm_min,
               &date.tm_sec);

        date.tm_year -= 1900;
        date.tm_mon  -= 1;
        date.tm_isdst = -1;

        d->date = make_time_from_utc(&date);

        get_string(fp, d->shot, PATHMAX);
        get_string(fp, d->file, PATHMAX);
        
        printf("shot: '%s'\n", d->shot);
        printf("file: '%s'\n", d->file);
        
        d->score = get_index(fp);

        /*
        d->time  = get_index(fp);
        d->goal  = get_index(fp);
        (void)     get_index(fp);
        d->score = get_index(fp);
        d->balls = get_index(fp);
        d->times = get_index(fp);
        */

        return 1;
    }
    return 0;
}

static void demo_header_write(fs_file fp, struct demo *d)
{
    char datestr[DATELEN];

    strftime(datestr, sizeof (datestr), "%Y-%m-%dT%H:%M:%S", gmtime(&d->date));

    put_index(fp, DEMO_MAGIC);
    put_index(fp, DEMO_VERSION);
/*
    put_index(fp, 0);
    put_index(fp, 0);
    put_index(fp, 0);
    put_index(fp, d->mode);

    put_string(fp, d->player);
*/
    put_string(fp, datestr);

    put_string(fp, d->shot);
    put_string(fp, d->file);
    
    put_index(fp, d->score);

#if 0
    put_index(fp, d->time);
    put_index(fp, d->goal);
    put_index(fp, 0);                   /* Unused (was goal enabled flag).   */
    put_index(fp, d->score);
    put_index(fp, d->balls);
    put_index(fp, d->times);
#endif
}

/*---------------------------------------------------------------------------*/

static struct demo demo_play;

/* Initializes demo file for writing */
int demo_play_init(const char *name)
{
    struct demo *d = &demo_play;
    
    memset(d, 0, sizeof(*d));
    
    SAFECPY(d->path, demo_path(name));
    SAFECPY(d->name, name);
    SAFECPY(d->shot, course_shot(course_curr()));
    SAFECPY(d->file, hole_file(curr_hole()));

    printf("hole_file: '%s'\n", d->file);

    if ((demo_fp = fs_open_write(d->path)))
    {
        printf("wrote demo file '%s'\n", d->path);
        fflush(stdout);
        demo_header_write(demo_fp, d);
        demo_recording = 1;
        return 1;
    }
    return 0;
}

void demo_play_stop(int d)
{
    if (demo_fp)
    {
        if (!fs_close(demo_fp))
        {
            puts("failed to close\n");
            fflush(stdout);
        }
        demo_fp = NULL;

        if (d) fs_remove(demo_play.path);
    }
    demo_recording = 0;
}

/*---------------------------------------------------------------------------*/

static struct demo demo_replay;

int demo_replay_init(const char *path)
{
    if ((demo_fp = fs_open_read(path)))
    {
        if (demo_header_read(demo_fp, &demo_replay))
        {
            SAFECPY(demo_replay.path, path);
            SAFECPY(demo_replay.name, demo_name(path));
            
            const char *hole_path = fs_resolve(demo_replay.file);
            
            if (hole_path)
            {
                hole_init(NULL);
                if (hole_load(0, hole_path) &&
                    hole_load(1, hole_path) &&
                    hole_goto(1, 1))
                {
                    demo_playing = 1;
                    goto_state(&st_next);
                    return 1;
                }
            }
        }
    }
    
    return 0;
}
