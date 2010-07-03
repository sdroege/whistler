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

/* Inspired by GStreamer's GstMiniObject */

#ifndef __WHS_OBJECT_H__
#define __WHS_OBJECT_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define WHS_TYPE_OBJECT          (whs_object_get_type())
#define WHS_IS_OBJECT(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WHS_TYPE_OBJECT))
#define WHS_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WHS_TYPE_OBJECT))
#define WHS_OBJECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WHS_TYPE_OBJECT, WhsObjectClass))
#define WHS_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), WHS_TYPE_OBJECT, WhsObject))
#define WHS_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), WHS_TYPE_OBJECT, WhsObjectClass))
#define WHS_OBJECT_CAST(obj)     ((WhsObject*)(obj))

typedef struct _WhsObject WhsObject;
typedef struct _WhsObjectClass WhsObjectClass;

typedef void (*WhsObjectFinalizeFunction) (WhsObject *obj);

struct _WhsObject {
  GTypeInstance instance;

  gint refcount;
};

struct _WhsObjectClass {
  GTypeClass type_class;

  WhsObjectFinalizeFunction finalize;
};

GType whs_object_get_type (void);

WhsObject* whs_object_new (GType type) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

WhsObject* whs_object_ref (gpointer object);
void whs_object_unref (gpointer object);
void whs_object_replace (WhsObject **olddata, WhsObject *newdata);


#define WHS_TYPE_PARAM_OBJECT (whs_param_spec_object_get_type())
#define WHS_IS_PARAM_SPEC_OBJECT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), WHS_TYPE_PARAM_OBJECT))
#define WHS_PARAM_SPEC_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), WHS_TYPE_PARAM_OBJECT, WhsParamSpecObject))

typedef struct _WhsParamSpecObject WhsParamSpecObject;
struct _WhsParamSpecObject
{
  GParamSpec parent_instance;
};

GType whs_param_spec_object_get_type (void);
GParamSpec* whs_param_spec_object (const char *name, const char *nick,
    const char *blurb, GType object_type,
    GParamFlags flags) G_GNUC_WARN_UNUSED_RESULT;


#define WHS_VALUE_HOLDS_OBJECT(value)  (G_VALUE_HOLDS(value, WHS_TYPE_OBJECT))

void whs_value_set_object (GValue *value, WhsObject *object);
void whs_value_take_object (GValue *value, WhsObject *object);
WhsObject *whs_value_get_object (const GValue *value) G_GNUC_WARN_UNUSED_RESULT;
WhsObject *whs_value_dup_object (const GValue * value) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* __WHS_OBJECT_H__ */

