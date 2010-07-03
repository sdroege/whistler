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

#include "whslearner.h"
#include "whsextractor.h"
#include "whsclassifier.h"
#include "classifier.h"
#include "whsbandpass.h"
#include "whsutils.h"
#include "whspatternprivate.h"
#include "whsprivate.h"

#include <glib/gstdio.h>
#include <string.h>

struct _WhsLearnerPrivate
{
  WhsExtractor *extractor;
  WhsClassifier *classifier;
  WhsBandpass *bandpass;

  gfloat *in;

  GList *vals;
  gint count;

  guint min_freq, max_freq;
};

#define WHS_LEARNER_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), WHS_TYPE_LEARNER, WhsLearnerPrivate))

static void whs_learner_init (WhsLearner * self);
static void whs_learner_class_init (WhsLearnerClass * klass);
static void whs_learner_finalize (WhsObject *object);

G_DEFINE_TYPE (WhsLearner, whs_learner, WHS_TYPE_OBJECT);

static WhsObjectClass *parent_class = NULL;

static void
whs_learner_class_init (WhsLearnerClass * klass)
{
  WhsObjectClass *o_klass = (WhsObjectClass *) klass;

  parent_class = WHS_OBJECT_CLASS (g_type_class_peek_parent (klass));

  g_type_class_add_private (klass, sizeof (WhsLearnerPrivate));

  o_klass->finalize = whs_learner_finalize;
}

static void
whs_learner_init (WhsLearner * self)
{
  self->priv = WHS_LEARNER_GET_PRIVATE (self);
}

static void
_result_free (WhsResultValue *res)
{
  g_slice_free (WhsResultValue, res);
}

static void
whs_learner_finalize (WhsObject *object)
{
  WhsLearner *self = WHS_LEARNER (object);

  if (self->priv->extractor) {
    whs_object_unref (self->priv->extractor);
    self->priv->extractor = NULL;
  }

  if (self->priv->vals) {
    g_list_foreach (self->priv->vals, (GFunc) _result_free, NULL);
    g_list_free (self->priv->vals);
    self->priv->vals = NULL;
    self->priv->count = 0;
  }

  if (self->priv->bandpass) {
    whs_bandpass_free (self->priv->bandpass);
    self->priv->bandpass = NULL;
  }

  if (self->priv->in) {
    g_free (self->priv->in);
    self->priv->in = NULL;
  }

  WHS_OBJECT_CLASS (parent_class)->finalize (object);
}

WhsLearner *
whs_learner_new (const gchar *classifier, guint sample_rate, guint frame_length, guint min_freq, guint max_freq, WhsPattern *pattern)
{
  WhsLearner *self;

  g_return_val_if_fail (frame_length > 0, NULL);
  g_return_val_if_fail (sample_rate > 0, NULL);
  g_return_val_if_fail (pattern == NULL || WHS_IS_PATTERN (pattern), NULL);
  g_return_val_if_fail ((min_freq == 0 && max_freq == 0) || (min_freq < max_freq), NULL);
  g_return_val_if_fail (max_freq <= sample_rate / 2, NULL);

  if (pattern != NULL) {
    guint min, max;

    whs_pattern_get_frequency_band (pattern, &min, &max);
    if (min != min_freq && max != max_freq) {
      g_warning ("Gave incompatible minimum and maximum frequency");
      return NULL;
    }

    guint sr = whs_pattern_get_sample_rate (pattern);
    if (sr != sample_rate) {
      g_warning ("Gave incompatible sampling rate");
      return NULL;
    }

    if (classifier && strcmp (classifier, whs_pattern_get_classifier_name (pattern)) != 0) {
      g_warning ("Incompatible classifier given");
      return NULL;
    } else {
      classifier = whs_pattern_get_classifier_name (pattern);
    }
  }

  if (!classifier)
    classifier = CLASSIFIER;

  self = WHS_LEARNER_CAST (g_type_create_instance (WHS_TYPE_LEARNER));
  self->sample_rate = sample_rate;
  self->frame_length = frame_length;

  self->priv->extractor = whs_extractor_new (sample_rate, frame_length, min_freq, max_freq);
  self->priv->classifier = whs_classifier_new (classifier, pattern);

  self->priv->min_freq = CLAMP (min_freq, 0, G_MAXUINT32);
  self->priv->max_freq = CLAMP (max_freq, 0, G_MAXUINT32);

  if (min_freq != 0 && max_freq != 0)
    self->priv->bandpass = whs_bandpass_new (sample_rate, 1, min_freq, max_freq);

  self->priv->in = g_new (gfloat, frame_length);

  return self;
}

static void
whs_learner_preprocess (WhsLearner *self, const gfloat *in)
{
  memcpy (self->priv->in, (gfloat *) in, sizeof (gfloat) * self->frame_length);

  if (self->priv->bandpass)
    whs_bandpass_process (self->priv->bandpass, &self->priv->in, self->frame_length);
}

gboolean
whs_learner_process (WhsLearner *self, gint result, const gfloat *in)
{
  g_return_val_if_fail (WHS_IS_LEARNER (self), FALSE);
  g_return_val_if_fail (in != NULL, FALSE);

  if (result < 0)
    return TRUE;

  whs_learner_preprocess (self, in);

  WhsResultValue *res = g_slice_new (WhsResultValue);
  
  res->result = result;
  whs_extractor_process (self->priv->extractor, self->priv->in, &res->vec);

  self->priv->vals = g_list_prepend (self->priv->vals, res);
  self->priv->count++;

  return TRUE;
}

WhsPattern *
whs_learner_generate_pattern (WhsLearner *self, gfloat rate)
{
  g_return_val_if_fail (WHS_IS_LEARNER (self), NULL);

  self->priv->vals = g_list_reverse (self->priv->vals);

  WhsPattern *ret = whs_classifier_learn (self->priv->classifier, self->priv->vals, self->priv->count, rate);
  
  self->priv->vals = g_list_reverse (self->priv->vals);

  whs_pattern_set_frequency_band (ret, self->priv->min_freq, self->priv->max_freq);
  whs_pattern_set_sample_rate (ret, self->sample_rate);

  return ret;
}

void
whs_learner_finish_sequence (WhsLearner *self)
{
  g_return_if_fail (WHS_IS_LEARNER (self));

  WhsResultValue *res = g_slice_new0 (WhsResultValue);
  
  res->result = G_MININT32;
  self->priv->vals = g_list_prepend (self->priv->vals, res);
  self->priv->count++;
}

gboolean
whs_learner_save_state (WhsLearner *self, const gchar *filename)
{
  g_return_val_if_fail (WHS_IS_LEARNER (self), FALSE);
  g_return_val_if_fail (filename != NULL && *filename != '\0', FALSE);


  // Prepend dummy value to detect where one sequence ends and another one starts
  // Assumes that the learner is saved after each sequence
  whs_learner_finish_sequence (self);

  guint32 tmp;
  FILE *f = g_fopen (filename, "wb");
  size_t ret;

  if (!f) {
    g_warning ("Can't open file");
    return FALSE;
  }

  // Header
  if ((ret = fwrite ("WHSL", 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  // Min and max frequency
  tmp = GUINT32_TO_BE (self->priv->min_freq);
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  tmp = GUINT32_TO_BE (self->priv->max_freq);
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  // Sample rate
  tmp = GUINT32_TO_BE (self->sample_rate);
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }
  
  // Data size
  tmp = self->priv->count * (sizeof (gint32) + 32 * sizeof (gfloat));
  tmp = GUINT32_TO_BE (tmp);
  if ((ret = fwrite (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Wrote only %d of 4 bytes", ret);
    else
      g_warning ("Write failed: %s", strerror (ret));

    fclose (f);
    return FALSE;
  }

  // Data
  for (GList *l = self->priv->vals; l != NULL; l = l->next) {
    WhsResultValue *res = (WhsResultValue *) l->data;
    gint32 result = GINT32_TO_BE (res->result);

    if ((ret = fwrite (&result, 1, 4, f)) < 4) {
      if (ret >= 0)
        g_warning ("Wrote only %d of 4 bytes", ret);
      else
        g_warning ("Write failed: %s", strerror (ret));
      fclose (f);
      return FALSE;
    }

    for (gint i = 0; i < 32; i++) {
      gfloat mfcc = GFLOAT_TO_BE (res->vec.mfcc[i]);
      
      if ((ret = fwrite (&mfcc, 1, 4, f)) < 4) {
        if (ret >= 0)
	  g_warning ("Wrote only %d of 4 bytes", ret);
	else
	  g_warning ("Write failed: %s", strerror (ret));
        fclose (f);
        return FALSE;
      }
    }
  }

  fclose (f);
  return TRUE;
}

WhsLearner *
whs_learner_new_from_state (const gchar *classifier, guint sample_rate, guint frame_length, const gchar *filename, WhsPattern *pattern)
{
  g_return_val_if_fail (filename != NULL && *filename != '\0', NULL);
  g_return_val_if_fail (frame_length > 0, NULL);
  g_return_val_if_fail (sample_rate >= 0, NULL);
  g_return_val_if_fail (pattern == NULL || WHS_IS_PATTERN (pattern), NULL);

  FILE *f = g_fopen (filename, "rb");

  if (!f) {
    g_warning ("Can't open file");
    return NULL;
  }

  guint min_freq = 0, max_freq = 0, sr = 0;

  if (pattern != NULL) {
    whs_pattern_get_frequency_band (pattern, &min_freq, &max_freq);
    sr = whs_pattern_get_sample_rate (pattern);

    if (sample_rate == 0)
      sample_rate = sr;

    if (sr != sample_rate) {
      g_warning ("Incompatible sample rate given");
      return NULL;
    }

    if (classifier && strcmp (classifier, whs_pattern_get_classifier_name (pattern)) != 0) {
      g_warning ("Incompatible classifier given");
      return NULL;
    } else if (!classifier) {
      classifier = whs_pattern_get_classifier_name (pattern);
    }
  }

  size_t ret;
  
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

  if (strncmp (header, "WHSL", 4) != 0) {
    g_warning ("Not a valid learner state file");
    fclose (f);
    return NULL;
  }

  // Min and max frequency
  guint32 tmp;
  if ((ret = fread (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    fclose (f);
    return NULL;
  }

  if (pattern != NULL && min_freq != GUINT32_FROM_BE (tmp)) {
    g_warning ("Incompatible minimum frequency given");
    return NULL;
  } else if (pattern == NULL) {
    min_freq = GUINT32_FROM_BE (tmp);
  }

  if ((ret = fread (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    fclose (f);
    return NULL;
  }

  if (pattern != NULL && max_freq != GUINT32_FROM_BE (tmp)) {
    g_warning ("Incompatible maximum frequency given");
    return NULL;
  } else if (pattern == NULL) {
    max_freq = GUINT32_FROM_BE (tmp);
  }

  // Sample rate
  if ((ret = fread (&tmp, 1, 4, f)) < 4) {
    if (ret >= 0)
      g_warning ("Read only %d of 4 bytes", ret);
    else
      g_warning ("Read failed: %s", strerror (ret));
    fclose (f);
    return NULL;
  }

  if (pattern != NULL && sample_rate != GUINT32_FROM_BE (tmp)) {
    g_warning ("Incompatible sampling rate given");
    return NULL;
  } else if (pattern == NULL) {
    sample_rate = GUINT32_FROM_BE (tmp);
  }

  // Data size
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

  if (size % (4 + 32 * 4) != 0) {
    g_warning ("Invalid size");
    fclose (f);
    return NULL;
  }

  WhsLearner *self = whs_learner_new (classifier, sample_rate, frame_length, min_freq, max_freq, pattern);

  // Data
  gint nresults = size / (4 + 32 * 4);

  for (gint i = 0; i < nresults; i++) {
    WhsResultValue *res = g_slice_new (WhsResultValue);

    if ((ret = fread (&res->result, 1, 4, f)) < 4) {
      if (ret >= 0)
        g_warning ("Read only %d of 4 bytes", ret);
      else
        g_warning ("Read failed: %s", strerror (ret));
      g_slice_free (WhsResultValue, res);
      fclose (f);
      whs_object_unref (self);
      return NULL;
    }
    res->result = GINT32_FROM_BE (res->result);

    for (gint j = 0; j < 32; j++) {
      if ((ret = fread (&res->vec.mfcc[j], 1, 4, f)) < 4) {
        if (ret >= 0)
	  g_warning ("Read only %d of 4 bytes", ret);
	else
	  g_warning ("Read failed: %s", strerror (ret));
        fclose (f);
        g_slice_free (WhsResultValue, res);
        whs_object_unref (self);
        return NULL;
      }
      res->vec.mfcc[j] = GFLOAT_FROM_BE (res->vec.mfcc[j]);
    }

    self->priv->vals = g_list_prepend (self->priv->vals, res);
  }
  self->priv->vals = g_list_reverse (self->priv->vals);
  self->priv->count = nresults;

  fclose (f);
 
  return self;
}

