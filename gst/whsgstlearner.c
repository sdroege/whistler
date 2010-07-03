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
#include <string.h>
#include <gst/gst.h>
#include <math.h>
#include <gst/audio/audio.h>
#include "whsgstlearner.h"

#include <whs/whstrainingdata.h>

GST_DEBUG_CATEGORY_STATIC (whs_gst_learner_debug);
#define GST_CAT_DEFAULT whs_gst_learner_debug


#define PAD_CAPS \
  "audio/x-raw-float, " \
  "width = (int) 32, " \
  "endianness = (int) BYTE_ORDER, " \
  "rate = (int) [ 1, MAX ], " \
  "channels = (int) 1"

enum
{
  PROP_0,
  PROP_FRAME_SIZE,
  PROP_TRAINING,
  PROP_STATUS,
  PROP_PATTERN,
  PROP_RATE,
  PROP_MIN_FREQ,
  PROP_MAX_FREQ,
  PROP_CLASSIFIER
};

GST_BOILERPLATE (WhsGstLearner, whs_gst_learner, GstAudioFilter,
    GST_TYPE_AUDIO_FILTER);

static void whs_gst_learner_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void whs_gst_learner_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void whs_gst_learner_finalize (GObject * obj);

static gboolean whs_gst_learner_stop (GstBaseTransform * trans);
static gboolean whs_gst_learner_event (GstBaseTransform * trans, GstEvent *event);
static GstFlowReturn whs_gst_learner_transform_ip (GstBaseTransform * trans,
    GstBuffer * in);

static gboolean whs_gst_learner_setup (GstAudioFilter * filter, GstRingBufferSpec * format);

static void
whs_gst_learner_base_init (gpointer g_class)
{
  GstElementClass *element_class = (GstElementClass *) g_class;
  GstAudioFilterClass *audio_filter_class = (GstAudioFilterClass *) g_class;
  GstCaps *caps;

  gst_element_class_set_details_simple (element_class, "Whistler Learner", "Filter/Analyzer/Audio",
      "Learns the sound of a whistle", "Sebastian Dröge <slomo@uni-paderborn.de");

  caps = gst_caps_from_string (PAD_CAPS);
  gst_audio_filter_class_add_pad_templates (audio_filter_class, caps);
  gst_caps_unref (caps);
}

static void
whs_gst_learner_class_init (WhsGstLearnerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstAudioFilterClass *audio_filter_class = GST_AUDIO_FILTER_CLASS (klass);

  gobject_class->set_property = whs_gst_learner_set_property;
  gobject_class->get_property = whs_gst_learner_get_property;
  gobject_class->finalize = whs_gst_learner_finalize;

  g_object_class_install_property (gobject_class, PROP_FRAME_SIZE,
      g_param_spec_uint ("frame-size", "Frame size",
          "Size of every frame to analyze",
          128, 4096, 512, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TRAINING,
      g_param_spec_string ("training", "Training file",
          "Training file", NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STATUS,
      g_param_spec_string ("status", "Status or pattern file",
          "Status file", NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PATTERN,
      g_param_spec_string ("pattern", "Pattern file",
          "Pattern file", NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RATE,
      g_param_spec_float ("rate", "Detection rate",
          "Desired detection rate",
	  0.0, 1.0, 0.95, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MIN_FREQ,
      g_param_spec_uint ("min-freq", "Minimum frequency",
          "Minimum frequency",
	  0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MAX_FREQ,
      g_param_spec_uint ("max-freq", "Maximum frequency",
          "Maximum frequency",
	  0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CLASSIFIER,
      g_param_spec_string ("classifier", "Classifier",
          "Classifier to use", NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (whs_gst_learner_debug, "whs_gst_learner", 0, "Whistler learner");

  trans_class->stop = GST_DEBUG_FUNCPTR (whs_gst_learner_stop);
  trans_class->event = GST_DEBUG_FUNCPTR (whs_gst_learner_event);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (whs_gst_learner_transform_ip);
  trans_class->passthrough_on_same_caps = TRUE;

  audio_filter_class->setup = GST_DEBUG_FUNCPTR (whs_gst_learner_setup);
}

static void
whs_gst_learner_reset (WhsGstLearner *learner)
{
  gst_adapter_clear (learner->adapter);

  if (learner->learner) {
    whs_object_unref (learner->learner);
    learner->learner = NULL;
  }

  whs_training_data_free (learner->results);
  learner->results = NULL;

  learner->current_sample = 0;
}

static void
whs_gst_learner_init (WhsGstLearner *learner, WhsGstLearnerClass * g_class)
{
  learner->adapter = gst_adapter_new ();
  learner->frame_size = 512;
  learner->rate = 0.95;
}

static void
whs_gst_learner_finalize (GObject * obj)
{
  WhsGstLearner *learner = WHS_GST_LEARNER (obj);

  if (learner->adapter) {
    g_object_unref (G_OBJECT (learner->adapter));
    learner->adapter = NULL;
  }

  if (learner->learner) {
    whs_object_unref (learner->learner);
    learner->learner = NULL;
  }

  g_free (learner->training_file);
  learner->training_file = NULL;
  g_free (learner->status_file);
  learner->status_file = NULL;
  g_free (learner->pattern_file);
  learner->pattern_file = NULL;
  g_free (learner->classifier);
  learner->classifier = NULL;

  whs_training_data_free (learner->results);
  learner->results = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
whs_gst_learner_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  WhsGstLearner *learner = WHS_GST_LEARNER (object);

  switch (prop_id) {
    case PROP_FRAME_SIZE:
      learner->frame_size = g_value_get_uint (value);
      break;
    case PROP_TRAINING:
      g_free (learner->training_file);
      learner->training_file = g_value_dup_string (value);
      break;
    case PROP_STATUS:
      g_free (learner->status_file);
      learner->status_file = g_value_dup_string (value);
      break;
    case PROP_PATTERN:
      g_free (learner->pattern_file);
      learner->pattern_file = g_value_dup_string (value);
      break;
    case PROP_RATE:
      learner->rate = g_value_get_float (value);
      break;
    case PROP_MIN_FREQ:
      learner->min_freq = g_value_get_uint (value);
      break;
    case PROP_MAX_FREQ:
      learner->max_freq = g_value_get_uint (value);
      break;
    case PROP_CLASSIFIER:
      g_free (learner->classifier);
      learner->classifier = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
whs_gst_learner_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  WhsGstLearner *learner = WHS_GST_LEARNER (object);

  switch (prop_id) {
    case PROP_FRAME_SIZE:
      g_value_set_uint (value, learner->frame_size);
      break;
    case PROP_TRAINING:
      g_value_set_string (value, learner->training_file);
      break;
    case PROP_STATUS:
      g_value_set_string (value, learner->status_file);
      break;
    case PROP_PATTERN:
      g_value_set_string (value, learner->pattern_file);
      break;
    case PROP_RATE:
      g_value_set_float (value, learner->rate);
      break;
    case PROP_MIN_FREQ:
      g_value_set_uint (value, learner->min_freq);
      break;
    case PROP_MAX_FREQ:
      g_value_set_uint (value, learner->max_freq);
      break;
    case PROP_CLASSIFIER:
      g_value_set_string (value, learner->classifier);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
whs_gst_learner_setup (GstAudioFilter * filter, GstRingBufferSpec * format)
{
  WhsGstLearner *learner = WHS_GST_LEARNER (filter);

  whs_gst_learner_reset (learner);

  return TRUE;
}

static gboolean
whs_gst_learner_stop (GstBaseTransform * trans)
{
  WhsGstLearner *learner = WHS_GST_LEARNER (trans);

  whs_gst_learner_reset (learner);

  return TRUE;
}

static gboolean
whs_gst_learner_event (GstBaseTransform * trans, GstEvent *event)
{
  WhsGstLearner *learner = WHS_GST_LEARNER (trans);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      break;
    case GST_EVENT_EOS:
      if (learner->pattern_file) {
        WhsPattern *pattern;
	
        pattern = whs_learner_generate_pattern (learner->learner, learner->rate); 
	whs_pattern_save (pattern, learner->pattern_file);
	whs_object_unref (pattern);
      }
      whs_learner_save_state (learner->learner, learner->status_file);
      break;
    case GST_EVENT_NEWSEGMENT:
    {
      gint64 start;
      GstFormat format;
      gdouble rate;

      whs_gst_learner_reset (learner);
      gst_event_parse_new_segment (event, NULL, &rate, &format, &start, NULL, NULL);

      if (format != GST_FORMAT_TIME) {
        GST_DEBUG ("NEWSEGMENT event not in TIME format, creating open ended event in TIME format");
	start = 0;
	gst_event_unref (event);
	event = gst_event_new_new_segment (FALSE, rate, GST_FORMAT_TIME, 0, -1, 0);
      }

      learner->current_sample = GST_CLOCK_TIME_TO_FRAMES (start, GST_AUDIO_FILTER (learner)->format.rate);

      break;
    }
    default:
      break;
  }

  return TRUE;
}

static GstFlowReturn
whs_gst_learner_transform_ip (GstBaseTransform * trans, GstBuffer * buffer)
{
  WhsGstLearner *learner = WHS_GST_LEARNER (trans);
  gint wanted = 4 * learner->frame_size;
  gint rate = GST_AUDIO_FILTER (learner)->format.rate;

  g_return_val_if_fail (learner->training_file != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (learner->status_file != NULL, GST_FLOW_ERROR);

  if (!learner->results)
    learner->results = whs_training_data_load_from_file (learner->training_file);

  if (!learner->learner) {
    WhsPattern *load_pattern = NULL;

    if (learner->pattern_file && g_file_test (learner->pattern_file, G_FILE_TEST_EXISTS)) {
      load_pattern = whs_pattern_load (learner->pattern_file);
      if (load_pattern && learner->classifier && strcmp (learner->classifier, whs_pattern_get_classifier_name (load_pattern)) != 0) {
        GST_ERROR ("Invalid classifier given");
	return GST_FLOW_ERROR;
      } else if (load_pattern && !learner->classifier) {
        learner->classifier = g_strdup (whs_pattern_get_classifier_name (load_pattern));
      }
    }

    if (!learner->classifier)
      learner->classifier = g_strdup ("WhsNNClassifier_32_32_32_1");

    if (g_file_test (learner->status_file, G_FILE_TEST_EXISTS))
      learner->learner = whs_learner_new_from_state (learner->classifier, rate, learner->frame_size, learner->status_file, load_pattern);
    else
      learner->learner = whs_learner_new (learner->classifier, rate, learner->frame_size, learner->min_freq, learner->max_freq, load_pattern);
  }

  gst_adapter_push (learner->adapter, gst_buffer_copy (buffer));

  while (gst_adapter_available (learner->adapter) >= wanted) {
    gfloat *in = (gfloat *) gst_adapter_peek (learner->adapter, wanted);
    GList *l;
    gint result = -1;

    for (l = learner->results; l != NULL; l = l->next) {
      WhsTrainingData *tdata = (WhsTrainingData *) l->data;

      // Only take frames that are completely in one result
      if (tdata->start > learner->current_sample) {
        break;	
      } else if (tdata->start <= learner->current_sample && tdata->stop >= learner->current_sample + learner->frame_size) {
        result = tdata->result;
	break;
      }
    }

    whs_learner_process (learner->learner, result, in);
    learner->current_sample += learner->frame_size;
        
    gst_adapter_flush (learner->adapter, wanted);
  }

  return GST_FLOW_OK;
}

