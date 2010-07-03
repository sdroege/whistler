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

#include "whsextractor.h"
#include "fft.h"

#include <math.h>
#include <string.h>

struct _WhsExtractorPrivate
{
  /* FFT data */
  struct {
    gdouble *freqdata;
    gint *ip;
    gdouble *w;
  } fft;
  struct {
    gdouble *freqdata;
    gint *ip;
    gdouble *w;
  } dct;
  gdouble *cos;
};

static inline gdouble
mel (double f)
{
  return 1127.014048 * log (1 + f / 700.0);
} G_GNUC_CONST

static inline gdouble
mel_inv (double f)
{
  return 700.0 * (exp (f / 1127.0140489) - 1.0);
} G_GNUC_CONST

static inline gdouble
triangle (gdouble start, gdouble stop, gdouble pos)
{
  g_assert (stop >= start);

  if (start == stop)
    return 1.0;

  if (pos <= start || pos >= stop)
    return 0.0;

  if (start == 0.0 && pos <= (stop-start)/2.0)
    return 1.0;

  /* Not really a triangular window but bartlett window (zero at the end points) */

  gdouble len = stop - start;
  gdouble ret = (2.0 / (len - 1.0)) * ((len-1.0)/2.0 - fabs ((pos - start) - (len - 1.0)/2.0));

  return CLAMP (ret, 0.0, 1.0);
} G_GNUC_CONST

#define WHS_EXTRACTOR_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), WHS_TYPE_EXTRACTOR, WhsExtractorPrivate))

static void whs_extractor_init (WhsExtractor * self);
static void whs_extractor_class_init (WhsExtractorClass * klass);
static void whs_extractor_finalize (WhsObject *object);

G_DEFINE_TYPE (WhsExtractor, whs_extractor, WHS_TYPE_OBJECT);

static WhsObjectClass *parent_class = NULL;

static void
whs_extractor_class_init (WhsExtractorClass * klass)
{
  WhsObjectClass *o_klass = (WhsObjectClass *) klass;

  parent_class = WHS_OBJECT_CLASS (g_type_class_peek_parent (klass));
  
  g_type_class_add_private (klass, sizeof (WhsExtractorPrivate));

  o_klass->finalize = whs_extractor_finalize;
}

static void
whs_extractor_init (WhsExtractor * self)
{
  self->priv = WHS_EXTRACTOR_GET_PRIVATE (self);
}

static void
whs_extractor_finalize (WhsObject *object)
{
  WhsExtractor *self = WHS_EXTRACTOR_CAST (object);

  g_free (self->priv->fft.freqdata);
  self->priv->fft.freqdata = NULL;
  g_free (self->priv->fft.ip);
  self->priv->fft.ip = NULL;
  g_free (self->priv->fft.w);
  self->priv->fft.w = NULL;

  g_free (self->priv->dct.freqdata);
  self->priv->dct.freqdata = NULL;
  g_free (self->priv->dct.ip);
  self->priv->dct.ip = NULL;
  g_free (self->priv->dct.w);
  self->priv->dct.w = NULL;

  g_free (self->priv->cos);

  WHS_OBJECT_CLASS (parent_class)->finalize (object);
}

WhsExtractor *
whs_extractor_new (guint sample_rate, guint frame_length, guint min_freq, guint max_freq)
{
  g_return_val_if_fail (frame_length > 0, NULL);
  g_return_val_if_fail (sample_rate > 0, NULL);

  WhsExtractor *self = WHS_EXTRACTOR_CAST (g_type_create_instance (WHS_TYPE_EXTRACTOR));
  self->sample_rate = sample_rate;
  self->frame_length = frame_length;
  self->min_freq = min_freq;
  self->max_freq = max_freq;

  self->priv->fft.freqdata = g_new0 (gdouble, frame_length);
  self->priv->fft.ip = g_new0 (gint, 3 + sqrt (frame_length / 2));
  self->priv->fft.w = g_new0 (gdouble, frame_length / 2 + 1);

  self->priv->dct.freqdata = NULL;
  self->priv->dct.ip = g_new0 (gint, 3 + sqrt (32 / 2));
  self->priv->dct.w = g_new0 (gdouble, 1 + (32 * 5 + 3) / 4); // dct on 32 bins

  self->priv->cos = g_new (gdouble, frame_length);
  
  for (gint i = 0; i < self->frame_length; i++)
    self->priv->cos[i] = 0.53836 - 0.46164 * cos (2.0 * M_PI * i / (self->frame_length-1));

  return self;
}

void
whs_extractor_process (WhsExtractor *self, const gfloat *in, WhsFeatureVector *ret)
{
  gdouble tmp;

  // Calculate MFCC
  // http://de.wikipedia.org/wiki/MFCC

  // Copy to our temporary array and apply hamming window
  
  gdouble *freqdata = self->priv->fft.freqdata;

  for (gint i = 0; i < self->frame_length; i++) {
    freqdata[i] = in[i] * self->priv->cos[i];
  }

  // Take FFT
  rdft (self->frame_length, 1, freqdata, self->priv->fft.ip, self->priv->fft.w);

  // Store magnitude spectrum in freqdata[0...n/2]
  for (guint i = 0; i < self->frame_length; i += 2) {
    gdouble cur;
    if (i == 0) {
      freqdata[0] = freqdata[0] * freqdata[0];
      freqdata[1] = freqdata[1] * freqdata[1];
    } else {
      cur = freqdata[i / 2 + 1] = freqdata[i] * freqdata[i] + freqdata[i+1] * freqdata[i+1];
    }
  }

  // Move freqdata[1] to the end, it's for the nyquist frequency!
  tmp = freqdata[1];
  g_memmove (&freqdata[1], &freqdata[2], sizeof (gdouble) * (self->frame_length / 2 - 1));
  freqdata[self->frame_length / 2] = tmp;

  // Take logarithms
  for (gint i = 0; i < self->frame_length / 2 + 1; i++) {
    if (freqdata[i] != 0.0)
      freqdata[i] = CLAMP (log10 (sqrt (freqdata[i]/(self->frame_length*self->frame_length))), -500.0, G_MAXDOUBLE);
    else
      freqdata[i] = -500.0;
  }

  // Convert to mel spectrum
  gdouble bins[32];

#if 0
  {
    gint bin = 0;
    gint step = mel (self->sample_rate / 2) / 32; // 32 bins

    for (bin = 0; bin < 32; bin++) {
      gint i;
      bins[bin] = 0.0;
      
      // Fill bin from 'start' to 'stop' with triangular weighting
      gdouble start = (bin > 0) ? bin * step - 0.5 * step : 0;
      gdouble stop = (bin < 31) ? (bin + 1) * step + 0.5 * step : (bin + 1) * step;

      for (i = 0; i < self->frame_length / 2 + 1; i++) {
        gdouble f = (((gfloat) i) / ((gfloat) (self->frame_length))) * self->sample_rate;
        bins[bin] += triangle (start, stop, mel (f)) * freqdata[i];
      }
    }
  }
#else
  {
#if 1
    gint i, j = 0, bin = 0;
    gint start_m = (self->min_freq > 0) ? mel (CLAMP (self->min_freq - (self->sample_rate / self->frame_length), 0, self->sample_rate / 2)) : 0;
    gint stop_m = (self->max_freq > 0) ? mel (CLAMP (self->max_freq + (self->sample_rate / self->frame_length), 0, self->sample_rate / 2)) : mel (self->sample_rate / 2);
    gint step = (stop_m - start_m) / 32; // 32 bins

#else
    gint i, j = 0, bin = 0;
    gint start_m = 0;
    gint step =  mel (self->sample_rate / 2) / 32; // 32 bins

#endif

    for (bin = i = 0; bin < 32; bin++, i += j) {
      bins[bin] = 0.0;
      
      // Fill bin
      for (j = 0; (i + j) <= self->frame_length / 2 && mel (((i + j) * (self->sample_rate / 2)) / (self->frame_length / 2)) <= start_m + step * (bin + 1); j++) {
        bins[bin] += freqdata[i+j];
      }

      // Normalize bin
      if (j != 0)
        bins[bin] /= j;
    }
  }
#endif


  // Calculate DCT

  ddct (32, -1, bins, self->priv->dct.ip, self->priv->dct.w);

  for (gint i = 0; i < 32; i++)
    ret->mfcc[i] = bins[i];
}
