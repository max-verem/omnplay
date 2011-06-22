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
        while( !feof(f) && c < (limit -1))
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

                for(i = 0, sp_b = l; (NULL != (sp_r = strtok(sp_b, "\t"))); i++, sp_b = NULL)
                {
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
            };
        }

        fclose(f);
    }
    else
        r = -1;

    /* free data */
    free(l);

    *pcount = c;

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
    fprintf(stderr, "requested: id=[%s]\n", item->id);
};

void omnplay_library_refresh(omnplay_instance_t* app)
{
    int count, i;
    playlist_item_t* items;


    items = (playlist_item_t*)malloc(sizeof(playlist_item_t) * MAX_LIBRARY_ITEMS);

    count = omnplay_get_content(app, items, MAX_LIBRARY_ITEMS, omnplay_get_content_cb, NULL);

    if(count > 0)
    {
        if(app->library.whois[0])
            omnplay_whois_list(app, items, &count);

        pthread_mutex_lock(&app->library.lock);

        for(i = 0; i < count; i++)
            app->library.item[i] = items[i];

        app->library.count = count;

        pthread_mutex_unlock(&app->library.lock);

        omnplay_library_draw(app);
    };

    free(items);
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
            -1 );
    }

    pthread_mutex_unlock(&app->library.lock);
};
