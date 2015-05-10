#include <pebble.h>
#include "effects.h"
#include "math.h"
  
  
// { ********* Graphics utility functions (probablu should be seaparated into anothe file?) *********

// set pixel color at given coordinates 
void set_pixel(uint8_t *bitmap_data, int bytes_per_row, int y, int x, uint8_t color) {
      
  #ifdef PBL_COLOR 
    bitmap_data[y*bytes_per_row + x] = color; // in Basalt - simple set entire byte
  #else
    bitmap_data[y*bytes_per_row + x / 8] ^= (-color ^ bitmap_data[y*bytes_per_row + x / 8]) & (1 << (x % 8)); // in Aplite - set the bit
  #endif
}

// get pixel color at given coordinates 
uint8_t get_pixel(uint8_t *bitmap_data, int bytes_per_row, int y, int x) {
  
  #ifdef PBL_COLOR
    return bitmap_data[y*bytes_per_row + x]; // in Basalt - simple get entire byte
  #else
    return (bitmap_data[y*bytes_per_row + x / 8] >> (x % 8)) & 1; // in Aplite - get the bit
  #endif
}
 

// THE EXTREMELY FAST LINE ALGORITHM Variation E (Addition Fixed Point PreCalc Small Display)
// Small Display (256x256) resolution.
// based on algorythm by Po-Han Lin at http://www.edepot.com
void set_line(uint8_t *bitmap_data, int bytes_per_row, int y, int x, int y2, int x2, uint8_t draw_color, uint8_t skip_color, uint8_t *visited) {
  bool yLonger = false;	int shortLen=y2-y; int longLen=x2-x;
  uint8_t temp_pixel;  int temp_x, temp_y;
  
	if (abs(shortLen)>abs(longLen)) {
		int swap=shortLen;
		shortLen=longLen;	longLen=swap;	yLonger=true;
	}
  
	int decInc;
	if (longLen==0) decInc=0;
	else decInc = (shortLen << 8) / longLen;

	if (yLonger) {
		if (longLen>0) {
			longLen+=y;
			for (int j=0x80+(x<<8);y<=longLen;++y) {
        temp_y = y; temp_x = j >> 8;
        if (temp_y >=0 && temp_y<168 && temp_x >=0 && temp_x < 144) {
          temp_pixel = get_pixel(bitmap_data, bytes_per_row,  temp_y, temp_x);
          #ifdef PBL_COLOR // for Basalt drawing pixel if it is not of original color or already drawn color
            if (temp_pixel != skip_color && temp_pixel != draw_color) set_pixel(bitmap_data, bytes_per_row, temp_y, temp_x, draw_color);
          #else
            if (get_pixel(visited, bytes_per_row,  temp_y, temp_x) != 1) { // for Aplite first check if pixel isn't already marked as set in user-defined array
              if (temp_pixel != skip_color) set_pixel(bitmap_data, bytes_per_row, temp_y, temp_x, draw_color); // if pixel isn't of original color - set it
              draw_color = 1 - draw_color; // revers pixel for "lined" effect
              set_pixel(visited, bytes_per_row, temp_y, temp_x, 1); //mark pixel as set
            }
          #endif
        }
				j+=decInc;
			}
			return;
		}
		longLen+=y;
		for (int j=0x80+(x<<8);y>=longLen;--y) {
      temp_y = y; temp_x = j >> 8;
      if (temp_y >=0 && temp_y<168 && temp_x >=0 && temp_x < 144) {
        temp_pixel = get_pixel(bitmap_data, bytes_per_row,  temp_y, temp_x);
          #ifdef PBL_COLOR // for Basalt drawing pixel if it is not of original color or already drawn color
            if (temp_pixel != skip_color && temp_pixel != draw_color) set_pixel(bitmap_data, bytes_per_row, temp_y, temp_x, draw_color);
          #else
            if (get_pixel(visited, bytes_per_row,  temp_y, temp_x) != 1) { // for Aplite first check if pixel isn't already marked as set in user-defined array
              if (temp_pixel != skip_color) set_pixel(bitmap_data, bytes_per_row, temp_y, temp_x, draw_color); // if pixel isn't of original color - set it
              draw_color = 1 - draw_color; // revers pixel for "lined" effect
              set_pixel(visited, bytes_per_row, temp_y, temp_x, 1); //mark pixel as set
            }
          #endif
      }
			j-=decInc;
		}
		return;	
	}

	if (longLen>0) {
		longLen+=x;
		for (int j=0x80+(y<<8);x<=longLen;++x) {
      temp_y = j >> 8; temp_x =  x;
      if (temp_y >=0 && temp_y<168 && temp_x >=0 && temp_x < 144) {
        temp_pixel = get_pixel(bitmap_data, bytes_per_row, temp_y, temp_x);
          #ifdef PBL_COLOR // for Basalt drawing pixel if it is not of original color or already drawn color
            if (temp_pixel != skip_color && temp_pixel != draw_color) set_pixel(bitmap_data, bytes_per_row, temp_y, temp_x, draw_color);
          #else
            if (get_pixel(visited, bytes_per_row,  temp_y, temp_x) != 1) { // for Aplite first check if pixel isn't already marked as set in user-defined array
              if (temp_pixel != skip_color) set_pixel(bitmap_data, bytes_per_row, temp_y, temp_x, draw_color); // if pixel isn't of original color - set it
              draw_color = 1 - draw_color; // revers pixel for "lined" effect
              set_pixel(visited, bytes_per_row, temp_y, temp_x, 1); //mark pixel as set
            }
          #endif
      }  
			j+=decInc;
		}
		return;
	}
	longLen+=x;
	for (int j=0x80+(y<<8);x>=longLen;--x) {
	  temp_y = j >> 8; temp_x =  x;
    if (temp_y >=0 && temp_y<168 && temp_x >=0 && temp_x < 144) {
      temp_pixel = get_pixel(bitmap_data, bytes_per_row, temp_y, temp_x);
          #ifdef PBL_COLOR // for Basalt drawing pixel if it is not of original color or already drawn color
            if (temp_pixel != skip_color && temp_pixel != draw_color) set_pixel(bitmap_data, bytes_per_row, temp_y, temp_x, draw_color);
          #else
            if (get_pixel(visited, bytes_per_row,  temp_y, temp_x) != 1) { // for Aplite first check if pixel isn't already marked as set in user-defined array
              if (temp_pixel != skip_color) set_pixel(bitmap_data, bytes_per_row, temp_y, temp_x, draw_color); // if pixel isn't of original color - set it
              draw_color = 1 - draw_color; // revers pixel for "lined" effect
              set_pixel(visited, bytes_per_row, temp_y, temp_x, 1); //mark pixel as set
            }
          #endif
    }  
		j-=decInc;
	}

}

//  ********* Graphics utility functions (probablu should be seaparated into anothe file?) ********* }

  

// inverter effect.
void effect_invert(GContext* ctx,  GRect position, void* param) {
  //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bitmap_data =  gbitmap_get_data(fb);
  int bytes_per_row = gbitmap_get_bytes_per_row(fb);

  
  for (int y = 0; y < position.size.h; y++)
     for (int x = 0; x < position.size.w; x++)
        #ifdef PBL_COLOR // on Basalt simple doing NOT on entire returned byte/pixel
          set_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x, ~get_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x));
        #else // on Aplite since only 1 and 0 is returning, doing "not" by 1 - pixel
          set_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x, 1 - get_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x));
        #endif
 
  graphics_release_frame_buffer(ctx, fb);          
          
}

// vertical mirror effect.
void effect_mirror_vertical(GContext* ctx, GRect position, void* param) {
  uint8_t temp_pixel;  
  
  //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bitmap_data =  gbitmap_get_data(fb);
  int bytes_per_row = gbitmap_get_bytes_per_row(fb);

  for (int y = 0; y < position.size.h / 2 ; y++)
     for (int x = 0; x < position.size.w; x++){
        temp_pixel = get_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x);
        set_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x, get_pixel(bitmap_data, bytes_per_row, position.origin.y + position.size.h - y - 2, x + position.origin.x));
        set_pixel(bitmap_data, bytes_per_row, position.origin.y + position.size.h - y - 2, x + position.origin.x, temp_pixel);
     }
  
  graphics_release_frame_buffer(ctx, fb);
}


// horizontal mirror effect.
void effect_mirror_horizontal(GContext* ctx, GRect position, void* param) {
  uint8_t temp_pixel;  
  
  //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bitmap_data =  gbitmap_get_data(fb);
  int bytes_per_row = gbitmap_get_bytes_per_row(fb);


  for (int y = 0; y < position.size.h; y++)
     for (int x = 0; x < position.size.w / 2; x++){
        temp_pixel = get_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x);
        set_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x, get_pixel(bitmap_data, bytes_per_row, y + position.origin.y, position.origin.x + position.size.w - x - 2));
        set_pixel(bitmap_data, bytes_per_row, y + position.origin.y, position.origin.x + position.size.w - x - 2, temp_pixel);
     }
  
  graphics_release_frame_buffer(ctx, fb);
}

// Rotate 90 degrees
// Added by Ron64
// Parameter:  true: rotate right/clockwise,  false: rotate left/counter_clockwise
void effect_rotate_90_degrees(GContext* ctx,  GRect position, void* param){

  //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bitmap_data =  gbitmap_get_data(fb);
  int bytes_per_row = gbitmap_get_bytes_per_row(fb);

  bool right = (bool)param;
  uint8_t qtr, xCn, yCn, temp_pixel;
  xCn= position.origin.x + position.size.w /2;
  yCn= position.origin.y + position.size.h /2;
  qtr=position.size.w;
  if (position.size.h < qtr)
    qtr= position.size.h;
  qtr= qtr/2;

  for (int c1 = 0; c1 < qtr; c1++)
    for (int c2 = 1; c2 < qtr; c2++){
      temp_pixel = get_pixel(bitmap_data, bytes_per_row, yCn +c1, xCn +c2);
      if (right){
        set_pixel(bitmap_data, bytes_per_row, yCn +c1, xCn +c2, get_pixel(bitmap_data, bytes_per_row, yCn -c2, xCn +c1));
        set_pixel(bitmap_data, bytes_per_row, yCn -c2, xCn +c1, get_pixel(bitmap_data, bytes_per_row, yCn -c1, xCn -c2));
        set_pixel(bitmap_data, bytes_per_row, yCn -c1, xCn -c2, get_pixel(bitmap_data, bytes_per_row, yCn +c2, xCn -c1));
        set_pixel(bitmap_data, bytes_per_row, yCn +c2, xCn -c1, temp_pixel);
      }
      else{
        set_pixel(bitmap_data, bytes_per_row, yCn +c1, xCn +c2, get_pixel(bitmap_data, bytes_per_row, yCn +c2, xCn -c1));
        set_pixel(bitmap_data, bytes_per_row, yCn +c2, xCn -c1, get_pixel(bitmap_data, bytes_per_row, yCn -c1, xCn -c2));
        set_pixel(bitmap_data, bytes_per_row, yCn -c1, xCn -c2, get_pixel(bitmap_data, bytes_per_row, yCn -c2, xCn +c1));
        set_pixel(bitmap_data, bytes_per_row, yCn -c2, xCn +c1, temp_pixel);
      }
     }
  
  graphics_release_frame_buffer(ctx, fb);
}

// Zoom effect.
// Added by Ron64
// Parameter: Y zoom (high byte) X zoom(low byte),  0x10 no zoom 0x20 200% 0x08 50%, 
// use the percentage macro EL_ZOOM(150,60). In this example: Y- zoom in 150%, X- zoom out to 60% 
void effect_zoom(GContext* ctx,  GRect position, void* param){
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bd =  gbitmap_get_data(fb);
  int bpr = gbitmap_get_bytes_per_row(fb);

  uint8_t xCn, yCn, Y1,X1, ratioY, ratioX;
  xCn= position.origin.x + position.size.w /2;
  yCn= position.origin.y + position.size.h /2;

  ratioY= (int32_t)param >>8 & 0xFF;
  ratioX= (int32_t)param & 0xFF;

  for (int y = 0; y <= position.size.h>>1; y++)
    for (int x = 0; x <= position.size.w>>1; x++)
    {
      //yS,xS scan source: centre to out or out to centre
      int8_t yS = (ratioY>16) ? (position.size.h/2)- y: y; 
      int8_t xS = (ratioX>16) ? (position.size.w/2)- x: x;
      Y1= (yS<<4) /ratioY;
      X1= (xS<<4) /ratioX;
      set_pixel(bd,bpr, yCn +yS, xCn +xS, get_pixel(bd,bpr, yCn +Y1, xCn +X1)); 
      set_pixel(bd,bpr, yCn +yS, xCn -xS, get_pixel(bd,bpr, yCn +Y1, xCn -X1));
      set_pixel(bd,bpr, yCn -yS, xCn +xS, get_pixel(bd,bpr, yCn -Y1, xCn +X1));
      set_pixel(bd,bpr, yCn -yS, xCn -xS, get_pixel(bd,bpr, yCn -Y1, xCn -X1));
    }
  graphics_release_frame_buffer(ctx, fb);
//Todo: Should probably reduce Y size on zoom out or limit reading beyond edge of screen.
}

// Lens effect.
// Added by Ron64
// Parameters: lens focal(high byte) and object distance(low byte)
void effect_lens(GContext* ctx,  GRect position, void* param){
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bd =  gbitmap_get_data(fb);
  int bpr = gbitmap_get_bytes_per_row(fb);
  uint8_t d,r, xCn, yCn;

  xCn= position.origin.x + position.size.w /2;
  yCn= position.origin.y + position.size.h /2;
  d=position.size.w;
  if (position.size.h < d)
    d= position.size.h;
  r= d/2; // radius of lens
  float focal =   (int32_t)param >>8 & 0xFF;// focal point of lens
  float obj_dis = (int32_t)param & 0xFF;//distance of object from focal point.
  
  for (int y = r; y >= 0; --y)
    for (int x = r; x >= 0; --x)
      if (x*x+y*y < r*r)
      {
        int Y1= my_tan(my_asin(y/focal))*obj_dis;
        int X1= my_tan(my_asin(x/focal))*obj_dis;
        set_pixel(bd,bpr, yCn +y, xCn +x, get_pixel(bd,bpr, yCn +Y1, xCn +X1)); 
        set_pixel(bd,bpr, yCn +y, xCn -x, get_pixel(bd,bpr, yCn +Y1, xCn -X1));
        set_pixel(bd,bpr, yCn -y, xCn +x, get_pixel(bd,bpr, yCn -Y1, xCn +X1));
        set_pixel(bd,bpr, yCn -y, xCn -x, get_pixel(bd,bpr, yCn -Y1, xCn -X1));
      }
    graphics_release_frame_buffer(ctx, fb);
//Todo: Change to lock-up arcsin table in the future. (Currently using floating point math library that is relatively big & slow)
}
  
// mask effect.
// see struct EffectMask for parameter description  
void effect_mask(GContext* ctx, GRect position, void* param) {
  GColor temp_pixel;  
  EffectMask *mask = (EffectMask *)param;

  //drawing background - only if real color is passed
  if (!gcolor_equal(mask->background_color, GColorClear)) {
    graphics_context_set_fill_color(ctx, mask->background_color);
    graphics_fill_rect(ctx, GRect(0, 0, position.size.w, position.size.h), 0, GCornerNone); 
  }  
  
  //if text mask is used - drawing text
  if (mask->text) {
     graphics_context_set_text_color(ctx, mask->mask_color);
     graphics_draw_text(ctx, mask->text, mask->font, GRect(0, 0, position.size.w, position.size.h), mask->text_overflow, mask->text_align, NULL);
  } else if (mask->bitmap_mask) { // othersise - bitmap mask is used - draw bimap
     graphics_draw_bitmap_in_rect(ctx, mask->bitmap_mask, GRect(0, 0, position.size.w, position.size.h));
  }
    
  //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bitmap_data =  gbitmap_get_data(fb);
  int bytes_per_row = gbitmap_get_bytes_per_row(fb);
  
  //capturing background bitmap
  uint8_t *bg_bitmap_data =  gbitmap_get_data(mask->bitmap_background);
  int bg_bytes_per_row = gbitmap_get_bytes_per_row(mask->bitmap_background);
    
  //looping throughout layer replacing mask with bg bitmap
  for (int y = 0; y < position.size.h; y++)
     for (int x = 0; x < position.size.w; x++) {
       temp_pixel = (GColor)get_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x);
       if (gcolor_equal(temp_pixel, mask->mask_color))
         set_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x, get_pixel(bg_bitmap_data, bg_bytes_per_row, y + position.origin.y, x + position.origin.x));
  }
  
  graphics_release_frame_buffer(ctx, fb);
  
}

void effect_fps(GContext* ctx, GRect position, void* param) {
  static GFont font = NULL;
  static char buff[16];
  time_t tt;
  uint16_t ms;
  
  if(((EffectFPS*)param)->starttt) {
    time_ms(&tt,&ms);
    ++((EffectFPS*)param)->frame;
    uint32_t fp100s = (100000*((EffectFPS*)param)->frame)/((tt-((EffectFPS*)param)->starttt)*1000+ms-((EffectFPS*)param)->startms);
    snprintf(buff,sizeof(buff),"FPS:%d.%02d",(int)fp100s/100,(int)fp100s%100);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_text(ctx, buff, font, GRect(0, 0, position.size.w, position.size.h), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }
  else {
    // First call
    if(!font) font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    time_ms(&((EffectFPS*)param)->starttt,&((EffectFPS*)param)->startms);
    ((EffectFPS*)param)->frame = 0;
  }
}


// mask effect.
// see struct EffecOffset for parameter description  
void effect_shadow(GContext* ctx, GRect position, void* param) {
  GColor temp_pixel;  
  int shadow_x, shadow_y;
  EffectOffset *shadow = (EffectOffset *)param;
  
  #ifndef PBL_COLOR
    uint8_t draw_color = gcolor_equal(shadow->offset_color, GColorWhite)? 1 : 0;
    uint8_t skip_color = gcolor_equal(shadow->orig_color, GColorWhite)? 1 : 0;
  #endif
  
   //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bitmap_data =  gbitmap_get_data(fb);
  int bytes_per_row = gbitmap_get_bytes_per_row(fb);

  
  //looping throughout making shadow
  for (int y = 0; y < position.size.h; y++)
     for (int x = 0; x < position.size.w; x++) {
       temp_pixel = (GColor)get_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x);
       
       if (gcolor_equal(temp_pixel, shadow->orig_color)) {
         shadow_x =  x + position.origin.x + shadow->offset_x;
         shadow_y =  y + position.origin.y + shadow->offset_y;
         
         if (shadow->option == 1) {
            #ifdef PBL_COLOR // for Basalt simple calling line-drawing routine
               set_line(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x, shadow_y, shadow_x, shadow->offset_color.argb, shadow->orig_color.argb, NULL);
            #else // for Aplite - passing user-defined array to determine if pixels have been set or not
               set_line(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x, shadow_y, shadow_x, draw_color, skip_color, shadow->aplite_visited); 
            #endif
           
         } else {
           
             if (shadow_x >= 0 && shadow_x <=143 && shadow_y >= 0 && shadow_y <= 167) {
             
               temp_pixel = (GColor)get_pixel(bitmap_data, bytes_per_row, shadow_y, shadow_x);
               if (!gcolor_equal(temp_pixel, shadow->orig_color) & !gcolor_equal(temp_pixel, shadow->offset_color) ) {
                 #ifdef PBL_COLOR
                    set_pixel(bitmap_data, bytes_per_row,  shadow_y, shadow_x, shadow->offset_color.argb);  
                 #else
                    set_pixel(bitmap_data, bytes_per_row,  shadow_y, shadow_x, gcolor_equal(shadow->offset_color, GColorWhite)? 1 : 0);
                 #endif
               }
             }
           
         }
         
         
       }
  }
         
  graphics_release_frame_buffer(ctx, fb);
 
}

void effect_outline(GContext* ctx, GRect position, void* param) {
  GColor temp_pixel;  
  int outlinex[4];
  int outliney[4];
  EffectOffset *outline = (EffectOffset *)param;
  
   //capturing framebuffer bitmap
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  uint8_t *bitmap_data =  gbitmap_get_data(fb);
  int bytes_per_row = gbitmap_get_bytes_per_row(fb);
  
  //loop through pixels from framebuffer
  for (int y = 0; y < position.size.h; y++)
     for (int x = 0; x < position.size.w; x++) {
       temp_pixel = (GColor)get_pixel(bitmap_data, bytes_per_row, y + position.origin.y, x + position.origin.x);
       
       if (gcolor_equal(temp_pixel, outline->orig_color)) {
          // TODO: there's probably a more efficient way to do this
          outlinex[0] = x + position.origin.x - outline->offset_x;
          outliney[0] = y + position.origin.y - outline->offset_y;
          outlinex[1] = x + position.origin.x + outline->offset_x;
          outliney[1] = y + position.origin.y + outline->offset_y;
          outlinex[2] = x + position.origin.x - outline->offset_x;
          outliney[2] = y + position.origin.y + outline->offset_y;
          outlinex[3] = x + position.origin.x + outline->offset_x;
          outliney[3] = y + position.origin.y - outline->offset_y;
          
         
          for (int i = 0; i < 4; i++) {
            // TODO: centralize the constants
            if (outlinex[i] >= 0 && outlinex[i] <=144 && outliney[i] >= 0 && outliney[i] <= 168) {
              temp_pixel = (GColor)get_pixel(bitmap_data, bytes_per_row, outliney[i], outlinex[i]);
              if (!gcolor_equal(temp_pixel, outline->orig_color)) {
                #ifdef PBL_COLOR
                   set_pixel(bitmap_data, bytes_per_row, outliney[i], outlinex[i], outline->offset_color.argb);  
                #else
                   set_pixel(bitmap_data, bytes_per_row, outliney[i], outlinex[i], gcolor_equal(outline->offset_color, GColorWhite)? 1 : 0);
                #endif
              }
            }
          }
       }
  }

  graphics_release_frame_buffer(ctx, fb);
}