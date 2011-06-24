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

int omnplay_get_content(omnplay_instance_t* app, playlist_item_t *items, int limit,
    omnplay_get_content_cb_proc proc, void* data)
{
    int r, c = 0;
    OmPlrClipInfo clip_info;
    char clip_name[omPlrMaxClipDirLen];

    pthread_mutex_lock(&app->players.lock);

    r = OmPlrClipGetFirst((OmPlrHandle)app->players.item[0].handle, clip_name, sizeof(clip_name));
    for(; c < limit && !r;)
    {
        /* get clip info */
        clip_info.maxMsTracks = 0;
        clip_info.size = sizeof(clip_info);

        r = OmPlrClipGetInfo((OmPlrHandle)app->players.item[0].handle, clip_name, &clip_info);

        if(!r)
        {
            /* copy item props */
            strncpy(items[c].id, clip_name, PATH_MAX);
            items[c].in = clip_info.firstFrame;
            items[c].dur = clip_info.lastFrame - clip_info.firstFrame;

            /* callback */
            pthread_mutex_unlock(&app->players.lock);
            if(proc)
                proc(app, &items[c], data);
            pthread_mutex_lock(&app->players.lock);

            c++;
        };

        r = OmPlrClipGetNext((OmPlrHandle)app->players.item[0].handle, clip_name, sizeof(clip_name));
    };

    pthread_mutex_unlock(&app->players.lock);

    return c;
};


static gboolean on_main_window_delete_event( GtkWidget *widget, GdkEvent *event, gpointer user_data )
{
    g_print ("delete event occurred [start]\n");
    gdk_threads_leave();
    omnplay_release((omnplay_instance_t*)user_data);
    gdk_threads_enter();
    g_print ("delete event occurred [finish]\n");

    return FALSE;
}

static void on_main_window_destroy( GtkWidget *widget, gpointer user_data )
{
    g_print ("destroy occurred\n");
    gtk_main_quit();
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

static int find_index_of_playlist_item(omnplay_instance_t* app, int start, int idx)
{
    while(1)
    {
        if(app->playlist.item[start].omn_idx == idx)
            return start;

        if(app->playlist.item[start].type & OMNPLAY_PLAYLIST_BLOCK_END)
            break;

        start++;
    };

    return -1;
};

static void omnplay_update_status(omnplay_player_t* player, OmPlrStatus *prev , OmPlrStatus *curr)
{
    int idx;
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

    /* update status in status page */
    gdk_threads_enter();
    gtk_label_set_text(GTK_LABEL (player->label_tc_cur), tc_cur);
    gtk_label_set_text(GTK_LABEL (player->label_tc_rem), tc_rem);
    gtk_label_set_text(GTK_LABEL (player->label_state), state);
    gtk_label_set_text(GTK_LABEL (player->label_status), status);
    gtk_label_set_text(GTK_LABEL (player->label_clip), clip);
    gdk_flush();
    gdk_threads_leave();

    /* update remaining time */
    gdk_threads_enter();
    pthread_mutex_lock(&player->app->playlist.lock);
    pthread_mutex_lock(&player->app->players.lock);
    if(curr->state == omPlrStatePlay || curr->state == omPlrStateCuePlay)
    {
        idx = find_index_of_playlist_item(player->app, player->playlist_start, curr->currClipNum);
        if(idx >= 0)
        {
            frames2tc(curr->currClipStartPos + curr->currClipLen - curr->pos, 25.0, tc_rem);
            omnplay_playlist_draw_item_rem(player->app, idx, tc_rem);
        }
        if(curr->currClipNum != prev->currClipNum && 1 != prev->numClips)
        {
            tc_rem[0] = 0;
            idx = find_index_of_playlist_item(player->app, player->playlist_start, prev->currClipNum);
            if(idx >= 0)
                omnplay_playlist_draw_item_rem(player->app, idx, tc_rem);
        };
    }
    else
    {
        tc_rem[0] = 0;
        idx = find_index_of_playlist_item(player->app, player->playlist_start, curr->currClipNum);
        if(idx >= 0)
            omnplay_playlist_draw_item_rem(player->app, idx, tc_rem);
        idx = find_index_of_playlist_item(player->app, player->playlist_start, prev->currClipNum);
        if(idx >= 0)
            omnplay_playlist_draw_item_rem(player->app, idx, tc_rem);
    };
    pthread_mutex_unlock(&player->app->players.lock);
    pthread_mutex_unlock(&player->app->playlist.lock);
    gdk_flush();
    gdk_threads_leave();


    memcpy(prev, curr, sizeof(OmPlrStatus));
};

static void* omnplay_thread_proc(void* data)
{
    int r;
    OmPlrStatus st_curr, st_prev;
    omnplay_player_t* player = (omnplay_player_t*)data;

    g_warning("omnplay_thread_proc\n");

    /* connect */
    pthread_mutex_lock(&player->app->players.lock);
    r = OmPlrOpen(player->host, player->name, (OmPlrHandle*)&player->handle);
    pthread_mutex_unlock(&player->app->players.lock);
    if(r)
    {
        g_warning("ERROR: OmPlrOpen(%s, %s) failed with 0x%.8X\n",
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
            g_warning("ERROR: OmPlrClipSetDirectory(%s) failed with 0x%.8X\n",
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
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100000);
#endif

        /* get status */
        pthread_mutex_lock(&player->app->players.lock);
        st_curr.size = sizeof(OmPlrStatus);
        r = OmPlrGetPlayerStatus((OmPlrHandle)player->handle, &st_curr);
        pthread_mutex_unlock(&player->app->players.lock);

        if(r)
            g_warning("ERROR: OmPlrGetPlayerStatus failed with 0x%.8X\n", r);
        else
            if(memcmp(&st_curr, &st_prev, sizeof(OmPlrStatus)))
                omnplay_update_status(player, &st_prev , &st_curr);
    };

    pthread_mutex_lock(&player->app->players.lock);
    OmPlrClose((OmPlrHandle)player->handle);
    pthread_mutex_unlock(&player->app->players.lock);

    return NULL;
};

void get_selected_items_playlist_proc(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    int idx, *list = (int*)data;
    gtk_tree_model_get(model, iter, 7, &idx, -1);
    list[list[0] + 1] = idx;
    list[0] = list[0] + 1;
};

static int* get_selected_items_playlist(omnplay_instance_t* app)
{
    int* list = NULL;
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->playlist_grid));
    if(selection)
    {
        list = (int*)malloc(sizeof(int) * (MAX_PLAYLIST_ITEMS + 1));
        memset(list, 0, sizeof(int) * (MAX_PLAYLIST_ITEMS + 1));

        gtk_tree_selection_selected_foreach(
            selection,
            get_selected_items_playlist_proc,
            list);

        if(!list[0])
        {
            free(list);
            list = NULL;
        };
    };

    return list;
};

static int idx_in_players_range(omnplay_instance_t* app, int idx)
{
    int i, r = 0;

    for(i = 0; i < app->players.count && !r; i++)
    {
        int a, b;

        a = app->players.item[i].playlist_start;
        b = app->players.item[i].playlist_length;

        if(b <= 0)
            continue;

        b = a + b - 1;

        if(idx >= a && idx <= b) r = 1;
    };

    return r;
};

static int idxs_in_players_range(omnplay_instance_t* app, int start, int stop)
{
    int i, r = 0;

    for(i = 0; i < app->players.count && !r; i++)
    {
        int a, b;

        a = app->players.item[i].playlist_start;
        b = app->players.item[i].playlist_length;

        if(b <= 0)
            continue;

        b = a + b - 1;

#define IN_RANGE(A,B,C) (A <= C && C <= B)
        if( IN_RANGE(a,b,start) ||
            IN_RANGE(a,b,stop) ||
            IN_RANGE(start,stop,a) ||
            IN_RANGE(start,stop,b))
            r = 1;
    };

    return r;
};

static void omnplay_playlist_block(omnplay_instance_t* app, control_buttons_t button)
{
    int start, stop, r, i;
    int* list = get_selected_items_playlist(app);

    if(!list)
        return;

    pthread_mutex_lock(&app->playlist.lock);
    pthread_mutex_lock(&app->players.lock);

    start = list[1];
    stop = list[list[0]];

    if(!idxs_in_players_range(app, start, stop))
    {
        int loop = (button == BUTTON_PLAYLIST_BLOCK_LOOP)?OMNPLAY_PLAYLIST_BLOCK_LOOP:0;

        /* update selected item */
        for(i = start; i <= stop; i++)
        {
            int t = OMNPLAY_PLAYLIST_BLOCK_BODY | loop;

            if(i == start)      t |= OMNPLAY_PLAYLIST_BLOCK_BEGIN;
            if(i == stop)       t |= OMNPLAY_PLAYLIST_BLOCK_END;

            app->playlist.item[i].type = (playlist_item_type_t)t;

            omnplay_playlist_draw_item(app, i);
        };

        /* update border items */
        if(start && !(app->playlist.item[start - 1].type & OMNPLAY_PLAYLIST_BLOCK_END))
        {
            app->playlist.item[start - 1].type = (playlist_item_type_t)(OMNPLAY_PLAYLIST_BLOCK_END
                | app->playlist.item[start - 1].type);
            omnplay_playlist_draw_item(app, start - 1);
        };
        if((stop + 1) < app->playlist.count && !(app->playlist.item[stop + 1].type & OMNPLAY_PLAYLIST_BLOCK_BEGIN))
        {
            app->playlist.item[stop + 1].type = (playlist_item_type_t)(OMNPLAY_PLAYLIST_BLOCK_BEGIN
                | app->playlist.item[stop + 1].type);
            omnplay_playlist_draw_item(app, stop + 1);
        };
    }
    else
        g_warning("omnplay_playlist_block: range [%d %d] do OVERLAP player\n",
            start, stop);

    pthread_mutex_unlock(&app->players.lock);
    pthread_mutex_unlock(&app->playlist.lock);

    free(list);
};

static int get_first_selected_item_playlist(omnplay_instance_t* app)
{
    int idx;
    int* list = get_selected_items_playlist(app);
    if(!list) return -1;
    idx = list[1];
    free(list);
    return idx;
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

    g_warning("get_playlist_block: range %d -> %d\n", start, stop);

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

static void omnplay_playlist_delete_items(omnplay_instance_t* app, int* idxs, int count)
{
    int i, j, idx;
    GtkTreePath* path;

    pthread_mutex_lock(&app->playlist.lock);
    pthread_mutex_lock(&app->players.lock);

    for(j = 0; j < count; j++)
    {
        idx = idxs[j] - j;

        /* fix block types */
        if( app->playlist.item[idx].type != OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY &&
            app->playlist.item[idx].type != OMNPLAY_PLAYLIST_ITEM_LOOP_BODY)
        {
            if(idx)
                app->playlist.item[idx - 1].type = (playlist_item_type_t)(app->playlist.item[idx - 1].type |
                    OMNPLAY_PLAYLIST_BLOCK_END);
            if(idx + 1 < app->playlist.count)
                app->playlist.item[idx + 1].type = (playlist_item_type_t)(app->playlist.item[idx + 1].type |
                    OMNPLAY_PLAYLIST_BLOCK_BEGIN);
        };

        /* shift playlist items */
        memmove
        (
            &app->playlist.item[idx],
            &app->playlist.item[idx + 1],
            (app->playlist.count - idx - 1) * sizeof(playlist_item_t)
        );

        /* decrement items count */
        app->playlist.count--;

        /* increment servers indexes */
        for(i = 0; i < app->players.count; i++)
            if(app->players.item[i].playlist_start >= idx)
                app->players.item[i].playlist_start--;


    };

    /* redraw playlist */
    omnplay_playlist_draw(app);

    /* select */
    path = gtk_tree_path_new_from_indices(idxs[0], -1);
    gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(app->playlist_grid)), path);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(app->playlist_grid), path, NULL, FALSE);
    gtk_tree_path_free(path);


    pthread_mutex_unlock(&app->players.lock);
    pthread_mutex_unlock(&app->playlist.lock);
};

static void omnplay_playlist_item_del(omnplay_instance_t* app)
{
    int i, idx, c;
    int *list, *list2;

    list = get_selected_items_playlist(app);
    if(!list) return;

    list2 = (int*)malloc(sizeof(int) * list[0]);

    for(i = 0, c = 0; i < list[0]; i++)
    {
        /* check for playing block */
        if(idx_in_players_range(app, list[i + 1]))
            continue;

        /* save index */
        list2[c++] = list[i + 1];
    };

    if(c)
        omnplay_playlist_delete_items(app, list2, c);

    free(list2);
    free(list);
};

static int omnplay_playlist_insert_check(omnplay_instance_t* app, int idx, playlist_item_type_t* t)
{
    *t = OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE;

    /* before or after playlist */
    if(!idx || idx == app->playlist.count)
        return 1;

    /* check for block borders */
    if( app->playlist.item[idx - 1].type & OMNPLAY_PLAYLIST_BLOCK_END &&
        app->playlist.item[idx + 0].type & OMNPLAY_PLAYLIST_BLOCK_BEGIN)
        return 1;

    /* check for playing block */
    if(idx_in_players_range(app, idx))
        return 0;

    if(app->playlist.item[idx].type & OMNPLAY_PLAYLIST_BLOCK_LOOP)
        *t = OMNPLAY_PLAYLIST_ITEM_LOOP_BODY;
    else
        *t = OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY;

    return 1;
};

static void omnplay_playlist_insert_items(omnplay_instance_t* app, int idx,
    playlist_item_t* items, int count)
{
    int i;
    GtkTreePath* path;

    pthread_mutex_lock(&app->playlist.lock);
    pthread_mutex_lock(&app->players.lock);

    /* shift playlist items */
    memmove
    (
        &app->playlist.item[idx + count],
        &app->playlist.item[idx],
        (app->playlist.count - idx) * sizeof(playlist_item_t)
    );

    /* copy new items */
    memcpy
    (
        &app->playlist.item[idx],
        items,
        count * sizeof(playlist_item_t)
    );

    /* increment servers indexes */
    for(i = 0; i < app->players.count; i++)
        if(app->players.item[i].playlist_start >= idx)
            app->players.item[i].playlist_start += idx;

    /* increment items count */
    app->playlist.count += count;

    /* redraw playlist */
    omnplay_playlist_draw(app);

    /* select */
    path = gtk_tree_path_new_from_indices(idx + count, -1);
    gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(app->playlist_grid)), path);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(app->playlist_grid), path, NULL, FALSE);
    gtk_tree_path_free(path);

    pthread_mutex_unlock(&app->players.lock);
    pthread_mutex_unlock(&app->playlist.lock);
};

static void omnplay_playlist_item_add(omnplay_instance_t* app, int after)
{
    int idx;
    playlist_item_t item;
    playlist_item_type_t t;

    /* find insert position */
    idx = get_first_selected_item_playlist(app);
    if(idx < 0)
        idx = 0;
    else
        idx += (after)?1:0;

    if(!omnplay_playlist_insert_check(app, idx, &t))
        return;

    g_warning("allowed insert into idx=%d\n", idx);

    /* clear item */
    memset(&item, 0, sizeof(playlist_item_t));
    if(ui_playlist_item_dialog(app, &item))
    {
        omnplay_library_normalize_item(app, &item);
        item.type = t;
        omnplay_playlist_insert_items(app, idx, &item, 1);
    };
};

static void omnplay_playlist_item_edit(omnplay_instance_t* app)
{
    int idx;
    playlist_item_t item;

    /* find insert position */
    idx = get_first_selected_item_playlist(app);

    if(idx < 0)
        return;

    /* check for playing block */
    if(idx_in_players_range(app, idx))
        return;

    item = app->playlist.item[idx];

    if(ui_playlist_item_dialog(app, &item))
    {
        omnplay_library_normalize_item(app, &item);
        app->playlist.item[idx] = item;
        omnplay_playlist_draw_item(app, idx);
    };
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

    g_warning("cue: selected item is %d\n", idx);

    if(get_playlist_block(app, idx, &start, &stop) < 0)
    {
        pthread_mutex_unlock(&app->playlist.lock);
        return;
    };

    g_warning("cue: range %d -> %d\n", start, stop);

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
        player->playlist_length = -1;
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

                g_warning("OmPlrClipGetInfo(%s): firstFrame=%d, lastFrame=%d\n",
                    app->playlist.item[i].id, clip.firstFrame, clip.lastFrame);

                /* should we fix playlist clip timings */
                if(!(
                    app->playlist.item[i].in >= clip.firstFrame &&
                    app->playlist.item[i].in + app->playlist.item[i].dur <= clip.lastFrame) ||
                    !app->playlist.item[i].dur)
                {
                    g_warning("cue: item [%s] will be updated [%d;%d]->[%d;%d]\n",
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
                g_warning("cue: failed with %d, %s\n", r, OmPlrGetErrorString((OmPlrError)r));
                app->playlist.item[i].omn_idx = -1;
                app->playlist.item[i].omn_offset = -1;
                app->playlist.item[i].error |= PLAYLIST_ITEM_ERROR_CUE;
            }
            else
            {
                app->playlist.item[i].omn_idx = c;
                app->playlist.item[i].omn_offset = o;
                app->playlist.item[i].error &= 0xF ^ PLAYLIST_ITEM_ERROR_CUE;

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

            /* setup loop */
            if(app->playlist.item[start].type & OMNPLAY_PLAYLIST_BLOCK_LOOP)
                OmPlrLoop((OmPlrHandle)player->handle, hs.minPos, hs.maxPos);
            else
                OmPlrLoop((OmPlrHandle)player->handle, hs.minPos, hs.minPos);

            player->playlist_start = start;
            player->playlist_length = stop - start + 1;

            /* Cue */
            OmPlrCuePlay((OmPlrHandle)player->handle, 0.0);
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

static void omnplay_playlist_item_swap(omnplay_instance_t* app, int dir)
{
    int sel, a, b, e = 1;
    GtkTreePath* path;
    playlist_item_t item;

    /* find insert position */
    sel = get_first_selected_item_playlist(app);
    if(sel < 0)
        return;

    if(dir < 0)
    {
        a = sel - 1;
        b = sel;
        sel = a;
    }
    else
    {
        a = sel;
        b = sel + 1;
        sel = b;
    };

    /* check for playing block */
    if(idx_in_players_range(app, a) || idx_in_players_range(app, b))
        return;

    pthread_mutex_lock(&app->playlist.lock);
    pthread_mutex_lock(&app->players.lock);

    /* swap */
    item = app->playlist.item[a];
    app->playlist.item[a] = app->playlist.item[b];
    app->playlist.item[b] = item;

    /* rewite type */
    if(app->playlist.item[a].type != app->playlist.item[b].type)
    {
        e = 0;
        app->playlist.item[a].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE;
        app->playlist.item[b].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE;
    };

    /* redraw main items */
    omnplay_playlist_draw_item(app, a);
    omnplay_playlist_draw_item(app, b);

    /* fix block types */
    if(a && !e)
    {
        app->playlist.item[a - 1].type = (playlist_item_type_t)(app->playlist.item[a - 1].type |
            OMNPLAY_PLAYLIST_BLOCK_END);
        omnplay_playlist_draw_item(app, a - 1);
    };
    if(b + 1 < app->playlist.count && !e)
    {
        app->playlist.item[b + 1].type = (playlist_item_type_t)(app->playlist.item[b + 1].type |
            OMNPLAY_PLAYLIST_BLOCK_BEGIN);
        omnplay_playlist_draw_item(app, b + 1);
    };

    /* select */
    path = gtk_tree_path_new_from_indices(sel, -1);
    gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(app->playlist_grid)), path);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(app->playlist_grid), path, NULL, FALSE);
    gtk_tree_path_free(path);

    pthread_mutex_unlock(&app->players.lock);
    pthread_mutex_unlock(&app->playlist.lock);
};

static void omnplay_library_add(omnplay_instance_t* app, int after)
{
    int idx, c, i;
    playlist_item_t* items;
    playlist_item_type_t t;

    /* find insert position */
    idx = get_first_selected_item_playlist(app);
    if(idx < 0)
        idx = 0;
    else
        idx += (after)?1:0;

    if(!omnplay_playlist_insert_check(app, idx, &t))
        return;

    items = omnplay_library_get_selected(app, &c);

    /* clear item */
    if(items)
    {
        for(i = 0; i < c; i++)
        {
            items[i].type = t;
            items[i].error = 0;
        };
        omnplay_playlist_insert_items(app, idx, items, c);
    };
};


static gboolean omnplay_button_click(omnplay_instance_t* app, control_buttons_t button)
{
    switch(button)
    {
        case BUTTON_PLAYLIST_ITEM_ADD:
            omnplay_playlist_item_add(app, 0);
            break;
        case BUTTON_PLAYLIST_ITEM_DEL:
            omnplay_playlist_item_del(app);
            break;
        case BUTTON_PLAYLIST_ITEM_EDIT:
            omnplay_playlist_item_edit(app);
            break;
        case BUTTON_PLAYLIST_LOAD:
            omnplay_playlist_load(app);
            break;
        case BUTTON_PLAYLIST_SAVE:
            omnplay_playlist_save(app);
            break;
        case BUTTON_PLAYLIST_BLOCK_SINGLE:
        case BUTTON_PLAYLIST_BLOCK_LOOP:
            omnplay_playlist_block(app, button);
            break;
        case BUTTON_PLAYLIST_ITEM_UP:
            omnplay_playlist_item_swap(app, -1);
            break;
        case BUTTON_PLAYLIST_ITEM_DOWN:
            omnplay_playlist_item_swap(app, +1);
            break;
        case BUTTON_PLAYER_CUE:
        case BUTTON_PLAYER_PLAY:
        case BUTTON_PLAYER_PAUSE:
        case BUTTON_PLAYER_STOP:
            omnplay_ctl(app, button);
            break;
        case BUTTON_LIBRARY_ADD:
            omnplay_library_add(app, 0);
            break;
        case BUTTON_LIBRARY_REFRESH:
            omnplay_library_refresh(app);
            break;
        case BUTTON_LIBRARY_FIND:
            omnplay_library_search(app, 0);
            break;
        case BUTTON_LIBRARY_FIND_NEXT:
            omnplay_library_search(app, 1);
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

static void omnplay_playlist_item_copy(omnplay_instance_t* app)
{
    int *list, i;

    list = get_selected_items_playlist(app);
    if(!list) return;

    for(i = 0; i < list[0]; i++)
        app->clipboard.item[i] = app->playlist.item[list[i + 1]];
    app->clipboard.count = list[0];

    free(list);
};

static void omnplay_playlist_item_paste(omnplay_instance_t* app, int after)
{
    int idx, i;
    playlist_item_t* items;
    playlist_item_type_t t;

    /* find insert position */
    idx = get_first_selected_item_playlist(app);
    if(idx < 0)
        idx = 0;
    else
        idx += (after)?1:0;

    if(!omnplay_playlist_insert_check(app, idx, &t))
        return;

    /* clear item */
    if(app->clipboard.count)
    {
        for(i = 0; i < app->clipboard.count; i++)
        {
            app->clipboard.item[i].type = t;
            app->clipboard.item[i].error = 0;
        };
        omnplay_playlist_insert_items(app, idx, app->clipboard.item, app->clipboard.count);
    };
};

static gboolean on_playlist_grid_key(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    omnplay_instance_t* app = (omnplay_instance_t*)data;

    switch(event->keyval)
    {
        case GDK_C:
        case GDK_c:
            if(event->state & GDK_CONTROL_MASK)
            {
                omnplay_playlist_item_copy(app);
                return TRUE;
            };
            break;
        case GDK_V:
        case GDK_v:
            if(event->state & GDK_CONTROL_MASK)
            {
                omnplay_playlist_item_paste(app, 0);
                return TRUE;
            };
            break;
        case GDK_X:
        case GDK_x:
            if(event->state & GDK_CONTROL_MASK)
            {
                omnplay_playlist_item_copy(app);
                omnplay_playlist_item_del(app);
                return TRUE;
            };
            break;
        case GDK_S:
        case GDK_s:
            if(event->state & GDK_CONTROL_MASK)
            {
                omnplay_playlist_save(app);
                return TRUE;
            };
            break;
        case GDK_O:
        case GDK_o:
            if(event->state & GDK_CONTROL_MASK)
            {
                omnplay_playlist_load(app);
                return TRUE;
            };
            break;
        case GDK_KEY_space:
            omnplay_ctl(app, BUTTON_PLAYER_PLAY);
            return TRUE;
        case GDK_KEY_Return:
            omnplay_ctl(app, BUTTON_PLAYER_CUE);
            return TRUE;
        case GDK_KEY_Insert:
            omnplay_playlist_item_add(app, 0);
            return TRUE;
        case GDK_KEY_Delete:
            omnplay_playlist_item_del(app);
            return TRUE;
        case GDK_KEY_BackSpace:
            omnplay_playlist_item_edit(app);
            return TRUE;
    };

    return FALSE;
};

static gboolean on_library_grid_key(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    omnplay_instance_t* app = (omnplay_instance_t*)data;

    switch(event->keyval)
    {
        case GDK_C:
        case GDK_c:
            if(event->state & GDK_CONTROL_MASK)
            {
                int count;
                playlist_item_t* items;

                items = omnplay_library_get_selected(app, &count);

                if(items)
                {
                    int i;

                    for(i = 0; i < count; i++)
                        app->clipboard.item[i] = items[i];

                    app->clipboard.count = count;
                };

                return TRUE;
            };
            break;
        case GDK_V:
        case GDK_v:
            if(event->state & GDK_CONTROL_MASK)
            {
                g_warning("CTRL+v\n");
                return TRUE;
            };
            break;
        case GDK_X:
        case GDK_x:
            if(event->state & GDK_CONTROL_MASK)
            {
                g_warning("CTRL+x\n");
                return TRUE;
            };
            break;
    };

    return FALSE;
};

static gboolean on_library_grid_button(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
//    g_warning("on_library_grid_button: event->button=%d, event->type=%d", event->button, event->type);

    if(event->button==1 && event->type==GDK_2BUTTON_PRESS)
    {
        omnplay_library_add((omnplay_instance_t* )data, 0);
        return TRUE;
    };

    return FALSE;
};

static gboolean on_playlist_grid_button(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
//    g_warning("on_playlist_grid_button");

    if(event->button==1 && event->type==GDK_2BUTTON_PRESS)
    {
        omnplay_ctl((omnplay_instance_t* )data, BUTTON_PLAYER_CUE);
        return TRUE;
    };

    return FALSE;
};

void omnplay_init(omnplay_instance_t* app)
{
    int i;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    gtk_signal_connect( GTK_OBJECT( app->window ), "delete-event",
        GTK_SIGNAL_FUNC(on_main_window_delete_event), app);

    gtk_signal_connect( GTK_OBJECT( app->window ), "destroy",
        GTK_SIGNAL_FUNC(on_main_window_destroy), app);

    gtk_widget_add_events(app->playlist_grid, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(app->playlist_grid, GDK_KEY_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(app->playlist_grid), "key-press-event",
        GTK_SIGNAL_FUNC(on_playlist_grid_key), app);

    gtk_widget_add_events(app->library_grid, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(app->library_grid, GDK_KEY_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(app->library_grid), "key-press-event",
        GTK_SIGNAL_FUNC(on_library_grid_key), app);

    gtk_signal_connect(GTK_OBJECT(app->playlist_grid), "button-press-event",
        GTK_SIGNAL_FUNC(on_playlist_grid_button), app);

    gtk_signal_connect(GTK_OBJECT(app->library_grid), "button-press-event",
        GTK_SIGNAL_FUNC(on_library_grid_button), app);

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

    /* create lock */
    pthread_mutex_init(&app->library.lock, &attr);

    /* load library */
    omnplay_library_load(app);

    pthread_mutexattr_destroy(&attr);
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

    /* load library */
    omnplay_library_save(app);

    /* destroy library lock */
    pthread_mutex_destroy(&app->library.lock);
};

void omnplay_playlist_normalize(omnplay_instance_t* app)
{
    int i;

    /* normalize playlist */
    for(i = 0; i < app->playlist.count; i++)
        if(omnplay_library_normalize_item(app, &app->playlist.item[i]))
            omnplay_playlist_draw_item(app, i);
};
