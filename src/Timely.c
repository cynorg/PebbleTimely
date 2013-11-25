#include <pebble.h>
#include <math.h>

/*
 * If you fork this code and release the resulting app, please be considerate and change all the values in appinfo.json.
 *
 * FIXME - Configuration for version 2.0.
 *
 * DESCRIPTION
 *  This watchface shows the current date and current time in the top 'half',
 *    and then a small calendar w/ 3 weeks: last, current, and next week, in the bottom 'half'
 * END DESCRIPTION Section
 *
 */

struct {
    TextLayer *text_time_layer;
    TextLayer *month_layer;
} ui;

bool black = true;        // Is the background black
bool grid = true;         // Show the grid
bool invert = true;       // Invert colors on today's date
bool vibe_hour = false;   // vibrate at the top of the hour?

// Offset days of week. Values can be between 0 and 6.
// 0 = weeks start on Sunday
// 1 =  weeks start on Monday
int dayOfWeekOffset = 0;

const char daysOfWeek[7][3] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

// How many days are/were in the month
int daysInMonth(int mon, int year)
{
    mon++;

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

void days_layer_update_callback(Layer *me, GContext *ctx)
{
    (void)me;

    GColor background, foreground;

    if (black) {
        background = GColorBlack;
        foreground = GColorWhite;
    } else {
        background = GColorWhite;
        foreground = GColorBlack;
    }

    struct tm *currentTime = get_time();

    int mon = currentTime->tm_mon;
    int year = currentTime->tm_year + 1900;
    int daysThisMonth = daysInMonth(mon, year);

    /* We're going to build an array to hold the dates to be shown in the calendar.
     *
     * There are four 'parts' we'll calculate for this:
     *
     *   daysVisPrevMonth = days from the previous month that are visible
     *   daysPriorToToday = days before today (including any days from previous month)
     *   ( today )
     *   ( days after today, including any days from next month )
     */
    int calendar[21];
    int cellNum = 0; // address for current day table cell: 0-20
    int daysVisPrevMonth = 0;
    int daysPriorToToday = 7 + currentTime->tm_wday - dayOfWeekOffset;
    int today = currentTime->tm_mday;

    // tm_wday is based on Sunday being the startOfWeek, but Sunday may not be our startOfWeek.
    if (currentTime->tm_wday < dayOfWeekOffset) {
        daysPriorToToday += 7; // we're <7, so in the 'first' week due to startOfWeek offset - 'add a week' before this one
    }

    if (daysPriorToToday >= today) {
        // We're showing more days before today than exist this month
        int daysInPrevMonth = daysInMonth(mon - 1, year); // year only matters for February, which will be the same 'from' March

        // Number of days we'll show from the previous month
        daysVisPrevMonth = daysPriorToToday - today + 1;

        for (int i = 0; i < daysVisPrevMonth; i++, cellNum++) {
            calendar[cellNum] = daysInPrevMonth + i - daysVisPrevMonth + 1;
        }
    }

    int firstDayShownThisMonth = daysVisPrevMonth + today - daysPriorToToday;

    // the current day's cell... we'll style this special
    int currentDay = cellNum + today - firstDayShownThisMonth;

    // Add days from this month and the next.
    int day = firstDayShownThisMonth;
    for (; cellNum < 21; cellNum++) {
        calendar[cellNum] = day;

        day++;
        if (day > daysThisMonth) {
            // Start at the beginning of next month.
            // We don't care how many days are in next month because
            // we will always show less than two weeks of it.
            day = 1;
        }
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

    int r = l + d * lw; // position of right side of right column
    int t = b - w * bh; // position of top of top row
    int cw = lw - 1; // width of textarea
    int cl = l + 1;
    int ch = bh - 1;

    // Draw the Gridlines
    if (grid) {
        graphics_context_set_stroke_color(ctx, foreground);

        // horizontal lines
        for (int i = 1; i <= w; i++) {
            graphics_draw_line(ctx, GPoint(l, b - i * bh), GPoint(r, b - i * bh));
        }

        // vertical lines
        for (int i = 1; i < d; i++) {
            graphics_draw_line(ctx, GPoint(l + i * lw, t), GPoint(l + i * lw, b));
        }
    }

    GFont dayFontNormal = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    GFont dayFontHighlight = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

    // Draw days of week
    for (int i = 0; i < 7; i++) {
        GFont whichDayFont;
        // highlight day of week
        int day = i + dayOfWeekOffset;

        if (day == currentTime->tm_wday) {
            whichDayFont = dayFontHighlight;
        } else {
            whichDayFont = dayFontNormal;
        }

        // Adjust labels by specified offset
        if (day > 6) {
            day -= 7;
        }

        if (day < 0) {
            day += 7;
        }

        graphics_draw_text(
            ctx,
            daysOfWeek[day],
            whichDayFont,
            GRect(cl + i * lw, 90, cw, 20),
            GTextOverflowModeWordWrap,
            GTextAlignmentCenter,
            NULL);
    }

    // Fill in the cells with the month days
    for (int i = 0; i < 21; i++) {
        int dow = i % 7;
        int wknum = (i - dow) / 7;

        // New Weeks begin on Sunday
        if (dow > 6) {
            dow = 0;
            wknum++;
        }

        // Is this today?  If so prep special today style
        GFont font;
        int fh;

        if (i == currentDay) {
            if (invert) {
                graphics_context_set_text_color(ctx, background);
                graphics_context_set_fill_color(ctx, foreground);
                graphics_fill_rect(
                    ctx,
                    GRect(
                        l + dow * lw + 1,
                        b - (w - wknum)*bh + 1,
                        cw,
                        ch)
                    , 0
                    , GCornerNone);
            }

            font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
            fh = 20;
        } else {
            // Normal (non-today) style
            font = dayFontNormal;
            fh = 16;
            graphics_context_set_text_color(ctx, foreground);
        }

        // Draw the day
        char date_text[3];
        snprintf(date_text, sizeof(date_text), "%d", calendar[i]);
        graphics_draw_text(
            ctx,
            date_text,
            font,
            GRect(
                cl + dow * lw,
                b - (-0.5 + w - wknum)*bh - fh / 2 - 1,
                cw,
                fh),
            GTextOverflowModeWordWrap,
            GTextAlignmentCenter,
            NULL);
    }
}

void update_month_text()
{
    struct tm *currentTime = get_time();

    static char month_text[20];
    // http://www.cplusplus.com/reference/ctime/strftime/
    strftime(month_text, sizeof(month_text), "%B %d, %Y", currentTime); // Month DD, YYYY
    //strftime(month_text, sizeof(month_text), "%d.%m.%Y", currentTime);  // DD.MM.YYYY

    text_layer_set_text(ui.month_layer, month_text);
}

void update_time_text()
{
    // Need to be static because used by the system later.
    static char time_text[6];
    clock_copy_time_string(time_text, sizeof(time_text));
    text_layer_set_text(ui.text_time_layer, time_text);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
    update_time_text();

    if (units_changed & MONTH_UNIT) {
        update_month_text();
    }

    if ((units_changed & HOUR_UNIT) && vibe_hour) {
        vibes_short_pulse();
    }

    // days_layer gets redrawn every time because time_text_layer is changed and all layers are redrawn together.
}

TextLayer *make_text_layer(Window *window, GRect rect, const char *font_key)
{
    TextLayer *layer = text_layer_create(rect);

    if (black) {
        text_layer_set_text_color(layer, GColorWhite);
    }

    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);
    text_layer_set_font(layer, fonts_get_system_font(font_key));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(layer));
    return layer;
}

int main()
{
    Window *window;

    //FIXME - Configuration

    window = window_create();
    window_stack_push(window, false);

    if (black) {
        window_set_background_color(window, GColorBlack);
    }

    ui.month_layer = make_text_layer(window, GRect(0, 0, 144, 30), FONT_KEY_GOTHIC_24);
    ui.text_time_layer = make_text_layer(window, GRect(0, 26, 144, 168 - 22), FONT_KEY_ROBOTO_BOLD_SUBSET_49);

    Layer *days_layer = layer_create(GRect(0, 0, 144, 168));
    layer_set_update_proc(days_layer, days_layer_update_callback);
    layer_add_child(window_get_root_layer(window), days_layer);

    update_time_text();
    update_month_text();

    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

    app_event_loop();
}
