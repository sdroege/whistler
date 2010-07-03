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

#ifndef __WHS_CLASSIFIER_H__
#define __WHS_CLASSIFIER_H__

#include <glib.h>
#include "whs.h"
#include "whsobject.h"
#include "whspattern.h"

#include "whsprivate.h"

G_BEGIN_DECLS

#define WHS_TYPE_CLASSIFIER          (whs_classifier_get_type())
#define WHS_IS_CLASSIFIER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WHS_TYPE_CLASSIFIER))
#define WHS_IS_CLASSIFIER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WHS_TYPE_CLASSIFIER))
#define WHS_CLASSIFIER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WHS_TYPE_CLASSIFIER, WhsClassifierClass))
#define WHS_CLASSIFIER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), WHS_TYPE_CLASSIFIER, WhsClassifier))
#define WHS_CLASSIFIER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), WHS_TYPE_CLASSIFIER, WhsClassifierClass))
#define WHS_CLASSIFIER_CAST(obj)     ((WhsClassifier*)(obj))

typedef struct _WhsClassifier WhsClassifier;
typedef struct _WhsClassifierClass WhsClassifierClass;
typedef struct _WhsClassifierPrivate WhsClassifierPrivate;

struct _WhsClassifier
{
  WhsObject parent;

  WhsPattern *pattern;
};

struct _WhsClassifierClass
{
  WhsObjectClass parent;

  WhsClassifier * (*constructor) (WhsPattern *pattern);
  void (*process) (WhsClassifier *self, const WhsFeatureVector *vec, WhsResult *res);
  WhsPattern * (*learn) (WhsClassifier *self, const GList *values, gint count, gfloat rate);
};

G_GNUC_INTERNAL GType whs_classifier_get_type (void);

G_GNUC_INTERNAL WhsClassifier *whs_classifier_new (const gchar *classifier, WhsPattern *pattern) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
G_GNUC_INTERNAL void whs_classifier_process (WhsClassifier *self, const WhsFeatureVector *vec, WhsResult *res);

G_GNUC_INTERNAL WhsPattern *whs_classifier_learn (WhsClassifier *self, const GList *values, gint count, gfloat rate) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* __WHS_CLASSIFIER_H__ */
