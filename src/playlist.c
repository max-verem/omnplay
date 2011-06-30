/*
 * playlist.c -- GTK+ 2 omnplay
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
#include "timecode.h"

static int load_file_ply(omnplay_instance_t* app, char* filename)
{
    FILE* f;
    char *ID, *CH, *B, *IN, *OUT, *DUR, *REST, *l;
    int count = 0, i;
    playlist_item_t* items;

    /* allocate space for strings and items */
    items = malloc(sizeof(playlist_item_t) * MAX_PLAYLIST_ITEMS);
    memset(items, 0, sizeof(playlist_item_t) * MAX_PLAYLIST_ITEMS);
    ID = malloc(PATH_MAX);
    CH = malloc(PATH_MAX);
    B = malloc(PATH_MAX);
    IN = malloc(PATH_MAX);
    OUT = malloc(PATH_MAX);
    DUR = malloc(PATH_MAX);
    REST = malloc(PATH_MAX);
    l = malloc(PATH_MAX);

    /* open and process file */
    f = fopen(filename, "rt");
    if(f)
    {
        while( !feof(f) )
        {
            char* s;

            /* load string */
            memset(l, 0, PATH_MAX);
            fgets(l, PATH_MAX, f);

            /* remove newlines */
            if( (s = strchr(l, '\n')) ) *s = 0;
            if( (s = strchr(l, '\r')) ) *s = 0;
            if( (s = strchr(l, '\t')) ) *s = 0;

            /* check for empty line */
            if(l[0] && l[0] != '#')
            {
                if (6 != sscanf(l, "%128[^,],%128[^,],%128[^,],%128[^,],%128[^,],%128[^,],%s",
                    ID, CH, B, IN, OUT, DUR, REST))
                {
                    int b = atol(B);
                    /* setup item */
                    tc2frames(IN, 25.0, &items[count].in);
                    tc2frames(DUR, 25.0, &items[count].dur);
                    strncpy(items[count].id, ID, PATH_MAX);
                    items[count].player = atol(CH) - 1;
                    switch(b)
                    {
                        case 1: items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE; break;
                        case 2: items[count].type = OMNPLAY_PLAYLIST_ITEM_LOOP_BEGIN; break;
                        case 3: items[count].type = OMNPLAY_PLAYLIST_ITEM_LOOP_BODY; break;
                        case 4: items[count].type = OMNPLAY_PLAYLIST_ITEM_LOOP_END; break;
                        case 6: items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_END; break;
                        case 0:
                            if(!count)
                                items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN;
                            else if(items[count - 1].type == OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN ||
                                    items[count - 1].type == OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY)
                                items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY;
                            else
                                items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN;
                            break;
                        default:
                            if(b >= 1024)
                                items[count].type = b - 1024;
                    };
#if 0
                    {
                        char* n;
                        switch(items[count].type)
                        {
                            case OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN:       n = "BLOCK_BEGIN"; break;
                            case OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY:        n = "BLOCK_BODY"; break;
                            case OMNPLAY_PLAYLIST_ITEM_BLOCK_END:         n = "BLOCK_END"; break;
                            case OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE:      n = "BLOCK_SINGLE"; break;
                            case OMNPLAY_PLAYLIST_ITEM_LOOP_BEGIN:        n = "LOOP_BEGIN"; break;
                            case OMNPLAY_PLAYLIST_ITEM_LOOP_BODY:         n = "LOOP_BODY"; break;
                            case OMNPLAY_PLAYLIST_ITEM_LOOP_END:          n = "LOOP_END"; break;
                            case OMNPLAY_PLAYLIST_ITEM_LOOP_SINGLE:       n = "LOOP_SINGLE"; break;
                        };
                        fprintf(stderr, "src=[%s]\ndst=[idx=%d,block=%s,block_id=%d,in=%d,out=%d]\n",
                            l, count, n, items[count].type, items[count].in, items[count].dur);
                    };
#endif

                    count++;
                }
            };
        }

        fclose(f);
    }

    /* add loaded items to playlist */
    if(count)
    {
        pthread_mutex_lock(&app->playlist.lock);
        for(i = 0; i < count && app->playlist.count + 1 < MAX_PLAYLIST_ITEMS; i++)
        {
            omnplay_library_normalize_item(app, &items[i]);
            app->playlist.item[app->playlist.count++] = items[i];
        };
        app->playlist.ver_curr++;
        pthread_mutex_unlock(&app->playlist.lock);
    }

    /* free data */
    free(items);
    free(ID);
    free(CH);
    free(IN);
    free(OUT);
    free(DUR);
    free(REST);
    free(l);

    return count;
};

void omnplay_playlist_load(omnplay_instance_t* app)
{
    int r;
    GtkWidget *dialog;
    GtkFileFilter *filter;

    dialog = gtk_file_chooser_dialog_new("Open File",
        GTK_WINDOW (app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
        (app->playlist.path)?app->playlist.path:getenv("HOME"));

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Playlist formatted (*.ply)");
    gtk_file_filter_add_pattern(filter, "*.ply");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All types (*.*)");
    gtk_file_filter_add_pattern(filter, "*.*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);

    r = gtk_dialog_run(GTK_DIALOG(dialog));

    if(r == GTK_RESPONSE_ACCEPT)
    {
        char *filename;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        r = load_file_ply(app, filename);

        if(r)
            omnplay_playlist_draw(app);

        if(app->playlist.path)
            g_free(app->playlist.path);
        if((app->playlist.path = filename))
        {
            char* e = strrchr(app->playlist.path, '/');
            if(e) *e = 0;
        }
    }

    gtk_widget_destroy (dialog);
};

static int save_file_ply(omnplay_instance_t* app, char* filename)
{
    int i;
    FILE* f;
    char tc1[12], tc2[12], tc3[12];

    if((f = fopen(filename, "wt")))
    {
        for(i = 0; i < app->playlist.count; i++)
            fprintf(f, "%s,%d,%d,%s,%s,%s,,,,,,,,\n",
                app->playlist.item[i].id,
                app->playlist.item[i].player + 1,
                app->playlist.item[i].type + 1024,
                frames2tc(app->playlist.item[i].in, 25.0, tc1),
                frames2tc(app->playlist.item[i].in + app->playlist.item[i].dur, 25.0, tc2),
                frames2tc(app->playlist.item[i].dur, 25.0, tc3));
    };

    return 0;
};

void omnplay_playlist_save(omnplay_instance_t* app)
{
    int r;
    GtkWidget *dialog;
    GtkFileFilter *filter;

    dialog = gtk_file_chooser_dialog_new("Save File",
        GTK_WINDOW (app->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
        (app->playlist.path)?app->playlist.path:getenv("HOME"));

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Playlist formatted (*.ply)");
    gtk_file_filter_add_pattern(filter, "*.ply");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);
    g_object_set_data(G_OBJECT(filter), "id", GINT_TO_POINTER(0));
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Text (*.txt)");
    gtk_file_filter_add_pattern(filter, "*.*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);
    g_object_set_data(G_OBJECT(filter), "id", GINT_TO_POINTER(1));

    r = gtk_dialog_run(GTK_DIALOG(dialog));

    if(r == GTK_RESPONSE_ACCEPT)
    {
        char *filename;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        r = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog))), "id"));

        r = save_file_ply(app, filename);

        if(app->playlist.path)
            g_free(app->playlist.path);
        if((app->playlist.path = filename))
        {
            char* e = strrchr(app->playlist.path, '/');
            if(e) *e = 0;
        }
    }

    gtk_widget_destroy (dialog);

};

void omnplay_playlist_draw(omnplay_instance_t* app)
{
    int i;
    int* sels;
    char tc1[12], tc2[12];
    GtkListStore *list_store;
    GtkTreeIter iter;

    sels = omnplay_selected_idxs_playlist(app);

    list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->playlist_grid)));
    gtk_list_store_clear(list_store);

    pthread_mutex_lock(&app->playlist.lock);

    for(i = 0;i < app->playlist.count; i++)
    {
        char ch[3];

        if(OMNPLAY_PLAYLIST_BLOCK_BEGIN & app->playlist.item[i].type)
            snprintf(ch, sizeof(ch), "%c", 'A' + app->playlist.item[i].player);
        else
            ch[0] = 0;

        gtk_list_store_append(list_store, &iter);

        gtk_list_store_set(list_store, &iter,
            0, "",
            1, app->playlist.block_icons[app->playlist.item[i].type],
            2, ch,
            3, app->playlist.item[i].id,
            4, frames2tc(app->playlist.item[i].in, 25.0, tc1),
            5, frames2tc(app->playlist.item[i].dur, 25.0, tc2),
            6, app->playlist.item[i].title,
            7, i,
            8, (app->playlist.item[i].error != 0),
            9, (app->playlist.item[i].error & PLAYLIST_ITEM_ERROR_LIB)?"red":"orange",
            -1 );
    }

    app->playlist.ver_prev = app->playlist.ver_curr;

    if(sels)
    {
        GtkTreePath *path;

        /* select */
        path = gtk_tree_path_new_from_indices(sels[1], -1);
        gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(app->playlist_grid)), path);
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(app->playlist_grid), path, NULL, FALSE);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(app->playlist_grid), path, NULL, FALSE, 0, 0);
        gtk_tree_path_free(path);

        free(sels);
    };

    pthread_mutex_unlock(&app->playlist.lock);
};

typedef struct omnplay_playlist_draw_item_desc
{
    GtkListStore *list_store;
    omnplay_instance_t* app;
    int idx;
} omnplay_playlist_draw_item_t;

static gboolean omnplay_playlist_draw_item_proc(
    GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
    int i;
    char tc1[12], tc2[12];
    char ch[3];
    omnplay_playlist_draw_item_t* item = (omnplay_playlist_draw_item_t*)user_data;
    omnplay_instance_t* app = item->app;

    gtk_tree_model_get(model, iter, 7, &i, -1);

    if(i != item->idx) return FALSE;

    if(OMNPLAY_PLAYLIST_BLOCK_BEGIN & app->playlist.item[i].type)
        snprintf(ch, sizeof(ch), "%c", 'A' + app->playlist.item[i].player);
    else
        ch[0] = 0;

    gtk_list_store_set(item->list_store, iter,
        0, "",
        1, app->playlist.block_icons[app->playlist.item[i].type],
        2, ch,
        3, app->playlist.item[i].id,
        4, frames2tc(app->playlist.item[i].in, 25.0, tc1),
        5, frames2tc(app->playlist.item[i].dur, 25.0, tc2),
        6, app->playlist.item[i].title,
        7, i,
        8, (app->playlist.item[i].error != 0),
        9, (app->playlist.item[i].error & PLAYLIST_ITEM_ERROR_LIB)?"red":"orange",
        -1 );

    return TRUE;
};

void omnplay_playlist_draw_item(omnplay_instance_t* app, int idx)
{
    GtkListStore *list_store;
    omnplay_playlist_draw_item_t item;

    list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->playlist_grid)));

    pthread_mutex_lock(&app->playlist.lock);

    item.idx = idx;
    item.app = app;
    item.list_store = list_store;
    gtk_tree_model_foreach(GTK_TREE_MODEL(list_store), omnplay_playlist_draw_item_proc, &item);

    pthread_mutex_unlock(&app->playlist.lock);
};

static gboolean omnplay_playlist_draw_item_rem_proc(
    GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
    int i;
    void** args                 = (void**)user_data;
    GtkListStore *list_store    = (GtkListStore *)args[1];
    int idx                     = (int)args[2];
    char* rem                   = (char*)args[3];

    gtk_tree_model_get(model, iter, 7, &i, -1);

    if(i != idx) return FALSE;

    gtk_list_store_set(list_store, iter, 0, rem, -1);

    return TRUE;
};

void omnplay_playlist_draw_item_rem(omnplay_instance_t* app, int idx, char* rem)
{
    void* item[4];
    GtkListStore *list_store;

    list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->playlist_grid)));

    item[0] = (void*)app;
    item[1] = (void*)list_store;
    item[2] = (void*)idx;
    item[3] = (void*)rem;

    gtk_tree_model_foreach(GTK_TREE_MODEL(list_store), omnplay_playlist_draw_item_rem_proc, item);
};
