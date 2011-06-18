/*
 * omnplay.c -- GTK+ 2 omnplay
 * Copyright (C) 2011 Maksym Veremeyenko <verem@m1stereo.tv>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pthread.h>

#include "omnplay.h"
#include "ui.h"
#include "opts.h"

#include "omplrclnt.h"

static char* frames2tc( int f, float fps, char* buf )
{
    int tc[4] = { 0, 0, 0, 0 };
    float d;
    int t;

    if ( fps && f >= 0)
    {
        d = f / fps;
        t = d;

        tc[0] = (d - t) * fps;
        tc[1] = t % 60; t /= 60;
        tc[2] = t % 60; t /= 60;
        tc[3] = t % 24;
    }

    sprintf(buf, "%.2d:%.2d:%.2d:%.2d", tc[3], tc[2], tc[1], tc[0]);

    return buf;
}


static gboolean on_main_window_delete_event( GtkWidget *widget, GdkEvent *event, gpointer user_data )
{
    gtk_exit(0);
    return TRUE;
}

omnplay_instance_t* omnplay_create(int argc, char** argv)
{
    int i, c;
    omnplay_instance_t* app;

    /* prepare application instance */
    app = (omnplay_instance_t*)malloc(sizeof(omnplay_instance_t));
    memset(app, 0, sizeof(omnplay_instance_t));

    /* load parameters from command line */
    if(!omnplay_opt(argc, argv, app) && app->players.count)
        app->window = ui_omnplay(app);
    else
        omnplay_usage();

    return app;
};

void omnplay_destroy(omnplay_instance_t* app)
{
    free(app);
};

static void omnplay_update_status(omnplay_player_t* player, OmPlrStatus *prev , OmPlrStatus *curr)
{
    char tc_cur[32], tc_rem[32], state[32], status[32];
    const char *clip;

    if(curr)
    {
        frames2tc(curr->pos - curr->minPos, 25.0, tc_cur);
        frames2tc(curr->maxPos - curr->pos, 25.0, tc_rem);
        strcpy(status, "ONLINE");
        clip = curr->currClipName;

        switch(curr->state)
        {
            case omPlrStateStopped:     strcpy(state, "STOPPED");       break;
            case omPlrStateCuePlay:     strcpy(state, "CUE_PLAY");      break;
            case omPlrStatePlay:        strcpy(state, "PLAY");          break;
            case omPlrStateCueRecord:   strcpy(state, "CUE_RECORD");    break;
            case omPlrStateRecord:      strcpy(state, "RECORD");        break;
        };
    }
    else
    {
        tc_cur[0] = 0;
        tc_rem[0] = 0;
        clip = "";
        state[0] = 0;
        strcpy(status, "OFFLINE");
    };

    gdk_threads_enter();
    gtk_label_set_text(GTK_LABEL (player->label_tc_cur), tc_cur);
    gtk_label_set_text(GTK_LABEL (player->label_tc_rem), tc_rem);
    gtk_label_set_text(GTK_LABEL (player->label_state), state);
    gtk_label_set_text(GTK_LABEL (player->label_status), status);
    gtk_label_set_text(GTK_LABEL (player->label_clip), clip);
    gdk_flush();
    gdk_threads_leave();

    memcpy(prev, curr, sizeof(OmPlrStatus));
};

static void* omnplay_thread_proc(void* data)
{
    int r;
    OmPlrStatus st_curr, st_prev;
    omnplay_player_t* player = (omnplay_player_t*)data;

    /* connect */
    pthread_mutex_lock(&player->lock);
    r = OmPlrOpen(player->host, player->name, (OmPlrHandle*)&player->handle);
    pthread_mutex_unlock(&player->lock);
    if(r)
    {
        fprintf(stderr, "ERROR: OmPlrOpen(%s, %s) failed with 0x%.8X\n",
            player->host, player->name, r);

        return (void*)r;
    };

    /* setup to do not reconnect */
    pthread_mutex_lock(&player->lock);
    OmPlrSetRetryOpen((OmPlrHandle)player->handle, 0);
    pthread_mutex_unlock(&player->lock);

    /* setup directory */
    if(player->app->players.path[0])
    {
        pthread_mutex_lock(&player->lock);
//        r = OmPlrClipSetDirectory((OmPlrHandle)player->handle, player->app->players.path);
        pthread_mutex_unlock(&player->lock);

        if(r)
        {
            fprintf(stderr, "ERROR: OmPlrClipSetDirectory(%s) failed with 0x%.8X\n",
                player->app->players.path, r);

            pthread_mutex_lock(&player->lock);
            OmPlrClose((OmPlrHandle)player->handle);
            pthread_mutex_unlock(&player->lock);

            return (void*)r;
        };
    };

    /* endless loop */
    for(r = 0 ; !player->app->f_exit && !r;)
    {
        /* sleep */
        usleep(100000);

        /* get status */
        pthread_mutex_lock(&player->lock);
        st_curr.size = sizeof(OmPlrStatus);
        r = OmPlrGetPlayerStatus((OmPlrHandle)player->handle, &st_curr);
        pthread_mutex_unlock(&player->lock);

        if(r)
            fprintf(stderr, "ERROR: OmPlrGetPlayerStatus failed with 0x%.8X\n", r);
        else
            if(memcmp(&st_curr, &st_prev, sizeof(OmPlrStatus)))
                omnplay_update_status(player, &st_prev , &st_curr);
    };

    pthread_mutex_lock(&player->lock);
    OmPlrClose((OmPlrHandle)player->handle);
    pthread_mutex_unlock(&player->lock);

    return NULL;
};

static gboolean omnplay_button_click(omnplay_instance_t* app, control_buttons_t button)
{
    switch(button)
    {
        case BUTTON_PLAYLIST_ITEM_ADD:
        case BUTTON_PLAYLIST_ITEM_DEL:
        case BUTTON_PLAYLIST_ITEM_EDIT:
        case BUTTON_PLAYLIST_LOAD:
        case BUTTON_PLAYLIST_SAVE:
        case BUTTON_PLAYLIST_BLOCK_SINGLE:
        case BUTTON_PLAYLIST_BLOCK_LOOP:
        case BUTTON_PLAYLIST_ITEM_UP:
        case BUTTON_PLAYLIST_ITEM_DOWN:
        case BUTTON_PLAYER_CUE:
        case BUTTON_PLAYER_PLAY:
        case BUTTON_PLAYER_PAUSE:
        case BUTTON_PLAYER_STOP:
        case BUTTON_LIBRARY_ADD:
        case BUTTON_LIBRARY_REFRESH:
            break;
    };

    return TRUE;
};

static gboolean on_button_click(GtkWidget *button, gpointer user_data)
{
    int i;
    omnplay_instance_t* app = (omnplay_instance_t*)user_data;

    for(i = 1; i < BUTTON_LAST; i++)
        if(app->buttons[i] == button)
            return omnplay_button_click(app, (control_buttons_t)i);

    return FALSE;
};

void omnplay_init(omnplay_instance_t* app)
{
    int i;

    gtk_signal_connect( GTK_OBJECT( app->window ), "destroy",
        GTK_SIGNAL_FUNC(on_main_window_delete_event), app);

    for(i = 0; i < app->players.count; i++)
    {
        /* create lock */
        pthread_mutex_init(&app->players.item[i].lock, NULL);

        /* create a omneon status thread */
        pthread_create(&app->players.item[i].thread, NULL,
            omnplay_thread_proc, &app->players.item[i]);
    };

    /* create lock */
    pthread_mutex_init(&app->playlist.lock, NULL);

    /* attach buttons click */
    for(i = 1; i < BUTTON_LAST; i++)
        gtk_signal_connect(GTK_OBJECT(app->buttons[i]), "clicked",
            GTK_SIGNAL_FUNC( on_button_click), app );

};

void omnplay_release(omnplay_instance_t* app)
{
    int i;
    void* r;

    app->f_exit = 1;

    for(i = 0; i < app->players.count; i++)
    {
        /* create a omneon status thread */
        pthread_join(app->players.item[i].thread, &r);

        /* create lock */
        pthread_mutex_destroy(&app->players.item[i].lock);

    };
};
