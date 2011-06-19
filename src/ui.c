/*
 * ui.c -- GTK+ 2 omnplay
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

typedef struct column_desc
{
    char* title;
    GType type;
} column_desc_t;

static const column_desc_t playlist_columns[] =
{
    {
        "REM",
        G_TYPE_STRING
    },
    {
        "B",
        G_TYPE_OBJECT
    },
    {
        "CH",
        G_TYPE_STRING
    },
    {
        "ID",
        G_TYPE_STRING
    },
    {
        "IN",
        G_TYPE_STRING
    },
    {
        "DUR",
        G_TYPE_STRING
    },
    {
        "TITLE",
        G_TYPE_STRING
    },
    {
        NULL,
        G_TYPE_STRING
    }
};

const static column_desc_t library_columns[] =
{
    {
        "ID",
        G_TYPE_STRING
    },
    {
        "DUR",
        G_TYPE_STRING
    },
    {
        "TITLE",
        G_TYPE_STRING
    },
    {
        NULL,
        G_TYPE_STRING
    }
};


static GtkWidget* create_label(GtkWidget* top, char* text, char* reg, GtkJustification jtype)
{
    GtkWidget* label;

    label = gtk_label_new ("");
    gtk_widget_show (label);

    gtk_label_set_justify (GTK_LABEL (label), jtype);

    if(reg)
        GLADE_HOOKUP_OBJECT (top, label, reg);

    if(text)
        gtk_label_set_text(GTK_LABEL (label), text);

    return label;
};

static GtkWidget* create_treeview(GtkWidget* top, char* name, const column_desc_t columns[])
{
    int i, count;

    GtkWidget *treeview;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *list_store;
    GType list_store_types[32];

    treeview = gtk_tree_view_new ();
    gtk_widget_show (treeview);

    for(i = 0, count = 0; columns[i].title; i++, count++)
        list_store_types[i] = (columns[i].type == G_TYPE_OBJECT)?GDK_TYPE_PIXBUF:columns[i].type;

    list_store = gtk_list_store_newv(count, list_store_types);
    gtk_tree_view_set_model( GTK_TREE_VIEW( treeview ), GTK_TREE_MODEL( list_store ) );

    for(i = 0; columns[i].title; i++)
    {
        char* prop;

        if(list_store_types[i] == G_TYPE_STRING)
        {
            renderer = gtk_cell_renderer_text_new();
            prop = "text";
        }
        else if(list_store_types[i] == G_TYPE_OBJECT)
        {
            renderer = gtk_cell_renderer_pixbuf_new();
            prop = "pixbuf";
        }
        else if(list_store_types[i] == G_TYPE_BOOLEAN)
        {
            renderer = gtk_cell_renderer_toggle_new();
            prop = "active";
        }
        else
            renderer = NULL;

        if(!renderer) continue;

        column = gtk_tree_view_column_new_with_attributes(
            columns[i].title, renderer, prop, i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW( treeview ), column);
    };

    g_object_unref(list_store);

    GLADE_HOOKUP_OBJECT (top, treeview, name);

    return treeview;
};

static GtkWidget* pane_library_grid(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget *scrolledwindow;

    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (scrolledwindow),
        app->library_grid = create_treeview(top, "treeview_library", library_columns));

    return scrolledwindow;
}

static GtkWidget* pane_library_buttons(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* hbox;

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);

    /* playlist modify buttons */
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_LIBRARY_ADD),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_LIBRARY_REFRESH),
            FALSE, FALSE, 0);

    /* spacer */
    gtk_box_pack_start (GTK_BOX (hbox),
        create_label(top, NULL, NULL, GTK_JUSTIFY_CENTER),
            TRUE, TRUE, 0);

    return hbox;
}

static GtkWidget* pane_library(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* vbox;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_widget_set_size_request(vbox, 300, -1);

    /* add buttons box */
    gtk_box_pack_start (GTK_BOX (vbox),
        pane_library_buttons(top, app),
        FALSE, FALSE, 0);

    /* add buttons box */
    gtk_box_pack_start (GTK_BOX (vbox),
        pane_library_grid(top, app),
        TRUE, TRUE, 0);

    return vbox;
}

static GtkWidget* create_channel_status(GtkWidget* top, omnplay_instance_t* app, int idx)
{
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* frame;
    omnplay_player_t* player;

    player = &app->players.item[idx];

    frame = gtk_frame_new(player->name);
    gtk_widget_show(frame);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_widget_show(vbox);

    /* status label */
    gtk_box_pack_start(GTK_BOX (vbox),
        player->label_status = create_label(top, "OFFLINE", NULL, GTK_JUSTIFY_LEFT),
            FALSE, FALSE, 0);

    /* spacel label */
    gtk_box_pack_start(GTK_BOX (vbox),
        create_label(top, " ", NULL, GTK_JUSTIFY_CENTER),
            FALSE, FALSE, 0);

    /* clip label */
    gtk_box_pack_start (GTK_BOX (vbox),
        player->label_clip = create_label(top, "U0002323", NULL, GTK_JUSTIFY_LEFT),
            FALSE, FALSE, 0);

    /* block state/current timecode */
    gtk_box_pack_start(GTK_BOX (vbox),
        hbox = gtk_hbox_new(TRUE, 0),
            FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    {
        /* clip state */
        gtk_box_pack_start(GTK_BOX (hbox),
            player->label_state = create_label(top, "PLAYING", NULL, GTK_JUSTIFY_LEFT),
                TRUE, TRUE, 0);

        /* current timecode */
        gtk_box_pack_start(GTK_BOX (hbox),
            player->label_tc_cur = create_label(top, "00:00:00:00", NULL, GTK_JUSTIFY_LEFT),
                TRUE, TRUE, 0);
    };

    /* block remain label/remain timecode */
    gtk_box_pack_start(GTK_BOX (vbox),
        hbox = gtk_hbox_new(TRUE, 0),
            FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    {
        /* label */
        gtk_box_pack_start(GTK_BOX (hbox),
            create_label(top, "remain:", NULL, GTK_JUSTIFY_LEFT),
                TRUE, TRUE, 0);

        /* remaining timecode */
        gtk_box_pack_start(GTK_BOX (hbox),
            player->label_tc_rem = create_label(top, "00:00:00:00", NULL, GTK_JUSTIFY_LEFT),
                TRUE, TRUE, 0);
    };

    return frame;
}

static GtkWidget* pane_operate_status(GtkWidget* top, omnplay_instance_t* app)
{
    int i;
    GtkWidget* vbox;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_widget_set_size_request(vbox, 250, -1);

    for(i = 0; i < app->players.count; i++)
    {
        gtk_box_pack_start (GTK_BOX (vbox),
            create_channel_status(top, app, i),
                FALSE, FALSE, 0);

        /* spacer */
        gtk_box_pack_start (GTK_BOX (vbox),
            create_label(top, NULL, NULL, GTK_JUSTIFY_CENTER),
                FALSE, FALSE, 0);
    }

    /* spacer */
    gtk_box_pack_start (GTK_BOX (vbox),
        create_label(top, NULL, NULL, GTK_JUSTIFY_CENTER),
            TRUE, TRUE, 0);

    return vbox;
}

static GtkWidget* pane_operate_buttons_playlist(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* hbox;

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);

    /* playlist load/save buttons */
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_LOAD),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_SAVE),
            FALSE, FALSE, 0);

    /* spacer */
    gtk_box_pack_start (GTK_BOX (hbox),
        create_label(top, "   ", NULL, GTK_JUSTIFY_CENTER),
            FALSE, FALSE, 0);

    /* playlist modify buttons */
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_ITEM_ADD),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_ITEM_EDIT),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_ITEM_DEL),
            FALSE, FALSE, 0);

    /* spacer */
    gtk_box_pack_start (GTK_BOX (hbox),
        create_label(top, "   ", NULL, GTK_JUSTIFY_CENTER),
            FALSE, FALSE, 0);

    /* playlist block buttons */
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_BLOCK_SINGLE),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_BLOCK_LOOP),
            FALSE, FALSE, 0);

    /* spacer */
    gtk_box_pack_start (GTK_BOX (hbox),
        create_label(top, "   ", NULL, GTK_JUSTIFY_CENTER),
            FALSE, FALSE, 0);

    /* playlist move items buttons */
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_ITEM_UP),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_ITEM_DOWN),
            FALSE, FALSE, 0);

    return hbox;
}

static GtkWidget* pane_operate_grid(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget *scrolledwindow;

    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (scrolledwindow),
        app->playlist_grid = create_treeview(top, "treeview_playlist", playlist_columns));

    return scrolledwindow;
}

static GtkWidget* pane_operate_buttons_operate(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* hbox;

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);

    /* spacer */
    gtk_box_pack_start (GTK_BOX (hbox),
        create_label(top, NULL, NULL, GTK_JUSTIFY_CENTER),
            TRUE, TRUE, 0);

    /* playlist modify buttons */
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYER_CUE),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYER_PLAY),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYER_PAUSE),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYER_STOP),
            FALSE, FALSE, 0);

    /* spacer */
    gtk_box_pack_start (GTK_BOX (hbox),
        create_label(top, NULL, NULL, GTK_JUSTIFY_CENTER),
            TRUE, TRUE, 0);

    return hbox;
}

static GtkWidget* pane_operate_operate(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* vbox;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

    /* add buttons box #1 */
    gtk_box_pack_start (GTK_BOX (vbox),
        pane_operate_buttons_playlist(top, app),
        FALSE, FALSE, 0);

    /* add buttons box */
    gtk_box_pack_start (GTK_BOX (vbox),
        pane_operate_grid(top, app),
        TRUE, TRUE, 0);

    /* add buttons box #1 */
    gtk_box_pack_start (GTK_BOX (vbox),
        pane_operate_buttons_operate(top, app),
        FALSE, FALSE, 0);

    return vbox;

}

static GtkWidget* pane_operate(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* hbox;

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);

    /* add buttons box */
    gtk_box_pack_start (GTK_BOX (hbox),
        pane_operate_status(top, app),
        FALSE, FALSE, 0);

    /* add buttons box */
    gtk_box_pack_start (GTK_BOX (hbox),
        pane_operate_operate(top, app),
        TRUE, TRUE, 0);

    return hbox;

};

static GtkWidget* pane_top(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* pane;

    pane = gtk_hpaned_new ();
    gtk_widget_show (pane);

    gtk_paned_set_position (GTK_PANED (pane), 800);

    gtk_paned_pack1 (GTK_PANED (pane),
        pane_operate(top, app),
        TRUE, FALSE);

    gtk_paned_pack2 (GTK_PANED (pane),
        pane_library(top, app),
        FALSE, FALSE);

    return pane;
}

GtkWidget* ui_omnplay (omnplay_instance_t* app)
{
    GtkWidget *wnd;

    wnd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GLADE_HOOKUP_OBJECT_NO_REF (wnd, wnd, "omnplay_window");

    gtk_window_set_title (GTK_WINDOW (wnd), _("Omneon Player"));
    gtk_window_set_default_size (GTK_WINDOW (wnd), 1024, 768);

    gtk_container_add (GTK_CONTAINER (wnd),
        pane_top(wnd, app));

    app->playlist.block_icons[OMNPLAY_PLAYLIST_ITEM_BLOCK_BEGIN] =
        create_pixbuf("block_type_block_start_16x16.png");
    app->playlist.block_icons[OMNPLAY_PLAYLIST_ITEM_BLOCK_BODY] =
        create_pixbuf("block_type_block_middle_16x16.png");
    app->playlist.block_icons[OMNPLAY_PLAYLIST_ITEM_BLOCK_END] =
        create_pixbuf("block_type_block_end_16x16.png");
    app->playlist.block_icons[OMNPLAY_PLAYLIST_ITEM_BLOCK_SINGLE] =
        create_pixbuf("block_type_block_single_16x16.png");
    app->playlist.block_icons[OMNPLAY_PLAYLIST_ITEM_LOOP_BEGIN] =
        create_pixbuf("block_type_loop_start_16x16.png");
    app->playlist.block_icons[OMNPLAY_PLAYLIST_ITEM_LOOP_BODY] =
        create_pixbuf("block_type_loop_middle_16x16.png");
    app->playlist.block_icons[OMNPLAY_PLAYLIST_ITEM_LOOP_END] =
        create_pixbuf("block_type_loop_end_16x16.png");
    app->playlist.block_icons[OMNPLAY_PLAYLIST_ITEM_LOOP_SINGLE] =
        create_pixbuf("block_type_block_loop_16x16.png");

    return wnd;
}
