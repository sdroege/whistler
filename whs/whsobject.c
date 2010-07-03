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

/* Inspired by GStreamer's WhsObject */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "whsobject.h"

#include <gobject/gvaluecollector.h>

static void whs_object_init (GTypeInstance * instance, gpointer klass);
static void whs_object_base_init (gpointer g_class);
static void whs_object_base_finalize (gpointer g_class);
static void whs_object_class_init (gpointer g_class, gpointer class_data);
static void whs_object_finalize (WhsObject *object);

static void whs_value_object_init (GValue * value);
static void whs_value_object_free (GValue * value);
static void whs_value_object_copy (const GValue *src, GValue *dest);
static gpointer whs_value_object_peek_pointer (const GValue * value);
static gchar *whs_value_object_collect (GValue * value,
    guint n_collect_values, GTypeCValue * collect_values, guint collect_flags);
static gchar *whs_value_object_lcopy (const GValue * value,
    guint n_collect_values, GTypeCValue * collect_values, guint collect_flags);

GType
whs_object_get_type (void)
{
  static volatile GType _whs_object_type = 0;

  if (G_UNLIKELY (_whs_object_type == 0)) {
    GTypeValueTable value_table = {
      whs_value_object_init,
      whs_value_object_free,
      whs_value_object_copy,
      whs_value_object_peek_pointer,
      "p",
      whs_value_object_collect,
      "p",
      whs_value_object_lcopy
    };
    GTypeInfo object_info = {
      sizeof (WhsObjectClass),
      whs_object_base_init,
      whs_object_base_finalize,
      whs_object_class_init,
      NULL,
      NULL,
      sizeof (WhsObject),
      0,
      (GInstanceInitFunc) whs_object_init,
      NULL
    };

    static const GTypeFundamentalInfo object_fundamental_info = {
      (G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE |
          G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE)
    };

    object_info.value_table = &value_table;

    _whs_object_type = g_type_fundamental_next ();
    g_type_register_fundamental (_whs_object_type, "WhsObject",
        &object_info, &object_fundamental_info, G_TYPE_FLAG_ABSTRACT);
  }

  return _whs_object_type;
}

static void
whs_object_base_init (gpointer g_class)
{
}

static void
whs_object_base_finalize (gpointer g_class)
{
}

static void
whs_object_class_init (gpointer g_class, gpointer class_data)
{
  WhsObjectClass *klass = (WhsObjectClass *) g_class;

  klass->finalize = whs_object_finalize;
}

static void
whs_object_init (GTypeInstance * instance, gpointer klass)
{
  WhsObject *object = WHS_OBJECT_CAST (instance);

  object->refcount = 1;
}

static void
whs_object_finalize (WhsObject *object)
{
}

WhsObject *
whs_object_new (GType type)
{
  WhsObject *object;

  g_return_val_if_fail (g_type_is_a (type, WHS_TYPE_OBJECT), NULL);

  object = WHS_OBJECT (g_type_create_instance (type));

  return object;
}

WhsObject *
whs_object_ref (gpointer obj)
{
  g_return_val_if_fail (WHS_IS_OBJECT (obj), NULL);
  
  WhsObject *object = WHS_OBJECT_CAST (obj);
  g_return_val_if_fail (object->refcount > 0, NULL);

  g_atomic_int_inc (&object->refcount);

  return object;
}

void
whs_object_unref (gpointer obj)
{
  g_return_if_fail (WHS_IS_OBJECT (obj));

  WhsObject *object = WHS_OBJECT_CAST (obj);
  g_return_if_fail (object->refcount > 0);

  if (g_atomic_int_dec_and_test (&object->refcount)) {
    WhsObjectClass *klass = WHS_OBJECT_GET_CLASS (object);

    klass->finalize (object);

    g_type_free_instance ((GTypeInstance *) object);
  }
}

void
whs_object_replace (WhsObject **old, WhsObject *new)
{
  WhsObject *old_val;

  g_return_if_fail (old != NULL);
  g_return_if_fail (*old == NULL || WHS_IS_OBJECT (old));
  g_return_if_fail (new == NULL || WHS_IS_OBJECT (new));

  if (new)
    whs_object_ref (new);

  do {
    old_val = g_atomic_pointer_get ((gpointer *) old);
  } while (!g_atomic_pointer_compare_and_exchange ((gpointer *) old,
      old_val, new));

  if (old_val)
    whs_object_unref (old_val);
}

static void
whs_value_object_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
whs_value_object_free (GValue *value)
{
  if (value->data[0].v_pointer)
    whs_object_unref (WHS_OBJECT_CAST (value->data[0].v_pointer));
}

static void
whs_value_object_copy (const GValue *src, GValue *dest)
{
  if (src->data[0].v_pointer)
    dest->data[0].v_pointer = whs_object_ref (WHS_OBJECT_CAST (src->data[0].v_pointer));
  else
    dest->data[0].v_pointer = NULL;
}

static gpointer
whs_value_object_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}

static gchar *
whs_value_object_collect (GValue *value, guint n_collect_values,
    GTypeCValue *collect_values, guint collect_flags)
{
  whs_value_set_object (value, collect_values[0].v_pointer);

  return NULL;
}

static gchar *
whs_value_object_lcopy (const GValue * value, guint n_collect_values,
    GTypeCValue * collect_values, guint collect_flags)
{
  gpointer *object_p = collect_values[0].v_pointer;

  if (!object_p)
    return g_strdup_printf ("value location for '%s' passed as NULL",
        G_VALUE_TYPE_NAME (value));

  if (!value->data[0].v_pointer)
    *object_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *object_p = value->data[0].v_pointer;
  else
    *object_p = whs_object_ref (WHS_OBJECT (value->data[0].v_pointer));

  return NULL;
}

void
whs_value_set_object (GValue *value, WhsObject *object)
{
  gpointer *object_p;

  g_return_if_fail (WHS_VALUE_HOLDS_OBJECT (value));
  g_return_if_fail (object == NULL || WHS_IS_OBJECT (object));

  object_p = &value->data[0].v_pointer;
  whs_object_replace ((WhsObject **) object_p, object);
}

void
whs_value_take_object (GValue * value, WhsObject * object)
{
  gpointer *object_p;

  g_return_if_fail (WHS_VALUE_HOLDS_OBJECT (value));
  g_return_if_fail (object == NULL || WHS_IS_OBJECT (object));

  object_p = &value->data[0].v_pointer;
  
  whs_object_replace ((WhsObject **) object_p, object);

  if (object)
    whs_object_unref (object);
}

WhsObject *
whs_value_get_object (const GValue * value)
{
  g_return_val_if_fail (WHS_VALUE_HOLDS_OBJECT (value), NULL);

  return value->data[0].v_pointer;
}

WhsObject *
whs_value_dup_object (const GValue * value)
{
  g_return_val_if_fail (WHS_VALUE_HOLDS_OBJECT (value), NULL);

  return whs_object_ref (value->data[0].v_pointer);
}

static void
param_object_init (GParamSpec * pspec)
{
}

static void
param_object_set_default (GParamSpec * pspec, GValue * value)
{
  value->data[0].v_pointer = NULL;
}

static gboolean
param_object_validate (GParamSpec * pspec, GValue * value)
{
  WhsParamSpecObject *ospec = WHS_PARAM_SPEC_OBJECT (pspec);
  WhsObject *object = value->data[0].v_pointer;
  gboolean changed = FALSE;

  if (object
      && !g_value_type_compatible (G_OBJECT_TYPE (object),
          G_PARAM_SPEC_VALUE_TYPE (ospec))) {
    whs_object_unref (object);
    value->data[0].v_pointer = NULL;
    changed = TRUE;
  }

  return changed;
}

static gint
param_object_values_cmp (GParamSpec * pspec,
    const GValue * value1, const GValue * value2)
{
  guint8 *p1 = value1->data[0].v_pointer;
  guint8 *p2 = value2->data[0].v_pointer;

  return p1 < p2 ? -1 : p1 > p2;
}

GType
whs_param_spec_object_get_type (void)
{
  static GType type;

  if (G_UNLIKELY (type) == 0) {
    static const GParamSpecTypeInfo pspec_info = {
      sizeof (WhsParamSpecObject),  /* instance_size */
      4,                            /* n_preallocs */
      param_object_init,            /* instance_init */
      G_TYPE_OBJECT,                /* value_type */
      NULL,                         /* finalize */
      param_object_set_default,     /* value_set_default */
      param_object_validate,        /* value_validate */
      param_object_values_cmp,      /* values_cmp */
    };
    type = g_param_type_register_static ("WhsParamSpecObject", &pspec_info);
  }

  return type;
}

GParamSpec *
whs_param_spec_object (const char *name, const char *nick,
    const char *blurb, GType object_type, GParamFlags flags)
{
  WhsParamSpecObject *ospec;

  g_return_val_if_fail (g_type_is_a (object_type, WHS_TYPE_OBJECT), NULL);

  ospec = g_param_spec_internal (WHS_TYPE_PARAM_OBJECT,
      name, nick, blurb, flags);
  G_PARAM_SPEC (ospec)->value_type = object_type;

  return G_PARAM_SPEC (ospec);
}

