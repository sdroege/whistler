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

#ifndef __WHS_PATTERN_PRIVATE_H__
#define __WHS_PATTERN_PRIVATE_H__

#include <glib.h>
#include "whsobject.h"
#include "whspattern.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL const guint8 * whs_pattern_get_classifier_data (WhsPattern *self, const gchar *classifier, gsize *size) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
G_GNUC_INTERNAL void whs_pattern_set_classifier_data (WhsPattern *self, const gchar *classifier, guint8 *data, gsize size);

G_GNUC_INTERNAL void whs_pattern_set_frequency_band (WhsPattern *self, guint min_freq, guint max_freq);
G_GNUC_INTERNAL void whs_pattern_get_frequency_band (WhsPattern *self, guint *min_freq, guint *max_freq);

G_GNUC_INTERNAL void whs_pattern_set_sample_rate (WhsPattern *self, guint sample_rate);
G_GNUC_INTERNAL guint whs_pattern_get_sample_rate (WhsPattern *self);

G_END_DECLS

#endif /* __WHS_PATTERN_PRIVATE_H__ */
