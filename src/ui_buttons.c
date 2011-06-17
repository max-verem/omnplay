/*
 * ui_buttons.h -- GTK+ 2 omnplay
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

#ifndef UI_BUTTONS_H
#define UI_BUTTONS_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "ui.h"
#include "ui_utils.h"
#include "ui_buttons.h"
#include "support.h"

typedef struct button_desc
{
    control_buttons_t id;
    char* tooltip;
    char* stock;
    char* label;
    char* hookup;
    char* filename;
} button_desc_t;

static const button_desc_t buttons[] =
{
    {
        BUTTON_PLAYLIST_ITEM_ADD,
        "Add item to playlist",
        NULL,
        NULL,
        "button_playlist_add",
        "Axialis_Team_item_add_16x116.png"
    },
    {
        BUTTON_PLAYLIST_ITEM_EDIT,
        "Edit item in playlist",
        NULL,
        NULL,
        "button_playlist_edit",
        "Axialis_Team_item_edit_16x116.png"
    },
    {
        BUTTON_PLAYLIST_ITEM_DEL,
        "Delete item from playlist",
        NULL,
        NULL,
        "button_playlist_del",
        "Axialis_Team_item_delete_16x116.png"
    },
    {
        BUTTON_PLAYLIST_ITEM_UP,
        "Move item ebove",
        NULL,
        NULL,
        "button_playlist_up",
        "Axialis_Team_item_move_up_16x116.png"
    },
    {
        BUTTON_PLAYLIST_ITEM_DOWN,
        "Move item below",
        NULL,
        NULL,
        "button_playlist_down",
        "Axialis_Team_item_move_down_16x116.png"
    },
    {
        BUTTON_PLAYLIST_LOAD,
        "Load playlist",
        NULL,
        NULL,
        "button_playlist_load",
        "Axialis_Team_playlist_open_16x16.png"
    },
    {
        BUTTON_PLAYLIST_SAVE,
        "Save playlist",
        NULL,
        NULL,
        "button_playlist_load",
        "Axialis_Team_playlist_save_16x16.png"
    },
    {
        BUTTON_PLAYLIST_BLOCK_LOOP,
        "Make a loop block",
        NULL,
        NULL,
        "button_playlist_block_loop",
        "Axialis_Team_item_loop_16x116.png"
    },
    {
        BUTTON_PLAYLIST_BLOCK_SINGLE,
        "Make a single block",
        NULL,
        NULL,
        "button_playlist_block_single",
        "Axialis_Team_item_block_16x116.png"
    },
    {
        BUTTON_PLAYER_CUE,
        "Cue block",
        NULL,
        NULL,
        "button_playlist_cue",
        "Axialis_Team_player_cue_32x32.png"
    },
    {
        BUTTON_PLAYER_PLAY,
        "Play",
        NULL,
        NULL,
        "button_playlist_play",
        "Axialis_Team_player_play_64x32.png"
    },
    {
        BUTTON_PLAYER_PAUSE,
        "Pause",
        NULL,
        NULL,
        "button_playlist_pause",
        "Axialis_Team_player_pause_32x32.png"
    },
    {
        BUTTON_PLAYER_STOP,
        "STOP",
        NULL,
        NULL,
        "button_playlist_stop",
        "Axialis_Team_player_stop_32x32.png"
    },
    {
        BUTTON_LIBRARY_ADD,
        "Add to playlist",
        NULL,
        NULL,
        "button_library_add",
        "Axialis_Team_item_add_from_library_16x16.png"
    },
    {
        BUTTON_LIBRARY_REFRESH,
        "Refresh library",
        NULL,
        NULL,
        "button_library_refresh",
        "Axialis_Team_library_refresh_16x16.png"
    },


    {
        BUTTON_LAST,
    }
};

GtkWidget* ui_create_button(GtkWidget* top, omnplay_instance_t* app, control_buttons_t id)
{
    int idx;
    GtkWidget *button, *alignment, *hbox, *image = NULL, *label;
    const button_desc_t* info = NULL;

    for(idx = 0; !info && buttons[idx].id != BUTTON_LAST; idx++)
        if(buttons[idx].id == id)
            info = &buttons[idx];

    if(!info)
        return NULL;

    button = gtk_button_new();
    gtk_widget_show(button);
    GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_button_set_relief(GTK_BUTTON (button), GTK_RELIEF_NONE);
    if(info->tooltip)
        gtk_widget_set_tooltip_text(button, info->tooltip);

    alignment = gtk_alignment_new(0, 0.5, 0, 0);
    gtk_widget_show(alignment);
    gtk_container_add(GTK_CONTAINER (button), alignment);

    hbox = gtk_hbox_new(FALSE, 2);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);

    if(info->stock)
        image = gtk_image_new_from_stock(info->stock, GTK_ICON_SIZE_MENU);
    else if(info->filename)
        image = gtk_image_new_from_pixbuf(create_pixbuf(info->filename));
    if(image)
    {
        gtk_widget_show(image);
        gtk_box_pack_start(GTK_BOX (hbox), image, FALSE, FALSE, 0);
    }

    label = gtk_label_new_with_mnemonic("");
    if(info->label)
        gtk_label_set_text(GTK_LABEL (label), info->label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL (label), GTK_JUSTIFY_LEFT);

    if(info->hookup)
        GLADE_HOOKUP_OBJECT (top, button, info->hookup);

    app->buttons[id] = button;

    return button;
}


#endif /* UI_BUTTONS_H */
