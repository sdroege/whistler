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
#include "whsgstidentifier.h"

GST_DEBUG_CATEGORY_STATIC (whs_gst_identifier_debug);
#define GST_CAT_DEFAULT whs_gst_identifier_debug


#define PAD_CAPS \
  "audio/x-raw-float, " \
  "width = (int) 32, " \
  "endianness = (int) BYTE_ORDER, " \
  "rate = (int) [ 1, MAX ], " \
  "channels = (int) 2"

enum
{
  PROP_0,
  PROP_FRAME_SIZE,
  PROP_DISTANCE,
  PROP_PATTERN
};

GST_BOILERPLATE (WhsGstIdentifier, whs_gst_identifier, GstAudioFilter,
    GST_TYPE_AUDIO_FILTER);

static void whs_gst_identifier_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void whs_gst_identifier_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void whs_gst_identifier_finalize (GObject * obj);

static gboolean whs_gst_identifier_stop (GstBaseTransform * trans);
static gboolean whs_gst_identifier_event (GstBaseTransform * trans, GstEvent *event);
static GstFlowReturn whs_gst_identifier_transform_ip (GstBaseTransform * trans,
    GstBuffer * in);

static gboolean whs_gst_identifier_setup (GstAudioFilter * filter, GstRingBufferSpec * format);

static void
whs_gst_identifier_base_init (gpointer g_class)
{
  GstElementClass *element_class = (GstElementClass *) g_class;
  GstAudioFilterClass *audio_filter_class = (GstAudioFilterClass *) g_class;
  GstCaps *caps;

  gst_element_class_set_details_simple (element_class, "Whistler Identifier", "Filter/Analyzer/Audio",
      "Identifies and localizes whistles", "Sebastian Dröge <slomo@uni-paderborn.de");

  caps = gst_caps_from_string (PAD_CAPS);
  gst_audio_filter_class_add_pad_templates (audio_filter_class, caps);
  gst_caps_unref (caps);
}

static void
whs_gst_identifier_class_init (WhsGstIdentifierClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstAudioFilterClass *audio_filter_class = GST_AUDIO_FILTER_CLASS (klass);

  gobject_class->set_property = whs_gst_identifier_set_property;
  gobject_class->get_property = whs_gst_identifier_get_property;
  gobject_class->finalize = whs_gst_identifier_finalize;

  g_object_class_install_property (gobject_class, PROP_FRAME_SIZE,
      g_param_spec_uint ("frame-size", "Frame size",
          "Size of every frame to analyze",
          128, 4096, 512, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DISTANCE,
      g_param_spec_uint ("distance", "Distance",
          "Distance between the microphones",
          1, G_MAXUINT, 10, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


  g_object_class_install_property (gobject_class, PROP_PATTERN,
      g_param_spec_string ("pattern", "Pattern",
          "Pattern filename",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (whs_gst_identifier_debug, "whs_gst_identifier", 0, "Whistler identifier");

  trans_class->stop = GST_DEBUG_FUNCPTR (whs_gst_identifier_stop);
  trans_class->event = GST_DEBUG_FUNCPTR (whs_gst_identifier_event);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (whs_gst_identifier_transform_ip);
  trans_class->passthrough_on_same_caps = TRUE;

  audio_filter_class->setup = GST_DEBUG_FUNCPTR (whs_gst_identifier_setup);
}

static void
whs_gst_identifier_reset (WhsGstIdentifier *identifier)
{
  gst_adapter_clear (identifier->adapter);

  if (identifier->identifier) {
    whs_object_unref (identifier->identifier);
    identifier->identifier = NULL;
  }
}

static void
whs_gst_identifier_init (WhsGstIdentifier *identifier, WhsGstIdentifierClass * g_class)
{
  identifier->adapter = gst_adapter_new ();
  identifier->frame_size = 512;
  identifier->distance = 10;
  identifier->current_timestamp = 0;
}

static void
whs_gst_identifier_finalize (GObject * obj)
{
  WhsGstIdentifier *identifier = WHS_GST_IDENTIFIER (obj);

  if (identifier->adapter) {
    g_object_unref (G_OBJECT (identifier->adapter));
    identifier->adapter = NULL;
  }

  if (identifier->identifier) {
    whs_object_unref (identifier->identifier);
    identifier->identifier = NULL;
  }

  g_free (identifier->pattern);
  identifier->pattern = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
whs_gst_identifier_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  WhsGstIdentifier *identifier = WHS_GST_IDENTIFIER (object);

  switch (prop_id) {
    case PROP_FRAME_SIZE:
      identifier->frame_size = g_value_get_uint (value);
      whs_gst_identifier_reset (identifier);
      break;
    case PROP_DISTANCE:
      identifier->distance = g_value_get_uint (value);
      whs_gst_identifier_reset (identifier);
      break;
    case PROP_PATTERN:
      identifier->pattern = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
whs_gst_identifier_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  WhsGstIdentifier *identifier = WHS_GST_IDENTIFIER (object);

  switch (prop_id) {
    case PROP_FRAME_SIZE:
      g_value_set_uint (value, identifier->frame_size);
      break;
    case PROP_DISTANCE:
      g_value_set_uint (value, identifier->distance);
      break;
    case PROP_PATTERN:
      g_value_set_string (value, identifier->pattern);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
whs_gst_identifier_setup (GstAudioFilter * filter, GstRingBufferSpec * format)
{
  WhsGstIdentifier *identifier = WHS_GST_IDENTIFIER (filter);

  whs_gst_identifier_reset (identifier);

  return TRUE;
}

static gboolean
whs_gst_identifier_stop (GstBaseTransform * trans)
{
  WhsGstIdentifier *identifier = WHS_GST_IDENTIFIER (trans);

  whs_gst_identifier_reset (identifier);

  return TRUE;
}

static gboolean
whs_gst_identifier_event (GstBaseTransform * trans, GstEvent *event)
{
  WhsGstIdentifier *identifier = WHS_GST_IDENTIFIER (trans);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
    case GST_EVENT_EOS:
      whs_gst_identifier_reset (identifier);
      break;
    case GST_EVENT_NEWSEGMENT:
    {
      gint64 start;
      GstFormat format;
      gdouble rate;

      whs_gst_identifier_reset (identifier);
      gst_event_parse_new_segment (event, NULL, &rate, &format, &start, NULL, NULL);

      if (format != GST_FORMAT_TIME) {
        GST_DEBUG ("NEWSEGMENT event not in TIME format, creating open ended event in TIME format");
	start = 0;
	gst_event_unref (event);
	event = gst_event_new_new_segment (FALSE, rate, GST_FORMAT_TIME, 0, -1, 0);
      }

      identifier->current_timestamp = start;
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static GstMessage *
whs_gst_identifier_message_new (WhsGstIdentifier * identifier, WhsResult *res, GstClockTime timestamp)
{
  GstStructure *s;
  GValue v = { 0, };

  s = gst_structure_new ("whs-identifier", "timestamp", GST_TYPE_CLOCK_TIME,
      timestamp, NULL);

  g_value_init (&v, G_TYPE_FLOAT);
  g_value_set_float (&v, res->location);
  gst_structure_set_value (s, "location", &v);
  g_value_unset (&v);

  g_value_init (&v, G_TYPE_FLOAT);
  g_value_set_float (&v, res->result);
  gst_structure_set_value (s, "result", &v);
  g_value_unset (&v);

  return gst_message_new_element (GST_OBJECT (identifier), s);
}

static GstFlowReturn
whs_gst_identifier_transform_ip (GstBaseTransform * trans, GstBuffer * buffer)
{
  WhsGstIdentifier *identifier = WHS_GST_IDENTIFIER (trans);
  gint wanted = 2 * 4 * identifier->frame_size;
  gint rate = GST_AUDIO_FILTER (identifier)->format.rate;

  if (!identifier->identifier) {
    WhsPattern *pattern;

    pattern = whs_pattern_load (identifier->pattern);
    identifier->identifier = whs_identifier_new (rate, identifier->frame_size, 2, identifier->distance, pattern);
    whs_object_unref (pattern);
 }

 gst_adapter_push (identifier->adapter, gst_buffer_copy (buffer));

  while (gst_adapter_available (identifier->adapter) >= wanted) {
    gfloat *in = (gfloat *) gst_adapter_peek (identifier->adapter, wanted);
    GstMessage *m;
    WhsResult *res;

    res = whs_identifier_process (identifier->identifier, in,
        WHS_IDENTIFIER_MODE_CLASSIFY | WHS_IDENTIFIER_MODE_LOCALIZE);
   
    m = whs_gst_identifier_message_new (identifier, res, identifier->current_timestamp);
    gst_element_post_message (GST_ELEMENT (identifier), m);
    g_free (res);
    
    gst_adapter_flush (identifier->adapter, wanted);
    identifier->current_timestamp += gst_util_uint64_scale_int (identifier->frame_size, GST_SECOND, rate);
  }

  return GST_FLOW_OK;
}

