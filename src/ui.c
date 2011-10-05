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
#include "timecode.h"

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

    if(jtype)
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
    GtkTreeSelection  *selection;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *list_store;
    GType list_store_types[32];

    treeview = gtk_tree_view_new ();
    gtk_widget_show (treeview);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(treeview), GTK_TREE_VIEW_GRID_LINES_BOTH);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    for(i = 0, count = 0; columns[i].title; i++, count++)
        list_store_types[i] = (columns[i].type == G_TYPE_OBJECT)?GDK_TYPE_PIXBUF:columns[i].type;
    list_store_types[count] = G_TYPE_INT;
    list_store_types[count + 1] = G_TYPE_BOOLEAN;
    list_store_types[count + 2] = G_TYPE_STRING;

    list_store = gtk_list_store_newv(count + 3, list_store_types);

    gtk_tree_view_set_model( GTK_TREE_VIEW( treeview ), GTK_TREE_MODEL( list_store ) );

    for(i = 0; columns[i].title; i++)
    {
        char* prop;
        column = NULL;

        if(columns[i].type == G_TYPE_OBJECT)
        {
            renderer = gtk_cell_renderer_pixbuf_new();
            gtk_cell_renderer_set_padding(renderer, 0, 0);
            prop = "pixbuf";
        }
        else if(columns[i].type == G_TYPE_BOOLEAN)
        {
            renderer = gtk_cell_renderer_toggle_new();
            prop = "active";
        }
        else
        {
            renderer = gtk_cell_renderer_text_new();
            prop = "text";

            column = gtk_tree_view_column_new_with_attributes(
                columns[i].title, renderer,
                    prop, i,
                    "background-set", count + 1,
                    "background", count + 2,
                    NULL);
        }

        if(!column)
            column = gtk_tree_view_column_new_with_attributes(
                columns[i].title, renderer,
                    prop, i,
                    NULL);

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

static GtkWidget* pane_library_search_buttons(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* hbox;

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);

    /* text entry */
    gtk_box_pack_start (GTK_BOX (hbox),
        app->library.search = gtk_entry_new(),
            TRUE, TRUE, 0);
    gtk_widget_show(app->library.search);

    /* playlist modify buttons */
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_LIBRARY_FIND),
            FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_LIBRARY_FIND_NEXT),
            FALSE, FALSE, 0);

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

    /* add grid */
    gtk_box_pack_start (GTK_BOX (vbox),
        pane_library_grid(top, app),
        TRUE, TRUE, 0);

    /* add search buttons */
    gtk_box_pack_start (GTK_BOX (vbox),
        pane_library_search_buttons(top, app),
        FALSE, FALSE, 0);

    return vbox;
}

static GtkWidget* create_channel_status(GtkWidget* top, omnplay_instance_t* app, int idx)
{
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* frame;
    char name[PATH_MAX];
    omnplay_player_t* player;

    player = &app->players.item[idx];

    snprintf(name, sizeof(name), "%c [%s]", idx + 'A', player->name);

    frame = gtk_frame_new(name);
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

    /* spacer */
    gtk_box_pack_start (GTK_BOX (hbox),
        create_label(top, "   ", NULL, GTK_JUSTIFY_CENTER),
            FALSE, FALSE, 0);

    /* playlist relink */
    gtk_box_pack_start (GTK_BOX (hbox),
        ui_create_button(top, app, BUTTON_PLAYLIST_RELINK),
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
    GtkWidget* vbox;

    wnd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GLADE_HOOKUP_OBJECT_NO_REF (wnd, wnd, "omnplay_window");

    gtk_window_set_title (GTK_WINDOW (wnd), _("Omneon Player"));
    gtk_window_set_default_size (GTK_WINDOW (wnd), 1024, 768);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);

    gtk_container_add(GTK_CONTAINER(wnd), vbox);

    gtk_box_pack_start (GTK_BOX (vbox),
        pane_top(wnd, app),
        TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox),
        app->status_label = create_label(wnd, "omnplay started", NULL, GTK_JUSTIFY_LEFT),
        FALSE, FALSE, 0);

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

int ui_playlist_item_dialog(omnplay_instance_t* app, playlist_item_t* item)
{
    int r, c;
    char tc[32];
    GtkWidget *dlg;
    gint response;
    GtkWidget *box, *table;
    GtkWidget *entry[4], *combo;

    dlg = gtk_dialog_new_with_buttons(
        "Playlist item",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
        NULL);

    box = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

    table = gtk_table_new(5, 2, TRUE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(box), table, TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(table),
        create_label(NULL, "ID:", NULL, 0),
        0, 1, 0, 1,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);

    gtk_table_attach(GTK_TABLE(table),
        create_label(NULL, "IN:", NULL, GTK_JUSTIFY_RIGHT),
        0, 1, 1, 2,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);

    gtk_table_attach(GTK_TABLE(table),
        create_label(NULL, "DUR:", NULL, GTK_JUSTIFY_RIGHT),
        0, 1, 2, 3,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);

    gtk_table_attach(GTK_TABLE(table),
        create_label(NULL, "TITLE:", NULL, GTK_JUSTIFY_RIGHT),
        0, 1, 3, 4,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);

    gtk_table_attach(GTK_TABLE(table),
        create_label(NULL, "CHANNEL:", NULL, GTK_JUSTIFY_RIGHT),
        0, 1, 4, 5,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);


    gtk_table_attach(GTK_TABLE(table),
        entry[0] = gtk_entry_new_with_max_length(32),
        1, 2, 0, 1,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);

    gtk_table_attach(GTK_TABLE(table),
        entry[1] = gtk_entry_new_with_max_length(12),
        1, 2, 1, 2,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);

    gtk_table_attach(GTK_TABLE(table),
        entry[2] = gtk_entry_new_with_max_length(12),
        1, 2, 2, 3,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);

    gtk_table_attach(GTK_TABLE(table),
        entry[3] = gtk_entry_new_with_max_length(128),
        1, 2, 3, 4,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);

    gtk_table_attach(GTK_TABLE(table),
        combo = gtk_combo_box_new_text(),
        1, 2, 4, 5,
        GTK_FILL/* | GTK_SHRINK */, GTK_FILL | GTK_SHRINK, 5, 5);


    /* setup data */
    gtk_entry_set_text(GTK_ENTRY(entry[0]), item->id);
    gtk_entry_set_text(GTK_ENTRY(entry[1]), frames2tc(item->in, 25.0, tc));
    gtk_entry_set_text(GTK_ENTRY(entry[2]), frames2tc(item->dur, 25.0, tc));
    gtk_entry_set_text(GTK_ENTRY(entry[3]), item->title);
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "A");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "B");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "C");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "D");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), item->player);

    gtk_widget_show_all(dlg);

    /* Run dialog */
    for(c = 1; c;)
    {
        response = gtk_dialog_run(GTK_DIALOG(dlg));

        if( GTK_RESPONSE_REJECT == response ||
            GTK_RESPONSE_DELETE_EVENT == response ||
            GTK_RESPONSE_CANCEL == response)
        {
            r = 0;
            c = 0;
        }
        else
        {
            r = 1;

            /* get item data back */
            strncpy(item->id, gtk_entry_get_text(GTK_ENTRY(entry[0])), PATH_MAX);
            tc2frames((char*)gtk_entry_get_text(GTK_ENTRY(entry[1])), 25.0, &item->in);
            tc2frames((char*)gtk_entry_get_text(GTK_ENTRY(entry[2])), 25.0, &item->dur);
            strncpy(item->title, gtk_entry_get_text(GTK_ENTRY(entry[3])), PATH_MAX);
            item->player = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

            /* check if all data entered correctly */
            if(item->id[0])
                c = 0;
        };
    };

    gtk_widget_hide(dlg);
    gtk_widget_destroy(dlg);

    return r;
};
