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

#ifndef __WHS_TRAINING_DATA_H__
#define __WHS_TRAINING_DATA_H__

#include <glib.h>
#include "whsobject.h"

G_BEGIN_DECLS

typedef struct _WhsTrainingData WhsTrainingData;

struct _WhsTrainingData
{
  gint result;
  guint64 start, stop;
};

void whs_training_data_free (GList *training_data);
GList *whs_training_data_load_from_file (const gchar *filename) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean whs_training_data_save (const GList *training_data, const gchar *filename);

WhsTrainingData *whs_training_data_new_element (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void whs_training_data_free_element (WhsTrainingData *element);

G_END_DECLS

#endif /* __WHS_TRAINING_DATA_H__ */

