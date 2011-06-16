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
#include "support.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

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

static GtkWidget* create_treeview(GtkWidget* top, char* name, const char* columns[])
{
    int i;

    GtkWidget *treeview;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *list_store;

    treeview = gtk_tree_view_new ();
    gtk_widget_show (treeview);

    list_store = gtk_list_store_new(7,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING);
    gtk_tree_view_set_model( GTK_TREE_VIEW( treeview ), GTK_TREE_MODEL( list_store ) );

    for(i = 0; columns[i]; i++)
    {
        renderer = gtk_cell_renderer_toggle_new();
        column = gtk_tree_view_column_new_with_attributes(
            columns[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW( treeview ), column);
    };

    GLADE_HOOKUP_OBJECT (top, treeview, name);

    return treeview;
};

static GtkWidget* pane_library_grid(GtkWidget* top, omnplay_instance_t* app)
{
//    static const char* columns[] = {"REM", "B", "CH", "ID", "IN", "DUR", "TITLE", NULL};
    static const char* columns[] = {"ID", "DUR", "TITLE", NULL};
    GtkWidget *scrolledwindow;

    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (scrolledwindow),
        create_treeview(top, "treeview_library", columns));

    return scrolledwindow;
}

static GtkWidget* pane_library_buttons(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* hbox;

    hbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (hbox);

    /* add buttons here: */
    gtk_box_pack_start (GTK_BOX (hbox),
        create_label(top, "BUTTONS HERE", NULL, GTK_JUSTIFY_LEFT),
            FALSE, TRUE, 0);

    return hbox;
}

static GtkWidget* pane_library(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* vbox;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

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

static GtkWidget* pane_operate_status(GtkWidget* top, omnplay_instance_t* app)
{
    GtkWidget* frame;

    frame = gtk_frame_new ("STATUS");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_widget_show(frame);

    gtk_container_add(GTK_CONTAINER(frame),
        create_label(top, "status here", NULL, GTK_JUSTIFY_CENTER));

    return frame;
}

static GtkWidget* pane_operate_operate(GtkWidget* top, omnplay_instance_t* app)
{
    return create_label(top, "pane_operate", NULL, GTK_JUSTIFY_CENTER);
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

    gtk_paned_set_position (GTK_PANED (pane), 300);

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

    return wnd;
}
