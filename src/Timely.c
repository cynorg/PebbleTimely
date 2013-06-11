#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <math.h>
#include "resource_ids.auto.h"

/* CONFIGURATION Section
 *
 * If you want to customize things, do so here...
 * If you fork this code and release the resulting app, please be considerate and change all the values in PBL_APP_INFO 
 *
 * comment out this define (// prefix )for the 'light' version, this will set everything appropriately throughout the code.
 */
#define TIMELY_DARK
/* 
 * Set START_OF_WEEK to a number between 0 and 6, 0 being Sunday, 6 being Saturday 
 */
#define START_OF_WEEK 0
/* END CONFIGURATION Section
 *
 *  This watchface shows the current date and current time in the top 'half',
 *    and then a small calendar w/ 3 weeks: last, current, and next week, in the bottom 'half'
 *
 */

#define TIMELY_MAJOR 1
#define TIMELY_MINOR 1
#define UUID_DARK { 0x55, 0xF7, 0x74, 0x4B, 0xF5, 0x86, 0x45, 0xB8, 0x85, 0x4C, 0x55, 0x4F, 0x34, 0x48, 0x9F, 0x21 }
#define UUID_LIGHT { 0x55, 0xF7, 0x74, 0x4B, 0xF5, 0x86, 0x45, 0xB8, 0x85, 0x4C, 0x55, 0x4F, 0x34, 0x48, 0x9F, 0x22 }

#ifdef TIMELY_DARK
PBL_APP_INFO(UUID_DARK,
             "Timely Dark", "Martin Norland (@cynorg)",
             TIMELY_MAJOR, TIMELY_MINOR, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON_DARK,
             APP_INFO_WATCH_FACE);
#else
PBL_APP_INFO(UUID_LIGHT,
             "Timely Light", "Martin Norland (@cynorg)",
             TIMELY_MAJOR, TIMELY_MINOR, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON_LIGHT,
             APP_INFO_WATCH_FACE);

#endif

Window window;

TextLayer text_time_layer;

Layer month_layer;
Layer days_layer;

#ifdef TIMELY_DARK
const bool black = true;       // Is the background black: don't change this, refer to the CONFIGURATION section at the top
#else
const bool black = false;      // Is the background black: don't change this, refer to the CONFIGURATION section at the top
#endif
const bool grid = true;        // Show the grid
const bool invert = true;      // Invert colors on today's date
const bool vibe_hour = false;  // vibrate at the top of the hour?

// Offset days of week. Values can be between 0 and 6.
// 0 = weeks start on Sunday
// 1 =  weeks start on Monday
const int  dayOfWeekOffset = START_OF_WEEK; // don't change this, refer to the CONFIGURATION section at the top

const char daysOfWeek[7][3] = {"Su","Mo","Tu","We","Th","Fr","Sa"};

char* intToStr(int val){

 	static char buf[32] = {0};
	
	int i = 30;	
	for(; val && i ; --i, val /= 10)
		buf[i] = "0123456789"[val % 10];
	
	return &buf[i+1];
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
    
    int j;
    int i;
    
    PblTm currentTime;
    get_time(&currentTime);
    int mon = currentTime.tm_mon;
    int year = currentTime.tm_year+1900;
    int daysThisMonth = daysInMonth(mon,year);

    /* We're going to build an array to hold the dates to be shown in the calendar.
     *
     * There are five 'parts' we'll calculate for this (though since we only display 3 weeks, we'll only ever see at most 4 of them)
     *
     *   daysVisPrevMonth = days from the previous month that are visible
     *   daysPriorToToday = days before today (including any days from previous month)
     *   ( today )
     *   daysAfterToday   = days after today (including any days from next month)
     *   daysVisNextMonth = days from the following month that are visible
     *
     *  daysPriorToToday + 1 + daysAfterToday = 21, since we display exactly 3 weeks.
     */
    int calendar[21];
    int cellNum = 0; // address for current day table cell: 0-20
    int daysVisPrevMonth = 0;
    int daysVisNextMonth = 0;
    int daysPriorToToday = 7 + currentTime.tm_wday - dayOfWeekOffset;
    int daysAfterToday   = 6 - currentTime.tm_wday + dayOfWeekOffset;
    // tm_wday is based on Sunday being the startOfWeek, but Sunday may not be our startOfWeek.
    if (currentTime.tm_wday < dayOfWeekOffset) { 
      daysPriorToToday += 7; // we're <7, so in the 'first' week due to startOfWeek offset - 'add a week' before this one
    } else {
      daysAfterToday += 7;   // otherwise, we're already in the second week, so 'add a week' after
    }
    
    if ( daysPriorToToday >= currentTime.tm_mday ) {
      // We're showing more days before today than exist this month
      int daysInPrevMonth = daysInMonth(mon-1,year); // year only matters for February, which will be the same 'from' March

      // Number of days we'll show from the previous month
      daysVisPrevMonth = daysPriorToToday - currentTime.tm_mday + 1;

      // TODO: trivialoptimize: *could* use just cellNum and drop the i, this time only, since it's the first.
      for( i=0; i<daysVisPrevMonth; i++,cellNum++ ) {
        calendar[cellNum] = daysInPrevMonth + i - daysVisPrevMonth + 1;
      }
    }

    // optimization: instantiate i to a hot mess, since the first day we show this month may not be the 1st of the month
    int firstDayShownThisMonth = (daysVisPrevMonth + currentTime.tm_mday - daysPriorToToday);
    for( i=firstDayShownThisMonth; i<currentTime.tm_mday; i++,cellNum++ ) {
      calendar[cellNum] = i;
    }

    int currentDay = cellNum; // the current day... we'll style this special
    calendar[cellNum] = currentTime.tm_mday;
    cellNum++;

    if ( currentTime.tm_mday + daysAfterToday > daysThisMonth ) {
      daysVisNextMonth = currentTime.tm_mday + daysAfterToday - daysThisMonth;
    }

    // add the days after today until the end of the month/next week, to our array...
    int daysLeftThisMonth = daysAfterToday - daysVisNextMonth;
    for( i=0; i<daysLeftThisMonth; i++,cellNum++ ) {
      calendar[cellNum] = i + currentTime.tm_mday + 1;
    }

    // add any days in the next month to our array...
    for( i=0; i<daysVisNextMonth; i++,cellNum++ ) {
      calendar[cellNum] = i + 1;
    }


// ---------------------------
// Now that we've calculated which days go where, we'll move on to the display logic.
// ---------------------------


    // Cell geometry
    
    int l = 2;      // position of left side of left column
    int b = 167;    // position of bottom of bottom row
    int d = 7;      // number of columns (days of the week)
    int lw = 20;    // width of columns 
    int w = 3;      // always display 3 weeks: previous, current, next
    
    int bh = 20;    // How tall rows should be depends on how many weeks there are
        
    int r = l+d*lw; // position of right side of right column
    int t = b-w*bh; // position of top of top row
    int cw = lw-1;  // width of textarea
    int cl = l+1;
    int ch = bh-1;
        
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

    const char* dayFonts[2] = { FONT_KEY_GOTHIC_14, FONT_KEY_GOTHIC_14_BOLD };

    int whichDayFont = 0; 
    // Draw days of week
    for(i=0;i<7;i++){
    
        whichDayFont = 0; 
        // highlight day of week
        if(i==currentTime.tm_wday){
            whichDayFont = 1; 
        }

        // Adjust labels by specified offset
        j = i+dayOfWeekOffset;
        if(j>6) j-=7;
        if(j<0) j+=7;
        graphics_text_draw(
            ctx, 
            daysOfWeek[j], 
            fonts_get_system_font(dayFonts[whichDayFont]), 
            GRect(cl+i*lw, 90, cw, 20), 
            GTextOverflowModeWordWrap, 
            GTextAlignmentCenter, 
            NULL); 
    }
    
    // Fill in the cells with the month days
    int fh;
    GFont font;
    int wknum = 0;
    int dow = 0;
    
    for(i=0;i<21;i++) {
    
        dow = i%7;
        wknum = (i-dow)/7; 
        // New Weeks begin on Sunday
        if(dow > 6){
            dow = 0;
            wknum ++;
        }

        // Is this today?  If so prep special today style
        if(i==currentDay){
            if(invert){
                setInvColors(ctx);
                graphics_fill_rect(
                    ctx,
                    GRect(
                        l+dow*lw+1, 
                        b-(w-wknum)*bh+1, 
                        cw, 
                        ch)
                    ,0
                    ,GCornerNone);
            }
            font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
            fh = 20;

        // Normal (non-today) style
        }else{
            font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
            fh = 16;
        }

        // Draw the day
        graphics_text_draw(
            ctx, 
            intToStr(calendar[i]),  
            font, 
            GRect(
                cl+dow*lw, 
                b-(-0.5+w-wknum)*bh-fh/2-1, 
                cw, 
                fh), 
            GTextOverflowModeWordWrap, 
            GTextAlignmentCenter, 
            NULL); 
        
        // Fix colors if inverted
        if(invert && i==currentDay ) setColors(ctx);
    
    }

}

void month_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    PblTm currentTime;
    get_time(&currentTime);
    
    setColors(ctx);
    
    char str[20] = ""; 
    // http://www.cplusplus.com/reference/ctime/strftime/
    string_format_time(str, sizeof(str), "%B %d, %Y", &currentTime); // Month DD, YYYY
    //string_format_time(str, sizeof(str), "%d.%m.%Y", &currentTime);  // DD.MM.YYYY

    
    // Draw the MONTH/YEAR String
    graphics_text_draw(
        ctx, 
        str,  
        fonts_get_system_font(FONT_KEY_GOTHIC_24), 
        GRect(0, 0, 144, 30), 
        GTextOverflowModeWordWrap, 
        GTextAlignmentCenter, 
        NULL);
}

void paint_time(PblTm* currentTime) {
  // Need to be static because used by the system later.
  static char time_text[] = "00:00";

  char *time_format;

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  string_format_time(time_text, sizeof(time_text), time_format, currentTime);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(&text_time_layer, time_text);

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
    
    resource_init_current_app(&APP_RESOURCES);

  text_layer_init(&text_time_layer, GRect(0, 26, 144, 168-22));
  if(black){
    text_layer_set_text_color(&text_time_layer, GColorWhite);
    text_layer_set_background_color(&text_time_layer, GColorClear);
  }else{
    text_layer_set_text_color(&text_time_layer, GColorBlack);
    text_layer_set_background_color(&text_time_layer, GColorClear);
  }
  text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  text_layer_set_text_alignment(&text_time_layer, GTextAlignmentCenter);
  layer_add_child(&window.layer, &text_time_layer.layer);

  PblTm currentTime;
  get_time(&currentTime);
  paint_time(&currentTime);

}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {

  (void)ctx;

  paint_time(t->tick_time);
  if (t->tick_time->tm_min == 0 && t->tick_time->tm_sec == 0) {
    // top of the hour, not simply changing to the watchface at 0 minutes past (tick handler is called after init)
    if (vibe_hour) { vibes_short_pulse(); }

    // TODO: Track the date and only call these updates when it's changed.
    // TODO:  this is especially important because marking layers dirty redraws all visible layers
    // TODO:  so it's not even like we're redrawing "just" half the screen every minute.
    // TODO:  http://forums.getpebble.com/discussion/4597/timers-and-click-handlers-dont-mix
    // TODO:  and http://forums.getpebble.com/discussion/4946/layers
    // TODO: Note: it's unclear, right now, if redrawing *any* layer redraws the whole screen ;(
    layer_mark_dirty(&month_layer);
    layer_mark_dirty(&days_layer);
  }
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
        .tick_handler = &handle_minute_tick,
        .tick_units = MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
