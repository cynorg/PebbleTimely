#include <pebble.h>
#define DEBUGLOG 0
#define TRANSLOG 0
#define CONFIG_VERSION "2.3.3"
/*
 * If you fork this code and release the resulting app, please be considerate and change all the appropriate values in appinfo.json 
 *
 * DESCRIPTION
 *  This watchface shows the current date and current time in the top 'half',
 *    and then a small calendar w/ 3 weeks: last, current, and next week, in the bottom 'half'
 *  The statusbar at the top shows the connection status, charging, and battery level - and it will vibrate on link lost.
 *  The settings for the face are configurable using the new PebbleKit JS configuration page
 * END DESCRIPTION Section
 *
 */

static Window *window;

static Layer *battery_layer;
static Layer *datetime_layer;
static TextLayer *date_layer;
static TextLayer *time_layer;
static TextLayer *week_layer;
static TextLayer *ampm_layer;
static TextLayer *day_layer;
static Layer *calendar_layer;
static Layer *splash_layer;
static Layer *weather_layer;
static Layer *statusbar;
static Layer *slot_status;
static Layer *slot_top;
static Layer *slot_bot;
static GFont unifont_16;
static GFont unifont_16_bold;
static GFont cal_normal;
static GFont cal_bold;
static GFont climacons;

static BitmapLayer *bmp_connection_layer;
static GBitmap *image_connection_icon;
static GBitmap *image_noconnection_icon;
static BitmapLayer *bmp_charging_layer;
static GBitmap *image_charging_icon;
static GBitmap *image_hourvibe_icon;
static GBitmap *image_dnd_icon;
static TextLayer *text_connection_layer;
static TextLayer *text_battery_layer;

static InverterLayer *inverter_layer;
static InverterLayer *battery_meter_layer;

// battery info, instantiate to 'worst scenario' to prevent false hopes
static uint8_t battery_percent = 10;
static bool battery_charging = false;
static bool battery_plugged = false;
AppTimer *battery_sending = NULL;
AppTimer *timezone_request = NULL;
AppTimer *weather_request = NULL;
AppTimer *bottom_toggle = NULL;
// connected info
static bool bluetooth_connected = false;
// suppress vibration
static bool vibe_suppression = true;
#define TIMEZONE_UNINITIALIZED 80
static int8_t timezone_offset = TIMEZONE_UNINITIALIZED;
struct tm *currentTime;
static int8_t seconds_shown = 0;
static bool dnd_period_active = false;
static bool vibe_period_active = false;
static bool showing_statusbar = true;

// define the persistent storage key(s)
#define PK_SETTINGS      0
#define PK_LANG_GEN      1 // updated    v10->v11 (utf8)
#define PK_LANG_DATETIME 2 // deprecated v10->v11 (utf8)
#define PK_LANG_MONTHS   3
#define PK_LANG_DAYS     4
#define PK_DEBUGGING     5
#define PK_ADV_SETTINGS  6

// define the appkeys used for appMessages
#define AK_STYLE_INV     0
#define AK_STYLE_DAY_INV 1
#define AK_STYLE_GRID    2
#define AK_VIBE_HOUR     3
#define AK_INTL_DOWO     4
#define AK_INTL_FMT_DATE 5 // INCOMPLETE
#define AK_STYLE_AM_PM   6
#define AK_STYLE_DAY     7
#define AK_STYLE_WEEK    8
#define AK_INTL_FMT_WEEK 9
#define AK_DEBUGGING_ON          10
#define AK_VIBE_PAT_DISCONNECT   11
#define AK_VIBE_PAT_CONNECT      12
#define AK_STRFTIME_FORMAT       13 // not implemented in config page, yet...
#define AK_TRACK_BATTERY         14 // UNUSED
#define AK_LANGUAGE              15
#define AK_DEBUGLANG_ON          16
#define AK_CAL_WEEK_PATTERN      17
#define AK_INV_SLOT_STAT         18 // UNUSED 
#define AK_INV_SLOT_TOP          19 // UNUSED
#define AK_INV_SLOT_BOT          20 // UNUSED
#define AK_SHOW_STAT_BAR         21
#define AK_SHOW_STAT_BATT        22
#define AK_SHOW_DATE             23 // UNUSED
#define AK_DND_START             24
#define AK_DND_STOP              25
#define AK_DND_NOACCEL           26 // UNUSED
#define AK_VIBE_START            27
#define AK_VIBE_STOP             28
#define AK_VIBE_DAYS             29 // UNUSED
#define AK_IDLE_REMINDER         30 // UNUSED
#define AK_IDLE_VIBE_PATT        31 // UNUSED
#define AK_IDLE_MESSAGE          32 // UNUSED
#define AK_IDLE_START            33 // UNUSED
#define AK_IDLE_STOP             34 // UNUSED
#define AK_WEATHER_FMT           35
#define AK_WEATHER_UPDATE        36

#define AK_MESSAGE_TYPE          99
#define AK_SEND_BATT_PERCENT    100
#define AK_SEND_BATT_CHARGING   101
#define AK_SEND_BATT_PLUGGED    102
#define AK_TIMEZONE_OFFSET      103
#define AK_SEND_WATCH_VERSION   104
#define AK_SEND_CONFIG_VERSION  105
#define AK_REQUEST_WEATHER      106
#define AK_WEATHER_TEMP         107
#define AK_WEATHER_COND         108

#define AK_TRANS_ABBR_SUNDAY    500
#define AK_TRANS_ABBR_MONDAY    501
#define AK_TRANS_ABBR_TUESDAY   502
#define AK_TRANS_ABBR_WEDSDAY   503
#define AK_TRANS_ABBR_THURSDAY  504
#define AK_TRANS_ABBR_FRIDAY    505
#define AK_TRANS_ABBR_SATURDAY  506
#define AK_TRANS_JANUARY    507
#define AK_TRANS_FEBRUARY   508
#define AK_TRANS_MARCH      509
#define AK_TRANS_APRIL      510
#define AK_TRANS_MAY        511
#define AK_TRANS_JUNE       512
#define AK_TRANS_JULY       513
#define AK_TRANS_AUGUST     514
#define AK_TRANS_SEPTEMBER  515
#define AK_TRANS_OCTOBER    516
#define AK_TRANS_NOVEMBER   517
#define AK_TRANS_DECEMBER   518
#define AK_TRANS_ALARM      519 // UNUSED
#define AK_TRANS_SUNDAY     520
#define AK_TRANS_MONDAY     521
#define AK_TRANS_TUESDAY    522
#define AK_TRANS_WEDSDAY    523
#define AK_TRANS_THURSDAY   524
#define AK_TRANS_FRIDAY     525
#define AK_TRANS_SATURDAY   526
#define AK_TRANS_CONNECTED     527
#define AK_TRANS_DISCONNECTED  528
#define AK_TRANS_TIME_AM    529
#define AK_TRANS_TIME_PM    530
#define AK_TRANS_ABBR_JANUARY    531
#define AK_TRANS_ABBR_FEBRUARY   532
#define AK_TRANS_ABBR_MARCH      533
#define AK_TRANS_ABBR_APRIL      534
#define AK_TRANS_ABBR_MAY        535
#define AK_TRANS_ABBR_JUNE       536
#define AK_TRANS_ABBR_JULY       537
#define AK_TRANS_ABBR_AUGUST     538
#define AK_TRANS_ABBR_SEPTEMBER  539
#define AK_TRANS_ABBR_OCTOBER    540
#define AK_TRANS_ABBR_NOVEMBER   541
#define AK_TRANS_ABBR_DECEMBER   542

// primary coordinates
#define DEVICE_WIDTH        144
#define DEVICE_HEIGHT       168
#define LAYOUT_STAT           0 // 20 tall
#define LAYOUT_SLOT_TOP      24 // 72 tall
#define LAYOUT_SLOT_BOT      96 // 72 tall, 4px gap above
#define LAYOUT_SLOT_HEIGHT   72
#define STAT_BATT_LEFT       96 // LEFT + WIDTH + NIB_WIDTH <= 143
#define STAT_BATT_TOP         4
#define STAT_BATT_WIDTH      44 // should be divisible by 10, after subtracting 4 (2 pixels/side for the 'border')
#define STAT_BATT_HEIGHT     15
#define STAT_BATT_NIB_WIDTH   3 // >= 3
#define STAT_BATT_NIB_HEIGHT  5 // >= 3
#define STAT_BT_ICON_LEFT    -2 // 0
#define STAT_BT_ICON_TOP      2
#define STAT_CHRG_ICON_LEFT  76
#define STAT_CHRG_ICON_TOP    2

// relative coordinates (relative to SLOTs)
#define REL_CLOCK_DATE_LEFT       2
#define REL_CLOCK_DATE_TOP        0
#define REL_CLOCK_DATE_HEIGHT    30 // date/time overlap, due to the way text is 'positioned'
#define REL_CLOCK_DATE_WIDTH    140
#define REL_CLOCK_TIME_LEFT       0
#define REL_CLOCK_TIME_TOP        7
#define REL_CLOCK_TIME_HEIGHT    60 // date/time overlap, due to the way text is 'positioned'
#define REL_CLOCK_SUBTEXT_TOP    56 // time/ampm overlap, due to the way text is 'positioned'

#define SLOT_ID_CLOCK_1  0
#define SLOT_ID_CALENDAR 1
#define SLOT_ID_WEATHER  2
#define SLOT_ID_CLOCK_2  3

// Create a struct to hold our persistent settings...
typedef struct persist { // 18 bytes
  uint8_t version;                // version key
  uint8_t inverted;               // Invert display
  uint8_t day_invert;             // Invert colors on today's date
  uint8_t grid;                   // Show the grid
  uint8_t vibe_hour;              // vibrate at the top of the hour?
  uint8_t dayOfWeekOffset;        // first day of our week
  uint8_t date_format;            // date format
  uint8_t show_am_pm;             // Show AM/PM below time
  uint8_t show_day;               // Show day name below time
  uint8_t show_week;              // Show week number below time
  uint8_t week_format;            // week format (calculation, e.g. ISO 8601)
  uint8_t vibe_pat_disconnect;    // vibration pattern for disconnect
  uint8_t vibe_pat_connect;       // vibration pattern for connect
  char *strftime_format;          // custom date_format string (date_format = 255)
  uint8_t track_battery;          // track battery information
} __attribute__((__packed__)) persist;

typedef struct persist_months_lang { // 252 bytes
  char monthsNames[12][21];       // 252: 10-20 UTF8 characters for each of 12 months
//                                   252 bytes
} __attribute__((__packed__)) persist_months_lang;

typedef struct persist_days_lang { // 238 bytes
  char DaysOfWeek[7][34];         //  238: 16-33 UTF8 characters for each of 7 weekdays
//                                    238 bytes
} __attribute__((__packed__)) persist_days_lang;

typedef struct persist_general_lang { // 253 bytes
  char statuses[2][26];           //  40: 12-25 characters for each of  2 statuses
  char abbrTime[2][12];           //  24:  5-11 characters for each of  2 abbreviations
  char abbrDaysOfWeek[7][6];      //  42:  2- 5 characters for each of  7 weekdays abbreviations
  char abbrMonthsNames[12][11];   // 132:  5-11 characters for each of 12 months abbreviations
  char language[3];               //   3:  2 characters for language (internal, stored as ascii for convenience)
//                                   253 bytes
} __attribute__((__packed__)) persist_general_lang;

typedef struct persist_debug { // 6 bytes
  bool general;              // debugging messages (general)
  bool language;             // debugging messages (language/translation)
  bool reserved_1;           // debugging messages (reserved to spare updates later)
  bool reserved_2;           // debugging messages (reserved to spare updates later)
  bool reserved_3;           // debugging messages (reserved to spare updates later)
  bool reserved_4;           // debugging messages (reserved to spare updates later)
} __attribute__((__packed__)) persist_debug;

typedef struct persist_adv_settings { // 243 bytes
  uint8_t week_pattern;    //  1 byte
  uint8_t invertStatBar;   //  1 byte
  uint8_t invertTopSlot;   //  1 byte
  uint8_t invertBotSlot;   //  1 byte
  uint8_t showStatus;      //  1 byte
  uint8_t showStatusBat;   //  1 byte
  uint8_t showDate;        //  1 byte
  uint8_t DND_start;       //  1 byte
  uint8_t DND_stop;        //  1 byte
  uint8_t DND_accel_off;   //  1 byte
  uint8_t vibe_hour_start; //  1 byte
  uint8_t vibe_hour_stop;  //  1 byte
  uint8_t vibe_hour_days;  //  1 byte
  uint8_t idle_reminder;   //  1 byte
  uint8_t idle_pattern;    //  1 byte
  char idle_message[32];   // 32 bytes
  uint8_t idle_start;      //  1 byte
  uint8_t idle_stop;       //  1 byte
  int8_t clock2_tz;        //  1 byte
  char clock2_desc[32];    // 32 bytes
  uint8_t weather_format;  //  1 byte
  uint8_t weather_update;  //  1 byte
  char weather_lat[8];     //  8 bytes
  char weather_lon[8];     //  8 bytes
  uint8_t clock_font;      //  1 byte
  uint8_t token_type[2];   //  2 bytes
  char token_code[2][65];  //130 bytes
  uint8_t slots[10];       // 10 bytes
} __attribute__((__packed__)) persist_adv_settings;

typedef struct weather_data {
  int16_t current;            // current temperature
  char condition[2];          // weather_conditions (mapped to single character in font)
} __attribute__((__packed__)) weather_data;
/*
*/

weather_data weather = {
  .current    = 999,
  .condition = {'h'},
};

persist settings = {
  .version    = 11,
  .inverted   = 0, // no, dark
  .day_invert = 1, // yes
  .grid       = 1, // yes
  .vibe_hour  = 0, // no
  .dayOfWeekOffset = 0, // 0 - 6, Sun - Sat
  .date_format = 0, // Month DD, YYYY
  .show_am_pm  = 0, // no AM/PM       [0:Hide, 1:AM/PM, 2:TZ,    3:Week,  4:DoY,  5:DLiY,   6:Seconds]
  .show_day    = 0, // no day name    [0:Hide, 1:Day,   2:Month, 3:TZ,    4:Week, 5:AM/PM   6:DoY/DLiY]
  .show_week   = 0, // no week number [0:Hide, 1:Week,  2:TZ,    3:AM/PM, 4:DoY,  5:DLiY,   6:Seconds]
  .week_format = 0, // ISO 8601
  .vibe_pat_disconnect = 2, // double vibe
  .vibe_pat_connect = 0, // no vibe
  .strftime_format = "%Y-%m-%d",
  .track_battery = 0, // no battery tracking by default
};

persist_months_lang lang_months = {
  .monthsNames = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" },
};

persist_days_lang lang_days = {
  .DaysOfWeek = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" },
};

persist_general_lang lang_gen = {
  .statuses = { "Linked", "NOLINK" },
  .abbrTime = { "AM", "PM" },
  .abbrDaysOfWeek = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" },
  .abbrMonthsNames = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" },
  .language = "EN",
};

persist_debug debug = {
  .general = false,     // debugging disabled by default
  .language  = false,   // debugging disabled by default
  .reserved_1 = false,  // debugging disabled by default
  .reserved_2 = false,  // debugging disabled by default
  .reserved_3 = false,  // debugging disabled by default
  .reserved_4 = false,  // debugging disabled by default
};

persist_adv_settings adv_settings = {
  // Calendar week pattern - which weeks to show
  .week_pattern = 0,    // 0:lcn, 1:llc, 2:cnn, 3:lc, 4:cn, 5:c
  // Inversions
  .invertStatBar = 0,   // 1: invert Statusbar
  .invertTopSlot = 0,   // 1: invert Top Slot
  .invertBotSlot = 0,   // 1: invert Bottom Slot 
  // Status bar auto-hiding...
  .showStatus = 1,      // Status: 0: never(!?), 1: always, 2: only when disconnected/charging/battery, 3: minimal always, 4: minimal #2
  .showStatusBat = 100, // Status: battery percentage above which to hide statusbar if hiding it is allowed
  // Date hiding... because I didn't reserve 0 in date_format ;)
  .showDate = 1,        // Date: 0: never, 1: always
  // DND start/stop (suppress vibrations, Weather updates, pretty much anything except updating the time)
    // ...hopefully this will become pointless with a SDK update which exposes the watches DND...
  .DND_start = 0,       // Do Not Disturb: 10 minute increments, 0 = 12:00am, 144 = 12:00am (following day)
  .DND_stop  = 0,       // Do Not Disturb: 10 minute increments, 0 = 12:00am, 144 = 12:00am (following day)
  .DND_accel_off = 0,   // Do Not Disturb: disable accelerometer polling during DND?
  // hourly vibration start/stop
  .vibe_hour_start = 0, // Hour Vibe: 10 minute increments, 0 = 12:00am, 144 = 12:00am (following day)
  .vibe_hour_stop  = 0, // Hour Vibe: 10 minute increments, 0 = 12:00am, 144 = 12:00am (following day)
  .vibe_hour_days  = 0, // Hour Vibe: days active [Su 1, Mo 2, Tu 4, We 8, Th 16, Fr 32, Sa 64 => 0 => 127]
  // Idle reminders (accelerometer)
  .idle_reminder = 0,   // Idle: 0 = off; minutes 1 - 255 (0:01 to 4:15; 4 1/4 hours = meal/bathroom reminder, haha)
  .idle_pattern = 0,    // Idle: vibration pattern
  .idle_message = { "Let's Move!" }, // Idle: reminder message
  .idle_start = 0,      // Idle: 10 minute increments, 0 = 12:00am, 143 = 12:00am (following day)
  .idle_stop  = 0,      // Idle: 10 minute increments, 0 = 12:00am, 143 = 12:00am (following day)
  // Second Clock
  .clock2_tz = 0,       // 2nd clock: tz offset in 15 minute increments
  .clock2_desc = { "Second Clock" }, // 2nd clock: desc / city name of 2nd clock
  // Weather
  .weather_format = 0,  // Weather: 0: fahrenheit, 1: celsius
  .weather_update = 15, // Weather: minutes between weather updates
  .weather_lat = "",    // latitude for 'static' weather lookups (GPS disabled)
  .weather_lon = "",   // longitude for 'static' weather lookups (GPS disabled)
  // Font
  .clock_font = 1,      // 1: default, 2: ?
  // Tokencode
  .token_type = { 0, 0 },   // Token: Google/other tokencode(s)
  .token_code = { "", "" }, // Token: Tokencode/seed
  // Slots... will require either app mode or fancy acceleration to access this many slots ;)
  .slots = { 0, 1, 2, 3, 0, 1, 0, 1, 0, 1 } // 0 = clock_1, 1 = calendar, 2 = weather, 3 = forecast
};

// How many days are/were in the month
int daysInMonth(int mon, int year)
{
    mon++; // dec = 0|12, lazily optimized

    // April, June, September and November have 30 Days
    if (mon == 4 || mon == 6 || mon == 9 || mon == 11) {
        return 30;
    } else if (mon == 2) {
        // Deal with Feburary & Leap years
        if (year % 400 == 0) {
            return 29;
        } else if (year % 100 == 0) {
            return 28;
        } else if (year % 4 == 0) {
            return 29;
        } else {
            return 28;
        }
    } else {
        // Most months have 31 days
        return 31;
    }
}

struct tm *get_time()
{
    time_t tt = time(0);
    return localtime(&tt);
}

void setColors(GContext* ctx) {
    window_set_background_color(window, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_text_color(ctx, GColorWhite);
}

void setInvColors(GContext* ctx) {
    window_set_background_color(window, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_text_color(ctx, GColorBlack);
}

void weather_layer_update_callback(Layer *me, GContext* ctx) {
  (void)me; // 144x72
  static char temp_current[] = "N/A ";
  static char cond_current[] = "0";
  if (weather.current < 900) {
    snprintf(temp_current, sizeof(temp_current), "%d\u00b0", weather.current);
  } else {
    snprintf(temp_current, sizeof(temp_current), "N/A ");
  }
  snprintf(cond_current, sizeof(cond_current), "%s", weather.condition);

  setColors(ctx);
  graphics_draw_text(ctx, cond_current, climacons, GRect(2,16,34,34), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 
  graphics_draw_text(ctx, temp_current, fonts_get_system_font(FONT_KEY_GOTHIC_24), GRect(2,42,36,36), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 

  if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Weather redrawing: %d, %s", weather.current, weather.condition); }
}

void splash_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me; // 144x72
    setColors(ctx);
    graphics_draw_text(ctx, "Timely", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GRect(0,0,144,36), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 
    graphics_draw_text(ctx, CONFIG_VERSION, fonts_get_system_font(FONT_KEY_GOTHIC_28), GRect(0,32,144,36), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 
}

void calendar_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;

    int mon = currentTime->tm_mon;
    int year = currentTime->tm_year + 1900;
    int daysThisMonth = daysInMonth(mon, year);
    int specialDay = currentTime->tm_wday - settings.dayOfWeekOffset; // specialDay is the column [0-6] which holds the current day
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
    int show_last = 1; // number of previous weeks to show 0-2
    int show_next = 1; // number of future weeks to show 0-2
    switch ( adv_settings.week_pattern ) {
      case 0:
        break;
      case 1:
        show_last = 2; show_next = 0;
        break;
      case 2:
        show_last = 0; show_next = 2;
        break;
/* FIXME: not presently implemented to support this... need to calculate weeks and adjust how we do 'rows' below...
      case 3:
        show_last = 1; show_next = 0;
        break;
      case 4:
        show_last = 0; show_next = 1;
        break;
      case 5:
        show_last = 0; show_next = 0;
        break;
*/
    }

    int calendar[21];
    int cellNum = 0;   // address for current day table cell: 0-20
    int daysVisPrevMonth = 0;
    int daysVisNextMonth = 0;
    int daysPriorToToday = specialDay;
    int daysAfterToday   = 6 - specialDay;

    // tm_wday is based on Sunday being the startOfWeek, but Sunday may not be our startOfWeek.
    if (currentTime->tm_wday < settings.dayOfWeekOffset) { 
        daysPriorToToday += 7 * (show_last+1); // we're <7, so in the 'first' week due to startOfWeek offset - 'add a week' before this one
        specialDay += 7;
    } else {
      daysPriorToToday += 7 * show_last; // 0 = unchanged
    }
    daysAfterToday += 7 * show_next; // 0 = unchanged

    if ( daysPriorToToday >= currentTime->tm_mday ) {
      // We're showing more days before today than exist this month
      int daysInPrevMonth = daysInMonth(mon - 1,year); // year only matters for February, which will be the same 'from' March

      // Number of days we'll show from the previous month
      daysVisPrevMonth = daysPriorToToday - currentTime->tm_mday + 1;

      for (int i = 0; i < daysVisPrevMonth; i++, cellNum++ ) {
        calendar[cellNum] = daysInPrevMonth + i - daysVisPrevMonth + 1;
      }
    }

    // optimization: instantiate i to a hot mess, since the first day we show this month may not be the 1st of the month
    int firstDayShownThisMonth = daysVisPrevMonth + currentTime->tm_mday - daysPriorToToday;
    for (int i = firstDayShownThisMonth; i < currentTime->tm_mday; i++, cellNum++ ) {
      calendar[cellNum] = i;
    }

    //int currentDay = cellNum; // the current day... we'll style this special
    calendar[cellNum] = currentTime->tm_mday;
    cellNum++;

    if ( currentTime->tm_mday + daysAfterToday > daysThisMonth ) {
      daysVisNextMonth = currentTime->tm_mday + daysAfterToday - daysThisMonth;
    }

    // add the days after today until the end of the month/next week, to our array...
    int daysLeftThisMonth = daysAfterToday - daysVisNextMonth;
    for (int i = 0; i < daysLeftThisMonth; i++, cellNum++ ) {
      calendar[cellNum] = i + currentTime->tm_mday + 1;
    }

    // add any days in the next month to our array...
    for (int i = 0; i < daysVisNextMonth; i++, cellNum++ ) {
      calendar[cellNum] = i + 1;
    }

    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Calendar - DOWO: %d, wday: %d, sDay: %d, dVPM: %d, dVNM: %d, dPTT: %d, dAT: %d", settings.dayOfWeekOffset, currentTime->tm_wday, specialDay, daysVisPrevMonth, daysVisNextMonth, daysPriorToToday, daysAfterToday); }

// ---------------------------
// Now that we've calculated which days go where, we'll move on to the display logic.
// ---------------------------

    #define CAL_DAYS   7   // number of columns (days of the week)
    #define CAL_WIDTH  20  // width of columns
    #define CAL_GAP    1   // gap around calendar
    #define CAL_LEFT   2   // left side of calendar
    #define CAL_HEIGHT 18  // How tall rows should be depends on how many weeks there are

    int weeks  =  3;  // always display 3 weeks: # previous, current, # next
        
    GFont current = cal_normal;
    int font_vert_offset = 0;
    if (strcmp(lang_gen.language,"RU") == 0 ) { font_vert_offset = -2; }

    // generate a light background for the calendar grid
    if (settings.grid) {
      setInvColors(ctx);
      graphics_fill_rect(ctx, GRect (CAL_LEFT + CAL_GAP, CAL_HEIGHT - CAL_GAP, DEVICE_WIDTH - 2 * (CAL_LEFT + CAL_GAP), CAL_HEIGHT * weeks), 0, GCornerNone);
      setColors(ctx);
    }
    for (int col = 0; col < CAL_DAYS; col++) {

      // Adjust labels by specified offset
      int weekday = col + settings.dayOfWeekOffset;
      if (weekday > 6) { weekday -= 7; }

      if (col == specialDay) {
        current = cal_bold;
        font_vert_offset = -3;
        if (strcmp(lang_gen.language,"RU") == 0 ) { font_vert_offset = -2; }
      }
      // draw the cell background
    //  graphics_fill_rect(ctx, GRect (CAL_WIDTH * col + CAL_LEFT + CAL_GAP, 0, CAL_WIDTH - CAL_GAP, CAL_HEIGHT - CAL_GAP), 0, GCornerNone);

      // draw the cell text
      graphics_draw_text(ctx, lang_gen.abbrDaysOfWeek[weekday], current, GRect(CAL_WIDTH * col + CAL_LEFT + CAL_GAP, CAL_GAP + font_vert_offset, CAL_WIDTH, CAL_HEIGHT), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 
      if (col == specialDay) {
        if (strcmp(lang_gen.language,"RU") == 0 ) {
          // we don't actually have a bold font for this, so we'll use font double-striking to simulate bold
          graphics_draw_text(ctx, lang_gen.abbrDaysOfWeek[weekday], current, GRect(CAL_WIDTH * col + CAL_LEFT + CAL_GAP + 1, CAL_GAP + font_vert_offset, CAL_WIDTH, CAL_HEIGHT), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 
        }
        current = cal_normal;
        font_vert_offset = 0;
        if (strcmp(lang_gen.language,"RU") == 0 ) { font_vert_offset = -2; }
      }
    }

    GFont normal = fonts_get_system_font(FONT_KEY_GOTHIC_14); // fh = 16
    GFont bold   = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD); // fh = 22
    current = normal;
    font_vert_offset = 0;

    // draw the individual calendar rows/columns
    int week = 0;
    int specialRow = show_last+1;
    
    for (int row = 1; row <= 3; row++) {
      week++;
      for (int col = 0; col < CAL_DAYS; col++) {
        if ( row == specialRow && col == specialDay) {
          if (settings.day_invert) {
            setInvColors(ctx);
          }
          current = bold;
          font_vert_offset = -3;
        }

        // draw the cell background
        graphics_fill_rect(ctx, GRect (CAL_WIDTH * col + CAL_LEFT + CAL_GAP, CAL_HEIGHT * week, CAL_WIDTH - CAL_GAP, CAL_HEIGHT - CAL_GAP), 0, GCornerNone);

        // draw the cell text
        char date_text[3];
        snprintf(date_text, sizeof(date_text), "%d", calendar[col + 7 * (row - 1)]);
        graphics_draw_text(ctx, date_text, current, GRect(CAL_WIDTH * col + CAL_LEFT, CAL_HEIGHT * week - CAL_GAP + font_vert_offset, CAL_WIDTH, CAL_HEIGHT), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 

        if ( row == specialRow && col == specialDay) {
          setColors(ctx);
          current = normal;
          font_vert_offset = 0;
        }
      }
    }
}

void update_date_text() {

    //September 11, 2013 => 18 chars, 9 of which could potentially be dual byte utf8 characters
    //123456789012345678
    const char *datestr[] = {
      "%d.%m.%Y", // 215 DD.MM.YYYY
      "%d-%m-%Y", // 216 DD-MM-YYYY
      "%d/%m/%Y", // 217 DD/MM/YYYY
      "%d %m %Y", // 218 DD MM YYYY
      "%d%m%Y",   // 219 DDMMYYYY
      // DD MM YY (%d %m %y)
      "%d.%m.%y", // 220 DD.MM.YY
      "%d-%m-%y", // 221 DD-MM-YY
      "%d/%m/%y", // 222 DD/MM/YY
      "%d %m %y", // 223 DD MM YY
      "%d%m%y",   // 224 DDMMYY
      // dd MM YYYY (%e %m %Y)
      "%e.%m.%Y", // 225 dd.MM.YYYY
      "%e-%m-%Y", // 226 dd-MM-YYYY
      "%e/%m/%Y", // 227 dd/MM/YYYY
      "%e %m %Y", // 228 dd MM YYYY
      "%e%m%Y",   // 229 ddMMYYYY
      // dd MM YY (%e %m %y)
      "%e.%m.%y", // 230 dd.MM.YY
      "%e-%m-%y", // 231 dd-MM-YY
      "%e/%m/%y", // 232 dd/MM/YY
      "%e %m %y", // 233 dd MM YY
      "%e%m%y"   // 234 ddMMYY
      // YYYY MM DD (%Y %m %d)
      "%Y.%m.%d", // 235 YYYY.MM.DD
      "%Y-%m-%d", // 236 YYYY-MM-DD
      "%Y/%m/%d", // 237 YYYY/MM/DD
      "%Y %m %d", // 238 YYYY MM DD
      "%Y%m%d",   // 239 YYYYMMDD
      // YY MM DD (%y %m %d)
      "%y.%m.%d", // 240 YY.MM.DD
      "%y-%m-%d", // 241 YY-MM-DD
      "%y/%m/%d", // 242 YY/MM/DD
      "%y %m %d", // 243 YY MM DD
      "%y%m%d",   // 244 YYMMDD
      // YYYY MM dd (%Y %m %e)
      "%Y.%m.%e", // 245 YYYY.MM.dd
      "%Y-%m-%e", // 246 YYYY-MM-dd
      "%Y/%m/%e", // 247 YYYY/MM/dd
      "%Y %m %e", // 248 YYYY MM dd
      "%Y%m%e",   // 249 YYYYMMdd
      // YY MM dd (%y %m %e)
      "%y.%m.%e", // 250 YY.MM.dd
      "%y-%m-%e", // 251 YY-MM-dd
      "%y/%m/%e", // 246 YY/MM/dd
      "%y %m %e", // 247 YY MM dd
      "%y%m%e",   // 248 YYMMdd
    };
    char date_text[24];
    static char date_string[48];
    // http://www.cplusplus.com/reference/ctime/strftime/

    if (settings.date_format < 215) { // localized date formats...
      char date_text_2[24];
      switch ( settings.date_format ) {
      case 0: // MMMM DD, YYYY (localized)
        strftime(date_text, sizeof(date_text), "%d, %Y", currentTime); // DD, YYYY
        snprintf(date_string, sizeof(date_string), "%s %s", lang_months.monthsNames[currentTime->tm_mon], date_text); // prefix Month
        break;
      case 1: // MMMM DD, 'YY (localized)
        strftime(date_text, sizeof(date_text), "%d, '%y", currentTime); // DD, 'YY
        snprintf(date_string, sizeof(date_string), "%s %s", lang_months.monthsNames[currentTime->tm_mon], date_text); // prefix Month
        break;
      case 2: // Mmm DD, YYYY (localized)
        strftime(date_text, sizeof(date_text), "%d, %Y", currentTime); // DD, YYYY
        snprintf(date_string, sizeof(date_string), "%s %s", lang_gen.abbrMonthsNames[currentTime->tm_mon], date_text); // prefix Mon
        break;
      case 3: // Mmm DD, 'YY (localized)
        strftime(date_text, sizeof(date_text), "%d, '%y", currentTime); // DD, 'YY
        snprintf(date_string, sizeof(date_string), "%s %s", lang_gen.abbrMonthsNames[currentTime->tm_mon], date_text); // prefix Mon
        break;
      case 11: // D MMMM YYYY (localized)
        strftime(date_text, sizeof(date_text), "%d", currentTime); // D
        strftime(date_text_2, sizeof(date_text_2), "%Y", currentTime); // YYYY
        snprintf(date_string, sizeof(date_string), "%s %s %s", date_text_2, lang_months.monthsNames[currentTime->tm_mon], date_text); // insert Month
        break;
      case 12: // D MMMM 'YY (localized)
        strftime(date_text, sizeof(date_text), "%d", currentTime); // D
        strftime(date_text_2, sizeof(date_text_2), "'%y", currentTime); // YY
        snprintf(date_string, sizeof(date_string), "%s %s %s", date_text_2, lang_months.monthsNames[currentTime->tm_mon], date_text); // insert Month
        break;
      case 13: // D Mmm YYYY (localized)
        strftime(date_text, sizeof(date_text), "%d", currentTime); // D
        strftime(date_text_2, sizeof(date_text_2), "%Y", currentTime); // YYYY
        snprintf(date_string, sizeof(date_string), "%s %s %s", date_text_2, lang_gen.abbrMonthsNames[currentTime->tm_mon], date_text); // insert Mon
        break;
      case 14: // D Mmm 'YY (localized)
        strftime(date_text, sizeof(date_text), "%d", currentTime); // D
        strftime(date_text_2, sizeof(date_text_2), "'%y", currentTime); // YY
        snprintf(date_string, sizeof(date_string), "%s %s %s", date_text_2, lang_gen.abbrMonthsNames[currentTime->tm_mon], date_text); // insert Mon
        break;
      }
    } else { // non-localized date formats, straight strftime function calls
      if ((settings.date_format>=215)||(settings.date_format<=254)) { // load from table
        strftime(date_text, sizeof(date_text), datestr[settings.date_format-215], currentTime);
      } else if (settings.date_format==255) {
        strftime(date_text, sizeof(date_text), settings.strftime_format, currentTime);  
      }

      snprintf(date_string, sizeof(date_string), "%s", date_text); // straight copy
    }

    text_layer_set_text(date_layer, date_string);
}

void update_time_text() {
  // Need to be static because used by the system later.
  static char time_text[] = "00:00";

  char *time_format;

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, currentTime);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  // I would love to just use clock_copy_time_string, but it refuses to center properly in 12-hour time (see Kludge above).
  //clock_copy_time_string(time_text, sizeof(time_text));
  text_layer_set_text(time_layer, time_text);

}

void update_day_text(TextLayer *which_layer) {
  text_layer_set_text(which_layer, lang_days.DaysOfWeek[currentTime->tm_wday]);
}

void update_month_text(TextLayer *which_layer) {
  text_layer_set_text(which_layer, lang_months.monthsNames[currentTime->tm_mon]);
}

void update_week_text(TextLayer *which_layer) {
  static char week_text[] = "W00";
  if (settings.week_format == 0) {
    // ISO 8601 week number (00-53)
    strftime(week_text, sizeof(week_text), "W%V", currentTime);
  } else if (settings.week_format == 1) {
    // Week number with the first Sunday as the first day of week one (00-53)
    strftime(week_text, sizeof(week_text), "W%U", currentTime);
  } else if (settings.week_format == 2) {
    // Week number with the first Monday as the first day of week one (00-53)
    strftime(week_text, sizeof(week_text), "W%W", currentTime);
  }
  text_layer_set_text(which_layer, week_text);
}

void update_ampm_text(TextLayer *which_layer) {
  if (currentTime->tm_hour < 12 ) {
    text_layer_set_text(which_layer, lang_gen.abbrTime[0]); //  0-11 AM
  } else {
    text_layer_set_text(which_layer, lang_gen.abbrTime[1]); // 12-23 PM
  }
}

void update_seconds_text(TextLayer *which_layer) {
  static char seconds_text[] = "00"; // 00-61
  strftime(seconds_text, sizeof(seconds_text), "%S", currentTime);
  text_layer_set_text(which_layer, seconds_text);
}

char * get_doy_text() {
  static char doy_text[] = "D000";
  strftime(doy_text, sizeof(doy_text), "D%j", currentTime);
  return doy_text;
}

char * get_dliy_text() {
  static char dliy_text[] = "R000";
  int daysThisFeb = daysInMonth(currentTime->tm_mon, currentTime->tm_year + 1900);
  int daysThisYear = 365;
  if (daysThisFeb == 29) { daysThisYear = 366; }
  int daysSinceJanFirst = currentTime->tm_yday; // 0-365 inclusive
  int daysLeftThisYear = daysThisYear - daysSinceJanFirst - 1;
  snprintf(dliy_text, sizeof(dliy_text), "R%03d", daysLeftThisYear);
  return dliy_text;
}

void update_doy_text(TextLayer *which_layer) {
  text_layer_set_text(which_layer, get_doy_text());
}

void update_dliy_text(TextLayer *which_layer) {
  text_layer_set_text(which_layer, get_dliy_text());
}

void update_doy_dliy_text(TextLayer *which_layer) {
  static char doy_dliy_text[] = "D000/R000";
  snprintf(doy_dliy_text, sizeof(doy_dliy_text), "%s/%s", get_doy_text(), get_dliy_text());
  text_layer_set_text(which_layer, doy_dliy_text);
}

void update_timezone_text(TextLayer *which_layer) {
  static char timezone_text[10];
  int tz_hours = 0;
  int tz_mins  = 0;
  tz_mins  = timezone_offset % 4;
  tz_hours = (timezone_offset - tz_mins)/ 4;
  tz_mins  = tz_mins * 15;
  if (timezone_offset == TIMEZONE_UNINITIALIZED) {
    snprintf(timezone_text, sizeof(timezone_text), "UTC ?");
  } else if (timezone_offset > 0) {
    if (tz_mins == 0) {
      //snprintf(timezone_text, sizeof(timezone_text), "UTC-%d:00", tz_hours);
      snprintf(timezone_text, sizeof(timezone_text), "UTC-%d", tz_hours);
    } else {
      snprintf(timezone_text, sizeof(timezone_text), "UTC-%d:%d", tz_hours, tz_mins);
    }
  } else {
    if (tz_mins == 0) {
      //snprintf(timezone_text, sizeof(timezone_text), "UTC+%d:00", abs(tz_hours));
      snprintf(timezone_text, sizeof(timezone_text), "UTC+%d", abs(tz_hours));
    } else {
      snprintf(timezone_text, sizeof(timezone_text), "UTC+%d:%d", abs(tz_hours), abs(tz_mins));
    }
  }
  text_layer_set_text(which_layer, timezone_text);
}

void process_show_week() { // LEFT
  switch ( settings.show_week ) {
  case 0: // Hide
    //layer_set_hidden(text_layer_get_layer(week_layer), true);
    return;
  case 1: // Show Week
    update_week_text(week_layer);
    break;
  case 2: // Show Timezone
    update_timezone_text(week_layer);
    break;
  case 3: // Show AM/PM
    update_ampm_text(week_layer);
    break;
  case 4: // Show Day of Year
    update_doy_text(week_layer);
    break;
  case 5: // Show Days Left in Year
    update_dliy_text(week_layer);
    break;
  case 6: // Show Seconds
    update_seconds_text(week_layer);
    break;
  }
}

void process_show_day() { // MIDDLE
  switch ( settings.show_day ) {
  case 0: // Hide
    //layer_set_hidden(text_layer_get_layer(day_layer), true);
    return;
  case 1: // Show Day
    update_day_text(day_layer);
    break;
  case 2: // Show Month
    update_month_text(day_layer);
    break;
  case 3: // Show Timezone
    update_timezone_text(day_layer);
    break;
  case 4: // Show Week
    update_week_text(day_layer);
    break;
  case 5: // Show AM/PM
    update_ampm_text(day_layer);
    break;
  case 6: // Show DoY/DLiY
    update_doy_dliy_text(day_layer);
    break;
  }
}

void process_show_ampm() { // RIGHT
  switch ( settings.show_am_pm ) {
  case 0: // Hide
    //layer_set_hidden(text_layer_get_layer(ampm_layer), true);
    return;
  case 1: // Show AM/PM
    update_ampm_text(ampm_layer);
    break;
  case 2: // Show Timezone
    update_timezone_text(ampm_layer);
    break;
  case 3: // Show Week
    update_week_text(ampm_layer);
    break;
  case 4: // Show Day of Year
    update_doy_text(ampm_layer);
    break;
  case 5: // Show Days Left in Year
    update_dliy_text(ampm_layer);
    break;
  case 6: // Show Seconds
    update_seconds_text(ampm_layer);
    break;
  }
}

void position_connection_layer() {
  static int connection_vert_offset = 0;
  // potentially adjust the connection position, depending on language/font
  if ( strcmp(lang_gen.language,"RU") == 0 ) { // Unicode font w/ Cyrillic characters
    connection_vert_offset = 2;
  } else { // Standard font
    connection_vert_offset = 0;
  }
  layer_set_frame( text_layer_get_layer(text_connection_layer), GRect(20+STAT_BT_ICON_LEFT, connection_vert_offset, 72, 22) );
}

void position_date_layer() {
  static int date_vert_offset = 0;
  // potentially adjust the date position, depending on language/font
  if ( strcmp(lang_gen.language,"RU") == 0 ) { // Unicode font w/ Cyrillic characters
    if (showing_statusbar) {
      date_vert_offset = -4;
    } else {
      date_vert_offset = 0;
    }
  } else { // Standard font (EN, etc.)
    if (showing_statusbar) {
      date_vert_offset = -9;
    } else {
      date_vert_offset = -5;
    }
  }
  layer_set_frame( text_layer_get_layer(date_layer), GRect(REL_CLOCK_DATE_LEFT, REL_CLOCK_DATE_TOP + date_vert_offset, REL_CLOCK_DATE_WIDTH, REL_CLOCK_DATE_HEIGHT) );
}

void position_day_layer() {
  // potentially adjust the day position, depending on language/font
  static int day_vert_offset = 0;
  if ( strcmp(lang_gen.language,"RU") == 0 ) { // Unicode font w/ Cyrillic characters
    day_vert_offset = -2;
  } else { // Standard font
    day_vert_offset = 0;
  }
  layer_set_frame( text_layer_get_layer(day_layer), GRect(REL_CLOCK_DATE_LEFT, REL_CLOCK_SUBTEXT_TOP + day_vert_offset, REL_CLOCK_DATE_WIDTH, REL_CLOCK_DATE_HEIGHT) );
}

void position_time_layer() {
  // potentially adjust the clock position, if we've added/removed the week, day, or AM/PM layers
  static int time_offset = 0;
  static int weather_offset = 0;
  if (!settings.show_day && !settings.show_week && !settings.show_am_pm) {
    time_offset = 12;
    weather_offset = 0;
  } else {
    time_offset = 2;
    weather_offset = -10;
  }
  layer_set_frame( text_layer_get_layer(time_layer), GRect(REL_CLOCK_TIME_LEFT, REL_CLOCK_TIME_TOP + time_offset, DEVICE_WIDTH, REL_CLOCK_TIME_HEIGHT) );
  layer_set_frame( weather_layer, GRect(REL_CLOCK_TIME_LEFT, weather_offset, DEVICE_WIDTH, LAYOUT_SLOT_HEIGHT) );
}

void update_datetime_subtext() {
    process_show_week();
    process_show_day();
    process_show_ampm();
    position_time_layer();
}

void datetime_layer_update_callback(Layer *me, GContext* ctx) {
    (void)me;
    // TODO: these calls can probably be reduced/optimized, but apparently not removed entirely!
    setColors(ctx);
    update_date_text();
    update_time_text();
    update_datetime_subtext();
}

void statusbar_visible() {
  if (adv_settings.showStatus == 0) {
    showing_statusbar = false;
  } else if (adv_settings.showStatus == 1) {
    showing_statusbar = true;
  } else if (battery_percent <= adv_settings.showStatusBat) {
    showing_statusbar = true;
  } else {
    showing_statusbar = false;
  }
}

void toggle_weather() {
  if (adv_settings.weather_update) {
    //if (!showing_statusbar) { text_layer_set_text_alignment(date_layer, GTextAlignmentRight); }
    text_layer_set_text_alignment(time_layer, GTextAlignmentRight);
    layer_set_hidden(weather_layer, false);
  } else {
    layer_set_hidden(weather_layer, true);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    //text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  }
}

void toggle_statusbar() {
  if (showing_statusbar) {
    // status
    layer_set_hidden(statusbar, false);
    // date
    layer_add_child(datetime_layer, text_layer_get_layer(date_layer));
    if (adv_settings.weather_update && (settings.show_day || settings.show_week || settings.show_am_pm)) {
      text_layer_set_text_alignment(date_layer, GTextAlignmentRight);
    } else {
      text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    }
    // icon(s)
    layer_add_child(statusbar, bitmap_layer_get_layer(bmp_charging_layer));
    //layer_set_frame( bitmap_layer_get_layer(bmp_charging_layer), GRect(STAT_CHRG_ICON_LEFT, STAT_CHRG_ICON_TOP, 20, 20) );
    layer_add_child(statusbar, battery_layer);
    layer_add_child(statusbar, inverter_layer_get_layer(battery_meter_layer));
  } else {
    // status
    layer_set_hidden(statusbar, true);
    // date
    layer_add_child(slot_status, text_layer_get_layer(date_layer));
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    // icon(s)
    layer_add_child(datetime_layer, bitmap_layer_get_layer(bmp_charging_layer));
    //layer_set_frame( bitmap_layer_get_layer(bmp_charging_layer), GRect(124, -2, 20, 20) );
    layer_add_child(datetime_layer, battery_layer);
    layer_add_child(datetime_layer, inverter_layer_get_layer(battery_meter_layer));
  }
  position_date_layer();
}

void slot_status_layer_update_callback(Layer *me, GContext* ctx) {
}

void statusbar_layer_update_callback(Layer *me, GContext* ctx) {
// XXX positioning tests... only valid if we leave statusbar's frame/bounds set to the whole watch...
/*
    setColors(ctx);
    graphics_draw_rect(ctx, GRect(0,  0, 144, 24)); // statusbar
    graphics_draw_rect(ctx, GRect(0, 24, 144, 72)); // top half
    graphics_draw_rect(ctx, GRect(0, 96, 144, 72)); // bottom half
*
    graphics_draw_rect(ctx, GRect(0, 50, 20, 20)); // linked
    graphics_draw_rect(ctx, GRect(0, 72, 20, 20)); // icon 2
    graphics_draw_rect(ctx, GRect(0, 50, 10, 42)); // battery l
    graphics_draw_rect(ctx, GRect(144-10, 50, 10, 42)); // battery r
    graphics_draw_rect(ctx, GRect(144-20, 50, 20, 20)); // icon 3
    graphics_draw_rect(ctx, GRect(144-20, 72, 20, 20)); // icon 4
    graphics_draw_rect(ctx, GRect(0, 46, 144, 50)); // targeting time
*/
}

void slot_top_layer_update_callback(Layer *me, GContext* ctx) {
// TODO: configurable: draw appropriate slot
}

void slot_bot_layer_update_callback(Layer *me, GContext* ctx) {
// TODO: configurable: draw appropriate slot
}

void battery_layer_update_callback(Layer *me, GContext* ctx) {
// simply draw the battery outline here - the text is a different layer, and we then 'fill' it with an inverterLayer
  setColors(ctx);
// battery outline
  graphics_draw_rect(ctx, GRect(STAT_BATT_LEFT, STAT_BATT_TOP, STAT_BATT_WIDTH, STAT_BATT_HEIGHT));
// battery 'nib' terminal
  graphics_draw_rect(ctx, GRect(STAT_BATT_LEFT + STAT_BATT_WIDTH - 1,
                                STAT_BATT_TOP + (STAT_BATT_HEIGHT - STAT_BATT_NIB_HEIGHT)/2,
                                STAT_BATT_NIB_WIDTH,
                                STAT_BATT_NIB_HEIGHT));
}

static void request_weather(void *data) {
  strncpy(weather.condition, "h", sizeof(weather.condition)-1); // h = updating 'cloud' icon
  layer_mark_dirty(weather_layer); // update UI element to indicate we're fetching weather...
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (iter == NULL) {
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "iterator is null: %d", result); }
    return;
  }
  if (dict_write_uint8(iter, AK_MESSAGE_TYPE, AK_REQUEST_WEATHER) != DICT_OK) {
    return;
  }
  if (dict_write_uint8(iter, AK_WEATHER_FMT, adv_settings.weather_format) != DICT_OK) {
    return;
  }
  app_message_outbox_send();
  weather_request = NULL;
}

static void request_timezone(void *data) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (iter == NULL) {
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "iterator is null: %d", result); }
    return;
  }
  if (dict_write_uint8(iter, AK_MESSAGE_TYPE, AK_TIMEZONE_OFFSET) != DICT_OK) {
    return;
  }
  app_message_outbox_send();
  timezone_request = NULL;
}

static void watch_version_send(void *data) {
  DictionaryIterator *iter;

  AppMessageResult result = app_message_outbox_begin(&iter);

  if (iter == NULL) {
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "iterator is null: %d", result); }
    return;
  }

  if (result != APP_MSG_OK) {
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Dict write failed to open outbox: %d", (AppMessageResult) result); }
    return;
  }

  if (dict_write_uint8(iter, AK_MESSAGE_TYPE, AK_SEND_WATCH_VERSION) != DICT_OK) {
    return;
  }
  if (dict_write_uint8(iter, AK_SEND_WATCH_VERSION, settings.version) != DICT_OK) {
    return;
  }
  if (dict_write_cstring(iter, AK_SEND_CONFIG_VERSION, CONFIG_VERSION) != DICT_OK) {
    return;
  }
  app_message_outbox_send();
}

static void battery_status_send(void *data) {
  static uint8_t sent_battery_percent = 10;
  static bool sent_battery_charging = false;
  static bool sent_battery_plugged = false;
  if (!settings.track_battery) {
    return; // if user has chosen not to track battery (saves power w/ appmessages)
  }
  if ( (battery_percent  == sent_battery_percent  )
     & (battery_charging == sent_battery_charging )
     & (battery_plugged  == sent_battery_plugged  ) ) {
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "repeat battery reading"); }
    battery_sending = NULL;
    return; // no need to resend the same value
  }
  DictionaryIterator *iter;

  AppMessageResult result = app_message_outbox_begin(&iter);

  if (iter == NULL) {
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "iterator is null: %d", result); }
    return;
  }

  if (result != APP_MSG_OK) {
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Dict write failed to open outbox: %d", (AppMessageResult) result); }
    return;
  }

  if (dict_write_uint8(iter, AK_MESSAGE_TYPE, AK_SEND_BATT_PERCENT) != DICT_OK) {
    return;
  }
  if (dict_write_uint8(iter, AK_SEND_BATT_PERCENT, battery_percent) != DICT_OK) {
    return;
  }
  if (dict_write_uint8(iter, AK_SEND_BATT_CHARGING, battery_charging ? 1: 0) != DICT_OK) {
    return;
  }
  if (dict_write_uint8(iter, AK_SEND_BATT_PLUGGED, battery_plugged ? 1: 0) != DICT_OK) {
    return;
  }
  app_message_outbox_send();
  sent_battery_percent  = battery_percent;
  sent_battery_charging = battery_charging;
  sent_battery_plugged  = battery_plugged;
  battery_sending = NULL;
}

void set_status_charging_icon() {
  // this icon shows either DND, hourly vibration, or charging...
  if (battery_charging) { // charging
    layer_set_hidden(bitmap_layer_get_layer(bmp_charging_layer), false);
    bitmap_layer_set_bitmap(bmp_charging_layer, image_charging_icon);
  } else { // not charging
    if (dnd_period_active) {
      layer_set_hidden(bitmap_layer_get_layer(bmp_charging_layer), false);
      bitmap_layer_set_bitmap(bmp_charging_layer, image_dnd_icon);
    } else {
      if (battery_plugged) { // plugged but not charging = charging complete...
        layer_set_hidden(bitmap_layer_get_layer(bmp_charging_layer), true);
      } else { // normal wear
        if (settings.vibe_hour && vibe_period_active) {
          layer_set_hidden(bitmap_layer_get_layer(bmp_charging_layer), false);
          bitmap_layer_set_bitmap(bmp_charging_layer, image_hourvibe_icon);
        } else {
          layer_set_hidden(bitmap_layer_get_layer(bmp_charging_layer), true);
        }
      }
    }
  }
}

static void toggle_slot_bottom(void *data) {
  watch_version_send(NULL); // no guarantee the JS is there to receive me...
  static Layer* last = NULL;
  Layer* which = (Layer*)data;
  if (last != NULL) { layer_set_hidden(last, true); } // hide visible layer 
  last = which; // we're about to show a layer, mark it as the visible layer 
  if (last != NULL) { layer_set_hidden(last, false); } // show new visible layer 
  bottom_toggle = NULL;
}

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100";

  battery_percent = charge_state.charge_percent;
  uint8_t battery_meter = battery_percent/10*(STAT_BATT_WIDTH-4)/10;
  battery_charging = charge_state.is_charging;
  battery_plugged = charge_state.is_plugged;

  // fill it in with current power
  layer_set_bounds(inverter_layer_get_layer(battery_meter_layer), GRect(STAT_BATT_LEFT+2, STAT_BATT_TOP+2, battery_meter, STAT_BATT_HEIGHT-4));
  layer_set_hidden(inverter_layer_get_layer(battery_meter_layer), false);

  //if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "battery reading"); }
  if (battery_sending == NULL) {
    // multiple battery events can fire in rapid succession, we'll let it settle down before logging it
    battery_sending = app_timer_register(5000, &battery_status_send, NULL);
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "battery timer queued"); }
  }

  set_status_charging_icon();

  snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
  text_layer_set_text(text_battery_layer, battery_text);
  layer_mark_dirty(battery_layer);
  statusbar_visible();
  toggle_statusbar();
}

void generate_vibe(uint32_t vibe_pattern_number) {
  if (vibe_suppression) { return; }
  vibes_cancel();
  switch ( vibe_pattern_number ) {
  case 0: // No Vibration
    return;
  case 1: // Single short
    vibes_short_pulse();
    break;
  case 2: // Double short
    vibes_double_pulse();
    break;
  case 3: // Triple
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {200, 100, 200, 100, 200},
      .num_segments = 5
    } );
  case 4: // Long
    vibes_long_pulse();
    break;
  case 5: // Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {50, 200, 50, 200, 50, 200, 50},
      .num_segments = 7
    } );
    break;
  case 6: // Less Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {100, 200, 100, 200, 100, 200, 100},
      .num_segments = 7
    } );
    break;
  case 7: // Not Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {500, 250, 500, 250, 500, 250, 500},
      .num_segments = 7
    } );
    break;
  default: // No Vibration
    return;
  }
}

void update_connection() {
  text_layer_set_text(text_connection_layer, bluetooth_connected ? lang_gen.statuses[0] : lang_gen.statuses[1]) ;
  if (bluetooth_connected) {
    generate_vibe(settings.vibe_pat_connect);  // non-op, by default
    bitmap_layer_set_bitmap(bmp_connection_layer, image_connection_icon);
  } else {
    generate_vibe(settings.vibe_pat_disconnect);  // because, this is bad...
    bitmap_layer_set_bitmap(bmp_connection_layer, image_noconnection_icon);
  }
}

static void handle_bluetooth(bool connected) {
  if (bluetooth_connected != connected) {
    bluetooth_connected = connected;
    update_connection();
    if (bluetooth_connected == true) {
      if ( (timezone_request == NULL) & (timezone_offset == TIMEZONE_UNINITIALIZED) ) {
        timezone_request = app_timer_register(5000, &request_timezone, NULL); // give it time to settle...
        if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "timezone request timer queued"); }
      }
    }
  }
}

static void set_unifont() {
  if ( strcmp(lang_gen.language,"RU") == 0 ) { // Unicode font w/ Cyrillic characters
    // set fonts...
    text_layer_set_font(day_layer,unifont_16);
    text_layer_set_font(text_connection_layer, unifont_16);
    text_layer_set_font(date_layer, unifont_16);
    // set fonts, for calendar
    cal_normal = unifont_16; // fh = 16
    cal_bold   = unifont_16_bold; // fh = 22 // XXX TODO need a bold unicode/unifont option... maybe invert it or box it or something?
  } else { // Standard font
    // set fonts...
    text_layer_set_font(day_layer,fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_font(text_connection_layer,fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_font(date_layer,fonts_get_system_font(FONT_KEY_GOTHIC_24));
    // set fonts, for calendar
    cal_normal = fonts_get_system_font(FONT_KEY_GOTHIC_14); // fh = 16
    cal_bold   = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD); // fh = 22
  }
  // set offsets...
  position_connection_layer();
  position_date_layer();
  position_time_layer();
  position_day_layer();
}

bool period_check(uint8_t start_incr, uint8_t stop_incr, bool retval_on_equal) {
  // takes two periods (uint8_t 0-144) in 10 minute increments, and returns whether the current time falls inside them.
  // periods are fully inclusive, presently
  if (start_incr == stop_incr) { return retval_on_equal; }
  bool inside_period = false;
  uint8_t current_min_incr = (currentTime->tm_min - (currentTime->tm_min%10))/10;
  uint8_t current_incr = currentTime->tm_hour * 6 + current_min_incr;
  if (start_incr > stop_incr) { // period crosses midnight of the day
    if (current_incr >= start_incr || current_incr <= stop_incr) {
      inside_period = true;
    }
  } else { // period occurs within a single day
    if (current_incr >= start_incr && current_incr <= stop_incr) {
      inside_period = true;
    }
  }
  if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Period Check... %d <= %d <= %d == %d*", start_incr, current_incr, stop_incr, (int)inside_period); }
  return inside_period;
}

bool dnd_period_check() {
  // TODO - adv_settings.DND_accel_off = 0,   // Do Not Disturb: disable accelerometer polling during DND?
  dnd_period_active = period_check(adv_settings.DND_start, adv_settings.DND_stop, false);
  if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Tested DND period... %d", (int)dnd_period_active); }
  return dnd_period_active;
}
bool hourvibe_period_check() {
  // TODO - adv_settings.vibe_hour_days  = 0, // Hour Vibe: days active [Su 1, Mo 2, Tu 4, We 8, Th 16, Fr 32, Sa 64 => 0 => 127]
  vibe_period_active = period_check(adv_settings.vibe_hour_start, adv_settings.vibe_hour_stop, true);
  if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Tested vibe period... %d", (int)vibe_period_active); }
  return vibe_period_active;
}

void set_layer_attr(TextLayer *textlayer, GTextAlignment Alignment) {
  text_layer_set_text_alignment(textlayer, Alignment);
  text_layer_set_text_color(textlayer, GColorWhite);
  text_layer_set_background_color(textlayer, GColorClear);
}

void set_layer_attr_sfont(TextLayer *textlayer, char *font_key, GTextAlignment Alignment) {
  set_layer_attr(textlayer, Alignment);
  text_layer_set_font(textlayer, fonts_get_system_font(font_key));
}

void set_layer_attr_cfont(TextLayer *textlayer, uint32_t FontResHandle, GTextAlignment Alignment) {
  set_layer_attr(textlayer, Alignment);
  text_layer_set_font(textlayer, fonts_load_custom_font(resource_get_handle(FontResHandle)));
}

static void window_load(Window *window) {

  unifont_16 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNICODE_16));
  unifont_16_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNICODE_BOLD_16));
  climacons  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CLIMACONS_32));
  cal_normal = unifont_16;
  cal_bold   = unifont_16_bold;

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  slot_status = layer_create(GRect(0,LAYOUT_STAT,DEVICE_WIDTH,LAYOUT_SLOT_TOP));
  //slot_status = layer_create(GRect(0,0,DEVICE_WIDTH,DEVICE_HEIGHT));
  layer_set_update_proc(slot_status, slot_status_layer_update_callback);
  layer_add_child(window_layer, slot_status);

  statusbar = layer_create(GRect(0,LAYOUT_STAT,DEVICE_WIDTH,LAYOUT_SLOT_TOP));
  layer_set_update_proc(statusbar, statusbar_layer_update_callback);
  layer_add_child(slot_status, statusbar);
  GRect stat_bounds = layer_get_bounds(statusbar);

  slot_top = layer_create(GRect(0,LAYOUT_SLOT_TOP,DEVICE_WIDTH,LAYOUT_SLOT_HEIGHT));
  layer_set_update_proc(slot_top, slot_top_layer_update_callback);
  layer_add_child(window_layer, slot_top);
  GRect slot_top_bounds = layer_get_bounds(slot_top);

  slot_bot = layer_create(GRect(0,LAYOUT_SLOT_BOT,DEVICE_WIDTH,LAYOUT_SLOT_HEIGHT));
  layer_set_update_proc(slot_bot, slot_bot_layer_update_callback);
  layer_add_child(window_layer, slot_bot);
  GRect slot_bot_bounds = layer_get_bounds(slot_bot);

  bmp_connection_layer = bitmap_layer_create( GRect(STAT_BT_ICON_LEFT, STAT_BT_ICON_TOP, 20, 20) );
  layer_add_child(statusbar, bitmap_layer_get_layer(bmp_connection_layer));
  image_connection_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_LINKED_ICON);
  image_noconnection_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_NOLINK_ICON);

  bmp_charging_layer = bitmap_layer_create( GRect(STAT_CHRG_ICON_LEFT, STAT_CHRG_ICON_TOP, 20, 20) );
  layer_add_child(statusbar, bitmap_layer_get_layer(bmp_charging_layer));
  image_charging_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_ICON);
  image_hourvibe_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HOURVIBE_ICON);
  image_dnd_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DONOTDISTURB_ICON);

  dnd_period_check();
  hourvibe_period_check();
  set_status_charging_icon();

  battery_layer = layer_create(stat_bounds);
  layer_set_update_proc(battery_layer, battery_layer_update_callback);
  layer_add_child(statusbar, battery_layer);

  datetime_layer = layer_create(slot_top_bounds);
  layer_set_update_proc(datetime_layer, datetime_layer_update_callback);
  layer_add_child(slot_top, datetime_layer);

  calendar_layer = layer_create(slot_bot_bounds);
  layer_set_update_proc(calendar_layer, calendar_layer_update_callback);
  layer_add_child(slot_bot, calendar_layer);
  layer_set_hidden(calendar_layer, true);

  splash_layer = layer_create(slot_bot_bounds);
  layer_set_update_proc(splash_layer, splash_layer_update_callback);
  layer_add_child(slot_bot, splash_layer);

  toggle_slot_bottom((void*)splash_layer);  // show @ start...
  bottom_toggle = app_timer_register(2000, &toggle_slot_bottom, (void*)calendar_layer); // queue calendar to reappear in 2 seconds

  date_layer = text_layer_create( GRect(REL_CLOCK_DATE_LEFT, REL_CLOCK_DATE_TOP, REL_CLOCK_DATE_WIDTH, REL_CLOCK_DATE_HEIGHT) ); // see position_date_layer()
  set_layer_attr_sfont(date_layer, FONT_KEY_GOTHIC_24, GTextAlignmentCenter);
  position_date_layer(); // depends on font/language
  update_date_text();
  layer_add_child(datetime_layer, text_layer_get_layer(date_layer));

  weather_layer = layer_create(slot_top_bounds);
  layer_set_update_proc(weather_layer, weather_layer_update_callback);
  layer_add_child(datetime_layer, weather_layer);

  time_layer = text_layer_create( GRect(REL_CLOCK_TIME_LEFT, REL_CLOCK_TIME_TOP, DEVICE_WIDTH - 2, REL_CLOCK_TIME_HEIGHT) ); // see position_time_layer()
  set_layer_attr_cfont(time_layer, RESOURCE_ID_FONT_FUTURA_CONDENSED_48, GTextAlignmentCenter);
  toggle_weather();
  position_time_layer(); // make use of our whitespace, if we have it...
  update_time_text();
  layer_add_child(datetime_layer, text_layer_get_layer(time_layer));

  week_layer = text_layer_create( GRect(4, REL_CLOCK_SUBTEXT_TOP, 140, 18) );
  set_layer_attr_sfont(week_layer, FONT_KEY_GOTHIC_14, GTextAlignmentLeft);
  layer_add_child(datetime_layer, text_layer_get_layer(week_layer));
  if ( settings.show_week == 0 ) {
    layer_set_hidden(text_layer_get_layer(week_layer), true);
  }

  day_layer = text_layer_create( GRect(4, REL_CLOCK_SUBTEXT_TOP, REL_CLOCK_DATE_WIDTH, 18) ); // see position_day_layer()
  set_layer_attr_sfont(day_layer, FONT_KEY_GOTHIC_14, GTextAlignmentCenter);
  position_day_layer(); // depends on font/language
  layer_add_child(datetime_layer, text_layer_get_layer(day_layer));
  if ( settings.show_day == 0 ) {
    layer_set_hidden(text_layer_get_layer(day_layer), true);
  }

  ampm_layer = text_layer_create( GRect(0, REL_CLOCK_SUBTEXT_TOP, 140, 18) );
  set_layer_attr_sfont(ampm_layer, FONT_KEY_GOTHIC_14, GTextAlignmentRight);
  layer_add_child(datetime_layer, text_layer_get_layer(ampm_layer));
  if ( settings.show_am_pm == 0 ) {
    layer_set_hidden(text_layer_get_layer(ampm_layer), true);
  }

  update_datetime_subtext();

  text_connection_layer = text_layer_create( GRect(20+STAT_BT_ICON_LEFT, 0, 72, 22) ); // see position_connection_layer()
  set_layer_attr_sfont(text_connection_layer, FONT_KEY_GOTHIC_18, GTextAlignmentLeft);
  update_connection();
  position_connection_layer(); // depends on font/language
  layer_add_child(statusbar, text_layer_get_layer(text_connection_layer));

  text_battery_layer = text_layer_create( GRect(STAT_BATT_LEFT, STAT_BATT_TOP-2, STAT_BATT_WIDTH, STAT_BATT_HEIGHT) );
  set_layer_attr_sfont(text_battery_layer, FONT_KEY_GOTHIC_14, GTextAlignmentCenter);
  text_layer_set_text(text_battery_layer, "-");

  layer_add_child(statusbar, text_layer_get_layer(text_battery_layer));

  set_unifont();

  // NOTE: No more adding layers below here - the inverter layers NEED to be the last to be on top!

  // hide battery meter, until we can fix the size/position later when subscribing
  battery_meter_layer = inverter_layer_create(stat_bounds);
  layer_set_hidden(inverter_layer_get_layer(battery_meter_layer), true);
  layer_add_child(statusbar, inverter_layer_get_layer(battery_meter_layer));

  statusbar_visible();
  toggle_statusbar();

  // topmost inverter layer, determines dark or light...
  inverter_layer = inverter_layer_create(bounds);
  if (settings.inverted==0) {
    layer_set_hidden(inverter_layer_get_layer(inverter_layer), true);
  }
  layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));

}

static void window_unload(Window *window) {
  // unload anything we loaded, destroy anything we created, remove anything we added
  layer_destroy(inverter_layer_get_layer(inverter_layer));
  layer_destroy(inverter_layer_get_layer(battery_meter_layer));
  layer_destroy(text_layer_get_layer(text_battery_layer));
  layer_destroy(text_layer_get_layer(text_connection_layer));
  layer_destroy(text_layer_get_layer(ampm_layer));
  layer_destroy(text_layer_get_layer(day_layer));
  layer_destroy(text_layer_get_layer(week_layer));
  layer_destroy(text_layer_get_layer(time_layer));
  layer_destroy(text_layer_get_layer(date_layer));
  layer_destroy(weather_layer);
  layer_destroy(splash_layer);
  layer_destroy(calendar_layer);
  layer_destroy(datetime_layer);
  layer_destroy(battery_layer);
  // custom fonts are automatically unloaded at exit - http://forums.getpebble.com/discussion/comment/35808/#Comment_35808
  layer_remove_from_parent(bitmap_layer_get_layer(bmp_charging_layer));
  layer_remove_from_parent(bitmap_layer_get_layer(bmp_connection_layer));
  bitmap_layer_destroy(bmp_charging_layer);
  bitmap_layer_destroy(bmp_connection_layer);
  gbitmap_destroy(image_connection_icon);
  gbitmap_destroy(image_noconnection_icon);
  gbitmap_destroy(image_charging_icon);
  gbitmap_destroy(image_hourvibe_icon);
  gbitmap_destroy(image_dnd_icon);
  layer_destroy(slot_bot);
  layer_destroy(slot_top);
  layer_destroy(statusbar);
}

static void deinit(void) {
  // deinit anything we init
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

void handle_vibe_suppression() {
  // control vibe_suppression events - we should never set vibe_suppression to false outside of this function
  // it is useful to set it true directly (briefly), to ensure suppression, and then call this function afterwards
  if (dnd_period_active) {
    vibe_suppression = true;
  } else if (settings.vibe_hour && vibe_period_active) {
    vibe_suppression = false;
  } else {
    vibe_suppression = false;
  }
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
  currentTime = tick_time;
  update_time_text();
  if ( currentTime->tm_min % 10 == 0) {
    dnd_period_check();
    hourvibe_period_check();
    set_status_charging_icon();
    handle_vibe_suppression();
  }
  if (adv_settings.weather_update && (currentTime->tm_min + 60) % adv_settings.weather_update == 0) {
    request_weather(NULL); // TODO: should make this a timer, or at least add logic for not requesting until connected again
  } else if (bluetooth_connected && adv_settings.weather_update && weather.current == 999) {
    request_weather(NULL); // ANDROIIIIIDRAGE  (or, someone who's got weather enabled but location services disabled)
  } 

//  if (units_changed & MONTH_UNIT) {
//  }

  if (units_changed & HOUR_UNIT) {
    request_timezone(NULL);
    update_datetime_subtext();
    if (settings.vibe_hour && vibe_period_active) {
      generate_vibe(settings.vibe_hour); // will be suppressed if within DND
    }
  }

  if (units_changed & DAY_UNIT) {
    layer_mark_dirty(datetime_layer);
    layer_mark_dirty(calendar_layer);
  }

  // calendar gets redrawn every time because time_layer is changed and all layers are redrawn together.
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed)
{
  // update the seconds layer(s)...
  if (settings.show_week == 6) {
    update_seconds_text(week_layer);
  }
  if (settings.show_am_pm == 6) {
    update_seconds_text(ampm_layer);
  }
  // redraw everything else if the minute changes...
  if (units_changed & MINUTE_UNIT) {
    handle_minute_tick(tick_time, units_changed);
  }
}

static int need_second_tick_handler(void) {
  if ((settings.show_week == 6) || (settings.show_am_pm == 6)) { return 1; }
  return 0; 
}

static void switch_tick_handler(void) {
  tick_timer_service_unsubscribe(); // safe to call even before we've subscribed
  seconds_shown = need_second_tick_handler();
  if (seconds_shown) {
    tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Seconds handler enabled"); }
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Seconds handler disabled"); }
  }
}

void my_out_sent_handler(DictionaryIterator *sent, void *context) {
// outgoing message was delivered
  if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "AppMessage Delivered"); }
}
void my_out_fail_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
// outgoing message failed
  if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "AppMessage Failed to Send: %d", reason); }
}

void in_js_ready_handler(DictionaryIterator *received, void *context) {
    watch_version_send(NULL);
    if (weather_request == NULL) { weather_request = app_timer_register(3000, &request_weather, NULL); } // for Android's slow JS...
}

void in_weather_handler(DictionaryIterator *received, void *context) {
    Tuple *appkey     = dict_find(received, AK_WEATHER_TEMP);
    if (appkey != NULL)     { weather.current = appkey->value->int16; }
    appkey = dict_find(received, AK_WEATHER_COND);
    if (appkey != NULL)     { strncpy(weather.condition, appkey->value->cstring, sizeof(weather.condition)-1); }
    layer_mark_dirty(weather_layer);
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Weather received: %d, %s", weather.current, weather.condition); }
}

void in_timezone_handler(DictionaryIterator *received, void *context) {
    Tuple *tz_offset = dict_find(received, AK_TIMEZONE_OFFSET);
    if (tz_offset != NULL) {
      timezone_offset = tz_offset->value->int8;
      update_datetime_subtext();
    }
  if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Timezone received: %d", timezone_offset); }
}

void in_configuration_handler(DictionaryIterator *received, void *context) {
    // debugging first (so we can catch this message)

    // AK_DEBUGGING_ON == general debugging
    Tuple *debugging = dict_find(received, AK_DEBUGGING_ON);
    if (debugging != NULL) {
      if (debugging->value->uint8 != 0) {
        debug.general = true;
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Debugging enabled.");
      } else {
        if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Debugging disabled."); }
        debug.general = false;
      } 
    }

    // AK_DEBUGLANG_ON == language / translation debugging
    Tuple *debuglang = dict_find(received, AK_DEBUGLANG_ON);
    if (debuglang != NULL) {
      if (debuglang->value->uint8 != 0) {
        debug.language = true;
        if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Language debugging enabled."); }
      } else {
        debug.language = false;
        if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Language debugging disabled."); }
      } 
    }

    // style_inv == inverted
    Tuple *style_inv = dict_find(received, AK_STYLE_INV);
    if (style_inv != NULL) {
      settings.inverted = style_inv->value->uint8;
      if (style_inv->value->uint8==0) {
        layer_set_hidden(inverter_layer_get_layer(inverter_layer), true); // hide inversion = dark
      } else {
        layer_set_hidden(inverter_layer_get_layer(inverter_layer), false); // show inversion = light
      }
    }

    // style_day_inv == day_invert
    Tuple *style_day_inv = dict_find(received, AK_STYLE_DAY_INV);
    if (style_day_inv != NULL) {
      settings.day_invert = style_day_inv->value->uint8;
    }

    // style_grid == grid
    Tuple *style_grid = dict_find(received, AK_STYLE_GRID);
    if (style_grid != NULL) {
      settings.grid = style_grid->value->uint8;
    }

    // AK_VIBE_HOUR == vibe_hour - vibration patterns for hourly vibration
    Tuple *vibe_hour = dict_find(received, AK_VIBE_HOUR);
    if (vibe_hour != NULL) {
      settings.vibe_hour = vibe_hour->value->uint8;
      set_status_charging_icon();
    }

    // INTL_DOWO == dayOfWeekOffset
    Tuple *INTL_DOWO = dict_find(received, AK_INTL_DOWO);
    if (INTL_DOWO != NULL) {
      settings.dayOfWeekOffset = INTL_DOWO->value->uint8;
    }

    // AK_INTL_FMT_DATE == date format (strftime + manual localization)
    Tuple *FMT_DATE = dict_find(received, AK_INTL_FMT_DATE);
    if (FMT_DATE != NULL) {
      settings.date_format = FMT_DATE->value->uint8;
      update_date_text();
    }

    // AK_STYLE_WEEK
    Tuple *style_week = dict_find(received, AK_STYLE_WEEK);
    if (style_week != NULL) {
      settings.show_week = style_week->value->uint8;
      if ( settings.show_week ) {
        layer_set_hidden(text_layer_get_layer(week_layer), false);
      }  else {
        layer_set_hidden(text_layer_get_layer(week_layer), true);
      }
    }

    // AK_INTL_FMT_WEEK == week format (strftime)
    Tuple *FMT_WEEK = dict_find(received, AK_INTL_FMT_WEEK);
    if (FMT_WEEK != NULL) {
      settings.week_format = FMT_WEEK->value->uint8;
    }

    // AK_STYLE_DAY
    Tuple *style_day = dict_find(received, AK_STYLE_DAY);
    if (style_day != NULL) {
      settings.show_day = style_day->value->uint8;
      if ( settings.show_day ) {
        layer_set_hidden(text_layer_get_layer(day_layer), false);
      }  else {
        layer_set_hidden(text_layer_get_layer(day_layer), true);
      }
    }

    // AK_STYLE_AM_PM
    Tuple *style_am_pm = dict_find(received, AK_STYLE_AM_PM);
    if (style_am_pm != NULL) {
      settings.show_am_pm = style_am_pm->value->uint8;
      if ( settings.show_am_pm ) {
        layer_set_hidden(text_layer_get_layer(ampm_layer), false);
      }  else {
        layer_set_hidden(text_layer_get_layer(ampm_layer), true);
      }
    }

    if (need_second_tick_handler() != seconds_shown) {
      switch_tick_handler();
    }

    // now that we've received any changes, redraw the subtext (which processes week, day, and AM/PM)
    update_datetime_subtext();

    // AK_VIBE_PAT_DISCONNECT / AK_VIBE_PAT_CONNECT == vibration patterns for connect and disconnect
    Tuple *VIBE_PAT_D = dict_find(received, AK_VIBE_PAT_DISCONNECT);
    if (VIBE_PAT_D != NULL) {
      settings.vibe_pat_disconnect = VIBE_PAT_D->value->uint8;
    }
    Tuple *VIBE_PAT_C = dict_find(received, AK_VIBE_PAT_CONNECT);
    if (VIBE_PAT_C != NULL) {
      settings.vibe_pat_connect = VIBE_PAT_C->value->uint8;
    }

    // AK_TRACK_BATTERY == whether or not to do battery tracking
    Tuple *track_battery = dict_find(received, AK_TRACK_BATTERY);
    if (track_battery != NULL) {
      settings.track_battery = track_battery->value->uint8;
      if (settings.track_battery) {
        battery_status_send(NULL); // either it was just turned on, or we'll get a bonus datapoint from running config.
      }
    }

    // AK_CAL_WEEK_PATTERN == which weeks are shown in calendar (last,current,next - etc.)
    Tuple *week_pattern = dict_find(received, AK_CAL_WEEK_PATTERN);
    if (week_pattern != NULL) {
      adv_settings.week_pattern = week_pattern->value->uint8;
    }

    Tuple *appkey;

    // AK_INV_SLOT_STAT == invert slot (or not!) // TODO, UNUSED
    appkey = dict_find(received, AK_INV_SLOT_STAT);
    if (appkey != NULL) { adv_settings.invertStatBar = appkey->value->uint8; }

    // AK_INV_SLOT_TOP == invert slot (or not!) // TODO, UNUSED
    appkey = dict_find(received, AK_INV_SLOT_TOP);
    if (appkey != NULL) { adv_settings.invertTopSlot = appkey->value->uint8; }

    // AK_INV_SLOT_BOT == invert slot (or not!) // TODO, UNUSED
    appkey = dict_find(received, AK_INV_SLOT_BOT);
    if (appkey != NULL) { adv_settings.invertBotSlot = appkey->value->uint8; }

    // AK_SHOW_STAT_BAR == show statusbar
    appkey = dict_find(received, AK_SHOW_STAT_BAR);
    if (appkey != NULL) { adv_settings.showStatus = appkey->value->uint8; }

    // AK_SHOW_STAT_BATT == statusbar battery limit
    appkey = dict_find(received, AK_SHOW_STAT_BATT);
    if (appkey != NULL) { adv_settings.showStatusBat = appkey->value->uint8; }

    // AK_SHOW_DATE == show date // TODO, UNUSED
    appkey = dict_find(received, AK_SHOW_DATE);
    if (appkey != NULL) { adv_settings.showDate = appkey->value->uint8; }

    // AK_DND_START == period start, DND
    appkey = dict_find(received, AK_DND_START);
    if (appkey != NULL) { adv_settings.DND_start = appkey->value->uint8; }

    // AK_DND_STOP == period stop, DND
    appkey = dict_find(received, AK_DND_STOP);
    if (appkey != NULL) { adv_settings.DND_stop = appkey->value->uint8; }

    // AK_DND_NOACCEL == [perhaps] disable accelerometer during DND // TODO, UNUSED
    appkey = dict_find(received, AK_DND_NOACCEL);
    if (appkey != NULL) { adv_settings.DND_accel_off = appkey->value->uint8; }

    // AK_VIBE_START == period start, VIBE
    appkey = dict_find(received, AK_VIBE_START);
    if (appkey != NULL) { adv_settings.vibe_hour_start = appkey->value->uint8; }

    // AK_VIBE_STOP == period stop, VIBE
    appkey = dict_find(received, AK_VIBE_STOP);
    if (appkey != NULL) { adv_settings.vibe_hour_stop = appkey->value->uint8; }

    // AK_VIBE_DAYS == days to do hourly vibration // TODO, UNUSED
    appkey = dict_find(received, AK_VIBE_DAYS);
    if (appkey != NULL) { adv_settings.vibe_hour_days = appkey->value->uint8; }

    // AK_IDLE_REMINDER == period stop, VIBE // TODO, UNUSED
    appkey = dict_find(received, AK_IDLE_REMINDER);
    if (appkey != NULL) { adv_settings.idle_reminder = appkey->value->uint8; }

    // AK_IDLE_VIBE_PATT == idle vibration pattern // TODO, UNUSED
    appkey = dict_find(received, AK_IDLE_VIBE_PATT);
    if (appkey != NULL) { adv_settings.idle_pattern = appkey->value->uint8; }

/* TODO
    // AK_IDLE_MESSAGE == Idle message // TODO, UNUSED
    appkey = dict_find(received, AK_IDLE_MESSAGE);
    if (appkey != NULL) { strncpy(adv_settings.idle_message, appkey->value->cstring, sizeof(adv_settings.idle_message)-1); }
*/

    // AK_IDLE_START == period start, Idle reminder // TODO, UNUSED
    appkey = dict_find(received, AK_IDLE_START);
    if (appkey != NULL) { adv_settings.idle_start = appkey->value->uint8; }

    // AK_IDLE_STOP == period stop, Idle reminder // TODO, UNUSED
    appkey = dict_find(received, AK_IDLE_STOP);
    if (appkey != NULL) { adv_settings.idle_stop = appkey->value->uint8; }

    // AK_WEATHER_FMT == weather format (0:C / 1:F)
    appkey = dict_find(received, AK_WEATHER_FMT);
    if (appkey != NULL) {
      adv_settings.weather_format = appkey->value->uint8;
      if (weather_request == NULL) { weather_request = app_timer_register(1000, &request_weather, NULL); }
    }

    // AK_WEATHER_UPDATE == weather update frequency
    appkey = dict_find(received, AK_WEATHER_UPDATE);
    if (appkey != NULL) {
      if (appkey->value->uint8 < adv_settings.weather_update && weather_request == NULL) {
        weather_request = app_timer_register(1000, &request_weather, NULL);
      }
      adv_settings.weather_update = appkey->value->uint8;
    }
    statusbar_visible();
    toggle_weather();
    toggle_statusbar();

    // begin translations...
    Tuple *translation;

    // AK_LANGUAGE == language, e.g. EN
    Tuple *chosen_language = dict_find(received, AK_LANGUAGE);
    if (chosen_language != NULL) {
      if (debug.language) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Language is set to %s", chosen_language->value->cstring); }
      strncpy(lang_gen.language, chosen_language->value->cstring, sizeof(lang_gen.language)-1);
      set_unifont();
    }

    // AK_TRANS_ABBR_*DAY == abbrDaysOfWeek // localized Su Mo Tu We Th Fr Sa, max 2 characters
    for (int i = AK_TRANS_ABBR_SUNDAY; i <= AK_TRANS_ABBR_SATURDAY; i++ ) {
      translation = dict_find(received, i);
      if (translation != NULL) {
        if (debug.language) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "translation for key %d is %s", i, translation->value->cstring); }
        strncpy(lang_gen.abbrDaysOfWeek[i - AK_TRANS_ABBR_SUNDAY], translation->value->cstring, sizeof(lang_gen.abbrDaysOfWeek[i - AK_TRANS_ABBR_SUNDAY])-1);
      }
    }

    // AK_TRANS_*DAY == daysOfWeek // localized Sunday through Saturday, max 12 characters
    for (int i = AK_TRANS_SUNDAY; i <= AK_TRANS_SATURDAY; i++ ) {
      translation = dict_find(received, i);
      if (translation != NULL) {
        if (debug.language) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "translation for key %d is %s", i, translation->value->cstring); }
        strncpy(lang_days.DaysOfWeek[i - AK_TRANS_SUNDAY], translation->value->cstring, sizeof(lang_days.DaysOfWeek[i - AK_TRANS_SUNDAY])-1);
      }
    }

    // AK_TRANS_ABBR_*MONTH == monthsOfYear // localized month name abbreviations, max 3 characters
    for (int i = AK_TRANS_ABBR_JANUARY; i <= AK_TRANS_ABBR_DECEMBER; i++ ) {
      translation = dict_find(received, i);
      if (translation != NULL) {
        if (debug.language) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "translation for key %d is %s", i, translation->value->cstring); }
        strncpy(lang_gen.abbrMonthsNames[i - AK_TRANS_ABBR_JANUARY], translation->value->cstring, sizeof(lang_gen.abbrMonthsNames[i - AK_TRANS_ABBR_JANUARY])-1);
      }
    }

    // AK_TRANS_*MONTH == monthsOfYear // localized month names, max 12 characters
    for (int i = AK_TRANS_JANUARY; i <= AK_TRANS_DECEMBER; i++ ) {
      translation = dict_find(received, i);
      if (translation != NULL) {
        if (debug.language) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "translation for key %d is %s", i, translation->value->cstring); }
        strncpy(lang_months.monthsNames[i - AK_TRANS_JANUARY], translation->value->cstring, sizeof(lang_months.monthsNames[i - AK_TRANS_JANUARY])-1);
      }
    }

    // AK_TRANS_CONNECTED / AK_TRANS_DISCONNECTED == status text, e.g. "Linked" "NOLINK", max 9 characters
    for (int i = AK_TRANS_CONNECTED; i <= AK_TRANS_DISCONNECTED; i++ ) {
      translation = dict_find(received, i);
      if (translation != NULL) {
        if (debug.language) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "translation for key %d is %s", i, translation->value->cstring); }
        strncpy(lang_gen.statuses[i - AK_TRANS_CONNECTED], translation->value->cstring, sizeof(lang_gen.statuses[i - AK_TRANS_CONNECTED])-1);
      }
    }
    vibe_suppression = true;
    update_connection();
    handle_vibe_suppression();

    // AK_TRANS_TIME_AM / AK_TRANS_TIME_PM == AM / PM text, e.g. "AM" "PM" :), max 6 characters
    for (int i = AK_TRANS_TIME_AM; i <= AK_TRANS_TIME_PM; i++ ) {
      translation = dict_find(received, i);
      if (translation != NULL) {
        if (debug.language) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "translation for key %d is %s", i, translation->value->cstring); }
        strncpy(lang_gen.abbrTime[i - AK_TRANS_TIME_AM], translation->value->cstring, sizeof(lang_gen.abbrTime[i - AK_TRANS_TIME_AM])-1);
      }
    }
    
    // end translations...

    int result = 0;
    result = persist_write_data(PK_SETTINGS, &settings, sizeof(settings) );
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wrote %d bytes into settings", result); }
    result = persist_write_data(PK_LANG_GEN, &lang_gen, sizeof(lang_gen) );
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wrote %d bytes into lang_gen", result); }
    result = persist_write_data(PK_LANG_MONTHS, &lang_months, sizeof(lang_months) );
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wrote %d bytes into lang_months", result); }
    result = persist_write_data(PK_LANG_DAYS, &lang_days, sizeof(lang_days) );
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wrote %d bytes into lang_days", result); }
    result = persist_write_data(PK_DEBUGGING, &debug, sizeof(debug) );
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wrote %d bytes into debug", result); }
    result = persist_write_data(PK_ADV_SETTINGS, &adv_settings, sizeof(adv_settings) );
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wrote %d bytes into adv_settings", result); }

    // ==== Implemented SDK ====
    // Battery
    // Connected
    // Persistent Storage
    // Screenshot Operation
    // ==== Available in SDK ====
    // Accelerometer
    // App Focus ( does this apply to Timely? )
    // PebbleKit JS - more accurate location data: enableHighAccuracy (takes a while on IOS)
    // ==== Waiting on / SDK gaps ====
    // Magnetometer
    // ==== Interesting SDK possibilities ====
    // PebbleKit JS - more information from phone
    // ==== Future improvements ====
    // Positioning - top, bottom, etc.
  if (1) { layer_mark_dirty(calendar_layer); } // TODO
  if (1) { layer_mark_dirty(datetime_layer); } // TODO
}

void my_in_rcv_handler(DictionaryIterator *received, void *context) {
// incoming message received
  Tuple *message_type = dict_find(received, AK_MESSAGE_TYPE);
  if (message_type != NULL) {
    if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Message type %d received", message_type->value->uint8); }
    switch ( message_type->value->uint8 ) {
    case AK_SEND_WATCH_VERSION:
      in_js_ready_handler(received, context);
      return;
    case AK_TIMEZONE_OFFSET:
      in_timezone_handler(received, context);
      return;
    case AK_REQUEST_WEATHER:
      in_weather_handler(received, context);
      return;
    }
  } else {
    // default to configuration, which may not send the message type...
    in_configuration_handler(received, context);
  }
}

void my_in_drp_handler(AppMessageResult reason, void *context) {
// incoming message dropped
  if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "AppMessage Dropped: %d", reason); }
}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(my_in_rcv_handler);
  app_message_register_inbox_dropped(my_in_drp_handler);
  app_message_register_outbox_sent(my_out_sent_handler);
  app_message_register_outbox_failed(my_out_fail_handler);
  // Init buffers
//  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "AM Inbox max %lu", app_message_inbox_size_maximum());
//[INFO    ] D Timely.c:2079 AM Inbox 2044 received
//  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "AM Outbox max %lu", app_message_outbox_size_maximum());
//[INFO    ] D Timely.c:2080 AM Outbox 636 received
  //app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_message_open(1280, 512);
}


static void init(void) {

  if (DEBUGLOG == 1) { debug.general = true; }
  if (TRANSLOG == 1) { debug.language = true; }
  currentTime = get_time();

  app_message_init();

  if (persist_exists(PK_SETTINGS)) {
    persist_read_data(PK_SETTINGS, &settings, sizeof(settings) );
    if (persist_exists(PK_LANG_GEN)) {
      persist_read_data(PK_LANG_GEN, &lang_gen, sizeof(lang_gen) );
    }
    if (persist_exists(PK_LANG_MONTHS)) {
      persist_read_data(PK_LANG_MONTHS, &lang_months, sizeof(lang_months) );
    }
    if (persist_exists(PK_LANG_DAYS)) {
      persist_read_data(PK_LANG_DAYS, &lang_days, sizeof(lang_days) );
    }
    if (persist_exists(PK_DEBUGGING)) {
      persist_read_data(PK_DEBUGGING, &debug, sizeof(debug) );
    }
    //persist_write_data(PK_ADV_SETTINGS, &adv_settings, sizeof(adv_settings) ); // XXX TODO reset to defaults, for testing...
    if (persist_exists(PK_ADV_SETTINGS)) {
      persist_read_data(PK_ADV_SETTINGS, &adv_settings, sizeof(adv_settings) );
    }
  }
  // re-initialize this, if it was set, since we're storing those values persistently as well...
  if (DEBUGLOG == 1) { debug.general = true; }
  if (TRANSLOG == 1) { debug.language = true; }

  if (adv_settings.weather_update) {
    weather_request = app_timer_register(1250, &request_weather, NULL);
    //request_weather(NULL);
  }

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_set_background_color(window, GColorBlack);
  window_stack_push(window, false);

  //update_time_text();

  switch_tick_handler();
  battery_state_service_subscribe(&handle_battery);
  handle_battery(battery_state_service_peek()); // initialize
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek()); // initialize
  handle_vibe_suppression();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
