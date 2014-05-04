/* Timely header file - structs and function prototypes
 * ( to be split out into pieces, for dynamic overlays, at a later date )
 */

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

char *translate_error(AppMessageResult result);
int daysInMonth(int mon, int year);
struct tm *get_time();
void setColors(GContext* ctx);
void setInvColors(GContext* ctx);
void weather_layer_update_callback(Layer *me, GContext* ctx);
void splash_layer_update_callback(Layer *me, GContext* ctx);
void calendar_layer_update_callback(Layer *me, GContext* ctx);
void update_date_text();
void update_time_text();
void update_day_text(TextLayer *which_layer);
void update_month_text(TextLayer *which_layer);
void update_week_text(TextLayer *which_layer);
void update_ampm_text(TextLayer *which_layer);
void update_seconds_text(TextLayer *which_layer);
char * get_doy_text();
char * get_dliy_text();
void update_doy_text(TextLayer *which_layer);
void update_dliy_text(TextLayer *which_layer);
void update_doy_dliy_text(TextLayer *which_layer);
void update_timezone_text(TextLayer *which_layer);
void process_show_week();
void process_show_day();
void process_show_ampm();
void position_connection_layer();
void position_date_layer();
void position_day_layer();
void position_time_layer();
void update_datetime_subtext();
void datetime_layer_update_callback(Layer *me, GContext* ctx);
void statusbar_visible();
void toggle_weather();
void toggle_statusbar();
void slot_status_layer_update_callback(Layer *me, GContext* ctx);
void statusbar_layer_update_callback(Layer *me, GContext* ctx);
void slot_top_layer_update_callback(Layer *me, GContext* ctx);
void slot_bot_layer_update_callback(Layer *me, GContext* ctx);
void battery_layer_update_callback(Layer *me, GContext* ctx);
static void request_weather(void *data);
static void request_timezone(void *data);
static void watch_version_send(void *data);
static void battery_status_send(void *data);
void set_status_charging_icon();
static void toggle_slot_bottom(void *data);
static void handle_battery(BatteryChargeState charge_state);
void generate_vibe(uint32_t vibe_pattern_number);
void update_connection();
static void handle_bluetooth(bool connected);
static void set_unifont();
bool period_check(uint8_t start_incr, uint8_t stop_incr, bool retval_on_equal);
bool dnd_period_check();
bool hourvibe_period_check();
void set_layer_attr(TextLayer *textlayer, GTextAlignment Alignment);
void set_layer_attr_sfont(TextLayer *textlayer, char *font_key, GTextAlignment Alignment);
void set_layer_attr_cfont(TextLayer *textlayer, uint32_t FontResHandle, GTextAlignment Alignment);
static void window_load(Window *window);
static void window_unload(Window *window);
static void deinit(void);
void handle_vibe_suppression();
static int need_second_tick_handler(void);
static void switch_tick_handler(void);
void my_out_sent_handler(DictionaryIterator *sent, void *context);
void my_out_fail_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);
void in_js_ready_handler(DictionaryIterator *received, void *context);
void in_weather_handler(DictionaryIterator *received, void *context);
void in_timezone_handler(DictionaryIterator *received, void *context);
void in_configuration_handler(DictionaryIterator *received, void *context);
void my_in_rcv_handler(DictionaryIterator *received, void *context);
void my_in_drp_handler(AppMessageResult reason, void *context);
static void app_message_init(void);
static void init(void);
int main(void);
