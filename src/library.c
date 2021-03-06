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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pthread.h>
#include <string.h>

#include "omnplay.h"
#include "ui.h"
#include "timecode.h"

playlist_item_t* omnplay_library_find(omnplay_instance_t* app, char* id)
{
    int i;
    playlist_item_t* item = NULL;

    pthread_mutex_lock(&app->library.lock);

    for(i = 0; i < app->library.count && !item; i++)
        if(!strcasecmp(id, app->library.item[i].id))
            item = &app->library.item[i];

    pthread_mutex_unlock(&app->library.lock);

    return item;
};

int omnplay_library_normalize_item(omnplay_instance_t* app, playlist_item_t* item)
{
    int r = 0;
    playlist_item_t* lib;
    playlist_item_t prev;

    pthread_mutex_lock(&app->library.lock);

    prev = *item;

    lib = omnplay_library_find(app, item->id);

    item->error = 0;

    if(lib)
    {
        if(!item->title[0])
        {
            strcpy(item->title, lib->title);
            g_warning("%s: [%s] title=[%s]->[%s]", __FUNCTION__, item->id, prev.title, item->title);
            r++;
        };

        if(item->in < lib->in || item->in >= (lib->in + lib->dur))
        {
            g_warning("%s: [%s] (item->in=[%d] < lib->in=[%d] || item->in=[%d] >= (lib->in=[%d] + lib->dur=[%d]))",
                __FUNCTION__, item->id,
                item->in, lib->in, item->in, lib->in, lib->dur);

            item->in = lib->in;

            g_warning("%s: [%s] in=[%d]->[%d]", __FUNCTION__, item->id, prev.in, item->in);

            r++;
        };

        if(!item->dur || (item->in + item->dur) > (lib->in + lib->dur))
        {
            g_warning("%s: [%s] (!item->dur=[%d] || (item->in=[%d] + item->dur=[%d]) > (lib->in=[%d] + lib->dur=[%d])",
                __FUNCTION__, item->id,
                item->dur, item->in, item->dur, lib->in, lib->dur);

            item->dur = lib->in + lib->dur - item->in;

            g_warning("%s: [%s] dur=[%d]->[%d]", __FUNCTION__, item->id, prev.dur, item->dur);

            r++;
        };
    }
    else
    {
        r = 1;
        item->error = PLAYLIST_ITEM_ERROR_LIB;
    };

    pthread_mutex_unlock(&app->library.lock);

    return r;
};

int omnplay_library_relink_item(omnplay_instance_t* app, playlist_item_t* item)
{
    int r = 0;
    playlist_item_t* lib;

    pthread_mutex_lock(&app->library.lock);

    lib = omnplay_library_find(app, item->id);

    item->error = 0;

    if(lib)
    {
        r = 1;
        strcpy(item->title, lib->title);
        item->dur = lib->dur;
        item->in = lib->in;
    }
    else
    {
        r = 1;
        item->error = PLAYLIST_ITEM_ERROR_LIB;
    };

    pthread_mutex_unlock(&app->library.lock);

    return r;
};

void omnplay_library_sort(omnplay_instance_t* app)
{
    int i, j, m;
    playlist_item_t item;

    for(i = 0; i < app->library.count; i++)
    {
        /* find max */
        for(j = i + 1, m = i; j < app->library.count; j++)
            if(strcasecmp(app->library.item[j].id, app->library.item[m].id) < 0)
                m = j;

        if(m != i)
        {
            item = app->library.item[i];
            app->library.item[i] = app->library.item[m];
            app->library.item[m] = item;
        };
    };
};

int omnplay_library_load_file(playlist_item_t* items, int *pcount, char* filename)
{
    int i, c = 0, r = 0;
    FILE* f;
    char *l;
    int limit = *pcount;
    playlist_item_t item;

    /* allocate space for strings and items */
    l = malloc(PATH_MAX);

    *pcount = 0;

    /* open and process file */
    if((f = fopen(filename, "rt")))
    {
        while(!feof(f) && c < limit)
        {
            char *s, *sp_r, *sp_b;

            /* load string */
            memset(l, 0, PATH_MAX);
            fgets(l, PATH_MAX, f);

            /* remove newlines */
            if( (s = strchr(l, '\n')) ) *s = 0;
            if( (s = strchr(l, '\r')) ) *s = 0;

            /* check for empty line */
            if(l[0] && l[0] != '#' && l[0] != '|')
            {
                memset(&item, 0, sizeof(playlist_item_t));

                g_warning("%s: [%s]", __FUNCTION__, l);

                for(i = 0, sp_b = l; (NULL != (sp_r = strtok(sp_b, "\t"))); i++, sp_b = NULL)
                {
                    g_warning("%s: [%d]=[%s]", __FUNCTION__, i, sp_r);

                    switch(i)
                    {
                        case 0: strncpy(item.id, sp_r, PATH_MAX); break;
                        case 1: tc2frames(sp_r, 25.0, &item.in); break;
                        case 2: tc2frames(sp_r, 25.0, &item.dur); break;
                        case 3: strncpy(item.title, sp_r, PATH_MAX); break;
                    };
                };

                /* insert item */
                items[c++] = item;
                g_warning("%s: id=[%s], in=[%d], dur=[%d]", __FUNCTION__, item.id, item.in, item.dur);
            }
            else
                g_warning("omnplay_library_load_file: ignored line [%s]\n", l);
        }

        fclose(f);
    }
    else
        r = -1;

    /* free data */
    free(l);

    *pcount = c;

    g_warning("omnplay_library_load_file: loaded [%d] items from [%s] file, limit [%d]\n", c, filename, limit);

    return r;
};

void omnplay_library_load(omnplay_instance_t* app)
{
    pthread_mutex_lock(&app->library.lock);

    if(app->library.filename[0])
    {
        app->library.count = MAX_LIBRARY_ITEMS;
        omnplay_library_load_file(app->library.item, &app->library.count, app->library.filename);
    };

    omnplay_library_sort(app);

    pthread_mutex_unlock(&app->library.lock);

    omnplay_library_draw(app);
};

static void omnplay_library_save_file(playlist_item_t* item, int count, char* filename)
{
    int i;
    FILE* f;

    if((f = fopen(filename, "wt")))
    {
        char tc_in[32], tc_dur[32];

        for(i = 0; i < count; i++)
            fprintf(f, "%s\t%s\t%s\t%s\n",
                item[i].id,
                frames2tc(item[i].in, 25.0, tc_in),
                frames2tc(item[i].dur, 25.0, tc_dur),
                item[i].title);
        fclose(f);
        g_warning("omnplay_library_save_file: written [%d] lines to file [%s]\n", count, filename);
    };
};

void omnplay_library_save(omnplay_instance_t* app)
{
    pthread_mutex_lock(&app->library.lock);

    if(app->library.filename[0])
        omnplay_library_save_file(app->library.item, app->library.count,
            app->library.filename);

    pthread_mutex_unlock(&app->library.lock);
};

static void omnplay_get_content_cb(omnplay_instance_t* app, playlist_item_t* item, void* data)
{
    if(!(app->library.id_display_idx % app->library.id_display_rate))
        omnplay_set_status(app, item->id);
    app->library.id_display_idx++;
};

static void* omnplay_library_refresh_proc(void* data)
{
    omnplay_instance_t* app = (omnplay_instance_t*)data;
    int count, i;
    playlist_item_t* items;

    gdk_threads_enter();
    gtk_widget_set_sensitive(app->window, FALSE);
    gdk_flush();
    gdk_threads_leave();

    omnplay_set_status(app, "Updating library...");

    items = (playlist_item_t*)malloc(sizeof(playlist_item_t) * MAX_LIBRARY_ITEMS);

    count = omnplay_get_content(app, items, MAX_LIBRARY_ITEMS, omnplay_get_content_cb, NULL);

    if(count > 0)
    {
        omnplay_set_status(app, "Quering whois...");

        if(app->library.whois[0])
            omnplay_whois_list(app, items, &count);

        omnplay_set_status(app, "Setting library...");

        pthread_mutex_lock(&app->library.lock);

        for(i = 0; i < count; i++)
            app->library.item[i] = items[i];

        app->library.count = count;

        omnplay_library_sort(app);

        pthread_mutex_unlock(&app->library.lock);

        gdk_threads_enter();
        omnplay_library_draw(app);
        gdk_flush();
        gdk_threads_leave();
    };

    omnplay_set_status(app, "Normalizing playlist...");

    free(items);

    gdk_threads_enter();
    omnplay_playlist_normalize(app);
    gdk_flush();
    gdk_threads_leave();

    omnplay_set_status(app, "");

    gdk_threads_enter();
    gtk_widget_set_sensitive(app->window, TRUE);
    gdk_flush();
    gdk_threads_leave();


    return NULL;
};

void omnplay_library_refresh(omnplay_instance_t* app)
{
    if(app->library.refresh_thread)
        g_thread_join(app->library.refresh_thread);

    app->library.refresh_thread = g_thread_create(
        omnplay_library_refresh_proc, app, TRUE, NULL);
};

void omnplay_library_draw(omnplay_instance_t* app)
{
    int i;
    char tc[12];
    GtkListStore *list_store;
    GtkTreeIter iter;

    list_store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->library_grid)));
    gtk_list_store_clear(list_store);

    pthread_mutex_lock(&app->library.lock);

    for(i = 0;i < app->library.count; i++)
    {
        gtk_list_store_append(list_store, &iter);

        gtk_list_store_set(list_store, &iter,
            0, app->library.item[i].id,
            1, frames2tc(app->library.item[i].dur, 25.0, tc),
            2, app->library.item[i].title,
            3, i,
            4, FALSE,
            5, "red",
            -1 );
    }

    pthread_mutex_unlock(&app->library.lock);
};

static void get_selected_idx_library_proc(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    int idx, *list = (int*)data;
    gtk_tree_model_get(model, iter, 3, &idx, -1);
    list[list[0] + 1] = idx;
    list[0] = list[0] + 1;
};

static int* get_selected_idx_library(omnplay_instance_t* app)
{
    int* list = NULL;
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->library_grid));
    if(selection)
    {
        list = (int*)malloc(sizeof(int) * (MAX_LIBRARY_ITEMS + 1));
        memset(list, 0, sizeof(int) * (MAX_LIBRARY_ITEMS + 1));

        gtk_tree_selection_selected_foreach(
            selection,
            get_selected_idx_library_proc,
            list);

        if(!list[0])
        {
            free(list);
            list = NULL;
        };
    };

    return list;
};


playlist_item_t* omnplay_library_get_selected(omnplay_instance_t* app, int *count)
{
    int* idxs;
    playlist_item_t* items = NULL;

    pthread_mutex_lock(&app->library.lock);

    *count = 0;

    idxs = get_selected_idx_library(app);

    if(idxs)
    {
        int i;

        /* alloc items */
        items = (playlist_item_t*)malloc(sizeof(playlist_item_t) * (idxs[0] + 1));

        /* clear last item */
        memset(&items[idxs[0]], 0, sizeof(playlist_item_t));

        /* copy items */
        for(i = 0; i < idxs[0]; i++)
            items[i] = app->library.item[idxs[i + 1]];

        *count = idxs[0];
        free(idxs);
    };

    pthread_mutex_unlock(&app->library.lock);

    return items;
};

void omnplay_library_search(omnplay_instance_t* app, int next)
{
    int idx = 0, i;
    int* idxs;
    const char *search;
    GtkTreePath* path;

    pthread_mutex_lock(&app->library.lock);

    idxs = get_selected_idx_library(app);
    if(idxs) idx = idxs[1];
    free(idxs);

    if(!next) idx = 0;
    else idx++;

    search = gtk_entry_get_text(GTK_ENTRY(app->library.search));

    if(search[0])
    {
        for(i = idx; i < app->library.count; i++)
            if( strcasestr(app->library.item[i].id, search) ||
                strcasestr(app->library.item[i].title, search))
                    break;

        if(i < app->library.count)
        {
            g_warning("found at pos=%d\n", i);

            /* select */
            path = gtk_tree_path_new_from_indices(i, -1);
            gtk_tree_selection_select_path(gtk_tree_view_get_selection(
                GTK_TREE_VIEW(app->library_grid)), path);
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(app->library_grid), path, NULL, FALSE);
            gtk_tree_path_free(path);
        };
    };

    pthread_mutex_unlock(&app->library.lock);
};
