#include "audio.h"
#include "config.h"
#include "game.h"
#include "gui.h"
#include "network.h"
#include "state.h"
#include "text.h"
#include "util.h"

#include "st_all.h"
#include "st_netsetup.h"

enum
{
    NETSETUP_OK = 100,
};

extern struct state st_netconnect;

/*---------------------------------------------------------------------------*/

static int ipaddr_id;
static int enter_id;

static int netjoin_action(int i, int val)
{
    switch (i)
    {
    case GUI_BACK:
        audio_play(AUD_MENU, 1.f);
        goto_state(&st_party);
        break;
    case NETSETUP_OK:
        audio_play(AUD_MENU, 1.f);
        goto_state(&st_netconnect);
        break;
    case GUI_CL:
        gui_keyboard_lock();
        break;
    case GUI_BS:
        text_input_del();
        break;
    case GUI_CHAR:
        printf("char: %c\n", val);
        fflush(stdout);
        text_input_char(val);
        break;
    }
    return 1;
}

static void on_text_input(int typing)
{
    if (ipaddr_id)
    {
        gui_set_label(ipaddr_id, text_input);

        if (typing)
            audio_play(AUD_MENU, 1.0f);
    }
}

static int netjoin_enter(struct state *st, struct state *prev)
{
    int id, jd;

    text_input_start(on_text_input);

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Enter Host Address"), GUI_MED, 0, 0);
        gui_space(id);
        
        ipaddr_id = gui_label(id, " ", GUI_MED, gui_yel, gui_yel);
        
        gui_space(id);
        gui_keyboard(id);
        gui_space(id);
        
        if ((jd = gui_hstack(id)))
        {
            enter_id = gui_start(jd, _("OK"), GUI_SML, NETSETUP_OK, 0);
            gui_space(jd);
            gui_state(jd, _("Back"), GUI_SML, GUI_BACK, 0);
        }
        
        gui_layout(id, 0, 0);
        
        gui_set_trunc(ipaddr_id, TRUNC_HEAD);
        gui_set_label(ipaddr_id, text_input);
    }

    return id;
}

static void netjoin_leave(struct state *st, struct state *next, int id)
{
    text_input_stop();

    gui_delete(id);
}

static void netjoin_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);
}

static void netjoin_timer(int id, float dt)
{
    gui_timer(id, dt);
}

static void netjoin_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static int netjoin_click(int b, int d)
{
    return gui_click(b, d) ? netjoin_action(gui_token(gui_active()), gui_value(gui_active())) : 1;
}

static int netjoin_keybd(int c, int d)
{
    if (d)
    {
        if (c == KEY_EXIT)
            return netjoin_action(GUI_BACK, 0);

        if (c == '\b' || c == 0x7F)
        {
            gui_focus(enter_id);
            return netjoin_action(GUI_BS, 0);
        }
        else
        {
            gui_focus(enter_id);
            return 1;
        }
    }
    return 1;
}

static int netjoin_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
        {
            int tok = gui_token(gui_active());
            int val = gui_value(gui_active());

            return netjoin_action(tok, (tok == GUI_CHAR ?
                                     gui_keyboard_char(val) :
                                     val));
        }
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_B, b))
            netjoin_action(GUI_BACK, 0);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

static int status_id;
static int connecting;

static int netconnect_action(int i)
{
    switch (i)
    {
    case GUI_BACK:
        audio_play(AUD_MENU, 1.f);
        goto_state(&st_netjoin);
        break;
    }
    return 1;
}

static int netconnect_enter(struct state *st, struct state *prev)
{
    int id, jd;

    connecting = 1;

    if ((id = gui_vstack(0)))
    {
        char buf[1000];
        
        snprintf(buf, sizeof(buf), _("Connecting to %s..."), text_input);
        status_id = gui_label(id, buf, GUI_SML, 0, 0);
        
        gui_space(id);
        
        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            gui_state(jd, _("Back"), GUI_SML, GUI_BACK, 0);
        }

        gui_layout(id, 0, 0);
    }
    
    network_client_init();
    
    return id;
}

static void netconnect_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
    network_client_close();
}

static void netconnect_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);
}

static void netconnect_timer(int id, float dt)
{
    if (connecting)
    {
        puts("connecting"); fflush(stdout);
        if (network_client_join(text_input))
        {
            gui_set_label(status_id, "Connected!");
            gui_set_color(status_id, gui_grn, gui_grn);
        }
        else
        {
            gui_set_label(status_id, network_error);
            gui_set_color(status_id, gui_red, gui_red);
        }

        connecting = 0;
    }
    gui_timer(id, dt);
}

static void netconnect_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static int netconnect_click(int b, int d)
{
    return gui_click(b, d) ? netconnect_action(gui_token(gui_active())) : 1;
}

/*---------------------------------------------------------------------------*/

static int player_list_id;

static int nethost_action(int i)
{
    switch (i)
    {
    case GUI_BACK:
        audio_play(AUD_MENU, 1.f);
        goto_state(&st_party);
        break;
    }
    return 1;
}

static int nethost_enter(struct state *st, struct state *prev)
{
    int id, jd;

    if ((id = gui_vstack(0)))
    {
        gui_label(id, _("Host Game"), GUI_MED, 0, 0);
        
        gui_space(id);
        
        gui_label(id, _("Waiting for players to join..."), GUI_SML, gui_wht, gui_wht);
        
        player_list_id = gui_harray(id);
        
        gui_space(id);
        
        if ((jd = gui_hstack(id)))
        {
            gui_filler(jd);
            gui_state(jd, _("Back"), GUI_SML, GUI_BACK, 0);
        }
        
        gui_layout(id, 0, 0);
    }
    
    network_server_init();
    
    return id;
}

static void nethost_leave(struct state *st, struct state *next, int id)
{
    gui_delete(id);
    network_server_close();
}

static void nethost_paint(int id, float t)
{
    game_draw(0, t);
    gui_paint(id);
}

static void nethost_timer(int id, float dt)
{
    gui_timer(id, dt);
    
    /* listen for joining players */
    network_listen();
}

static void nethost_point(int id, int x, int y, int dx, int dy)
{
    gui_pulse(gui_point(id, x, y), 1.2f);
}

static int nethost_click(int b, int d)
{
    return gui_click(b, d) ? nethost_action(gui_token(gui_active())) : 1;
}

static int nethost_buttn(int b, int d)
{
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_netjoin = {
    netjoin_enter,
    netjoin_leave,
    netjoin_paint,
    netjoin_timer,
    netjoin_point,
    NULL,
    NULL,
    netjoin_click,
    netjoin_keybd,
    netjoin_buttn,
};

struct state st_netconnect = {
    netconnect_enter,
    netconnect_leave,
    netconnect_paint,
    netconnect_timer,
    netconnect_point,
    NULL,
    NULL,
    netconnect_click,
    NULL,
    NULL,
};

struct state st_nethost = {
    nethost_enter,
    nethost_leave,
    nethost_paint,
    nethost_timer,
    nethost_point,
    NULL,
    NULL,
    nethost_click,
    NULL,
    nethost_buttn,
};
