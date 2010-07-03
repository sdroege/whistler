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

#ifndef __WHS_PATTERN_H__
#define __WHS_PATTERN_H__

#include <glib.h>
#include "whsobject.h"

G_BEGIN_DECLS

#define WHS_TYPE_PATTERN          (whs_pattern_get_type())
#define WHS_IS_PATTERN(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WHS_TYPE_PATTERN))
#define WHS_IS_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WHS_TYPE_PATTERN))
#define WHS_PATTERN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WHS_TYPE_PATTERN, WhsPatternClass))
#define WHS_PATTERN(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), WHS_TYPE_PATTERN, WhsPattern))
#define WHS_PATTERN_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), WHS_TYPE_PATTERN, WhsPatternClass))
#define WHS_PATTERN_CAST(obj)     ((WhsPattern*)(obj))

typedef struct _WhsPattern WhsPattern;
typedef struct _WhsPatternClass WhsPatternClass;
typedef struct _WhsPatternPrivate WhsPatternPrivate;

struct _WhsPattern
{
  WhsObject parent;

  WhsPatternPrivate *priv;
};

struct _WhsPatternClass
{
  WhsObjectClass parent;
};

GType whs_pattern_get_type (void);

WhsPattern * whs_pattern_load (const gchar *filename) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean whs_pattern_save (WhsPattern *self, const gchar *filename);

const gchar * whs_pattern_get_classifier_name (WhsPattern *self);

G_END_DECLS

#endif /* __WHS_PATTERN_H__ */

