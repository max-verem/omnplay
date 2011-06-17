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

#include "omnplay.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

GtkWidget* ui_create_button(GtkWidget* top, omnplay_instance_t* app, control_buttons_t id);

#ifdef __cplusplus
};
#endif /* __cplusplus */


#endif /* UI_BUTTONS_H */
