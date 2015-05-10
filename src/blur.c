#include <pebble.h>

#include "effects.h"

#ifdef PBL_COLOR
static void blur_(uint8_t *bitmap_data, int bytes_per_row, GRect position, uint16_t line, uint8_t *dest, uint8_t radius){
  uint8_t (*fb_a)[bytes_per_row] = (uint8_t (*)[bytes_per_row])bitmap_data;
  uint16_t total[3] = {0,0,0};
  uint8_t  nb_points = 0;
  GPoint p = {0,0};
  for (uint16_t x = 0; x < position.size.w; ++x) {
    total[0] = total[1] = total[2] = 0;
    nb_points = 0;
    p.y = position.origin.y + line - radius;
    for (uint8_t ky = 0; ky <= 2*radius; ++ky){
      p.x = position.origin.x + x - radius;
      for (uint8_t kx = 0; kx <= 2*radius; ++kx){
        if(grect_contains_point(&position, &p)){
          GColor8 color = (GColor8)fb_a[p.y][p.x];
          total[0] += color.r;
          total[1] += color.g;
          total[2] += color.b;
          nb_points++;
        }
        p.x++;
      }
      p.y++;
    }
    total[0] = (total[0] * 0x55) / nb_points;
    total[1] = (total[1] * 0x55) / nb_points;
    total[2] = (total[2] * 0x55) / nb_points;
    dest[x] = GColorFromRGB(total[0], total[1], total[2]).argb; 
  }
}
#endif

void effect_blur(GContext* ctx,  GRect position, void* param){
#ifdef PBL_COLOR
  //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bitmap_data =  gbitmap_get_data(fb);
  int bytes_per_row = gbitmap_get_bytes_per_row(fb);

  
  
  uint8_t radius = (uint8_t)(uint32_t)param; // Not very elegant... sorry
  uint8_t (*fb_a)[bytes_per_row] = (uint8_t (*)[bytes_per_row])bitmap_data;
  uint16_t offset_x = position.origin.x;
  uint16_t offset_y = position.origin.y;
  uint16_t width    = position.size.w;
  uint16_t height   = position.size.h;
 
  uint8_t *buffer = malloc(width * (radius + 1));
 
  uint16_t h=0;
  for(; h<(radius+1); h++){
    blur_(bitmap_data, bytes_per_row, position, h, buffer + h*width, radius);
  }
 
  for(; h<height; h++){
    memcpy(&fb_a[offset_y + h - (radius + 1)][offset_x], buffer, width);
    memcpy(buffer, buffer + width, radius * width);
    blur_(bitmap_data, bytes_per_row, position, h, buffer + radius*width, radius);
  }

  h=0;
  for(; h<radius; h++){
    memcpy(&fb_a[offset_y + height - (radius + 1) + h][offset_x] , buffer + h*width, width);
  }
  
  free(buffer);
  
  graphics_release_frame_buffer(ctx, fb);
#endif
}
