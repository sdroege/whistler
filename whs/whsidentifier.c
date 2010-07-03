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

#include "whsidentifier.h"
#include "whsextractor.h"
#include "whslocalizer.h"
#include "whsclassifier.h"
#include "whspatternprivate.h"
#include "whsbandpass.h"
#include "whsprivate.h"

#include <math.h>

struct _WhsIdentifierPrivate
{
  gfloat **input, *mono;
  WhsExtractor *extractor;
  WhsLocalizer *localizer;
  WhsClassifier *classifier;
  WhsBandpass *bandpass[2];

  gfloat last_results[10];
  gfloat last_locations[10];
};

#define WHS_IDENTIFIER_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), WHS_TYPE_IDENTIFIER, WhsIdentifierPrivate))

static void whs_identifier_init (WhsIdentifier * self);
static void whs_identifier_class_init (WhsIdentifierClass * klass);
static void whs_identifier_finalize (WhsObject *object);

G_DEFINE_TYPE (WhsIdentifier, whs_identifier, WHS_TYPE_OBJECT);

static WhsObjectClass *parent_class = NULL;

static void
whs_identifier_class_init (WhsIdentifierClass * klass)
{
  WhsObjectClass *o_klass = (WhsObjectClass *) klass;

  parent_class = WHS_OBJECT_CLASS (g_type_class_peek_parent (klass));

  g_type_class_add_private (klass, sizeof (WhsIdentifierPrivate));

  o_klass->finalize = whs_identifier_finalize;
}

static void
whs_identifier_init (WhsIdentifier * self)
{
  self->priv = WHS_IDENTIFIER_GET_PRIVATE (self);
}

static void
whs_identifier_finalize (WhsObject *object)
{
  WhsIdentifier *self = WHS_IDENTIFIER (object);

  if (self->priv->extractor) {
    whs_object_unref (self->priv->extractor);
    self->priv->extractor = NULL;
  }

  if (self->priv->localizer) {
    whs_object_unref (self->priv->localizer);
    self->priv->localizer = NULL;
  }

  if (self->priv->classifier) {
    whs_object_unref (self->priv->classifier);
    self->priv->classifier = NULL;
  }

  if (self->priv->input)
    for (gint i = 0; i < self->nchannels; i++)
      g_free (self->priv->input[i]);

  if (self->priv->bandpass[0]) {
    whs_bandpass_free (self->priv->bandpass[0]);
    self->priv->bandpass[0] = NULL;
  }
  if (self->priv->bandpass[1]) {
    whs_bandpass_free (self->priv->bandpass[1]);
    self->priv->bandpass[1] = NULL;
  }

  g_free (self->priv->input);
  self->priv->input = NULL;
  g_free (self->priv->mono);
  self->priv->mono = NULL;

  WHS_OBJECT_CLASS (parent_class)->finalize (object);
}

WhsIdentifier *
whs_identifier_new (guint sample_rate, guint frame_length, guint nchannels, guint distance, WhsPattern *pattern)
{
  g_return_val_if_fail (frame_length > 0, NULL);
  g_return_val_if_fail (sample_rate > 0, NULL);
  g_return_val_if_fail (nchannels > 0, NULL);
  g_return_val_if_fail (WHS_IS_PATTERN (pattern), NULL);

  WhsIdentifier *self = WHS_IDENTIFIER_CAST (g_type_create_instance (WHS_TYPE_IDENTIFIER));
  self->sample_rate = sample_rate;
  self->frame_length = frame_length;
  self->nchannels = nchannels;

  guint min_freq, max_freq, sr;
  whs_pattern_get_frequency_band (pattern, &min_freq, &max_freq);
  if (min_freq != 0 && max_freq != 0) {
    self->priv->bandpass[0] = whs_bandpass_new (sample_rate, nchannels, min_freq, max_freq);
    self->priv->bandpass[1] = whs_bandpass_new (sample_rate, 1, min_freq, max_freq);
  }

  sr = whs_pattern_get_sample_rate (pattern);
  if (sr != sample_rate) {
    whs_object_unref (self);
    g_warning ("Incompatible sampling rate");
    return NULL;
  }

  self->priv->extractor = whs_extractor_new (sample_rate, frame_length, min_freq, max_freq);
  self->priv->localizer = whs_localizer_new (sample_rate, frame_length, nchannels, distance);
  self->priv->classifier = whs_classifier_new (whs_pattern_get_classifier_name (pattern), pattern);

  self->priv->input = g_new0 (gfloat *, nchannels);
  for (gint i = 0; i < nchannels; i++)
    self->priv->input[i] = g_new0 (gfloat, frame_length);
  
  self->priv->mono = g_new0 (gfloat, frame_length);

  for (gint i = 0; i < 10; i++)
    self->priv->last_results[i] = 0.5;

  return self;
}

static gboolean
whs_identifier_preprocess (WhsIdentifier *self, const gfloat *in)
{
  gdouble rms = 0.0;

  for (gint i = 0; i < self->frame_length; i++) {
    self->priv->mono[i] = 0.0;
    for (gint j = 0; j < self->nchannels; j++) {
      self->priv->input[j][i] = in[i * self->nchannels + j];
      self->priv->mono[i] += in[i * self->nchannels + j];
    }
    self->priv->mono[i] /= self->nchannels;
    rms += self->priv->mono[i] * self->priv->mono[i];
  }
  rms /= self->frame_length;
  rms = sqrt (rms);

  // Fast path if this frame doesn't contain anything useful
  if (rms <= 0.0001)
    return FALSE;

  if (self->priv->bandpass[0] && self->priv->bandpass[1]) {
    whs_bandpass_process (self->priv->bandpass[0], self->priv->input, self->frame_length);
    whs_bandpass_process (self->priv->bandpass[1], &self->priv->mono, self->frame_length);
  }

  return TRUE;
}

static void
whs_identifier_postprocess (WhsIdentifier *self, WhsResult *res)
{
  for (gint i = 0; i < 9; i++)
    self->priv->last_results[i] = self->priv->last_results[i+1];
  self->priv->last_results[9] = res->result;

  gfloat average = 0.0;
  for (gint i = 0; i < 10; i++)
    average += self->priv->last_results[i];
  average /= 10.0;
  res->result = average;

  for (gint i = 0; i < 9; i++)
    self->priv->last_locations[i] = self->priv->last_locations[i+1];
  self->priv->last_locations[9] = res->location;

  average = 0.0;
  for (gint i = 0; i < 10; i++)
    average += self->priv->last_locations[i];
  average /= 10.0;
  res->location = average;
}

WhsResult *
whs_identifier_process (WhsIdentifier *self, const gfloat *in,
    WhsIdentifierMode mode)
{
  g_return_val_if_fail (WHS_IS_IDENTIFIER (self), NULL);
  g_return_val_if_fail (in != NULL, NULL);
  g_return_val_if_fail ((mode & WHS_IDENTIFIER_MODE_CLASSIFY)
      && (mode & WHS_IDENTIFIER_MODE_LOCALIZE), NULL);

  //TODO: use overlapping windows, overlap by 1/2 or 1/4 on each side maybe

  WhsResult *res = g_new0 (WhsResult, 1);

  // Fast path if the current frame doesn't contain anything useful
  if (!whs_identifier_preprocess (self, in))
    return res;

  //FIXME: maybe use the channel with largest RMS after preprocessing

  WhsFeatureVector vec = { .mfcc = {0.0,}, };
  
  whs_extractor_process (self->priv->extractor, self->priv->mono, &vec);
  if (mode & WHS_IDENTIFIER_MODE_CLASSIFY) {
    whs_classifier_process (self->priv->classifier, &vec, res);
  }

  if (mode & WHS_IDENTIFIER_MODE_LOCALIZE) {
    whs_localizer_process (self->priv->localizer, (const gfloat **) self->priv->input, &vec, res);
  }

  whs_identifier_postprocess (self, res);

  return res;
}

