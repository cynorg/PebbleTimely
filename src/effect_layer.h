#pragma once
#include <pebble.h>  
#include "effects.h"
  
//number of supported effects on a single effect_layer (must be <= 255)
#define MAX_EFFECTS 4
  
// structure of effect layer
typedef struct {
  Layer*      layer;
  effect_cb*  effects[MAX_EFFECTS];
  void*       params[MAX_EFFECTS];
  uint8_t     next_effect;
} EffectLayer;


//creates effect layer
EffectLayer* effect_layer_create(GRect frame);

//destroys effect layer
void effect_layer_destroy(EffectLayer *effect_layer);

//sets effect for the layer
void effect_layer_add_effect(EffectLayer *effect_layer, effect_cb* effect, void* param);

//gets layer
Layer* effect_layer_get_layer(EffectLayer *effect_layer);

// Recreate inverter_layer for BASALT
#ifndef PBL_PLATFORM_APLITE
  #define InverterLayer EffectLayer
  #define inverter_layer_create(frame)({ EffectLayer* _el=effect_layer_create(frame); effect_layer_add_effect(_el,effect_invert,NULL);_el; })
  #define inverter_layer_get_layer effect_layer_get_layer
  #define inverter_layer_destroy effect_layer_destroy
#endif