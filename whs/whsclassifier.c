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

#include "whsclassifier.h"
#include "whsprivate.h"
#include "whsutils.h"
#include "whspatternprivate.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

static void whs_classifier_init (WhsClassifier * self);
static void whs_classifier_class_init (WhsClassifierClass * klass);
static void whs_classifier_finalize (WhsObject *object);

G_DEFINE_ABSTRACT_TYPE (WhsClassifier, whs_classifier, WHS_TYPE_OBJECT);

static WhsObjectClass *parent_class = NULL;

static void
whs_classifier_class_init (WhsClassifierClass * klass)
{
  WhsObjectClass *o_klass = (WhsObjectClass *) klass;

  parent_class = WHS_OBJECT_CLASS (g_type_class_peek_parent (klass));

  o_klass->finalize = whs_classifier_finalize;
}

static void
whs_classifier_init (WhsClassifier * self)
{
}

static void
whs_classifier_finalize (WhsObject *object)
{
  WhsClassifier *self = WHS_CLASSIFIER (object);

  if (self->pattern) {
    whs_object_unref (self->pattern);
    self->pattern = NULL;
  }

  WHS_OBJECT_CLASS (parent_class)->finalize (object);
}

WhsClassifier *
whs_classifier_new (const gchar *classifier, WhsPattern *pattern)
{
  g_return_val_if_fail (pattern == NULL || WHS_IS_PATTERN (pattern), NULL);
  g_return_val_if_fail (classifier != NULL && *classifier != '\0', NULL);

  GType type = g_type_from_name (classifier);
  g_return_val_if_fail (type != G_TYPE_INVALID  && g_type_is_a (type, WHS_TYPE_CLASSIFIER), NULL);

  WhsClassifierClass *klass = WHS_CLASSIFIER_CLASS (g_type_class_ref (type));

  WhsClassifier *self = klass->constructor (pattern);
  self->pattern = (pattern) ? WHS_PATTERN_CAST (whs_object_ref (pattern)) : NULL;

  g_type_class_unref (klass);

  return self;
}

void
whs_classifier_process (WhsClassifier *self, const WhsFeatureVector *vec, WhsResult *res)
{
  g_return_if_fail (WHS_IS_CLASSIFIER (self));
  g_return_if_fail (vec != NULL);
  g_return_if_fail (res != NULL);

  WHS_CLASSIFIER_GET_CLASS (self)->process (self, vec, res); 
}

WhsPattern *
whs_classifier_learn (WhsClassifier *self, const GList *values, gint count, gfloat rate)
{
  g_return_val_if_fail (WHS_IS_CLASSIFIER (self), NULL);
  g_return_val_if_fail (values != NULL && count > 0, NULL);
  g_return_val_if_fail (rate >= 0.0 && rate <= 1.0, NULL);

  WhsPattern *ret = WHS_CLASSIFIER_GET_CLASS (self)->learn (self, values, count, rate); 

  return ret;
}

