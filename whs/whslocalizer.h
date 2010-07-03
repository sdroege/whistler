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

#ifndef __WHS_LOCALIZER_H__
#define __WHS_LOCALIZER_H__

#include <glib.h>
#include "whs.h"
#include "whsobject.h"

#include "whsprivate.h"

G_BEGIN_DECLS

#define WHS_TYPE_LOCALIZER          (whs_localizer_get_type())
#define WHS_IS_LOCALIZER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WHS_TYPE_LOCALIZER))
#define WHS_IS_LOCALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WHS_TYPE_LOCALIZER))
#define WHS_LOCALIZER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WHS_TYPE_LOCALIZER, WhsLocalizerClass))
#define WHS_LOCALIZER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), WHS_TYPE_LOCALIZER, WhsLocalizer))
#define WHS_LOCALIZER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), WHS_TYPE_LOCALIZER, WhsLocalizerClass))
#define WHS_LOCALIZER_CAST(obj)     ((WhsLocalizer*)(obj))

typedef struct _WhsLocalizer WhsLocalizer;
typedef struct _WhsLocalizerClass WhsLocalizerClass;
typedef struct _WhsLocalizerPrivate WhsLocalizerPrivate;

struct _WhsLocalizer
{
  WhsObject parent;

  guint sample_rate;
  guint frame_length;
  guint distance;

  WhsLocalizerPrivate *priv;
};

struct _WhsLocalizerClass
{
  WhsObjectClass parent;
};

G_GNUC_INTERNAL GType whs_localizer_get_type (void);

G_GNUC_INTERNAL WhsLocalizer *whs_localizer_new (guint sample_rate, guint frame_length, guint nchannels, guint distance) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
G_GNUC_INTERNAL void whs_localizer_process (WhsLocalizer *self, const gfloat **in, const WhsFeatureVector *vec, WhsResult *res);

G_END_DECLS

#endif /* __WHS_LOCALIZER_H__ */
