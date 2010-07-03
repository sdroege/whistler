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

#include <gst/gst.h>
#include <whs/whs.h>

#include "whsgstidentifier.h"
#include "whsgstlearner.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  return whs_init () &&
      gst_element_register (plugin, "whsidentifier", GST_RANK_NONE, WHS_GST_TYPE_IDENTIFIER) &&
      gst_element_register (plugin, "whslearner", GST_RANK_NONE, WHS_GST_TYPE_LEARNER);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "whistler",
    "Whistle detection plugin",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, "http://none.yet");

