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

#include <glib.h>
#include <whs/whs.h>
#include <whs/whslearner.h>
#include <whs/whspattern.h>

#define CLASSIFIER "WhsNNClassifier_32_32_32_1"

int
main(int argc, char **argv)
{
  if (argc != 4 && argc != 5) {
    g_print ("usage: learn [CLASSIFIER] RATE IN-FILE OUT-FILE\n");
    return -1;
  }

  whs_init ();

  WhsPattern *load_pattern = NULL;
  const gchar *classifier, *rate_s, *in_file, *out_file;

  rate_s = argv[argc-3];
  in_file = argv[argc-2];
  out_file = argv[argc-1];

  if (g_file_test (out_file, G_FILE_TEST_EXISTS))
    load_pattern = whs_pattern_load (out_file);


  if (argc == 5) {
    classifier = argv[1];
  } else if (load_pattern) {
    classifier = whs_pattern_get_classifier_name (load_pattern);
  } else {
    classifier = CLASSIFIER;
  }

  WhsLearner *learner = whs_learner_new_from_state (classifier, 0, 512, in_file, load_pattern);

  if (load_pattern)
    whs_object_unref (load_pattern);

  if (!learner) {
    g_warning ("Could not create learner");
    return -2;
  }

  gdouble rate = -1.0;
  rate = g_strtod (rate_s, NULL);
  if (rate < 0.0 || rate > 1.0) {
    g_warning ("Wrong rate");
    whs_object_unref (learner);
    return -4;
  }

  WhsPattern *pattern = whs_learner_generate_pattern (learner, rate);

  if (!pattern) {
    g_warning ("Could not generate pattern");
    whs_object_unref (learner);
    return -3;
  }

  whs_pattern_save (pattern, out_file);
  whs_object_unref (pattern);
  whs_object_unref (learner);

  return 0;
}
