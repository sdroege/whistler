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

#ifndef __WHS_BANDPASS_H__
#define __WHS_BANDPASS_H__

#include <glib.h>

typedef struct _WhsBandpass WhsBandpass;

G_GNUC_INTERNAL WhsBandpass *whs_bandpass_new (guint sample_rate, guint channels, guint min, guint max) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_GNUC_INTERNAL WhsBandpass *whs_bandpass_new_full (guint sample_rate, guint type, guint poles, gfloat ripple, guint channels, guint min, guint max) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_GNUC_INTERNAL void whs_bandpass_process (WhsBandpass *self, gfloat **in, guint len);

G_GNUC_INTERNAL void whs_bandpass_free (WhsBandpass *self);

#endif /* __WHS_BANDPASS_H__ */
