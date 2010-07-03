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

#ifndef __WHS_IDENTIFIER_H__
#define __WHS_IDENTIFIER_H__

#include <glib.h>
#include "whs.h"
#include "whsobject.h"
#include "whspattern.h"

G_BEGIN_DECLS

typedef enum {
  WHS_IDENTIFIER_MODE_CLASSIFY = 1 << 0,
  WHS_IDENTIFIER_MODE_LOCALIZE = 1 << 1
} WhsIdentifierMode;

#define WHS_TYPE_IDENTIFIER          (whs_identifier_get_type())
#define WHS_IS_IDENTIFIER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WHS_TYPE_IDENTIFIER))
#define WHS_IS_IDENTIFIER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WHS_TYPE_IDENTIFIER))
#define WHS_IDENTIFIER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WHS_TYPE_IDENTIFIER, WhsIdentifierClass))
#define WHS_IDENTIFIER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), WHS_TYPE_IDENTIFIER, WhsIdentifier))
#define WHS_IDENTIFIER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), WHS_TYPE_IDENTIFIER, WhsIdentifierClass))
#define WHS_IDENTIFIER_CAST(obj)     ((WhsIdentifier*)(obj))

typedef struct _WhsIdentifier WhsIdentifier;
typedef struct _WhsIdentifierClass WhsIdentifierClass;
typedef struct _WhsIdentifierPrivate WhsIdentifierPrivate;

struct _WhsIdentifier
{
  WhsObject parent;

  guint sample_rate;
  guint frame_length;
  guint nchannels;
  WhsIdentifierPrivate *priv;
};

struct _WhsIdentifierClass
{
  WhsObjectClass parent;
};

GType whs_identifier_get_type (void);

WhsIdentifier * whs_identifier_new (guint sample_rate, guint frame_length,
    guint nchannels, guint distance, WhsPattern *pattern) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
WhsResult * whs_identifier_process (WhsIdentifier *self, const gfloat *in,
    WhsIdentifierMode mode) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* __WHS_IDENTIFIER_H__ */
