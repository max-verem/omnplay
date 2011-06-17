/*
 * omnplay.h -- GTK+ 2 omnplay
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

#ifndef OMNPLAY_H
#define OMNPLAY_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef enum control_buttons
{
    BUTTON_PLAYLIST_ITEM_ADD = 1,
    BUTTON_PLAYLIST_ITEM_DEL,
    BUTTON_PLAYLIST_ITEM_EDIT,

    BUTTON_PLAYLIST_LOAD,
    BUTTON_PLAYLIST_SAVE,

    BUTTON_PLAYLIST_BLOCK_SINGLE,
    BUTTON_PLAYLIST_BLOCK_LOOP,

    BUTTON_PLAYLIST_ITEM_UP,
    BUTTON_PLAYLIST_ITEM_DOWN,

    BUTTON_PLAYER_CUE,
    BUTTON_PLAYER_PLAY,
    BUTTON_PLAYER_PAUSE,
    BUTTON_PLAYER_STOP,

    BUTTON_LIBRARY_ADD,
    BUTTON_LIBRARY_REFRESH,

    BUTTON_LAST
} control_buttons_t;

#define MAX_PLAYERS 4

typedef struct omnplay_player
{
    char name[PATH_MAX];
    char host[PATH_MAX];
    void* handle;
    pthread_t thread;
    pthread_mutex_t lock;
    GtkWidget *label_status, *label_state, *label_tc_cur, *label_tc_rem, *label_clip;
}
omnplay_player_t;

typedef struct omnplay_instance
{
    GtkWidget *window;
    GtkWidget *buttons[BUTTON_LAST + 1];
    struct
    {
        omnplay_player_t item[MAX_PLAYERS];
        int count;
        char path[PATH_MAX];
    } players;
    int f_exit;
}
omnplay_instance_t;

omnplay_instance_t* omnplay_create(int argc, char** argv);
void omnplay_init(omnplay_instance_t* app);
void omnplay_release(omnplay_instance_t* app);
void omnplay_destroy(omnplay_instance_t* app);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* OMNPLAY_H */
