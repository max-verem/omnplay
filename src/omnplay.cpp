/*
 * omnplay.c -- GTK+ 2 omnplay
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

#include "omnplay.h"
#include "ui.h"

static gboolean on_main_window_delete_event( GtkWidget *widget, GdkEvent *event, gpointer user_data )
{
    gtk_exit(0);
    return TRUE;
}


omnplay_instance_t* omnplay_create(int argc, char** argv)
{
    omnplay_instance_t* app;

    app = (omnplay_instance_t*)malloc(sizeof(omnplay_instance_t));
    memset(app, 0, sizeof(omnplay_instance_t));

    app->window = ui_omnplay(app);

    return app;
};

void omnplay_close(omnplay_instance_t* app)
{
//OmPlrHandle plrHandle;
//OmPlrClose(plrHandle);
};

void omnplay_init(omnplay_instance_t* app)
{
    gtk_signal_connect( GTK_OBJECT( app->window ), "destroy",
        GTK_SIGNAL_FUNC(on_main_window_delete_event), app);
};
