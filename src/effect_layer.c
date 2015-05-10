#include <pebble.h>
#include "effect_layer.h"
#include "effects.h"  

// Find the offset of parent layer pointer  
static uint8_t find_parent_offset() {
  Layer* p = layer_create(GRect(0,0,32,32));
  Layer* l = layer_create(GRect(0,0,16,16));
  layer_add_child(p,l);

  uint8_t i=0;
  while(i<16 && *(((Layer**)(void*)l)+i)!=p) ++i;

  if(*(((Layer**)(void*)l)+i)!=p) {
    i=0xff;
    APP_LOG(APP_LOG_LEVEL_ERROR,"EffectLayer library was unable to find the parent layer offset! Your app will probably crash (sorry) :(");
  }

  layer_destroy(l);
  layer_destroy(p);
  return i;
}

// on layer update - apply effect
static void effect_layer_update_proc(Layer *me, GContext* ctx) {
  static uint8_t parent_layer_offset = 0xff;
  if(parent_layer_offset == 0xff) {
    parent_layer_offset = find_parent_offset();
  }
  
  // retrieving layer and its real coordinates
  EffectLayer* effect_layer = (EffectLayer*)(layer_get_data(me));
  GRect layer_frame = layer_get_frame(me);
  Layer* l = me;
  while((l=((Layer**)(void*)l)[parent_layer_offset])) {
    GRect parent_frame = layer_get_frame(l);
    layer_frame.origin.x += parent_frame.origin.x;
    layer_frame.origin.y += parent_frame.origin.y;
  }
  
  // Applying effects
  for(uint8_t i=0; effect_layer->effects[i] && i<MAX_EFFECTS;++i) effect_layer->effects[i](ctx, layer_frame, effect_layer->params[i]);
}  

// create effect layer
EffectLayer* effect_layer_create(GRect frame) {
    
  //creating base layer
  Layer* layer =layer_create_with_data(frame, sizeof(EffectLayer));
  layer_set_update_proc(layer, effect_layer_update_proc);
  EffectLayer* effect_layer = (EffectLayer*)layer_get_data(layer);
  memset(effect_layer,0,sizeof(EffectLayer));
  effect_layer->layer = layer;

  return effect_layer;                    
}

//destroy effect layer
void effect_layer_destroy(EffectLayer *effect_layer) {
  layer_destroy(effect_layer->layer);
}

// returns base layer
Layer* effect_layer_get_layer(EffectLayer *effect_layer){
  return effect_layer->layer;
}

//sets effect for the layer
void effect_layer_add_effect(EffectLayer *effect_layer, effect_cb* effect, void* param) {
  if(effect_layer->next_effect<MAX_EFFECTS) {
    effect_layer->effects[effect_layer->next_effect] = effect;
    effect_layer->params[effect_layer->next_effect] = param;  
    ++effect_layer->next_effect;
  }
}
