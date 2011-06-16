/*
 * omnplay.h -- GTK+ 2 omnplay
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

#ifndef OMNPLAY_H
#define OMNPLAY_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


typedef struct omnplay_instance
{
    GtkWidget *window;
}
omnplay_instance_t;

omnplay_instance_t* omnplay_create(int argc, char** argv);
void omnplay_init(omnplay_instance_t* app);
void omnplay_close(omnplay_instance_t* app);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* OMNPLAY_H */
