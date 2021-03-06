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

#ifndef __WHS_GST_IDENTIFIER_H__
#define __WHS_GST_IDENTIFIER_H__


#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/gstaudiofilter.h>

#include <whs/whs.h>
#include <whs/whsidentifier.h>

G_BEGIN_DECLS

#define WHS_GST_TYPE_IDENTIFIER \
  (whs_gst_identifier_get_type())
#define WHS_GST_IDENTIFIER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),WHS_GST_TYPE_IDENTIFIER,WhsGstIdentifier))
#define WHS_GST_IDENTIFIER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),WHS_GST_TYPE_IDENTIFIER,WhsGstIdentifierClass))
#define WHS_GST_IDENTIFIER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),WHS_GST_TYPE_IDENTIFIER,WhsGstIdentifierClass))
#define WHS_GST_IS_IDENTIFIER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),WHS_GST_TYPE_IDENTIFIER))
#define WHS_GST_IS_IDENTIFIER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),WHS_GST_TYPE_IDENTIFIER))


typedef struct _WhsGstIdentifier WhsGstIdentifier;
typedef struct _WhsGstIdentifierClass WhsGstIdentifierClass;

/**
 * WhsGstIdentifier:
 *
 * Opaque data structure.
 */
struct _WhsGstIdentifier {
  GstAudioFilter element;

  GstAdapter *adapter;

  guint frame_size;
  guint distance;
  gchar *pattern;

  WhsIdentifier *identifier;
  GstClockTime current_timestamp;
};

struct _WhsGstIdentifierClass {
  GstAudioFilter parent_class;
};

GType whs_gst_identifier_get_type (void);


G_END_DECLS


#endif /* __WHS_GST_IDENTIFIER_H__ */
