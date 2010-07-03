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

#ifndef __CLASSIFIER_H__
#define __CLASSIFIER_H__

#include "whsclassifier.h"

#include "classifier/whsnnclassifier32-16-1.h"
#include "classifier/whsnnclassifier32-32-1.h"
#include "classifier/whsnnclassifier32-32-32-1.h"

G_GNUC_INTERNAL void whs_classifier_register (void);

#endif /* __CLASSIFIER_H__ */
