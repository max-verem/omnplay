/*
 * main.c -- GTK+ 2 omnplay
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

#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#include <stdio.h>
#include "ui.h"
#include "omnplay.h"
#include "support.h"

int main(int argc, char **argv)
{
    char path[ 512 ], *buf;
    omnplay_instance_t *app = NULL;

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    g_thread_init(NULL);
    gdk_threads_init();
    gtk_set_locale();
    gtk_init(&argc, &argv);

#ifdef _WIN32
#else
    // Linux hack to determine path of the executable
    memset(path, 0, sizeof(path));
    readlink( "/proc/self/exe", path, sizeof(path));
    g_warning ("path=(%s)\n", path);
    if((buf = strstr(path, "/bin/omnplay")))
    {
        buf[0] = 0;
        strcat(path, "/share/rugen/pixmaps");
    }
    else if((buf = strstr(path, "/src/omnplay")))
    {
        buf[0] = 0;
        strcat( path, "/pixmaps" );
    }
    else
        snprintf(path, sizeof(path), "%s/%s/pixmaps", PACKAGE_DATA_DIR, PACKAGE);

    add_pixmap_directory( path );
    g_warning ("add_pixmap_directory(%s)\n", path);

#endif /* _WIN32 */

    app = omnplay_create(argc, argv);
    gtk_widget_show (app->window);

    gdk_threads_enter();
    omnplay_init(app);
    gtk_main ();
    gdk_threads_leave();

    omnplay_close(app);

    return 0;
}

