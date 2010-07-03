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

#include "whspattern.h"
#include "whspatternprivate.h"

#include <glib/gstdio.h>
#include <string.h>

struct _WhsPatternPrivate
{
  gchar *classifier;
  guint8 *classifier_data;
  gsize size;

  guint32 min_freq, max_freq;

  guint32 sample_rate;
};

#define WHS_PATTERN_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), WHS_TYPE_PATTERN, WhsPatternPrivate))

static void whs_pattern_init (WhsPattern * self);
static void whs_pattern_class_init (WhsPatternClass * klass);
static void whs_pattern_finalize (WhsObject *object);

G_DEFINE_TYPE (WhsPattern, whs_pattern, WHS_TYPE_OBJECT);

static WhsObjectClass *parent_class = NULL;

static void
whs_pattern_class_init (WhsPatternClass * klass)
{
  WhsObjectClass *o_klass = (WhsObjectClass *) klass;

  parent_class = WHS_OBJECT_CLASS (g_type_class_peek_parent (klass));

  g_type_class_add_private (klass, sizeof (WhsPatternPrivate));

  o_klass->finalize = whs_pattern_finalize;
}

static void
whs_pattern_init (WhsPattern * self)
{
  self->priv = WHS_PATTERN_GET_PRIVATE (self);
}

static void
whs_pattern_finalize (WhsObject *object)
{
  WhsPattern *self = WHS_PATTERN (object);

  g_free (self->priv->classifier_data);
  self->priv->classifier_data = NULL;
  self->priv->size = 0;

  g_free (self->priv->classifier);
  self->priv->classifier = NULL;

  WHS_OBJECT_CLASS (parent_class)->finalize (object);
}

WhsPattern *
whs_pattern_load (const gchar *filename)
{
  g_return_val_if_fail (filename != NULL && *filename != '\0', NULL);
  
  FILE *f = g_fopen (filename, "rb");
  size_t ret;

  if (!f) {
    g_warning ("Can't open file");
    return NULL;
  }

  // Header
  gchar header[4];
  if ((ret = fread (&header, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    fclose (f);
    return NULL;
  }

  if (strncmp (header, "WHSP", 4) != 0) {
    g_warning ("Not a valid pattern file");
    fclose (f);
    return NULL;
  }
 
  // Min and max frequency
  guint32 min_freq, max_freq;
  if ((ret = fread (&min_freq, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    fclose (f);
    return NULL;
  }
  min_freq = GUINT32_FROM_BE (min_freq);

  if ((ret = fread (&max_freq, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    fclose (f);
    return NULL;
  }
  max_freq = GUINT32_FROM_BE (max_freq);

  // Sample rate
  guint32 sample_rate;
  if ((ret = fread (&sample_rate, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    fclose (f);
    return NULL;
  }
  sample_rate = GUINT32_FROM_BE (sample_rate);

  // Classifier name size
  guint32 size;
  if ((ret = fread (&size, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    fclose (f);
    return NULL;
  }
  size = GUINT32_FROM_BE (size);

  if (size <= 0) {
    g_warning ("No classifier name");
    fclose (f);
    return NULL;
  }

  // Classifier name
  gchar *classifier = g_new0 (gchar, size);
  if ((ret = fread (classifier, 1, size, f)) < size) {
    if (ret >= 0)
      g_warning ("Read only %d of %d bytes", ret, size);
    else
      g_warning ("Read failed: %s", strerror (ret));

    g_free (classifier);
    fclose (f);
    return NULL;
  }

  // Classifier data size
  if ((ret = fread (&size, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    g_free (classifier);
    fclose (f);
    return NULL;
  }
  size = GUINT32_FROM_BE (size);

  if (size <= 0) {
    g_warning ("No classifier data");
    fclose (f);
    g_free (classifier);
    return NULL;
  }

  // Classifier data
  guint8 *data = g_new0 (guint8, size);
  if ((ret = fread (data, 1, size, f)) < size) {
    if (ret >= 0)
      g_warning ("Read only %d of %d bytes", ret, size);
    else
      g_warning ("Read failed: %s", strerror (ret));

    g_free (classifier);
    g_free (data);
    fclose (f);
    return NULL;
  }


  WhsPattern *self = WHS_PATTERN_CAST (g_type_create_instance (WHS_TYPE_PATTERN));

  self->priv->classifier = classifier;
  self->priv->classifier_data = data;
  self->priv->size = size;
  self->priv->min_freq = min_freq;
  self->priv->max_freq = max_freq;
  self->priv->sample_rate = sample_rate;

  fclose (f);

  return self;
}

gboolean
whs_pattern_save (WhsPattern *self, const gchar *filename)
{
  g_return_val_if_fail (WHS_IS_PATTERN (self), FALSE);
  g_return_val_if_fail (filename != NULL && *filename != '\0', FALSE);

  g_return_val_if_fail (self->priv->classifier_data != NULL || self->priv->size <= 0, FALSE);

  guint32 tmp;
  FILE *f = g_fopen (filename, "wb");
  size_t ret;

  if (!f) {
    g_warning ("Can't open file");
    return FALSE;
  }

  // Header
  if ((ret = fwrite ("WHSP", 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  // Min and max frequency
  tmp = GUINT32_TO_BE (CLAMP (self->priv->min_freq, 0, G_MAXUINT32));
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  tmp = GUINT32_TO_BE (CLAMP (self->priv->max_freq, 0, G_MAXUINT32));
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  // Sample rate

  tmp = GUINT32_TO_BE (CLAMP (self->priv->sample_rate, 0, G_MAXUINT32));
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  // Size of the classifier name
  guint32 len = strlen (self->priv->classifier) + 1;
  tmp = GUINT32_TO_BE (len);
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  // Classifier name  
  if ((ret = fwrite (self->priv->classifier, 1, len, f)) < len) {
    if (ret >= 0)
      g_warning ("Wrote only %d of %d bytes", ret, len);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }


  // Size of the data
  tmp = GUINT32_TO_BE (self->priv->size);
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }
  
  // Classifier data
  if ((ret = fwrite (self->priv->classifier_data, 1, self->priv->size, f)) < self->priv->size) {
    if (ret >= 0)
      g_warning ("Wrote only %d of %d bytes", ret, self->priv->size);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  fclose (f);

  return FALSE;
}

const guint8 *
whs_pattern_get_classifier_data (WhsPattern *self, const gchar *classifier, gsize *size)
{
  g_return_val_if_fail (WHS_IS_PATTERN (self), NULL);
  g_return_val_if_fail (size != NULL, NULL);
  g_return_val_if_fail (classifier != NULL && *classifier != '\0', NULL);

  if (strcmp (classifier, self->priv->classifier) != 0) {
    g_warning ("Wrong classifier");
    return NULL;
  }

  *size = self->priv->size;
  return self->priv->classifier_data;
}

void
whs_pattern_set_classifier_data (WhsPattern *self, const gchar *classifier, guint8 *data, gsize size)
{
  g_return_if_fail (WHS_IS_PATTERN (self));
  g_return_if_fail (classifier != NULL && *classifier != '\0');

  g_free (self->priv->classifier_data);
  g_free (self->priv->classifier);

  self->priv->classifier_data = data;
  self->priv->size = size;
  self->priv->classifier = g_strdup (classifier);
}

void
whs_pattern_set_frequency_band (WhsPattern *self, guint min_freq, guint max_freq)
{
  g_return_if_fail (WHS_IS_PATTERN (self));

  self->priv->min_freq = CLAMP (min_freq, 0, G_MAXUINT32);
  self->priv->max_freq = CLAMP (max_freq, 0, G_MAXUINT32);
}

void
whs_pattern_get_frequency_band (WhsPattern *self, guint *min_freq, guint *max_freq)
{
  g_return_if_fail (WHS_IS_PATTERN (self));

  if (min_freq)
    *min_freq = self->priv->min_freq;
  if (max_freq)
    *max_freq = self->priv->max_freq;
}

void
whs_pattern_set_sample_rate (WhsPattern *self, guint sample_rate)
{
  g_return_if_fail (WHS_IS_PATTERN (self));

  self->priv->sample_rate = CLAMP (sample_rate, 0, G_MAXUINT32);
}

guint
whs_pattern_get_sample_rate (WhsPattern *self)
{
  g_return_val_if_fail (WHS_IS_PATTERN (self), 0);
  
  return self->priv->sample_rate;
}

const gchar *
whs_pattern_get_classifier_name (WhsPattern *self)
{
  g_return_val_if_fail (WHS_IS_PATTERN (self), NULL);

  return self->priv->classifier;
}

