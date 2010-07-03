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

#ifndef __WHS_PRIVATE_H__
#define __WHS_PRIVATE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _WhsFeatureVector WhsFeatureVector;
typedef struct _WhsResultValue WhsResultValue;

struct _WhsFeatureVector
{
  gfloat mfcc[32];
};

struct _WhsResultValue
{
  gint32 result;
  WhsFeatureVector vec;
};

G_END_DECLS

#endif /* __WHS_PRIVATE_H__ */
