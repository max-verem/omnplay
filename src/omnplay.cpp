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
#include "opts.h"

#include "omplrclnt.h"

static gboolean on_main_window_delete_event( GtkWidget *widget, GdkEvent *event, gpointer user_data )
{
    gtk_exit(0);
    return TRUE;
}


omnplay_instance_t* omnplay_create(int argc, char** argv)
{
    int i, c;
    omnplay_instance_t* app;

    /* prepare application instance */
    app = (omnplay_instance_t*)malloc(sizeof(omnplay_instance_t));
    memset(app, 0, sizeof(omnplay_instance_t));

    /* load parameters from command line */
    if(!omnplay_opt(argc, argv, app) && app->players.count)
        app->window = ui_omnplay(app);
    else
        omnplay_usage();

    return app;
};


void omnplay_destroy(omnplay_instance_t* app)
{
    free(app);
};


#if 0
static void test()
{
    int r;
    OmPlrClipInfo clip_info;
    char clip_name[omPlrMaxClipDirLen];
    OmPlrHandle omn;

    /* open director */
    r = OmPlrOpen
    (
        "omneon-1b.internal.m1stereo.tv",
        "Play_11",
        &omn
    );

    if(r)
        fprintf(stderr, "ERROR: OmPlrOpen failed with 0x%.8X\n", r);
    else
    {
        /* fetch all clips known in Omneon */
        r = OmPlrClipGetFirst
        (
            omn,
            clip_name, sizeof(clip_name)
        );

        if(r)
            fprintf(stderr, "ERROR: OmPlrClipGetFirst failed with 0x%.8X\n", r);
        else
        {
            fprintf(stderr, "OmPlrClipGetFirst=[%s]\n", clip_name);

            clip_info.maxMsTracks = 0;
            clip_info.size = sizeof(clip_info);
            r = OmPlrClipGetInfo(omn, clip_name, &clip_info);

            if(r)
                fprintf(stderr, "ERROR: OmPlrClipGetInfo failed with 0x%.8X\n", r);
            else
            {
                fprintf(stderr, "OmPlrClipGetInfo(%s)=firstFrame=%d, lastFrame=%d",
                    clip_name, clip_info.firstFrame, clip_info.lastFrame);
            }
        };


        OmPlrClose(omn);
    };
};
#endif

void omnplay_init(omnplay_instance_t* app)
{
    gtk_signal_connect( GTK_OBJECT( app->window ), "destroy",
        GTK_SIGNAL_FUNC(on_main_window_delete_event), app);
#if 0
    test();
#endif
};

void omnplay_release(omnplay_instance_t* app)
{
};
