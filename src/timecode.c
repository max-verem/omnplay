/*
 * timecode.c -- GTK+ 2 omnplay
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

#include <stdio.h>

char* frames2tc(int f, float fps, char* buf)
{
    int tc[4] = { 0, 0, 0, 0 };
    float d;
    int t;

    if ( fps && f >= 0)
    {
        d = f / fps;
        t = d;

        tc[0] = (d - t) * fps;
        tc[1] = t % 60; t /= 60;
        tc[2] = t % 60; t /= 60;
        tc[3] = t % 24;
    }

    sprintf(buf, "%.2d:%.2d:%.2d:%.2d", tc[3], tc[2], tc[1], tc[0]);

    return buf;
}


