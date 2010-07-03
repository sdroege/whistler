/* This file is part of whistler
 *
 * Copyright (C) 2007-2008 Sebastian Dröge <slomo@upb.de>
 * 
 * Whistler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Whistler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Whistler. If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gstdio.h>
#include "whstrainingdata.h"

void
whs_training_data_free_element (WhsTrainingData *element)
{
  g_return_if_fail (element != NULL);

  g_slice_free (WhsTrainingData, element);
}

WhsTrainingData *
whs_training_data_new_element (void)
{
  return g_slice_new (WhsTrainingData);
}


void
whs_training_data_free (GList *training_data)
{
  g_list_foreach (training_data, (GFunc) whs_training_data_free_element, NULL);
  g_list_free (training_data);
}

GList *
whs_training_data_load_from_file (const gchar *filename)
{
  g_return_val_if_fail (filename != NULL && *filename != '\0', NULL);

  gchar *data = NULL, *p;
  gsize length = -1;

  GList *ret = NULL;
  WhsTrainingData *tdata = NULL;


  if (! g_file_get_contents (filename, &data, &length, NULL)) {
    g_warning ("Could not load file");
    return NULL;
  }

  p = data;

  if (strncmp (data, "WHST\n", 5) != 0) {
    goto error;
  }

  p += 5;

  guint64 start = 0, stop = 0;

  while (p - data < length) {
    tdata = g_slice_new (WhsTrainingData);
    gchar *endptr;

    tdata->result = g_ascii_strtoull (p, &endptr, 10);
    p = endptr;

    if (*p != '=')
      goto error;
    p++;

    tdata->start = g_ascii_strtoull (p, &endptr, 10);
    
    if (stop > tdata->start)
      goto error;

    start = tdata->start;
    p = endptr;

    if (*p != ',')
      goto error;
    p++;

    tdata->stop = g_ascii_strtoull (p, &endptr, 10);

    if (start > tdata->stop)
      goto error;

    stop = tdata->stop;
    p = endptr;

    if (*p != '\n')
      goto error;
    p++;

    ret = g_list_prepend (ret, tdata);
    tdata = NULL;
  }

  return g_list_reverse (ret);

error:
  g_warning ("Not a valid training data file");

  if (tdata != NULL)
    whs_training_data_free_element (tdata);

  whs_training_data_free (ret);
  return NULL;
}

gboolean
whs_training_data_save (const GList *training_data, const gchar *filename)
{
  FILE *f = g_fopen (filename, "wb");
  size_t ret;

  if (f == NULL) {
    g_warning ("Could not create file");
    return FALSE;
  }

  if ((ret = fwrite ("WHST\n", 1, 5, f)) < 5) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 5 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  guint64 start = 0, stop = 0;

  for (const GList *l = training_data; l != NULL; l = l->next) {
    WhsTrainingData *tdata = (WhsTrainingData *) l->data;

    if (tdata->result == -1)
      continue;

    if (stop > tdata->start) {
      g_warning ("start before last training data");
      continue;
    }

    start = tdata->start;
    stop = tdata->stop;
    if (stop < start) {
      g_warning ("start after stop, ignoring");
      continue;
    }

    gchar *line = g_strdup_printf ("%d=%" G_GUINT64_FORMAT ",%" G_GUINT64_FORMAT "\n",
        tdata->result, tdata->start, tdata->stop);
    size_t len = strlen (line);

    if ((ret = fwrite (line, 1, len, f)) < len) {
      if (ret >= 0)
        g_warning ("Wrote only %d of %d bytes", ret, len);
      else
        g_warning ("Write failed: %s", strerror (ret));

      g_free (line);
      fclose (f);
      return FALSE;
    }

    g_free (line);
  }

  fclose (f);
  return TRUE;
}

