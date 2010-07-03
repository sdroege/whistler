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


#include "whsbandpass.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>


/* Adapted from GStreamer's audiochebband element which was written by me.
 * 
 * Chebyshev type 1 filter design based on
 * "The Scientist and Engineer's Guide to DSP", Chapter 20.
 * http://www.dspguide.com/
 *
 * For type 2 and Chebyshev filters in general read
 * http://en.wikipedia.org/wiki/Chebyshev_filter
 *
 * Transformation from lowpass to bandpass/bandreject:
 * http://docs.dewresearch.com/DspHelp/html/IDH_LinearSystems_LowpassToBandPassZ.htm
 * http://docs.dewresearch.com/DspHelp/html/IDH_LinearSystems_LowpassToBandStopZ.htm
 * 
 */

typedef struct
{
  gdouble *x;
  gint x_pos;
  gdouble *y;
  gint y_pos;
} WhsBandpassChannelCtx;

struct _WhsBandpass {
  guint nchannels;

  gdouble *a;
  gint num_a;
  gdouble *b;
  gint num_b;
  WhsBandpassChannelCtx *channels;
};

static inline gdouble
process (WhsBandpass * bp,
    WhsBandpassChannelCtx * ctx, gdouble x0)
{
  gdouble val = bp->a[0] * x0;
  gint i, j;

  for (i = 1, j = ctx->x_pos; i < bp->num_a; i++) {
    val += bp->a[i] * ctx->x[j];
    j--;
    if (j < 0)
      j = bp->num_a - 1;
  }

  for (i = 1, j = ctx->y_pos; i < bp->num_b; i++) {
    val += bp->b[i] * ctx->y[j];
    j--;
    if (j < 0)
      j = bp->num_b - 1;
  }

  if (ctx->x) {
    ctx->x_pos++;
    if (ctx->x_pos > bp->num_a - 1)
      ctx->x_pos = 0;
    ctx->x[ctx->x_pos] = x0;
  }

  if (ctx->y) {
    ctx->y_pos++;
    if (ctx->y_pos > bp->num_b - 1)
      ctx->y_pos = 0;

    ctx->y[ctx->y_pos] = val;
  }

  return val;
}

static void
generate_biquad_coefficients (WhsBandpass * bp,
    guint poles, gfloat ripple, guint type, gfloat min, gfloat max, guint sample_rate,
    gint p, gdouble * a0, gdouble * a1, gdouble * a2, gdouble * a3,
    gdouble * a4, gdouble * b1, gdouble * b2, gdouble * b3, gdouble * b4)
{
  gint np = poles / 2;

  /* pole location in s-plane */
  gdouble rp, ip;

  /* zero location in s-plane */
  gdouble rz = 0.0, iz = 0.0;

  /* transfer function coefficients for the z-plane */
  gdouble x0, x1, x2, y1, y2;

  /* Calculate pole location for lowpass at frequency 1 */
  {
    gdouble angle = (M_PI / 2.0) * (2.0 * p - 1) / np;

    rp = -sin (angle);
    ip = cos (angle);
  }

  /* If we allow ripple, move the pole from the unit
   * circle to an ellipse and keep cutoff at frequency 1 */
  if (ripple > 0 && type == 1) {
    gdouble es, vx;

    es = sqrt (pow (10.0, ripple / 10.0) - 1.0);

    vx = (1.0 / np) * asinh (1.0 / es);
    rp = rp * sinh (vx);
    ip = ip * cosh (vx);
  } else if (type == 2) {
    gdouble es, vx;

    es = sqrt (pow (10.0, ripple / 10.0) - 1.0);
    vx = (1.0 / np) * asinh (es);
    rp = rp * sinh (vx);
    ip = ip * cosh (vx);
  }

  /* Calculate inverse of the pole location to move from
   * type I to type II */
  if (type == 2) {
    gdouble mag2 = rp * rp + ip * ip;

    rp /= mag2;
    ip /= mag2;
  }

  /* Calculate zero location for frequency 1 on the
   * unit circle for type 2 */
  if (type == 2) {
    gdouble angle = M_PI / (np * 2.0) + ((p - 1) * M_PI) / (np);
    gdouble mag2;

    rz = 0.0;
    iz = cos (angle);
    mag2 = rz * rz + iz * iz;
    rz /= mag2;
    iz /= mag2;
  }

  /* Convert from s-domain to z-domain by
   * using the bilinear Z-transform, i.e.
   * substitute s by (2/t)*((z-1)/(z+1))
   * with t = 2 * tan(0.5).
   */
  if (type == 1) {
    gdouble t, m, d;

    t = 2.0 * tan (0.5);
    m = rp * rp + ip * ip;
    d = 4.0 - 4.0 * rp * t + m * t * t;

    x0 = (t * t) / d;
    x1 = 2.0 * x0;
    x2 = x0;
    y1 = (8.0 - 2.0 * m * t * t) / d;
    y2 = (-4.0 - 4.0 * rp * t - m * t * t) / d;
  } else {
    gdouble t, m, d;

    t = 2.0 * tan (0.5);
    m = rp * rp + ip * ip;
    d = 4.0 - 4.0 * rp * t + m * t * t;

    x0 = (t * t * iz * iz + 4.0) / d;
    x1 = (-8.0 + 2.0 * iz * iz * t * t) / d;
    x2 = x0;
    y1 = (8.0 - 2.0 * m * t * t) / d;
    y2 = (-4.0 - 4.0 * rp * t - m * t * t) / d;
  }

  /* Convert from lowpass at frequency 1 to bandpass
   *
   * Substitute z^(-1) with:
   *
   *   -2            -1
   * -z   + alpha * z   - beta
   * ----------------------------
   *         -2            -1
   * beta * z   - alpha * z   + 1
   *
   * alpha = (2*a*b)/(1+b)
   * beta = (b-1)/(b+1)
   * a = cos((w1 + w0)/2) / cos((w1 - w0)/2)
   * b = tan(1/2) * cot((w1 - w0)/2)
   */
  {
    gdouble a, b, d;
    gdouble alpha, beta;
    gdouble w0 =
        2.0 * M_PI * (min / sample_rate);
    gdouble w1 =
        2.0 * M_PI * (max / sample_rate);

    a = cos ((w1 + w0) / 2.0) / cos ((w1 - w0) / 2.0);
    b = tan (1.0 / 2.0) / tan ((w1 - w0) / 2.0);

    alpha = (2.0 * a * b) / (1.0 + b);
    beta = (b - 1.0) / (b + 1.0);

    d = 1.0 + beta * (y1 - beta * y2);

    *a0 = (x0 + beta * (-x1 + beta * x2)) / d;
    *a1 = (alpha * (-2.0 * x0 + x1 + beta * x1 - 2.0 * beta * x2)) / d;
    *a2 =
        (-x1 - beta * beta * x1 + 2.0 * beta * (x0 + x2) +
        alpha * alpha * (x0 - x1 + x2)) / d;
    *a3 = (alpha * (x1 + beta * (-2.0 * x0 + x1) - 2.0 * x2)) / d;
    *a4 = (beta * (beta * x0 - x1) + x2) / d;
    *b1 = (alpha * (2.0 + y1 + beta * y1 - 2.0 * beta * y2)) / d;
    *b2 =
        (-y1 - beta * beta * y1 - alpha * alpha * (1.0 + y1 - y2) +
        2.0 * beta * (-1.0 + y2)) / d;
    *b3 = (alpha * (y1 + beta * (2.0 + y1) - 2.0 * y2)) / d;
    *b4 = (-beta * beta - beta * y1 + y2) / d;
  }
}

static gdouble
calculate_gain (gdouble * a, gdouble * b, gint num_a, gint num_b, gdouble zr,
    gdouble zi)
{
  gdouble sum_ar, sum_ai;
  gdouble sum_br, sum_bi;
  gdouble gain_r, gain_i;

  gdouble sum_r_old;
  gdouble sum_i_old;

  gint i;

  sum_ar = 0.0;
  sum_ai = 0.0;
  for (i = num_a; i >= 0; i--) {
    sum_r_old = sum_ar;
    sum_i_old = sum_ai;

    sum_ar = (sum_r_old * zr - sum_i_old * zi) + a[i];
    sum_ai = (sum_r_old * zi + sum_i_old * zr) + 0.0;
  }

  sum_br = 0.0;
  sum_bi = 0.0;
  for (i = num_b; i >= 0; i--) {
    sum_r_old = sum_br;
    sum_i_old = sum_bi;

    sum_br = (sum_r_old * zr - sum_i_old * zi) - b[i];
    sum_bi = (sum_r_old * zi + sum_i_old * zr) - 0.0;
  }
  sum_br += 1.0;
  sum_bi += 0.0;

  gain_r =
      (sum_ar * sum_br + sum_ai * sum_bi) / (sum_br * sum_br + sum_bi * sum_bi);
  gain_i =
      (sum_ai * sum_br - sum_ar * sum_bi) / (sum_br * sum_br + sum_bi * sum_bi);

  return (sqrt (gain_r * gain_r + gain_i * gain_i));
}

WhsBandpass *
whs_bandpass_new_full (guint sample_rate, guint type, guint poles, gfloat ripple, guint channels, guint min, guint max)
{
  g_return_val_if_fail (sample_rate > 0, NULL);
  g_return_val_if_fail (channels > 0, NULL);
  g_return_val_if_fail (min < max, NULL);
  g_return_val_if_fail (max <= sample_rate / 2, NULL);

  WhsBandpass *self = g_slice_new0 (WhsBandpass);

  self->nchannels = channels;

  /* Calculate coefficients for the chebyshev filter */
  {
    gdouble *a, *b;
    gint i, p;

    self->num_a = poles + 1;
    self->a = a = g_new0 (gdouble, poles + 5);
    self->num_b = poles + 1;
    self->b = b = g_new0 (gdouble, poles + 5);

    self->channels = g_new0 (WhsBandpassChannelCtx, channels);
    for (i = 0; i < channels; i++) {
      WhsBandpassChannelCtx *ctx = &self->channels[i];

      ctx->x = g_new0 (gdouble, poles + 1);
      ctx->y = g_new0 (gdouble, poles + 1);
    }

    /* Calculate transfer function coefficients */
    a[4] = 1.0;
    b[4] = 1.0;

    for (p = 1; p <= poles / 4; p++) {
      gdouble a0, a1, a2, a3, a4, b1, b2, b3, b4;
      gdouble *ta = g_new0 (gdouble, poles + 5);
      gdouble *tb = g_new0 (gdouble, poles + 5);

      generate_biquad_coefficients (self, poles, ripple, type, min, max, sample_rate, p, &a0, &a1, &a2, &a3, &a4, &b1,
          &b2, &b3, &b4);

      memcpy (ta, a, sizeof (gdouble) * (poles + 5));
      memcpy (tb, b, sizeof (gdouble) * (poles + 5));

      /* add the new coefficients for the new two poles
       * to the cascade by multiplication of the transfer
       * functions */
      for (i = 4; i < poles + 5; i++) {
        a[i] =
            a0 * ta[i] + a1 * ta[i - 1] + a2 * ta[i - 2] + a3 * ta[i - 3] +
            a4 * ta[i - 4];
        b[i] =
            tb[i] - b1 * tb[i - 1] - b2 * tb[i - 2] - b3 * tb[i - 3] -
            b4 * tb[i - 4];
      }
      g_free (ta);
      g_free (tb);
    }

    /* Move coefficients to the beginning of the array
     * and multiply the b coefficients with -1 to move from
     * the transfer function's coefficients to the difference
     * equation's coefficients */
    b[4] = 0.0;
    for (i = 0; i <= poles; i++) {
      a[i] = a[i + 4];
      b[i] = -b[i + 4];
    }

    /* Normalize to unity gain at band center frequency/
     * gain is H(wc), wc = center frequency */
    gdouble w1 =
        2.0 * M_PI * (((gfloat) min) / ((gfloat) sample_rate));
    gdouble w2 =
        2.0 * M_PI * (((gfloat) max) / ((gfloat) sample_rate));
    gdouble w0 = (w2 + w1) / 2.0;
    gdouble zr = cos (w0), zi = sin (w0);
    gdouble gain = calculate_gain (a, b, poles, poles, zr, zi);

    for (i = 0; i <= poles; i++) {
      a[i] /= gain;
    }
  }

  return self;
}

WhsBandpass *
whs_bandpass_new (guint sample_rate, guint channels, guint min, guint max)
{
  return whs_bandpass_new_full (sample_rate, 1, 8, 0.0, channels, min, max);
}

void
whs_bandpass_process (WhsBandpass *self, gfloat **in, guint len)
{
  for (gint i = 0; i < self->nchannels; i++) {
    for (gint j = 0; j < len; j++) {
      in[i][j] = process (self, &self->channels[i], in[i][j]);
    }
  }
}

void
whs_bandpass_free (WhsBandpass *self)
{
  if (self->channels) {
    WhsBandpassChannelCtx *ctx;
    gint i;

    for (i = 0; i < self->nchannels; i++) {
      ctx = &self->channels[i];
      g_free (ctx->x);
      g_free (ctx->y);
    }

    g_free (self->channels);
    self->channels = NULL;
  }

  if (self->a) {
    g_free (self->a);
    self->a = NULL;
  }

  if (self->b) {
    g_free (self->b);
    self->b = NULL;
  }

  g_slice_free (WhsBandpass, self);
}

