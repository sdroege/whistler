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

#ifndef __WHS_NN_CLASSIFIER_32_32_1_H__
#define __WHS_NN_CLASSIFIER_32_32_1_H__

#include <glib.h>
#include "whs.h"
#include "whsobject.h"
#include "whsclassifier.h"

G_BEGIN_DECLS

#define WHS_TYPE_NN_CLASSIFIER_32_32_1          (whs_nn_classifier_32_32_1_get_type())
#define WHS_IS_NN_CLASSIFIER_32_32_1(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WHS_TYPE_NN_CLASSIFIER_32_32_1))
#define WHS_IS_NN_CLASSIFIER_32_32_1_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WHS_TYPE_NN_CLASSIFIER_32_32_1))
#define WHS_NN_CLASSIFIER_32_32_1_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WHS_TYPE_NN_CLASSIFIER_32_32_1, WhsNNClassifier_32_32_1Class))
#define WHS_NN_CLASSIFIER_32_32_1(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), WHS_TYPE_NN_CLASSIFIER_32_32_1, WhsNNClassifier_32_32_1))
#define WHS_NN_CLASSIFIER_32_32_1_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), WHS_TYPE_NN_CLASSIFIER_32_32_1, WhsNNClassifier_32_32_1Class))
#define WHS_NN_CLASSIFIER_32_32_1_CAST(obj)     ((WhsNNClassifier_32_32_1*)(obj))

typedef struct _WhsNNClassifier_32_32_1 WhsNNClassifier_32_32_1;
typedef struct _WhsNNClassifier_32_32_1Class WhsNNClassifier_32_32_1Class;
typedef struct _WhsNNClassifier_32_32_1Private WhsNNClassifier_32_32_1Private;

struct _WhsNNClassifier_32_32_1
{
  WhsClassifier parent;

  WhsNNClassifier_32_32_1Private *priv;
};

struct _WhsNNClassifier_32_32_1Class
{
  WhsClassifierClass parent;
};

G_GNUC_INTERNAL GType whs_nn_classifier_32_32_1_get_type (void);

G_END_DECLS

#endif /* __WHS_NN_CLASSIFIER_32_32_1_H__ */
