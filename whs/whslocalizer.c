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

#include "whslocalizer.h"
#include "fft.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

// Define for using the phase delay that happens most often
//#define WHS_LOCALIZER_PHASE_DELAY
#define V_SOUND (34400.0)

struct _WhsLocalizerPrivate
{
  #ifdef WHS_LOCALIZER_PHASE_DELAY
  gdouble **fft;
  gint *ip;
  gdouble *w;

  gfloat *cos;
  #else
  gfloat **input;
  #endif
};

#define WHS_LOCALIZER_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), WHS_TYPE_LOCALIZER, WhsLocalizerPrivate))

static void whs_localizer_init (WhsLocalizer * self);
static void whs_localizer_class_init (WhsLocalizerClass * klass);
static void whs_localizer_finalize (WhsObject *object);

G_DEFINE_TYPE (WhsLocalizer, whs_localizer, WHS_TYPE_OBJECT);

static WhsObjectClass *parent_class = NULL;

static void
whs_localizer_class_init (WhsLocalizerClass * klass)
{
  WhsObjectClass *o_klass = (WhsObjectClass *) klass;

  parent_class = WHS_OBJECT_CLASS (g_type_class_peek_parent (klass));

  g_type_class_add_private (klass, sizeof (WhsLocalizerPrivate));

  o_klass->finalize = whs_localizer_finalize;
}

static void
whs_localizer_init (WhsLocalizer * self)
{
  self->priv = WHS_LOCALIZER_GET_PRIVATE (self);
}

static void
whs_localizer_finalize (WhsObject *object)
{
  WhsLocalizer *self = WHS_LOCALIZER (object);

#ifdef WHS_LOCALIZER_PHASE_DELAY
  for (gint i = 0; i < 3; i++)
    g_free (self->priv->fft[i]);
  g_free (self->priv->fft);
  g_free (self->priv->ip);
  g_free (self->priv->w);
  g_free (self->priv->cos);
#else
  for (gint i = 0; i < 2; i++)
    g_free (self->priv->input[i]);
  g_free (self->priv->input);
#endif

  WHS_OBJECT_CLASS (parent_class)->finalize (object);
}

WhsLocalizer *
whs_localizer_new (guint sample_rate, guint frame_length, guint nchannels, guint distance)
{
  g_return_val_if_fail (frame_length > 0, NULL);
  g_return_val_if_fail (sample_rate > 0, NULL);

  g_return_val_if_fail (nchannels == 2, NULL);

  WhsLocalizer *self = WHS_LOCALIZER_CAST (g_type_create_instance (WHS_TYPE_LOCALIZER));
  self->sample_rate = sample_rate;
  self->frame_length = frame_length;
  self->distance = distance;

#ifdef WHS_LOCALIZER_PHASE_DELAY
  self->priv->fft = g_new (gdouble *, 3);
  for (gint i = 0; i < 3; i++)
    self->priv->fft[i] = g_new (gdouble, frame_length + 2);

  self->priv->ip = g_new (gint, 3 + sqrt (frame_length / 2));
  self->priv->w = g_new (gdouble, frame_length / 2 + 1);
  
  self->priv->cos = g_new (gfloat, frame_length);

  for (gint i = 0; i < frame_length; i++)
    self->priv->cos[i] = 0.53836 - 0.46164 * cos (2.0 * M_PI * i / (frame_length-1));
#else
  self->priv->input = g_new (gfloat *, 2);
  for (gint i = 0; i < 2; i++)
    self->priv->input[i] = g_new0 (gfloat, frame_length * 2);
#endif

  return self;
}

/* http://www.ise.ncsu.edu/kay/msf/sound.htm */
void
whs_localizer_process (WhsLocalizer *self, const gfloat **in, const WhsFeatureVector *vec, WhsResult *res)
{
  g_return_if_fail (WHS_IS_LOCALIZER (self));
  g_return_if_fail (in != NULL && in[0] != NULL && in[1] != NULL);

  gint frame_length = self->frame_length;

#ifdef WHS_LOCALIZER_PHASE_DELAY
  gint *ip = self->priv->ip;
  gdouble *w = self->priv->w;
  gdouble **fft = self->priv->fft;

  memset (w, 0, frame_length / 2 + 1);
  memset (ip, 0, (int)(3 + sqrt (frame_length / 2)));

  for (gint i = 0; i < self->frame_length; i++) {
    fft[0][i] = in[0][i] * self->priv->cos[i];
    fft[1][i] = in[1][i] * self->priv->cos[i];
  }
  rdft (frame_length, 1, fft[0], ip, w);
  rdft (frame_length, 1, fft[1], ip, w);

  fft[2][0] = fft[0][0] * fft[1][0];
  fft[2][1] = fft[0][1] * fft[1][1];
  for (gint i = 2; i < frame_length; i += 2) {
    fft[2][i] = fft[0][i] * fft[1][i] + fft[0][i+1] * fft[1][i+1];
    fft[2][i+1] = -fft[0][i] * fft[1][i+1] + fft[0][i+1] * fft[1][i];
  }
  gdouble tmp = fft[2][1];
  fft[2][1] = 0;
  fft[2][frame_length] = tmp;
  fft[2][frame_length+1] = 0;

  gint angle[181] = {0, };
  for (gint i = 0; i < frame_length + 2; i += 2) {
    gdouble itd = atan2 (fft[2][i+1], fft[2][i]) / (((gdouble) i / frame_length) * (self->sample_rate / 2.0));

    itd *= V_SOUND;
    itd /= self->distance;
    
    if (itd > 1.0)
      itd = 1.0;
    else if (itd < -1.0)
      itd = -1.0;
    
    angle[(gint) ((asin (itd) * 180.0) / M_PI) + 90]++;
  }
  gint max = 0, maxv = 0;

  for (gint i = 0; i < 180; i++) {
    if (angle[i] > maxv)
      max = (i - 90);
  }
  res->location = (max * M_PI) / 180.0;  
#else
  gint max_range = 1 + (self->distance * self->sample_rate) / V_SOUND;
  gint max = G_MININT;
  gfloat maxv = - G_MAXFLOAT;
  gfloat **input = self->priv->input;
  gdouble xcorr[max_range * 2];

  for (gint i = 0; i < 2; i++) {
    g_memmove (input[i], &input[i][frame_length], frame_length * sizeof (gfloat));
    g_memmove (&input[i][frame_length], in[i], frame_length * sizeof (gfloat));
  }

  // cross correlation
  for (gint i = - max_range; i < max_range; i++) {
    xcorr[i + max_range] = 0.0;

    for (gint j = 0; j < frame_length; j++){
      xcorr[i + max_range] += input[0][j + frame_length / 2] * input[1][j + frame_length / 2 + i];
    }
 
    if (xcorr[i + max_range] > maxv) {
      maxv = xcorr[i + max_range];
      max = i;
    }
  }
 
  gdouble itd = ((gdouble) max) / ((gdouble) self->sample_rate);
  itd *= V_SOUND;
  itd /= self->distance;

  if (itd > 1.0)
    itd = 1.0;
  else if (itd < -1.0)
    itd = -1.0;

  res->location = asin (itd);
#endif
}

