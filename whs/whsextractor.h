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

#ifndef __WHS_EXTRACTOR_H__
#define __WHS_EXTRACTOR_H__

#include <glib.h>
#include "whs.h"
#include "whsobject.h"
#include "whsprivate.h"

G_BEGIN_DECLS

#define WHS_TYPE_EXTRACTOR          (whs_extractor_get_type())
#define WHS_IS_EXTRACTOR(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WHS_TYPE_EXTRACTOR))
#define WHS_IS_EXTRACTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WHS_TYPE_EXTRACTOR))
#define WHS_EXTRACTOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WHS_TYPE_EXTRACTOR, WhsExtractorClass))
#define WHS_EXTRACTOR(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), WHS_TYPE_EXTRACTOR, WhsExtractor))
#define WHS_EXTRACTOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), WHS_TYPE_EXTRACTOR, WhsExtractorClass))
#define WHS_EXTRACTOR_CAST(obj)     ((WhsExtractor*)(obj))

typedef struct _WhsExtractor WhsExtractor;
typedef struct _WhsExtractorPrivate WhsExtractorPrivate;
typedef struct _WhsExtractorClass WhsExtractorClass;

struct _WhsExtractor
{
  WhsObject parent;

  guint sample_rate;
  guint frame_length;
  guint min_freq, max_freq;

  WhsExtractorPrivate *priv;
};

struct _WhsExtractorClass
{
  WhsObjectClass parent;
};

G_GNUC_INTERNAL GType whs_extractor_get_type (void);

G_GNUC_INTERNAL WhsExtractor *whs_extractor_new (guint sample_rate, guint frame_length, guint min_freq, guint max_freq) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_GNUC_INTERNAL void whs_extractor_process (WhsExtractor *self, const gfloat *in, WhsFeatureVector *vec);

G_END_DECLS

#endif /* __WHS_EXTRACTOR_H__ */
