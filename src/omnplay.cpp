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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#include "timecode.h"

#include "omplrclnt.h"

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
    pthread_mutex_lock(&player->app->players.lock);
    r = OmPlrOpen(player->host, player->name, (OmPlrHandle*)&player->handle);
    pthread_mutex_unlock(&player->app->players.lock);
    if(r)
    {
        fprintf(stderr, "ERROR: OmPlrOpen(%s, %s) failed with 0x%.8X\n",
            player->host, player->name, r);

        return (void*)r;
    };

    /* setup to do not reconnect */
    pthread_mutex_lock(&player->app->players.lock);
    OmPlrSetRetryOpen((OmPlrHandle)player->handle, 0);
    pthread_mutex_unlock(&player->app->players.lock);

    /* setup directory */
    if(player->app->players.path[0])
    {
        pthread_mutex_lock(&player->app->players.lock);
        r = OmPlrClipSetDirectory((OmPlrHandle)player->handle, player->app->players.path);
        pthread_mutex_unlock(&player->app->players.lock);

        if(r)
        {
            fprintf(stderr, "ERROR: OmPlrClipSetDirectory(%s) failed with 0x%.8X\n",
                player->app->players.path, r);

            pthread_mutex_lock(&player->app->players.lock);
            OmPlrClose((OmPlrHandle)player->handle);
            pthread_mutex_unlock(&player->app->players.lock);

            return (void*)r;
        };
    };

    /* endless loop */
    for(r = 0 ; !player->app->f_exit && !r;)
    {
        /* sleep */
        usleep(100000);

        /* get status */
        pthread_mutex_lock(&player->app->players.lock);
        st_curr.size = sizeof(OmPlrStatus);
        r = OmPlrGetPlayerStatus((OmPlrHandle)player->handle, &st_curr);
        pthread_mutex_unlock(&player->app->players.lock);

        if(r)
            fprintf(stderr, "ERROR: OmPlrGetPlayerStatus failed with 0x%.8X\n", r);
        else
            if(memcmp(&st_curr, &st_prev, sizeof(OmPlrStatus)))
                omnplay_update_status(player, &st_prev , &st_curr);
    };

    pthread_mutex_lock(&player->app->players.lock);
    OmPlrClose((OmPlrHandle)player->handle);
    pthread_mutex_unlock(&player->app->players.lock);

    return NULL;
};

static int get_first_selected_item_playlist(omnplay_instance_t* app)
{
    int idx;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->playlist_grid));
    if(selection && gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_tree_model_get(model, &iter, 7, &idx, -1);
        return idx;
    };
    return -1;
};

static int get_playlist_block(omnplay_instance_t* app, int idx, int* start_ptr, int* stop_ptr)
{
    int start, stop;

    for(start = idx; start >= 0; start--)
        if(app->playlist.item[start].type & OMNPLAY_PLAYLIST_BLOCK_BEGIN)
            break;

    for(stop = idx; stop < app->playlist.count; stop++)
        if(app->playlist.item[stop].type & OMNPLAY_PLAYLIST_BLOCK_END)
            break;

    fprintf(stderr, "get_playlist_block: range %d -> %d\n", start, stop);

    /* check block range */
    if(start >= 0 && stop < app->playlist.count)
    {
        *start_ptr = start;
        *stop_ptr = stop;
        return (stop - start + 1);
    };

    return -1;
};

static omnplay_player_t *get_player_at_pos(omnplay_instance_t* app, int pos)
{
    /* check player range */
    if(app->playlist.item[pos].player > -1 && app->playlist.item[pos].player < app->players.count)
        return &app->players.item[app->playlist.item[pos].player];

    return NULL;
};

static void omnplay_ctl(omnplay_instance_t* app, control_buttons_t button)
{
    int i, r;
    int idx, start, stop;
    omnplay_player_t *player;

    pthread_mutex_lock(&app->playlist.lock);

    idx = get_first_selected_item_playlist(app);

    if(idx < 0)
    {
        pthread_mutex_unlock(&app->playlist.lock);
        return;
    };

    fprintf(stderr, "cue: selected item is %d\n", idx);

    if(get_playlist_block(app, idx, &start, &stop) < 0)
    {
        pthread_mutex_unlock(&app->playlist.lock);
        return;
    };

    fprintf(stderr, "cue: range %d -> %d\n", start, stop);

    player = get_player_at_pos(app, start);

    if(!player)
    {
        pthread_mutex_unlock(&app->playlist.lock);
        return;
    };

    pthread_mutex_lock(&app->players.lock);

    if(BUTTON_PLAYER_STOP == button || BUTTON_PLAYER_CUE == button)
    {
        /* stop */
        OmPlrStop((OmPlrHandle)player->handle);

        /* detach previous clips */
        player->playlist_start = -1;
        player->playlist_count = -1;
        OmPlrDetachAllClips((OmPlrHandle)player->handle);
    };

    if(BUTTON_PLAYER_CUE == button)
    {
        int o, c, p = 0;

        /* Attach clips to timeline */
        for(i = start, c = 0, o = 0; i <= stop; i++)
        {
            OmPlrClipInfo clip;

            /* get clip info */
            clip.maxMsTracks = 0; 
            clip.size = sizeof(clip);
            r = OmPlrClipGetInfo((OmPlrHandle)player->handle, app->playlist.item[i].id, &clip);

            if(!r)
            {
                unsigned int l;

                fprintf(stderr, "OmPlrClipGetInfo(%s): firstFrame=%d, lastFrame=%d\n",
                    app->playlist.item[i].id, clip.firstFrame, clip.lastFrame);

                /* should we fix playlist clip timings */
                if(!(
                    app->playlist.item[i].in >= clip.firstFrame &&
                    app->playlist.item[i].in + app->playlist.item[i].dur <= clip.lastFrame) ||
                    !app->playlist.item[i].dur)
                {
                    fprintf(stderr, "cue: item [%s] will be updated [%d;%d]->[%d;%d]\n",
                        app->playlist.item[i].id,
                        app->playlist.item[i].in, app->playlist.item[i].dur,
                        clip.firstFrame, clip.lastFrame - clip.firstFrame);

                    app->playlist.item[i].in = clip.firstFrame;
                    app->playlist.item[i].dur = clip.lastFrame - clip.firstFrame;
                    omnplay_playlist_draw_item(app, i);
                };

                r = OmPlrAttach((OmPlrHandle)player->handle,
                    app->playlist.item[i].id,
                    app->playlist.item[i].in,
                    app->playlist.item[i].in + app->playlist.item[i].dur,
                    0, omPlrShiftModeAfter, &l);
            };

            if(r)
            {
                fprintf(stderr, "cue: failed with %d, %s\n", r, OmPlrGetErrorString((OmPlrError)r));
                app->playlist.item[i].omn_idx = -1;
                app->playlist.item[i].omn_offset = -1;
            }
            else
            {
                app->playlist.item[i].omn_idx = c;
                app->playlist.item[i].omn_offset = o;

                /* save selected item offset */
                if(i == idx) p = o;

                c++;
                o += app->playlist.item[i].dur;
            };
        };

        if(c)
        {
            OmPlrStatus hs;

            /* Set timeline min/max */
            OmPlrSetMinPosMin((OmPlrHandle)player->handle);
            OmPlrSetMaxPosMax((OmPlrHandle)player->handle);

            /* Set timeline position */
            hs.minPos = 0;
            hs.size = sizeof(OmPlrStatus);
            OmPlrGetPlayerStatus((OmPlrHandle)player->handle, &hs);
            OmPlrSetPos((OmPlrHandle)player->handle, hs.minPos + p);

            /* Cue */
            OmPlrCuePlay((OmPlrHandle)player->handle, 0.0);
            OmPlrPlay((OmPlrHandle)player->handle, 0.0);

            player->playlist_start = start;
            player->playlist_count = stop - start + 1;
        };
    };

    if(BUTTON_PLAYER_PLAY == button)
    {
        /* play */
        OmPlrPlay((OmPlrHandle)player->handle, 1.0);
    };

    if(BUTTON_PLAYER_PAUSE == button)
        /* pause */
        OmPlrPlay((OmPlrHandle)player->handle, 0.0);

    pthread_mutex_unlock(&app->players.lock);

    pthread_mutex_unlock(&app->playlist.lock);
};

static gboolean omnplay_button_click(omnplay_instance_t* app, control_buttons_t button)
{
    switch(button)
    {
        case BUTTON_PLAYLIST_ITEM_ADD:
        case BUTTON_PLAYLIST_ITEM_DEL:
        case BUTTON_PLAYLIST_ITEM_EDIT:
            break;
        case BUTTON_PLAYLIST_LOAD:
            omnplay_playlist_load(app);
            break;
        case BUTTON_PLAYLIST_SAVE:
            omnplay_playlist_save(app);
            break;
        case BUTTON_PLAYLIST_BLOCK_SINGLE:
        case BUTTON_PLAYLIST_BLOCK_LOOP:
            break;
        case BUTTON_PLAYLIST_ITEM_UP:
        case BUTTON_PLAYLIST_ITEM_DOWN:
            break;
        case BUTTON_PLAYER_CUE:
        case BUTTON_PLAYER_PLAY:
        case BUTTON_PLAYER_PAUSE:
        case BUTTON_PLAYER_STOP:
            omnplay_ctl(app, button);
            break;
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
    pthread_mutexattr_t attr;

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    gtk_signal_connect( GTK_OBJECT( app->window ), "destroy",
        GTK_SIGNAL_FUNC(on_main_window_delete_event), app);

    /* create lock */
    pthread_mutex_init(&app->players.lock, &attr);

    /* create a omneon status thread */
    for(i = 0; i < app->players.count; i++)
        pthread_create(&app->players.item[i].thread, NULL,
            omnplay_thread_proc, &app->players.item[i]);

    /* create lock */
    pthread_mutex_init(&app->playlist.lock, &attr);

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
        /* create a omneon status thread */
        pthread_join(app->players.item[i].thread, &r);

    /* destroy lock */
    pthread_mutex_destroy(&app->players.lock);

    /* destroy lock */
    pthread_mutex_destroy(&app->playlist.lock);
};
