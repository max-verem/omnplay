/*
 * whois.c -- GTK+ 2 omnplay
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
#include <pthread.h>
#include "curl/curl.h"

#include "omnplay.h"
#include "ui.h"
#include "timecode.h"


static size_t upload_cb_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
    return fwrite(ptr, size, nmemb, (FILE*)stream);
};

static int save_list(playlist_item_t* item, int count, char* filename)
{
    int i, r = 0;
    FILE* f;

    if((f = fopen(filename, "wt")))
    {
        char tc_in[32], tc_dur[32];

        for(i = 0; i < count; i++)
            fprintf(f, "%-40s%-15s%s\n",
                item[i].id,
                frames2tc(item[i].in, 25.0, tc_in),
                frames2tc(item[i].dur, 25.0, tc_dur));

        fclose(f);
    }
    else
        r = 1;

    return r;
};

static int post_file(char* url, char* file_in, char* file_out, char* curl_error_msg)
{
    int r = 0;
    FILE* f;
    CURL *curl;
    long http_responce = 0;
    struct curl_httppost* post = NULL;
    struct curl_httppost* last = NULL;

    /* open out file */
    f = fopen(file_out, "wt");
    if(!f)
    {
        snprintf(curl_error_msg, CURL_ERROR_SIZE, "failed to create output file [%s]", file_out);
        return -1;
    };

    /* prepare CURL to HTTP GET request*/
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL , 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, upload_cb_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_msg);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

    curl_formadd(&post, &last,
        CURLFORM_COPYNAME, "src_file",
        CURLFORM_FILE, file_in,
        CURLFORM_END);

    /* Set the form info */
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

    /* POST data to server */
    r = curl_easy_perform(curl);

    /* close file */
    fclose(f);

    /* check results */
    if(!r)
    {
        curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_responce);
        if(200 != http_responce)
        {
            snprintf(curl_error_msg, CURL_ERROR_SIZE, "FAILED [http_responce=%ld]", http_responce);
            r = -2;
        }
    };

    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_formfree(post);

    return r;
};

int omnplay_whois_list(omnplay_instance_t* app, playlist_item_t *items, int* pcount)
{
    int r;
    char *filenames[2];
    char *curl_error_msg;

    /* alloc filenames */
    filenames[0] = (char *)malloc(PATH_MAX);
    filenames[1] = (char *)malloc(PATH_MAX);
    curl_error_msg = (char*)malloc(CURL_ERROR_SIZE);

    /* compose filenames */
#ifdef _WIN32
    snprintf(filenames[0], PATH_MAX, "%s\\omnplay.whois.in", getenv("TEMP"));
    snprintf(filenames[1], PATH_MAX, "%s\\omnplay.whois.out", getenv("TEMP"));
#else
    snprintf(filenames[0], PATH_MAX, "%s/omnplay.whois.in", getenv("HOME"));
    snprintf(filenames[1], PATH_MAX, "%s/omnplay.whois.out", getenv("HOME"));
#endif

    r = save_list(items, *pcount, filenames[0]);

    if(r)
        fprintf(stderr, "Failed to save list to [%s]\n", filenames[0]);
    else
    {
        r = post_file(app->library.whois, filenames[0], filenames[1], curl_error_msg);
        if(r)
            fprintf(stderr, "Failed to whois: {%s}\n", curl_error_msg);
        else
            r = omnplay_library_load_file(items, pcount, filenames[1]);
    };

    free(filenames[0]);
    free(filenames[1]);
    free(curl_error_msg);

    return r;
};

