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

#ifndef __WHS_LEARNER_H__
#define __WHS_LEARNER_H__

#include <glib.h>
#include "whsobject.h"
#include "whspattern.h"

G_BEGIN_DECLS

#define WHS_TYPE_LEARNER          (whs_learner_get_type())
#define WHS_IS_LEARNER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WHS_TYPE_LEARNER))
#define WHS_IS_LEARNER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WHS_TYPE_LEARNER))
#define WHS_LEARNER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WHS_TYPE_LEARNER, WhsLearnerClass))
#define WHS_LEARNER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), WHS_TYPE_LEARNER, WhsLearner))
#define WHS_LEARNER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), WHS_TYPE_LEARNER, WhsLearnerClass))
#define WHS_LEARNER_CAST(obj)     ((WhsLearner*)(obj))

typedef struct _WhsLearner WhsLearner;
typedef struct _WhsLearnerClass WhsLearnerClass;
typedef struct _WhsLearnerPrivate WhsLearnerPrivate;

struct _WhsLearner
{
  WhsObject parent;

  guint sample_rate;
  guint frame_length;

  WhsLearnerPrivate *priv;
};

struct _WhsLearnerClass
{
  WhsObjectClass parent;
};

GType whs_learner_get_type (void);

WhsLearner * whs_learner_new (const gchar *classifier, guint sample_rate, guint frame_length, guint min_freq, guint max_freq, WhsPattern *pattern) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean whs_learner_process (WhsLearner *self, gint result, const gfloat *in);

void whs_learner_finish_sequence (WhsLearner *self);

WhsPattern * whs_learner_generate_pattern (WhsLearner *self, gfloat rate) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gboolean whs_learner_save_state (WhsLearner *self, const gchar *filename);
WhsLearner * whs_learner_new_from_state (const gchar *classifier, guint sample_rate, guint frame_length, const gchar *filename, WhsPattern *pattern) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* __WHS_LEARNER_H__ */
