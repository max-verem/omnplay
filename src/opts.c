/*
 * opts.c -- GTK+ 2 omnplay
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
#include <getopt.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "opts.h"

static const char short_options [] = "h";
static const struct option long_options [] =
{
    { "directory",              required_argument,    NULL,   '0'},
    { "player",                 required_argument,    NULL,   '1'},
    { "help",                   no_argument,          NULL,   'h'},
    { 0,                        0,                    0,      0}
};



int omnplay_opt(int argc, char** argv, omnplay_instance_t* app)
{
    char* p;
    int c, index = 0;

    /* reset datas */
    optind = 0; opterr = 0; optopt = 0;

    /* program arguments processing */
    while(1)
    {
        c = getopt_long (argc, argv, short_options, long_options, &index);

        if((-1) == c) break;

        switch(c)
        {
            case 0: break;

            /** --direcotry */
            case '0':
                strncpy(app->players.path, optarg, PATH_MAX);
                break;

            /** --player */
            case '1':
                p = strchr(optarg, '@');
                if(p)
                {
                    *p = 0;
                    strncpy(app->players.item[app->players.count].name, optarg, PATH_MAX);
                    strncpy(app->players.item[app->players.count].host, p + 1, PATH_MAX);
                    app->players.count++;
                };
                break;

            default:
                fprintf(stderr, "ERROR: Incorrect argument!\n");
                return 1;
                break;
        };
    };

    return 0;
};

void omnplay_usage(void)
{
    fprintf
    (
        stderr,
        "Usage:\n"
        "\t--directory=<PATH> Directory to override default\n"
        "\t--player=<STRING>  Player to use in a form <player_name>@<mediadirector host>\n"
    );
};
