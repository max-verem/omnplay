/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "support.h"

#include "pixmap2-internal.h"

static const void* pixmap2_map[] =
{
    "Axialis_Team_item_add_16x116.png", Axialis_Team_item_add_16x116_png,
    "Axialis_Team_item_add_from_library_16x16.png", Axialis_Team_item_add_from_library_16x16_png,
    "Axialis_Team_item_block_16x116.png", Axialis_Team_item_block_16x116_png,
    "Axialis_Team_item_delete_16x116.png", Axialis_Team_item_delete_16x116_png,
    "Axialis_Team_item_edit_16x116.png", Axialis_Team_item_edit_16x116_png,
    "Axialis_Team_item_loop_16x116.png", Axialis_Team_item_loop_16x116_png,
    "Axialis_Team_item_move_down_16x116.png", Axialis_Team_item_move_down_16x116_png,
    "Axialis_Team_item_move_up_16x116.png", Axialis_Team_item_move_up_16x116_png,
    "Axialis_Team_library_refresh_16x16.png", Axialis_Team_library_refresh_16x16_png,
    "Axialis_Team_player_cue_32x32.png", Axialis_Team_player_cue_32x32_png,
    "Axialis_Team_player_pause_32x32.png", Axialis_Team_player_pause_32x32_png,
    "Axialis_Team_player_play_64x32.png", Axialis_Team_player_play_64x32_png,
    "Axialis_Team_player_stop_32x32.png", Axialis_Team_player_stop_32x32_png,
    "Axialis_Team_playlist_open_16x16.png", Axialis_Team_playlist_open_16x16_png,
    "Axialis_Team_playlist_relink_16x16.png", Axialis_Team_playlist_relink_16x16_png,
    "Axialis_Team_playlist_save_16x16.png", Axialis_Team_playlist_save_16x16_png,
    "Axialis_Team_search_in_library_16x16.png", Axialis_Team_search_in_library_16x16_png,
    "Axialis_Team_search_next_16x16.png", Axialis_Team_search_next_16x16_png,
    "block_type_block_end_16x16.png", block_type_block_end_16x16_png,
    "block_type_block_loop_16x16.png", block_type_block_loop_16x16_png,
    "block_type_block_middle_16x16.png", block_type_block_middle_16x16_png,
    "block_type_block_single_16x16.png", block_type_block_single_16x16_png,
    "block_type_block_start_16x16.png", block_type_block_start_16x16_png,
    "block_type_loop_end_16x16.png", block_type_loop_end_16x16_png,
    "block_type_loop_middle_16x16.png", block_type_loop_middle_16x16_png,
    "block_type_loop_start_16x16.png", block_type_loop_start_16x16_png,
    NULL, NULL
};

GdkPixbuf* create_pixbuf2(const gchar *filename)
{
    int i;

    for(i = 0; pixmap2_map[i]; i += 2)
        if(!strcmp((char*)pixmap2_map[i], filename))
            return gdk_pixbuf_new_from_inline(-1, pixmap2_map[i + 1], FALSE, NULL);

    return create_pixbuf(filename);
}


GtkWidget*
lookup_widget                          (GtkWidget       *widget,
                                        const gchar     *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (!parent)
        parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
                                                 widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}

static GList *pixmaps_directories = NULL;

/* Use this function to set the directory containing installed pixmaps. */
void
add_pixmap_directory                   (const gchar     *directory)
{
  pixmaps_directories = g_list_prepend (pixmaps_directories,
                                        g_strdup (directory));
}

/* This is an internally used function to find pixmap files. */
static gchar*
find_pixmap_file                       (const gchar     *filename)
{
  GList *elem;

  /* We step through each of the pixmaps directory to find it. */
  elem = pixmaps_directories;
  while (elem)
    {
      gchar *pathname = g_strdup_printf ("%s%s%s", (gchar*)elem->data,
                                         G_DIR_SEPARATOR_S, filename);
      if (g_file_test (pathname, G_FILE_TEST_EXISTS))
        return pathname;
      g_free (pathname);
      elem = elem->next;
    }
  return NULL;
}

/* This is an internally used function to create pixmaps. */
GtkWidget*
create_pixmap                          (GtkWidget       *widget,
                                        const gchar     *filename)
{
  gchar *pathname = NULL;
  GtkWidget *pixmap;

  if (!filename || !filename[0])
      return gtk_image_new ();

  pathname = find_pixmap_file (filename);

  if (!pathname)
    {
      g_warning (_("Couldn't find pixmap file: %s"), filename);
      return gtk_image_new ();
    }

  pixmap = gtk_image_new_from_file (pathname);
  g_free (pathname);
  return pixmap;
}

/* This is an internally used function to create pixmaps. */
GdkPixbuf*
create_pixbuf                          (const gchar     *filename)
{
  gchar *pathname = NULL;
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  if (!filename || !filename[0])
      return NULL;

  pathname = find_pixmap_file (filename);

  if (!pathname)
    {
      g_warning (_("Couldn't find pixmap file: %s"), filename);
      return NULL;
    }

  pixbuf = gdk_pixbuf_new_from_file (pathname, &error);
  if (!pixbuf)
    {
      fprintf (stderr, "Failed to load pixbuf file: %s: %s\n",
               pathname, error->message);
      g_error_free (error);
    }
  g_free (pathname);
  return pixbuf;
}

/* This is used to set ATK action descriptions. */
void
glade_set_atk_action_description       (AtkAction       *action,
                                        const gchar     *action_name,
                                        const gchar     *description)
{
  gint n_actions, i;

  n_actions = atk_action_get_n_actions (action);
  for (i = 0; i < n_actions; i++)
    {
      if (!strcmp (atk_action_get_name (action, i), action_name))
        atk_action_set_description (action, i, description);
    }
}

