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
                    /* setup item */
                    tc2frames(IN, 25.0, &items[count].in);
                    tc2frames(DUR, 25.0, &items[count].dur);
                    strncpy(items[count].id, ID, PATH_MAX);
                    items[count].player = atol(CH) - 1;
                    switch(atol(B))
                    {
                        case 1: items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE; break;
                        case 2: items[count].type = OMNPLAY_PLAYLIST_ITEM_LOOP_BEGIN; break;
                        case 3: items[count].type = OMNPLAY_PLAYLIST_ITEM_LOOP_BODY; break;
                        case 4: items[count].type = OMNPLAY_PLAYLIST_ITEM_LOOP_END; break;
                        case 6: items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_END; break;
                        case 0:
                            if(!count || items[count - 1].type != OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY)
                                items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN;
                            else
                                items[count].type = OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY;
                            break;
                    };

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
            app->playlist.item[app->playlist.count++] = items[i];
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

    dialog = gtk_file_chooser_dialog_new("Open File",
        GTK_WINDOW (app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
        (app->playlist.path)?app->playlist.path:getenv("HOME"));

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

void omnplay_playlist_save(omnplay_instance_t* app)
{
};

void omnplay_playlist_draw(omnplay_instance_t* app)
{
};
