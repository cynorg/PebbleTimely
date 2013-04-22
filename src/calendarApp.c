#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <math.h>
#include "resource_ids.auto.h"

#define MY_UUID { 0xB4, 0x1E, 0x3D, 0xCF, 0x61, 0x62, 0x41, 0x47, 0x9C, 0x58, 0x64, 0x3E, 0x10, 0x91, 0xFB, 0x93 }
PBL_APP_INFO(MY_UUID,
             "Calendar", "William Heaton",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_STANDARD_APP);

static int offset = 0;

Window window;

Layer month_layer;
Layer days_layer;

const bool black = false;  // Is the background black
const bool grid = true; // show the grid
const bool invert = true; // Invert colors on today's date

const char daysOfWeek[7][3] = {"S","M","T","W","Th","F","S"};
const char months[12][12] = {"January","Feburary","March","April","May","June","July","August","September","October", "November", "December"};

char* intToStr(int val){

 	static char buf[32] = {0};
	
	int i = 30;	
	for(; val && i ; --i, val /= 10)
		buf[i] = "0123456789"[val % 10];
	
	return &buf[i+1];
}
// Calculate what day of the week it was on the first day of the month, if mday was a wday
int wdayOfFirst(int wday,int mday){
    int a = wday - ((mday-1)%7);
    if(a<0) a += 7;
    return a;
}
// Calculate what day of the week it was/will be X days from the first day of the month, if mday was a wday
int wdayOfFirstOffset(int wday,int mday,int ofs){
    int a = wday - ((mday-1)%7);
    if(a<0) a += 7;

    int b;
    if(ofs>0)
        b = a + (abs(ofs)%7); 
    else
        b = a - (abs(ofs)%7); 
    
    if(b<0) b += 7;
    if(b>6) b -= 7;
    
    return b;
}

// How many days are/were in the month
int daysInMonth(int mon, int year){
    mon++;
    
    // April, June, September and November have 30 Days
    if(mon == 4 || mon == 6 || mon == 9 || mon == 11)
        return 30;
        
    // Deal with Feburary & Leap years
    else if( mon == 2 ){
        if(year%400==0)
            return 29;
        else if(year%100==0)
            return 28;
        else if(year%4==0)
            return 29;
        else 
            return 28;
    }
    // Most months have 31 days
    else
        return 31;
}
void setColors(GContext* ctx){
    if(black){
        window_set_background_color(&window, GColorBlack);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_text_color(ctx, GColorWhite);
    }else{
        window_set_background_color(&window, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_text_color(ctx, GColorBlack);
    }
}
void setInvColors(GContext* ctx){
    if(!black){
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_text_color(ctx, GColorWhite);
    }else{
        window_set_background_color(&window, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_text_color(ctx, GColorBlack);
    }
}


void days_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    
    PblTm currentTime;
    get_time(&currentTime);
    int mon = currentTime.tm_mon;
    int year = currentTime.tm_year+1900;
    
    
    // Figure out which month & year we are going to be looking at based on the selected offset
    // Calculate how many days are between the first of this month and the first of the month we are interested in
    int j = 0;
    int od = 0;
    while(j < abs(offset)){
        j++;
        if(offset > 0){
            od = od + daysInMonth(mon,year);
            mon++;
            if(mon>11){
                mon -= 12;
                year++;
            }
        }else{
            mon--;
            if(mon < 0){
                mon += 12;
                year--;
            }
            od -= daysInMonth(mon,year);
        }
    }
    
    // Days in the target month
    int dom = daysInMonth(mon,year);
    
    // Day of the week for the first day in the target month 
    int dow = wdayOfFirstOffset(currentTime.tm_wday,currentTime.tm_mday,od);
    
    
    // Cell geometry
    
    int l = 2;      // position of left side of left column
    int b = 167;    // position of bottom of bottom row
    int d = 7;      // number of columns (days of the week)
    int lw = 20;    // width of columns 
    int w = ceil(((float) dow + (float) dom)/7); // number of weeks this month
    
    int bh;    // How tall rows should be depends on how many weeks there are
    if(w == 4)      bh = 30;
    else if(w == 5) bh = 24;
    else            bh = 20;
        
    int r = l+d*lw; // position of right side of right column
    int t = b-w*bh; // position of top of top row
    int cw = lw-1;  // width of textarea
    int cl = l+1;
    int ch = bh-1;
    int i;
    
        
    setColors(ctx);
    
    // Draw the Gridlines
    if(grid){
        // horizontal lines
        for(i=1;i<=w;i++){
            graphics_draw_line(ctx, GPoint(l, b-i*bh), GPoint(r, b-i*bh));
        }
        // vertical lines
        for(i=1;i<d;i++){
            graphics_draw_line(ctx, GPoint(l+i*lw, t), GPoint(l+i*lw, b));
        }
    }
    // Draw days of week
    for(i=0;i<7;i++){
        graphics_text_draw(ctx, daysOfWeek[i],  fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(cl+i*lw, 30, cw, 20), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 
    }
    
    
    
    // Fill in the cells with the month days
    int fh;
    GFont font;
    int wknum = 0;
    
    for(i=1;i<=dom;i++){
    
        // New Weeks begin on Sunday
        if(dow > 6){
            dow = 0;
            wknum ++;
        }

        // Is this today?  If so prep special today style
        if(i==currentTime.tm_mday && offset == 0){
            if(invert){
                setInvColors(ctx);
                graphics_fill_rect(ctx,GRect(l+dow*lw+1, b-(w-wknum)*bh+1, cw, ch),0,GCornerNone);
            }
            font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
            fh = 20;

        // Normal (non-today) style
        }else{
            font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
            fh = 16;
        }
        
        // Draw the day
        graphics_text_draw(ctx, intToStr(i),  font, GRect(cl+dow*lw, b-(-0.5+w-wknum)*bh-fh/2-1, cw, fh), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 
        
        // Fix colors if inverted
        if(invert && offset == 0 && i==currentTime.tm_mday ) setColors(ctx);
        
        // and on to the next day
        dow++;   
    }
}

void month_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    PblTm currentTime;
    get_time(&currentTime);
    
    setColors(ctx);
    
    // Add month offset to current month/year
    int mon = currentTime.tm_mon+offset;
    int year = currentTime.tm_year+1900;
    
    // Fix the momtn and year to be sane values
    while(mon > 11 || mon < 0){
        if(mon>11){
            mon -= 12;
            year++;
        }else if(mon < 0){
            mon += 12;
            year--;
        }
    }
    
    // Build the MONTH YEAR string
    char str[20];
    strcpy (str,months[mon]);
    strcat (str," ");
    strcat (str,intToStr(year));
    
    // Draw the MONTH/YEAR String
    graphics_text_draw(ctx, str,  fonts_get_system_font(FONT_KEY_GOTHIC_24), GRect(0, 0, 144, 30), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}



void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;
    offset--;
    layer_mark_dirty(&month_layer);
    layer_mark_dirty(&days_layer);
}


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;
    offset++;
    layer_mark_dirty(&month_layer);
    layer_mark_dirty(&days_layer);
}

void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    (void)recognizer;
    (void)window;
    offset = 0;
    layer_mark_dirty(&month_layer);
    layer_mark_dirty(&days_layer);
}

void config_provider(ClickConfig **config, Window *window) {
    (void)window;
    config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;
    config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
    config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;
    config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
    config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

    window_init(&window, "Calendar");
    window_stack_push(&window, false /* Animated */);
    window_set_fullscreen(&window, true);
    
    setColors(ctx);
    
    layer_init(&month_layer, window.layer.frame);
    month_layer.update_proc = &month_layer_update_callback;
    layer_add_child(&window.layer, &month_layer);

    layer_init(&days_layer, window.layer.frame);
    days_layer.update_proc = &days_layer_update_callback;
    layer_add_child(&window.layer, &days_layer);
    
    window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);
}

void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
    (void)ctx;
    layer_mark_dirty(&month_layer);
    layer_mark_dirty(&days_layer);
}
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
        .tick_handler = &handle_tick,
        .tick_units = DAY_UNIT
    }
  };
  app_event_loop(params, &handlers);
}


