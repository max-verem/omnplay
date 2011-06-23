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

    BUTTON_LIBRARY_FIND,
    BUTTON_LIBRARY_FIND_NEXT,

    BUTTON_LAST
} control_buttons_t;

#define OMNPLAY_PLAYLIST_BLOCK_BEGIN    1
#define OMNPLAY_PLAYLIST_BLOCK_BODY     0
#define OMNPLAY_PLAYLIST_BLOCK_END      2
#define OMNPLAY_PLAYLIST_BLOCK_LOOP     4

typedef enum playlist_item_type
{
    // 1
    OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN   =       OMNPLAY_PLAYLIST_BLOCK_BEGIN,
    // 0
    OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY    =       OMNPLAY_PLAYLIST_BLOCK_BODY,
    // 2
    OMNPLAY_PLAYLIST_ITEM_BLOCK_END     =       OMNPLAY_PLAYLIST_BLOCK_END,
    // 3
    OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE  =       OMNPLAY_PLAYLIST_BLOCK_BEGIN    |
                                                OMNPLAY_PLAYLIST_BLOCK_BODY     |
                                                OMNPLAY_PLAYLIST_BLOCK_END,
    // 5
    OMNPLAY_PLAYLIST_ITEM_LOOP_BEGIN    =       OMNPLAY_PLAYLIST_BLOCK_BEGIN    |
                                                OMNPLAY_PLAYLIST_BLOCK_LOOP,
    // 4
    OMNPLAY_PLAYLIST_ITEM_LOOP_BODY     =       OMNPLAY_PLAYLIST_BLOCK_BODY     |
                                                OMNPLAY_PLAYLIST_BLOCK_LOOP,
    // 6
    OMNPLAY_PLAYLIST_ITEM_LOOP_END      =       OMNPLAY_PLAYLIST_BLOCK_END      |
                                                OMNPLAY_PLAYLIST_BLOCK_LOOP,
    // 7
    OMNPLAY_PLAYLIST_ITEM_LOOP_SINGLE   =       OMNPLAY_PLAYLIST_BLOCK_BEGIN    |
                                                OMNPLAY_PLAYLIST_BLOCK_BODY     |
                                                OMNPLAY_PLAYLIST_BLOCK_END      |
                                                OMNPLAY_PLAYLIST_BLOCK_LOOP,
} playlist_item_type_t;

#define MAX_PLAYLIST_ITEMS      1024
#define MAX_LIBRARY_ITEMS       10240
#define PLAYLIST_ITEM_ERROR_LIB 1
#define PLAYLIST_ITEM_ERROR_CUE 2

typedef struct playlist_item
{
    char id[PATH_MAX];
    char title[PATH_MAX];
    int in;
    int dur;
    int player;
    playlist_item_type_t type;
    int omn_idx;
    int omn_offset;
    int error;
} playlist_item_t;

#define MAX_PLAYERS 4

struct omnplay_instance;

typedef struct omnplay_player
{
    int idx;
    char name[PATH_MAX];
    char host[PATH_MAX];
    void* handle;
    pthread_t thread;
    GtkWidget *label_status, *label_state, *label_tc_cur, *label_tc_rem, *label_clip;
    struct omnplay_instance *app;
    int playlist_start;
    int playlist_length;
} omnplay_player_t;

typedef struct omnplay_instance
{
    GtkWidget *window;
    GtkWidget *playlist_grid;
    GtkWidget *library_grid;
    GtkWidget *buttons[BUTTON_LAST + 1];
    struct
    {
        omnplay_player_t item[MAX_PLAYERS];
        int count;
        char path[PATH_MAX];
        pthread_mutex_t lock;
    } players;
    int f_exit;
    struct
    {
        playlist_item_t item[MAX_PLAYLIST_ITEMS];
        int count;
        int ver_curr;
        int ver_prev;
        pthread_mutex_t lock;
        char* path;
        GdkPixbuf *block_icons[8];
    } playlist;
    struct
    {
        playlist_item_t item[MAX_LIBRARY_ITEMS];
        int count;
        char filename[PATH_MAX];
        char whois[PATH_MAX];
        pthread_mutex_t lock;
        pthread_t refresh_thread;
        GtkWidget *refresh_ui[2];
        GtkWidget *search;
    } library;
    struct
    {
        playlist_item_t item[MAX_LIBRARY_ITEMS];
        int count;
    } clipboard;
} omnplay_instance_t;

omnplay_instance_t* omnplay_create(int argc, char** argv);
void omnplay_init(omnplay_instance_t* app);
void omnplay_release(omnplay_instance_t* app);
void omnplay_destroy(omnplay_instance_t* app);
void omnplay_playlist_load(omnplay_instance_t* app);
void omnplay_playlist_save(omnplay_instance_t* app);
void omnplay_playlist_draw(omnplay_instance_t* app);
void omnplay_playlist_draw_item(omnplay_instance_t* app, int idx);
void omnplay_playlist_draw_item_rem(omnplay_instance_t* app, int idx, char* rem);
void omnplay_library_load(omnplay_instance_t* app);
void omnplay_library_save(omnplay_instance_t* app);
void omnplay_library_refresh(omnplay_instance_t* app);
void omnplay_library_draw(omnplay_instance_t* app);
typedef void (*omnplay_get_content_cb_proc)(omnplay_instance_t* app, playlist_item_t *items, void* data);
int omnplay_get_content(omnplay_instance_t* app, playlist_item_t *items, int limit,
    omnplay_get_content_cb_proc proc, void* data);
int omnplay_whois_list(omnplay_instance_t* app, playlist_item_t *items, int* plimit);
int omnplay_library_load_file(playlist_item_t* items, int *pcount, char* filename);
playlist_item_t* omnplay_library_find(omnplay_instance_t* app, char* id);
int omnplay_library_normalize_item(omnplay_instance_t* app, playlist_item_t* item);
playlist_item_t* omnplay_library_get_selected(omnplay_instance_t* app, int *count);
void omnplay_playlist_normalize(omnplay_instance_t* app);
void omnplay_library_search(omnplay_instance_t* app, int next);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* OMNPLAY_H */
