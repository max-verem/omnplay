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

#if !HAVE_STRSEP
/*
    https://gitorious.org/mingw-unofficial/msys/source/956185bef3c2dafb0e6f5f9eadd925ec55139f2e:rt/src/winsup/cygwin/strsep.cc
*/
static char * strsep (char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
    if((s = *stringp) == NULL)
        return (NULL);

    for(tok = s;;)
    {
        c = *s++;
        spanp = delim;
        do
        {
            if((sc = *spanp++) == c)
            {
                if(c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;

                return (tok);
            }
        } while (sc != 0);
    }

    /* NOTREACHED */
};

#endif

static int load_file_ply(omnplay_instance_t* app, char* filename)
{
    FILE* f;
    char *l;
    int count = 0, i;
    playlist_item_t* items;

    g_warning("%s:%d filename = [%s]", __FUNCTION__, __LINE__, filename);

    /* open and process file */
    f = fopen(filename, "rt");
    if(!f)
    {
        g_warning("%s:%d failed to open filename = [%s]", __FUNCTION__, __LINE__, filename);
        return 0;
    };

    /* allocate space for strings and items */
    items = malloc(sizeof(playlist_item_t) * MAX_PLAYLIST_ITEMS);
    memset(items, 0, sizeof(playlist_item_t) * MAX_PLAYLIST_ITEMS);
    l = malloc(PATH_MAX);

    while(!feof(f))
    {
        char* s;
        char *save, *str, *tok;
        playlist_item_t item;

        /* load string */
        l[0] = 0;
        fgets(l, PATH_MAX, f);

        /* remove newlines */
        while((s = strchr(l, '\n'))) *s = 0;
        while((s = strchr(l, '\r'))) *s = 0;
        while((s = strchr(l, '\t'))) *s = 0;

        /* check for empty line */
        if(!l[0] || l[0] == '#')
            continue;

        /* process split tokens */
        memset(&item, 0, sizeof(item));
        i = 0;
        str = save = strdup(l);
        while((tok = strsep(&str, ",")) != NULL)
        {
            g_warning("%s:%d tok[%d]=[%s]", __FUNCTION__, __LINE__, i, tok);

            if(0 == i)
                strncpy(item.id, tok, PATH_MAX);
            else if(1 == i)
                item.player = atol(tok) - 1;
            else if(2 == i)
            {
                int b = atol(tok);
                switch(b)
                {
                    case 1: item.type = OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE; break;
                    case 2: item.type = OMNPLAY_PLAYLIST_ITEM_LOOP_BEGIN; break;
                    case 3: item.type = OMNPLAY_PLAYLIST_ITEM_LOOP_BODY; break;
                    case 4: item.type = OMNPLAY_PLAYLIST_ITEM_LOOP_END; break;
                    case 6: item.type = OMNPLAY_PLAYLIST_ITEM_BLOCK_END; break;
                    case 0:
                        if(!count)
                            item.type = OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN;
                        else if
                        (
                            items[count - 1].type == OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN
                            ||
                            items[count - 1].type == OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY
                        )
                            item.type = OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY;
                        else
                            item.type = OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN;
                        break;
                    default:
                        if(b >= 1024)
                            item.type = b - 1024;
                }
            }
            else if(3 == i)
                tc2frames(tok, 25.0, &item.in);
            else if(5 == i)
                tc2frames(tok, 25.0, &item.dur);

            i++;
        }

        if(i > 5)
        {
            g_warning("%s:%d count=[%d] id={%s} in={%d} dur={%d}", __FUNCTION__, __LINE__, count, item.id, item.in, item.dur);
            items[count] = item;
            count++;
        }
        else
            g_warning("%s:%d i = %d", __FUNCTION__, __LINE__, i);
    }

    fclose(f);

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
    char* fname = filename;

    filename = (char*)malloc(PATH_MAX);
    strncpy(filename, fname, PATH_MAX);
    i = strlen(filename);
    if(i < 4 || strcasecmp(filename + i - 4, ".ply"))
        strcat(filename, ".ply");

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

        close(f);
    };

    free(filename);

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
