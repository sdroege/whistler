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

#include "whsnnclassifier32-16-1.h"
#include "whsclassifier.h"
#include "whsprivate.h"
#include "whsutils.h"
#include "whspatternprivate.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

static inline gdouble
sigmoid (gdouble x)
{
  return 1.0 / (1.0 + exp (-x));
}

typedef struct _WhsNeuralNetwork
{
  struct {
    gfloat x[32];
    gfloat w[33];
    gfloat o;
  } hidden_layer1[16];

  struct {
    gfloat x[16];
    gfloat w[17];
    gfloat o;
  } output_layer[1];
} WhsNeuralNetwork;

struct _WhsNNClassifier_32_16_1Private
{
  WhsNeuralNetwork network;
};

static gfloat
calculate_neural_network (WhsNeuralNetwork *network, const gfloat *in)
{
  for (gint i = 0; i < 16; i++) {
    gdouble out = network->hidden_layer1[i].w[0];
    for (gint j = 0; j < 32; j++) {
      network->hidden_layer1[i].x[j] = in[j];
      out += in[j] * network->hidden_layer1[i].w[j+1];
    }
    network->hidden_layer1[i].o = sigmoid (out);
  }

  gdouble out = network->output_layer[0].w[0];
  for (gint i = 0; i < 16; i++) {
    network->output_layer[0].x[i] = network->hidden_layer1[i].o;
    out += network->hidden_layer1[i].o * network->output_layer[0].w[i+1];
  }
  network->output_layer[0].o = sigmoid (out);
  return network->output_layer[0].o;
}

static void
randomize_neural_network (WhsNeuralNetwork *network)
{
  for (gint i = 0; i < 16; i++) {
    for (gint j = 0; j < 33; j++) {
      network->hidden_layer1[i].w[j] = g_random_double_range (-2.0, 2.0);
    }
  }

  for (gint i = 0; i < 17; i++) {
    network->output_layer[0].w[i] = g_random_double_range (-2.0, 2.0);
  }
}

#define WHS_NN_CLASSIFIER_32_16_1_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), WHS_TYPE_NN_CLASSIFIER_32_16_1, WhsNNClassifier_32_16_1Private))

static void whs_nn_classifier_32_16_1_init (WhsNNClassifier_32_16_1 * self);
static void whs_nn_classifier_32_16_1_class_init (WhsNNClassifier_32_16_1Class * klass);
static void whs_nn_classifier_32_16_1_finalize (WhsObject *object);

static WhsClassifier * whs_nn_classifier_32_16_1_constructor (WhsPattern *pattern);
static void whs_nn_classifier_32_16_1_process (WhsClassifier *classifier, const WhsFeatureVector *vec, WhsResult *res);
static WhsPattern * whs_nn_classifier_32_16_1_learn (WhsClassifier *classifier, const GList *values, gint count, gfloat rate);

G_DEFINE_TYPE (WhsNNClassifier_32_16_1, whs_nn_classifier_32_16_1, WHS_TYPE_CLASSIFIER);

static WhsClassifierClass *parent_class = NULL;

static void
whs_nn_classifier_32_16_1_class_init (WhsNNClassifier_32_16_1Class * klass)
{
  WhsObjectClass *o_klass = (WhsObjectClass *) klass;
  WhsClassifierClass *c_klass = (WhsClassifierClass *) klass;

  parent_class = WHS_CLASSIFIER_CLASS (g_type_class_peek_parent (klass));

  g_type_class_add_private (klass, sizeof (WhsNNClassifier_32_16_1Private));

  o_klass->finalize = whs_nn_classifier_32_16_1_finalize;
  c_klass->constructor = whs_nn_classifier_32_16_1_constructor;
  c_klass->learn = whs_nn_classifier_32_16_1_learn;
  c_klass->process = whs_nn_classifier_32_16_1_process;
}

static void
whs_nn_classifier_32_16_1_init (WhsNNClassifier_32_16_1 * self)
{
  self->priv = WHS_NN_CLASSIFIER_32_16_1_GET_PRIVATE (self);
}

static void
whs_nn_classifier_32_16_1_finalize (WhsObject *object)
{

  WHS_OBJECT_CLASS (parent_class)->finalize (object);
}

static WhsClassifier *
whs_nn_classifier_32_16_1_constructor (WhsPattern *pattern)
{
  WhsNNClassifier_32_16_1 *self = WHS_NN_CLASSIFIER_32_16_1_CAST (g_type_create_instance (WHS_TYPE_NN_CLASSIFIER_32_16_1));

  if (pattern == NULL) {
    randomize_neural_network (&self->priv->network);
    return WHS_CLASSIFIER_CAST (self);
  }

  gsize size;
  const gfloat *data = (const gfloat *) whs_pattern_get_classifier_data (pattern, g_type_name (WHS_TYPE_NN_CLASSIFIER_32_16_1), &size);

  g_return_val_if_fail (size % (sizeof (gfloat) * (16 * 33 + 17)) == 0, NULL);

  WhsNeuralNetwork *network = &self->priv->network;

  for (gint i = 0; i < 16; i++) {
    for (gint j = 0; j < 33; j++) {
      network->hidden_layer1[i].w[j] = GFLOAT_FROM_BE (data[i*33 + j]);
    }
  }

  for (gint i = 0; i < 17; i++) {
    network->output_layer[0].w[i] = GFLOAT_FROM_BE (data[16 * 33 + i]);
  }

  return WHS_CLASSIFIER_CAST (self);
}

static void
whs_nn_classifier_32_16_1_process (WhsClassifier *classifier, const WhsFeatureVector *vec, WhsResult *res)
{
  WhsNNClassifier_32_16_1 *self = WHS_NN_CLASSIFIER_32_16_1 (classifier);

  res->result = calculate_neural_network (&self->priv->network, vec->mfcc);
}

/* Learn rate */
#define N (0.0001)
#define A (0.25)

static WhsPattern *
whs_nn_classifier_32_16_1_learn (WhsClassifier *classifier, const GList *values, gint count, gfloat rate)
{
  WhsNNClassifier_32_16_1 *self = WHS_NN_CLASSIFIER_32_16_1 (classifier);

  WhsNeuralNetwork *network = &self->priv->network;

  gdouble mse;
  gfloat last_change_out[17] = {0.0, };
  gfloat last_change_hidden[32 * 17] = {0.0, };

  gint correct;
  gint run = 0;

retry:
  mse = 0.0;
  correct = 0;
  for (const GList *l = values; l != NULL; l = l->next) {
    WhsResultValue *val = (WhsResultValue *) l->data;

    if (val->result < 0)
      continue;

    calculate_neural_network (network, val->vec.mfcc);

    // Calculate errors

    gfloat d_hidden[16] = {0.0, };
    gfloat d_out[1] = {0.0, };

    d_out[0] = network->output_layer[0].o * (1.0 - network->output_layer[0].o) * (val->result - network->output_layer[0].o);

    for (gint i = 0; i < 16; i++) {
      d_hidden[i] = network->output_layer[0].w[i+1] * d_out[0];
      d_hidden[i] = network->hidden_layer1[i].o * (1.0 - network->hidden_layer1[i].o) * d_hidden[i];
    }

    // Adjust weights
    for (gint i = 0; i < 16; i++) {
      network->hidden_layer1[i].w[0] += (last_change_hidden[i*16] = N * d_hidden[i] + A * last_change_hidden[i*16]);
      for (gint j = 0; j < 32; j++) {
        network->hidden_layer1[i].w[j+1] += (last_change_hidden[i*16 + j + 1] = N * d_hidden[i] * network->hidden_layer1[i].x[j] + A * last_change_hidden[i*16 + j + 1]);
      }
    }

    network->output_layer[0].w[0] += N * d_out[0] + A * last_change_out[0];
    for (gint i = 0; i < 16; i++)
      network->output_layer[0].w[i+1] += (last_change_out[i] = N * d_out[0] * network->output_layer[0].x[i] + A * last_change_out[i]);
  }

  for (const GList *l = values; l != NULL; l = l->next) {
    WhsResultValue *val = (WhsResultValue *) l->data;
    
    if (val->result < 0)
      continue;

    gfloat res = calculate_neural_network (network, val->vec.mfcc);
    mse += fabs (res - val->result) * fabs (res - val->result);
    if ((val->result == 0 && res < 0.5) ||
        (val->result == 1 && res >= 0.5))
      correct++;
  }

  mse /= count;
  g_print ("run %d, %d of %d, rate: %f, mse: %lf\n", run, correct, count, ((gfloat) (correct) / ((gfloat) count)), mse);

  if (((gfloat) (correct) / ((gfloat) count)) < rate) {
    run++;
    goto retry;
  }

  WhsPattern *ret = WHS_PATTERN_CAST (g_type_create_instance (WHS_TYPE_PATTERN));
  gfloat *data = g_new0 (gfloat, 16*33 + 17);

 for (gint i = 0; i < 16; i++) {
    for (gint j = 0; j < 33; j++) {
      data[i*33 + j] = GFLOAT_TO_BE (network->hidden_layer1[i].w[j]);
    }
  }

  for (gint i = 0; i < 17; i++) {
    data[16*33 + i] = GFLOAT_TO_BE (network->output_layer[0].w[i]);
  }

  whs_pattern_set_classifier_data (ret, g_type_name (WHS_TYPE_NN_CLASSIFIER_32_16_1), (guint8 *)data, sizeof (gfloat) * (16 * 33 + 17));

  return ret;
}
