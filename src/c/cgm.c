#include "pebble.h"
#include "stddef.h"
#include "string.h"
#include "stdlib.h"
#include "pebble_chart.h"
#include "data-processor.h"

#define ANTIALIASING true
#define BATTERY_OUTLINE_WIDTH PBL_IF_ROUND_ELSE((14), (20))
#define BATTERY_OUTLINE_HEIGHT PBL_IF_ROUND_ELSE((6), (8))
#define ROUGE PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed , GColorDarkGray)
#define MED PBL_IF_COLOR_ELSE(GColorChromeYellow , GColorLightGray)
#define GColorWhiteInit { GColorWhiteARGB8 }
#define GColorDarkInit { GColorOxfordBlueARGB8 }
#define GColorLightInit { GColorPictonBlueARGB8 }
#define GColorBGInit { GColorMidnightGreenARGB8 }
#define PBLUE_ELSE_WHITE PBL_IF_ROUND_ELSE(GColorPictonBlue, GColorWhite)
#define WHITE_ELSE_BATTERY PBL_IF_COLOR_ELSE(GColorWhite, battery_colour)
Window *window_cgm = NULL;
Layer *window_layer_cgm = NULL;
ChartLayer* chart_layer = NULL;

// MAIN WINDOW LAYER
TextLayer *tophalf_layer = NULL;
TextLayer *bg_layer = NULL;
TextLayer *cgmtime_layer = NULL;
TextLayer *message_layer = NULL;    // BG DELTA & MESSAGE LAYER
TextLayer *watch_battlevel_layer = NULL;
TextLayer *rig_battlevel_layer = NULL;
TextLayer *t1dname_layer = NULL;
TextLayer *time_watch_layer = NULL;
TextLayer *date_app_layer = NULL;
TextLayer *happymsg_layer = NULL;
TextLayer *raw_calc_layer = NULL;
TextLayer *noise_layer = NULL;
TextLayer *cob_layer = NULL;
TextLayer *basal_layer = NULL;
TextLayer *time_layer = NULL;
TextLayer *s_layer =NULL;
TextLayer *predict_layer =NULL;

// main window layer
BitmapLayer *icon_layer = NULL;
BitmapLayer *perfectbg_layer = NULL;
BitmapLayer *battery_layer = NULL;
BitmapLayer *symbol_layer = NULL;

// main window layer
GBitmap *icon_bitmap = NULL;
GBitmap *specialvalue_bitmap = NULL;
GBitmap *perfectbg_bitmap = NULL;
GBitmap *battery_bitmap = NULL;
GBitmap *symbol_bitmap = NULL;

static Layer *circle_layer;
static Layer *cob_circle_layer;
static Layer *name_circle_layer;
static Layer *predict_circle_layer;

static Layer *batteryGraphicsLayer;

static GPath *batteryOutlinePath = NULL;

//BATTERY CODE
static const GPathInfo BATTERY_OUTLINE = {
    .num_points = 4,
    .points = (GPoint []) {{0, 0}, {BATTERY_OUTLINE_WIDTH, 0}, {BATTERY_OUTLINE_WIDTH, BATTERY_OUTLINE_HEIGHT}, {0, BATTERY_OUTLINE_HEIGHT}}
};
static int batteryLevel = 0;

//ANIMATION
PropertyAnimation *perfectbg_animation = NULL;
PropertyAnimation *happymsg_animation = NULL;

static char time_watch_text[] = "00:00";
static char date_app_text[] = "Wed 13 ";

// variables for AppSync
AppSync sync_cgm;
uint8_t AppSyncErrAlert = 100;

// BUFFER
static uint8_t sync_buffer_cgm[560]; //was 408

// variables for timers and time
AppTimer *timer_cgm = NULL;
AppTimer *BT_timer = NULL;
AppTimer *timer = NULL;
AppTimer *layer_timer = NULL;
time_t cgm_time_now = 0;
time_t app_time_now = 0;
int timeformat = 0;


//SHAKE
void timer_callback(void *data) {
  layer_set_hidden(text_layer_get_layer(s_layer), true);
}
  
// global variable for bluetooth connection
bool bt_connected=true;
// global variables for sync tuple functions
static char current_icon[4] = {0};
static char last_bg[6] = {0};
static char last_battlevel[4] = {0};
uint8_t current_battlevel = 0;
uint32_t current_cgm_time = 0;
uint32_t stored_cgm_time = 0;
uint32_t current_cgm_timeago = 0;
uint8_t init_loading_cgm_timeago = 111;
char cgm_label_buffer[6] = {0};
int cgm_timeago_diff = 0;

// global variable for single state machine
// sometimes we have to know we have cleared an outage, but the outage flags
// have not been cleared in their single states; this is for that condition
uint8_t ClearedOutage = 100;
uint8_t ClearedBTOutage = 100;

uint32_t current_app_time = 0;
static char current_bg_delta[8] = {0};
static char last_calc_raw[6] = {0};
static char current_symbol[4] = {0};
static char current_cob[8] = {0};
static char current_name[6] = {0};
static char current_basal[8] = {0};
static char last_ok_time[6] = {1};
static char pump_status[60] = {0};
static char predict[6] = {0};

int color_value = 0;
GColor top_colour = GColorWhiteInit;
GColor bottom_colour = GColorDarkInit;
GColor text_colour = GColorDarkInit;
GColor plot_colour = GColorDarkInit;
GColor chart_colour = GColorLightInit;
GColor battery_colour = GColorLightInit;
GColor bg_colour = GColorBGInit;

uint8_t current_noise_value = 0;
int current_bg = 0;
int current_calc_raw = 0;
int current_calc_raw1 = 0;
uint8_t currentBG_isMMOL = 100;
int converted_bgDelta = 0;
static char current_values[40] = {0};
uint8_t HaveCalcRaw = 100;

// chart values
int conv_last_bgsx = 0;
int conv_current_bgsx = 0;
int bgsx_array_counter = 0;
static int * bgsx_array;
int last_bgty = 0;
int current_bgty = 0;
int bgty_array_counter = 0;
static int * bgty_array;
bool bgsx_array_set = false;
bool bgty_array_set = false;
static int tag_raw = 0;

// global BG snooze timer
static uint8_t lastAlertTime = 0;

// global special value alert
static uint8_t specvalue_alert = 100;
// global overwrite variables for vibrating when hit a specific BG if already in a snooze
static uint8_t specvalue_overwrite = 100;
static uint8_t hypolow_overwrite = 100;
static uint8_t biglow_overwrite = 100;
static uint8_t midlow_overwrite = 100;
static uint8_t low_overwrite = 100;
static uint8_t high_overwrite = 100;
static uint8_t midhigh_overwrite = 100;
static uint8_t bighigh_overwrite = 100;

// global retries counters for timeout problems
static uint8_t appsyncandmsg_retries_counter = 0;
static uint8_t dataoffline_retries_counter = 0;

// global variables for vibrating in special conditions
static uint8_t BluetoothAlert = 100;
static uint8_t BT_timer_pop = 100;
static uint8_t DataOfflineAlert = 100;
static uint8_t CGMOffAlert = 100;
static uint8_t PhoneOffAlert = 100;
static uint8_t LoopOutAlert = 100;

// global constants for time durations; seconds
static const uint8_t  MINUTEAGO = 60;
static const uint16_t HOURAGO = 60*(60); //removed () from second 60
static const uint32_t DAYAGO = 24*(60*60);
static const uint32_t WEEKAGO = 7*(24*60*60);
static const uint32_t TWOYEARSAGO = 2*365*(24*60*60);
static const uint16_t MS_IN_A_SECOND = 1000;

// Constants for string buffers
// If add month to date, buffer size needs to increase to 12; also need to reformat date_app_text init string
static const uint8_t TIME_TEXTBUFF_SIZE = 6;
static const uint8_t DATE_TEXTBUFF_SIZE = 8;
//static const uint8_t LABEL_BUFFER_SIZE = 6;
//static const uint8_t TIMEAGO_BUFFER_SIZE = 10;
//static const uint8_t BATTLEVEL_FORMAT_SIZE = 12;

// ** START OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **
// ** FOR MMOL, ALL VALUES ARE STORED AS INTEGER; LAST DIGIT IS USED AS DECIMAL **
// ** BE EXTRA CAREFUL OF CHANGING SPECIAL VALUES OR TIMERS; DO NOT CHANGE WITHOUT EXPERT HELP **

// FOR BG RANGES
// DO NOT SET ANY BG RANGES EQUAL TO ANOTHER; LOW CAN NOT EQUAL MIDLOW
// LOW BG RANGES MUST BE IN ASCENDING ORDER; SPECVALUE < HYPOLOW < BIGLOW < MIDLOW < LOW
// HIGH BG RANGES MUST BE IN ASCENDING ORDER; HIGH < MIDHIGH < BIGHIGH
// DO NOT ADJUST SPECVALUE UNLESS YOU HAVE A VERY GOOD REASON
// DO NOT USE NEGATIVE NUMBERS OR DECIMAL POINTS OR ANYTHING OTHER THAN A NUMBER

// BG Ranges, MG/DL
uint16_t SPECVALUE_BG_MGDL = 20;
uint16_t SHOWLOW_BG_MGDL = 40;
uint16_t HYPOLOW_BG_MGDL = 55;
uint16_t BIGLOW_BG_MGDL = 60;
uint16_t MIDLOW_BG_MGDL = 70;
uint16_t LOW_BG_MGDL = 80;

uint16_t HIGH_BG_MGDL = 180;
uint16_t MIDHIGH_BG_MGDL = 240;
uint16_t BIGHIGH_BG_MGDL = 300;
uint16_t SHOWHIGH_BG_MGDL = 400;

// BG Ranges, MMOL
// VALUES ARE IN INT, NOT FLOATING POINT, LAST DIGIT IS DECIMAL
// FOR EXAMPLE : SPECVALUE IS 1.1, BIGHIGH IS 16.6
// ALWAYS USE ONE AND ONLY ONE DECIMAL POINT FOR LAST DIGIT
// GOOD : 5.0, 12.2 // BAD : 7 , 14.44
uint16_t SPECVALUE_BG_MMOL = 11;
uint16_t SHOWLOW_BG_MMOL = 23;
uint16_t HYPOLOW_BG_MMOL = 30;
uint16_t BIGLOW_BG_MMOL = 33;
uint16_t MIDLOW_BG_MMOL = 39;
uint16_t LOW_BG_MMOL = 44;

uint16_t HIGH_BG_MMOL = 100;
uint16_t MIDHIGH_BG_MMOL = 133;
uint16_t BIGHIGH_BG_MMOL = 166;
uint16_t SHOWHIGH_BG_MMOL = 222;

// BG Snooze Times, in Minutes; controls when vibrate again
// RANGE 0-240
uint8_t SPECVALUE_SNZ_MIN = 30;
uint8_t HYPOLOW_SNZ_MIN = 5;
uint8_t BIGLOW_SNZ_MIN = 5;
uint8_t MIDLOW_SNZ_MIN = 10;
uint8_t LOW_SNZ_MIN = 15;
uint8_t HIGH_SNZ_MIN = 30;
uint8_t MIDHIGH_SNZ_MIN = 30;
uint8_t BIGHIGH_SNZ_MIN = 30;

// Vibration Levels; 0 = NONE; 1 = LOW; 2 = MEDIUM; 3 = HIGH
// IF YOU DO NOT WANT A SPECIFIC VIBRATION, SET TO 0
uint8_t SPECVALUE_VIBE = 2;
uint8_t HYPOLOWBG_VIBE = 3;
uint8_t BIGLOWBG_VIBE = 3;
uint8_t LOWBG_VIBE = 3;
uint8_t HIGHBG_VIBE = 2;
uint8_t BIGHIGHBG_VIBE = 2;
uint8_t DOUBLEDOWN_VIBE = 3;
uint8_t APPSYNC_ERR_VIBE = 1;
uint8_t BTOUT_VIBE = 1;
uint8_t CGMOUT_VIBE = 1;
uint8_t PHONEOUT_VIBE = 1;
uint8_t LOWBATTERY_VIBE = 1;
uint8_t DATAOFFLINE_VIBE = 1;
uint8_t NOLOOP_VIBE = 1;

// Icon Cross Out & Vibrate Once Wait Times, in Minutes
// RANGE 0-240
// IF YOU WANT TO WAIT LONGER TO GET CONDITION, INCREASE NUMBER
static const uint8_t CGMOUT_WAIT_MIN = 15;
static const uint8_t CGMOUT_INIT_WAIT_MIN = 7;
static const uint8_t PHONEOUT_WAIT_MIN = 5;

// Chart Size CURRENT RANGE 0-10
static const uint8_t MAX_BG_ARRAY_SIZE = 12;

// Control Messages
// IF YOU DO NOT WANT A SPECIFIC MESSAGE, SET TO 111 (true)
static const uint8_t TurnOff_NOBLUETOOTH_Msg = 100;
static const uint8_t TurnOff_CHECKCGM_Msg = 100;
static const uint8_t TurnOff_CHECKPHONE_Msg = 100;

// Control Vibrations
// SPECIAL FLAG TO HARD CODE VIBRATIONS OFF; If you want no vibrations, SET TO 111 (true)
// Use for Sleep Face or anyone else for a custom load
uint8_t HardCodeNoVibrations = 100;

// Control Animations
// SPECIAL FLAG TO HARD CODE ANIMATIONS OFF; If you want no animations, SET TO 111 (true)
// SPECIAL FLAG TO HARD CODE ANIMATIONS ALL ON; If you want all animations, SET TO 111 (true)
// This is for people who want old ones too
// Use for a custom load
uint8_t HardCodeNoAnimations = 100;
uint8_t HardCodeAllAnimations = 100;

// Control Raw data
// If you want to turn off vibrations for calculated raw, set to 111 (true)
uint8_t TurnOffVibrationsCalcRaw = 100;
// If you want to see unfiltered raw, set to 111 (true)
//uint8_t TurnOnUnfilteredRaw = 111;

// ** END OF CONSTANTS THAT CAN BE CHANGED; DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING **

// Control Vibrations for Config File
// IF YOU WANT NO VIBRATIONS, SET TO 111 (true)
uint8_t TurnOffAllVibrations = 100;
// IF YOU WANT LESS INTENSE VIBRATIONS, SET TO 111 (true)
uint8_t TurnOffStrongVibrations = 100;

// Bluetooth Timer Wait Time, in Seconds
// RANGE 0-240
// THIS IS ONLY FOR BAD BLUETOOTH CONNECTIONS
// TRY EXTENDING THIS TIME TO SEE IF IT WIL HELP SMOOTH CONNECTION
// CGM DATA RECEIVED EVERY 60 SECONDS, GOING BEYOND THAT MAY RESULT IN MISSED DATA
static const uint8_t BT_ALERT_WAIT_SECS = 55;

// Message Timer & Animate Wait Times, in Seconds
static const uint8_t WATCH_MSGSEND_SECS = 57; //CHANGED FROM 60 DEC 1
static const uint8_t LOADING_MSGSEND_SECS = 5; //CHANGED FROM 60 DEC 1
static const uint8_t PERFECTBG_ANIMATE_SECS = 10;
static const uint8_t HAPPYMSG_ANIMATE_SECS = 10;
//static const uint8_t LAYER_ANIMATE_SECS = 10;

// App Sync / Message retries, for timeout / busy problems
// Change to see if there is a temp or long term problem
// This is approximately number of seconds, so if set to 50, timeout is at 50 seconds
// However, this can vary widely - can be up to 6 seconds .... for 50, timeout can be up to 3 minutes
static const uint8_t APPSYNCANDMSG_RETRIES_MAX = 50;

// HTML Request retries, for timeout / busy problems
// Change to see if there is a temp or long term problem
// This is number of minutes, so if set to 11 timeout is at 11 minutes
static const uint8_t DATAOFFLINE_RETRIES_MAX = 14;
//REMOVE
//global variable for mode share
//uint8_t current_mode_value = 0;// US_Share 1, Other_Share 2, Nightscout 3

enum CgmKey {
    CGM_ICON_KEY = 0x0, // TUPLE_CSTRING, MAX 2 BYTES (10)
    CGM_BG_KEY = 0x1, // TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
    CGM_TCGM_KEY = 0x2, // TUPLE_INT, 4 BYTES (CGM TIME)
    CGM_TAPP_KEY = 0x3, // TUPLE_INT, 4 BYTES (APP / PHONE TIME)
    CGM_DLTA_KEY = 0x4, // TUPLE_CSTRING, MAX 5 BYTES (BG DELTA, -100 or -10.0)
    CGM_UBAT_KEY = 0x5, // TUPLE_CSTRING, MAX 3 BYTES (UPLOADER BATTERY, 100)
    CGM_NAME_KEY = 0x6, 
    CGM_VALS_KEY = 0x7,   // TUPLE_CSTRING, MAX 60 BYTES (0,000,000,000,000,0,0,0,0,0)
    CGM_CLRW_KEY = 0x8,   // TUPLE_CSTRING, MAX 4 BYTES (253 OR 22.2)
    CGM_BGSX_KEY = 0x9, // TUPLE_CSTRING, MAX 28 BYTES
    CGM_COB_KEY = 0xA, // COB MAX 4 BYTES
    CGM_SYM_KEY = 0xB,
    CGM_TIME_KEY = 0xC,
    CGM_BASAL_KEY = 0xD,
    CGM_PUMP_KEY = 0xE,
    CGM_PREDICT_KEY = 0xF,

};

// ARRAY OF SPECIAL VALUE ICONS
static const uint8_t SPECIAL_VALUE_ICONS[] = {
    RESOURCE_ID_IMAGE_PIXEL,   //0
    RESOURCE_ID_IMAGE_BROKEN_ANTENNA,   //1
    RESOURCE_ID_IMAGE_BLOOD_DROP,       //2
    RESOURCE_ID_IMAGE_STOP_LIGHT,       //3
    RESOURCE_ID_IMAGE_HOURGLASS,        //4
    RESOURCE_ID_IMAGE_QUESTION_MARKS,   //5
    RESOURCE_ID_IMAGE_LOGO,      //6
    RESOURCE_ID_IMAGE_LIGHTNING, //7
    RESOURCE_ID_IMAGE_LOOP, //8
    RESOURCE_ID_IMAGE_X, //9
    RESOURCE_ID_IMAGE_CIRCLE, //10
    RESOURCE_ID_IMAGE_WARNING, //11
};

// INDEX FOR ARRAY OF SPECIAL VALUE ICONS
static const uint8_t NONE_ICON_INDX = 0;
static const uint8_t BROKEN_ANTENNA_ICON_INDX = 1;
static const uint8_t BLOOD_DROP_ICON_INDX = 2;
static const uint8_t STOP_LIGHT_ICON_INDX = 3;
static const uint8_t HOURGLASS_ICON_INDX = 4;
static const uint8_t QUESTION_MARKS_ICON_INDX = 5;
static const uint8_t LOGOSPECIAL_ICON_INDX = 6;
static const uint8_t LIGHTNING_ICON_INDX = 7;
static const uint8_t LOOP_ICON_INDX = 8;
static const uint8_t X_ICON_INDX = 9;
static const uint8_t CIRCLE_ICON_INDX = 10;
static const uint8_t WARNING_ICON_INDX = 11;


//SHAKE HANDLER
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  bool did_vibrate = NULL;
  if(!did_vibrate) {
    if((axis == ACCEL_AXIS_X) || (axis == ACCEL_AXIS_Z) || (axis == ACCEL_AXIS_Y)) {
    layer_set_hidden(text_layer_get_layer(s_layer), false);
    timer = app_timer_register(5000, timer_callback, NULL);
}
  } else {
    // Discard with a warning
    //APP_LOG(APP_LOG_LEVEL_WARNING, "Vibration occured during collection");
  }
}  
//ADD WAKEUP
static void prv_restart(void* data) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "restarting...");

  // will close the app
  window_stack_pop_all(false);

  // schedule a wakeup 2s after now to make sure the watchface gets restarted
  // NOTE: we do 2s instead of 1s to avoid a potential rare race condition where the
  // current second flips after we obtained the current time
  const time_t future_time = time(NULL) + 2;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "will restart at %lu", future_time);
  wakeup_schedule(future_time, 0, false);
}

static char *translate_app_error(AppMessageResult result) {
    switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "APP UNKNOWN ERROR";
    }
}

static char *translate_dict_error(DictionaryResult result) {
    switch (result) {
    case DICT_OK: return "DICT_OK";
    case DICT_NOT_ENOUGH_STORAGE: return "DICT_NOT_ENOUGH_STORAGE";
    case DICT_INVALID_ARGS: return "DICT_INVALID_ARGS";
    case DICT_INTERNAL_INCONSISTENCY: return "DICT_INTERNAL_INCONSISTENCY";
    case DICT_MALLOC_FAILED: return "DICT_MALLOC_FAILED";
    default: return "DICT UNKNOWN ERROR";
    }
}

char *strtok(s, delim)
    register char *s;
register const char *delim;
{
    register char *spanp;
    register int c, sc;
    char *tok;
    static char *last;


    if (s == NULL && (s = last) == NULL)
        return (NULL);

    /*
    * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
    */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc)
            goto cont;
    }

    if (c == 0) { /* no non-delimiter characters */
        last = NULL;
        return (NULL);
    }
    tok = s - 1;

    /*
    * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
    * Note that delim must have one NUL; we stop if we see that, too.
    */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                last = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}


int myBGAtoi(char *str) {

    // VARIABLES
    int res = 0; // Initialize result
    // CODE START

    // initialize currentBG_isMMOL flag
    currentBG_isMMOL = 100;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, START currentBG is MMOL: %i", currentBG_isMMOL );
    // Iterate through all characters of input string and update result
    for (int i = 0; str[i] != '\0'; ++i) {

        //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, STRING IN: %s", &str[i] );

        if (str[i] == ('.')) {
            currentBG_isMMOL = 111;
        }
        else if ( (str[i] >= ('0')) && (str[i] <= ('9')) ) {
            res = res*10 + str[i] - '0';
        }

        //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, FOR RESULT OUT: %i", res );
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, currentBG is MMOL: %i", currentBG_isMMOL );
    }
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "myBGAtoi, FINAL RESULT OUT: %i", res );
    return res;
} // end myBGAtoi

static void load_colour() {

  //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD COLOR, FUNCTION START");
//  int number_of_colors = 2;

  // can select colors

  switch (color_value)  {
    // Blues
    case 0:;
      top_colour = GColorWhite;
      bg_colour = GColorMidnightGreen;
      bottom_colour = GColorOxfordBlue;
      battery_colour = GColorPictonBlue;
      text_colour = GColorOxfordBlue;
      plot_colour = GColorOxfordBlue ;
      chart_colour = GColorPictonBlue;
      text_layer_set_background_color(happymsg_layer, GColorPictonBlue);
      text_layer_set_text_color(happymsg_layer, GColorOxfordBlue);
      window_set_background_color(window_cgm, GColorOxfordBlue);
      chart_layer_set_plot_color(chart_layer, GColorOxfordBlue);
      chart_layer_set_canvas_color(chart_layer, GColorPictonBlue);
      text_layer_set_text_color(date_app_layer, GColorPictonBlue);
      text_layer_set_text_color(message_layer, GColorOxfordBlue);
      text_layer_set_text_color(t1dname_layer, GColorOxfordBlue);
      text_layer_set_text_color(cob_layer, GColorOxfordBlue);
      break;

    // Purples
    case 1:;
      top_colour =    GColorWhite;
      bg_colour = GColorIndigo;
      battery_colour = GColorBabyBlueEyes;
      bottom_colour = GColorIndigo ;
      text_colour =   GColorIndigo;
      plot_colour =   GColorIndigo;
      chart_colour =  GColorBabyBlueEyes;
      window_set_background_color(window_cgm, GColorIndigo);
      chart_layer_set_plot_color(chart_layer, GColorIndigo);
      chart_layer_set_canvas_color(chart_layer, GColorBabyBlueEyes);
      text_layer_set_background_color(happymsg_layer, GColorBabyBlueEyes);
      text_layer_set_text_color(happymsg_layer, GColorIndigo);
      text_layer_set_text_color(date_app_layer, GColorBabyBlueEyes);
      text_layer_set_text_color(raw_calc_layer, GColorIndigo);
      text_layer_set_text_color(message_layer, GColorIndigo);
      text_layer_set_text_color(t1dname_layer, GColorIndigo);
      text_layer_set_text_color(cob_layer, GColorIndigo);

      break;
    // Black and White
    case 2:;
      top_colour = GColorWhite;
      bg_colour = GColorMidnightGreen;
      battery_colour = GColorWhite;
      bottom_colour = GColorBlack ;
      text_colour = GColorBlack;
      plot_colour = GColorBlack;
      chart_colour = GColorWhite;
      window_set_background_color(window_cgm, GColorBlack);
      chart_layer_set_plot_color(chart_layer, GColorBlack);
      chart_layer_set_canvas_color(chart_layer, GColorWhite);
      text_layer_set_background_color(happymsg_layer, GColorWhite);
      text_layer_set_text_color(happymsg_layer, GColorBlack);
      text_layer_set_text_color(date_app_layer, GColorWhite);
      //text_layer_set_background_color(tophalf_layer, GColorWhite);
      text_layer_set_text_color(raw_calc_layer, GColorBlack);
      text_layer_set_text_color(message_layer, GColorBlack);
      text_layer_set_text_color(t1dname_layer, GColorBlack);
      text_layer_set_text_color(cob_layer, GColorBlack);

      break;
    // Greens
    case 3:;
      top_colour = GColorWhite;
      bg_colour = GColorMidnightGreen;
      battery_colour = GColorMalachite;
      bottom_colour = GColorDarkGreen ;
      text_colour = GColorDarkGreen ;
      plot_colour = GColorDarkGreen ;
      chart_colour = GColorJaegerGreen;
      window_set_background_color(window_cgm, GColorDarkGreen);
      chart_layer_set_plot_color(chart_layer, GColorWhite);
      chart_layer_set_canvas_color(chart_layer, GColorJaegerGreen);
      text_layer_set_background_color(happymsg_layer, GColorJaegerGreen);
      text_layer_set_text_color(happymsg_layer, GColorWhite);
      text_layer_set_text_color(date_app_layer, GColorMalachite );
     // text_layer_set_background_color(tophalf_layer, GColorWhite);
      text_layer_set_text_color(raw_calc_layer, GColorDarkGreen);
      text_layer_set_text_color(message_layer, GColorDarkGreen);
      text_layer_set_text_color(t1dname_layer, GColorDarkGreen);
      text_layer_set_text_color(cob_layer, GColorDarkGreen);
   break;
    // Pinks
    case 4:;
      top_colour = GColorRichBrilliantLavender;
      bg_colour = GColorJazzberryJam;
      battery_colour = GColorRichBrilliantLavender;
      bottom_colour = GColorJazzberryJam  ;
      text_colour = GColorJazzberryJam ;
      plot_colour = GColorWhite ;
      chart_colour = GColorFashionMagenta;
      window_set_background_color(window_cgm, GColorJazzberryJam);
      chart_layer_set_plot_color(chart_layer, GColorWhite );
      chart_layer_set_canvas_color(chart_layer, GColorFashionMagenta);
      text_layer_set_background_color(happymsg_layer, GColorFashionMagenta);
      text_layer_set_text_color(happymsg_layer, GColorWhite);
      text_layer_set_text_color(date_app_layer, GColorRichBrilliantLavender);
      text_layer_set_text_color(raw_calc_layer, GColorJazzberryJam);
      //text_layer_set_background_color(tophalf_layer, GColorRichBrilliantLavender);
      text_layer_set_text_color(message_layer, GColorJazzberryJam);
      text_layer_set_text_color(t1dname_layer, GColorJazzberryJam);
      text_layer_set_text_color(cob_layer, GColorJazzberryJam);
    break;
    }
    //layer_mark_dirty(tophalf_layer);

} // end load_colour

static void load_values(){
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"Loaded Values: %s", current_values);

    int num_a_items = 0;
    char *o;
    int mgormm = 0;
    int vibes = 0;
    int rawvibrate = 0;
    int vibeon = 0;
    int animateon = 0;

    if (current_values == NULL) {
        return;
    } else {
        o = strtok(current_values,",");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "mg or mm: %s", o);
        mgormm = atoi(o);

        while(o != NULL) {
            num_a_items++;
            switch (num_a_items) {
            case 2:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "lowbg: %s", o);
                if (mgormm == 0){
                    LOW_BG_MGDL = atoi(o);
                    if (LOW_BG_MGDL < 60) {
                        MIDLOW_BG_MGDL = 55;
                        BIGLOW_BG_MGDL = 50;
                        HYPOLOW_BG_MGDL = 45;
                    } else if (LOW_BG_MGDL < 70) {
                        MIDLOW_BG_MGDL = 60;
                        BIGLOW_BG_MGDL = 55;
                        HYPOLOW_BG_MGDL = 50;
                    }
                } else {
                    LOW_BG_MMOL = atoi(o);
                    if (LOW_BG_MMOL < 33) {
                        MIDLOW_BG_MMOL = 31;
                        BIGLOW_BG_MMOL = 28;
                        HYPOLOW_BG_MMOL = 25;
                    } else if (LOW_BG_MMOL < 39) {
                        MIDLOW_BG_MMOL = 33;
                        BIGLOW_BG_MMOL = 31;
                        HYPOLOW_BG_MMOL = 28;
                    }
                }
                break;
            case 3:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "highbg: %s", o);
                if (mgormm == 0){
                    HIGH_BG_MGDL = atoi(o);
                    if (HIGH_BG_MGDL > 239) {
                        MIDHIGH_BG_MGDL = 300;
                        BIGHIGH_BG_MGDL = 350;
                    }
                } else {
                    HIGH_BG_MMOL = atoi(o);
                    if (HIGH_BG_MMOL > 132) {
                        MIDHIGH_BG_MMOL = 166;
                        BIGHIGH_BG_MMOL =  200;
                    }
                }
                break;
            case 4:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "lowsnooze: %s", o);
                LOW_SNZ_MIN = atoi(o);
                break;
            case 5:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "highsnooze: %s", o);
                HIGH_SNZ_MIN = atoi(o);
                break;
            case 6:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "lowvibe: %s", o);
                LOWBG_VIBE = atoi(o);
                break;
            case 7:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "highvibe: %s", o);
                HIGHBG_VIBE = atoi(o);
                break;
            case 8:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "vibepattern: %s", o);
                vibes = atoi(o);
                if (vibes == 0){
                    TurnOffAllVibrations = 111;
                    TurnOffStrongVibrations = 111;
                } else if (vibes == 1){
                    TurnOffAllVibrations = 100;
                    TurnOffStrongVibrations = 111;
                } else if (vibes == 2){
                    TurnOffAllVibrations = 100;
                    TurnOffStrongVibrations = 100;
                }
                break;
            case 9:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "timeformat: %s", o);
                timeformat = atoi(o);
                break;
            case 10:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "rawvibrate: %s", o);
                rawvibrate = atoi(o);
                if (rawvibrate == 0) { TurnOffVibrationsCalcRaw = 111; }
                else { TurnOffVibrationsCalcRaw = 100; }
                break;
              
            case 11:              
              //APP_LOG(APP_LOG_LEVEL_DEBUG, "color_value: %s", o);
                color_value = atoi(o);
                break;
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "vibeon: %s", o);
         case 12:
              vibeon = atoi(o);
          if (vibeon == 0) { 
            // turn vibrator off; emoji will be set to sleep so have visual indication
            HardCodeNoVibrations = 111; 
          }
          else { 
            // turn vibrator on 
            HardCodeNoVibrations = 100;
          }
          break;
            case 13:
               //APP_LOG(APP_LOG_LEVEL_DEBUG, "animateon: %s", o);
          animateon = atoi(o);
          if (animateon == 0) { HardCodeNoAnimations = 111; }
          else { HardCodeNoAnimations = 100; }
          break; 

            }
            o = strtok(NULL,",");
        }
    }
} //End load_values

//CONTAINER FOR ARROWS & SPECIAL ICONS
static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
    GBitmap *old_image = *bmp_image;
    //destroy_null_GBitmap(bmp_image);

    *bmp_image = gbitmap_create_with_resource(resource_id);
    GRect bitmap_bounds = gbitmap_get_bounds((*bmp_image));
    GRect frame = GRect(origin.x, origin.y, bitmap_bounds.size.w, bitmap_bounds.size.h);
    bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
    layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

    if (old_image != NULL) {
        gbitmap_destroy(old_image);
    }
       // APP_LOG(APP_LOG_LEVEL_INFO, "1 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

}
//END CONTAINER
//NULL AND CANCEL TIMER
static void null_and_cancel_timer (AppTimer **timer_to_null, bool cancel_timer) {

    if (*timer_to_null != NULL) {
      if (cancel_timer == true) { app_timer_cancel(*timer_to_null); }
        *timer_to_null = NULL;
    }
} // end null_and_cancel_timer

//DESTROY NULL BITMAP
static void destroy_null_GBitmap(GBitmap **GBmp_image) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: ENTER CODE");
    if (*GBmp_image != NULL) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: POINTER EXISTS, DESTROY BITMAP IMAGE");
        gbitmap_destroy(*GBmp_image);
        if (*GBmp_image != NULL) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: POINTER EXISTS, SET POINTER TO NULL");
            *GBmp_image = NULL;
        }
    }
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL GBITMAP: EXIT CODE");
} // end destroy_null_GBitmap
//DESTROY NULL BITMAP LAYER
static void destroy_null_BitmapLayer(BitmapLayer **bmp_layer) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: ENTER CODE");
    if (*bmp_layer != NULL) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: POINTER EXISTS, DESTROY BITMAP LAYER");
        bitmap_layer_destroy(*bmp_layer);
        if (*bmp_layer != NULL) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: POINTER EXISTS, SET POINTER TO NULL");
            *bmp_layer = NULL;
        }
    }
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL BITMAP: EXIT CODE");
} // end destroy_null_BitmapLayer

//DESTROY NULL TEXT LAYER
static void destroy_null_TextLayer(TextLayer **txt_layer) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: ENTER CODE");
    if (*txt_layer != NULL) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: POINTER EXISTS, DESTROY TEXT LAYER");
        text_layer_destroy(*txt_layer);
        if (*txt_layer != NULL) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: POINTER EXISTS, SET POINTER TO NULL");
            *txt_layer = NULL;
        }
    }
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL TEXT LAYER: EXIT CODE");
} // end destroy_null_TextLayer

//DESTROY NULL LAYER
static void destroy_null_Layer(Layer **layer) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL LAYER: ENTER CODE");
    if (*layer != NULL) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL LAYER: POINTER EXISTS, DESTROY LAYER");
        layer_destroy(*layer);
        if (*layer != NULL) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "DESTROY NULL LAYER: POINTER EXISTS, SET POINTER TO NULL");
            *layer = NULL;
        }
    }
} // end destroy_null_Layer

static void create_update_bitmap(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id) {
    //APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: ENTER CODE");

    // if bitmap pointer exists, destroy and set to NULL
    destroy_null_GBitmap(bmp_image);
    // create bitmap and pointer
    //APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: CREATE BITMAP");
    *bmp_image = gbitmap_create_with_resource(resource_id);

    if (*bmp_image == NULL) {
        // couldn't create bitmap, return so don't crash
        //APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: COULDNT CREATE BITMAP, RETURN");
        return;
    }
    else {
        // set bitmap
        //APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: SET BITMAP");
        bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
    }
    //APP_LOG(APP_LOG_LEVEL_INFO, " CREATE UPDATE BITMAP: EXIT CODE");
} // end create_update_bitmap

//SET MESSAGE LAYER
void set_message_layer (char *msg_string, char *msg_buffer, bool use_msg_buffer, GColor msg_colour) {
  text_layer_set_text_color(message_layer, msg_colour);
  if (use_msg_buffer) { text_layer_set_text(message_layer, (char *)msg_buffer); }
  else { text_layer_set_text(message_layer, (char *)msg_string); }
} // end set_message_layer

//CLEAR TIME AGO
void clear_cgm_timeago () {

  // erase cgm timeago time
  text_layer_set_text(cgmtime_layer, "");
  init_loading_cgm_timeago = 111;

} // end clear_cgm_timeago
static void alert_handler_cgm(uint8_t alertValue) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER");
   // APP_LOG(APP_LOG_LEVEL_DEBUG, "ALERT CODE: %d", alertValue);
    // CONSTANTS
    // constants for vibrations patterns; has to be uint32_t, measured in ms, maximum duration 10000ms
    // Vibe pattern: ON, OFF, ON, OFF; ON for 500ms, OFF for 100ms, ON for 100ms;
    // CURRENT PATTERNS
    const uint32_t highalert_fast[] = { 300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300,100,50,100,300 };
    const uint32_t medalert_long[] = { 500,100,100,100,500,100,100,100,500,100,100,100,500,100,100,100,500 };
    const uint32_t lowalert_beebuzz[] = { 75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75,50,50,50,75 };
    // PATTERN DURATION
    const uint8_t HIGHALERT_FAST_STRONG = 33;
    const uint8_t HIGHALERT_FAST_SHORT = (33/2);
    const uint8_t MEDALERT_LONG_STRONG = 17;
    const uint8_t MEDALERT_LONG_SHORT = (17/2);
    const uint8_t LOWALERT_BEEBUZZ_STRONG = 25;
    const uint8_t LOWALERT_BEEBUZZ_SHORT = (25/2);

    // CODE START
    if ( (TurnOffAllVibrations == 111) || (HardCodeNoVibrations == 111) ) {
        //turn off all vibrations is set, return out here
        return;
    }
    switch (alertValue) {

    case 0:
        //No alert
        //Normal (new data, in range, trend okay)
        break;

    case 1:;
        //Low
        //APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: LOW ALERT");
        VibePattern low_alert_pat = {
            .durations = lowalert_beebuzz,
            .num_segments = LOWALERT_BEEBUZZ_STRONG,
        };
        if (TurnOffStrongVibrations == 111) { low_alert_pat.num_segments = LOWALERT_BEEBUZZ_SHORT; };
        vibes_enqueue_custom_pattern(low_alert_pat);
        break;

    case 2:;
        // Medium Alert
        //APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: MEDIUM ALERT");
        VibePattern med_alert_pat = {
            .durations = medalert_long,
            .num_segments = MEDALERT_LONG_STRONG,
        };
        if (TurnOffStrongVibrations == 111) { med_alert_pat.num_segments = MEDALERT_LONG_SHORT; };
        vibes_enqueue_custom_pattern(med_alert_pat);
        break;

    case 3:;
        // High Alert
        //APP_LOG(APP_LOG_LEVEL_INFO, "ALERT HANDLER: HIGH ALERT");
        VibePattern high_alert_pat = {
            .durations = highalert_fast,
            .num_segments = HIGHALERT_FAST_STRONG,
        };
        if (TurnOffStrongVibrations == 111) { high_alert_pat.num_segments = HIGHALERT_FAST_SHORT; };
        vibes_enqueue_custom_pattern(high_alert_pat);
        break;
      //APP_LOG(APP_LOG_LEVEL_INFO, "2 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

    } // switch alertValue
} // end alert_handler_cgm

void BT_timer_callback(void *data);
static void load_bg_delta();
static void load_cgmtime();
// BLUETOOTH
void bt_handler(bool bt_connected) {
    if (bt_connected) {
        // APP_LOG(APP_LOG_LEVEL_INFO, "Phone is connected!");
        return;
    } else {
        //  APP_LOG(APP_LOG_LEVEL_INFO, "Phone is not connected!");
        text_layer_set_text(message_layer, "√PHN\0");
        alert_handler_cgm(BTOUT_VIBE);
        if (BluetoothAlert == 111) {
            return;
        }
    }
    // Check to see if the BT_timer needs to be set; if BT_timer is not null we're still waiting
    if (BT_timer == NULL) {
        // check to see if timer has popped
        if (BT_timer_pop == 100) {
            //set timer
            BT_timer = app_timer_register((BT_ALERT_WAIT_SECS*MS_IN_A_SECOND), BT_timer_callback, NULL);
            // have set timer; next time we come through we will see that the timer has popped
            return;
        }
    }
    else {
        // BT_timer is not null and we're still waiting
        return;
    }
    // timer has popped
    // Vibrate; BluetoothAlert takes over until Bluetooth connection comes back on
    //APP_LOG(APP_LOG_LEVEL_INFO, "BT HANDLER: TIMER POP, NO BLUETOOTH, VIBRATE");
    alert_handler_cgm(BTOUT_VIBE);
    BluetoothAlert = 100;
    // Reset timer pop
    BT_timer_pop = 100;
    //APP_LOG(APP_LOG_LEVEL_INFO, "NO BLUETOOTH");
    if (TurnOff_NOBLUETOOTH_Msg == 111) {
        //text_layer_set_text(message_layer, "NO BT\0");
         // window_stack_pop_all(false); //ADDED Oct13
          set_message_layer("NO BT\0", "", false, text_colour);
    }
    //}
    else {
        // Bluetooth is on, reset BluetoothAlert
        //APP_LOG(APP_LOG_LEVEL_INFO, "HANDLE BT: BLUETOOTH ON");
       // APP_LOG(APP_LOG_LEVEL_INFO, "BluetoothAlert: %i", BluetoothAlert);
    //  window_stack_pop_all(false);
    }
}

void BT_timer_callback(void *data) {
    //   APP_LOG(APP_LOG_LEVEL_INFO, "BT TIMER CALLBACK: ENTER CODE");
    // reset timer pop and timer
    BT_timer_pop = 111;
    /*if (BT_timer != NULL) {
        BT_timer = NULL;
    }*/
    null_and_cancel_timer(&BT_timer, false);

    // check bluetooth and call handler
    bt_connected = connection_service_peek_pebble_app_connection(); //was bluetooth_connection_service_peek
    bt_handler(bt_connected);
} // end BT_timer_callback

//WATCH BATTERY ICON
static void batteryGraphicsLayerDraw( Layer *layer, GContext *ctx ) {
    // Stroke the path:
   #ifdef PBL_PLATFORM_CHALK
    graphics_context_set_stroke_color(ctx, GColorWhite);
   #else
    graphics_context_set_stroke_color(ctx, battery_colour);
   #endif
    gpath_draw_outline(ctx, batteryOutlinePath);

    // Signify the percentage:

    if (batteryLevel <=50) {
        graphics_context_set_fill_color(ctx, ROUGE);
        graphics_fill_rect( ctx, GRect( 0, 0, (batteryLevel/100.) * BATTERY_OUTLINE_WIDTH, BATTERY_OUTLINE_HEIGHT ), 0, 0 );
    }
    else{
#define TIME_WATCH_OFFSET (GRect(0, 141, 180, 30), GRect(4, 139, 144, 30))

        graphics_context_set_fill_color (ctx, (WHITE_ELSE_BATTERY));
        graphics_fill_rect( ctx, GRect( 0, 0, (batteryLevel/100.) * BATTERY_OUTLINE_WIDTH, BATTERY_OUTLINE_HEIGHT ), 0, 0 );
    }
     // APP_LOG(APP_LOG_LEVEL_INFO, "3 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

}

//WATCH BATTERY TEXT
void handle_watch_battery_cgm(BatteryChargeState watch_charge_state) {

    static char watch_battery_text[] = " ";

    if (watch_charge_state.is_charging) {
        bitmap_layer_set_background_color(battery_layer, GColorMidnightGreen);
    }
    else{
        bitmap_layer_set_background_color(battery_layer, GColorClear);

    }
    batteryLevel = watch_charge_state.charge_percent;
    text_layer_set_text(watch_battlevel_layer, watch_battery_text);
}

static void draw_date_from_app() {

    // VARIABLES
    time_t d_app = time(NULL);
    struct tm *current_d_app = localtime(&d_app);
    size_t draw_return = 0;

    // CODE START

    // format current date from app
    if (strcmp(time_watch_text, "00:00") == 0) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "TimeFormat: %d", timeformat);
        if (timeformat == 0){
            draw_return = strftime(time_watch_text, sizeof(time_watch_text), "%l:%M", current_d_app);
        } else {
            draw_return = strftime(time_watch_text, sizeof(time_watch_text), "%H:%M", current_d_app);
        }

        if (draw_return != 0) {
            text_layer_set_text(time_watch_layer, time_watch_text);
        }
    }

    draw_return = strftime(date_app_text, sizeof(date_app_text), "%a %d", current_d_app);
    if (draw_return != 0) {
        text_layer_set_text(date_app_layer, date_app_text);
    }

} // end draw_date_from_app

//NULL DICT BUFFER
static void null_dict_buffer (DictionaryIterator **iter_to_null) {

  if (*iter_to_null != NULL) {
    *iter_to_null = NULL;
  }

} // null_dict_buffer

static void null_context_buffer (void **context_to_null) {

  if (*context_to_null != NULL) {
    *context_to_null = NULL;
  }

} // null_context_buffer

static void null_tuple_buffer (const Tuple **tuple_to_null) {

  if (*tuple_to_null != NULL) {
    *tuple_to_null = NULL;
  }

} // null_tuple_buffer

void sync_error_callback_cgm(DictionaryResult appsync_dict_error, AppMessageResult appsync_error, void *context) {

    // VARIABLES
    DictionaryIterator *iter = NULL;
    AppMessageResult appsync_err_openerr = APP_MSG_OK;
    AppMessageResult appsync_err_senderr = APP_MSG_OK;

//    bool bt_connected_syncerror = false;

    // CODE START
    null_dict_buffer(&iter);

    bt_connected = connection_service_peek_pebble_app_connection();
    if (bt_connected == false) {
        // bluetooth is out, BT message already set; return out
        return;
    }

    // increment app sync retries counter
    appsyncandmsg_retries_counter++;

    // if hit max counter, skip resend and flag user
    if (appsyncandmsg_retries_counter < APPSYNCANDMSG_RETRIES_MAX) {

        // APPSYNC ERROR debug logs
        //APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC ERROR");
        APP_LOG(APP_LOG_LEVEL_DEBUG, "APPSYNC ERR, MSG: %i RES: %s DICT: %i RES: %s RETRIES: %i",
            appsync_error, translate_app_error(appsync_error), appsync_dict_error, translate_dict_error(appsync_dict_error), appsyncandmsg_retries_counter);

        // try to resend the message; open app message outbox
      null_dict_buffer(&iter);
      appsync_err_openerr = app_message_outbox_begin(&iter);
        if (appsync_err_openerr == APP_MSG_OK) {
            // could open app message outbox; send message
            appsync_err_senderr = app_message_outbox_send();
            if (appsync_err_senderr == APP_MSG_OK) {
                // everything OK, reset AppSyncErrAlert so no vibrate
                if (AppSyncErrAlert == 111) {
                    ClearedOutage = 111;
                    //APP_LOG(APP_LOG_LEVEL_DEBUG, "APPSYNC ERR, SET CLEARED OUTAGE: %i ", ClearedOutage);
                }
                AppSyncErrAlert = 100;
                // sent message OK; return
                return;
            } // if appsync_err_senderr
        } // if appsync_err_openerr
    } // if appsyncandmsg_retries_counter

    // flag resend error
  if (appsyncandmsg_retries_counter <= APPSYNCANDMSG_RETRIES_MAX) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "APP SYNC RESEND ERROR");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPSYNC RESEND ERR, OPEN: %i RES: %s SEND: %i RES: %s RETRIES: %i",
            appsync_err_openerr, translate_app_error(appsync_err_openerr), appsync_err_senderr, translate_app_error(appsync_err_senderr), appsyncandmsg_retries_counter);
    return;
  }

  // check bluetooth again
  //bluetooth_connected_syncerror = connection_service_peek_pebble_app_connection();
  bt_connected = connection_service_peek_pebble_app_connection ();
  if (bt_connected == false) {
    // bluetooth is out, BT message already set; return out
    return;
  }

    // set message to RESTART WATCH -> PHONE
    text_layer_set_text(message_layer, "RSTR\0");
    text_layer_set_text(bg_layer, "ERR\0");
//WAKEUP
    app_timer_register(5000, prv_restart, NULL);
	  alert_handler_cgm(CGMOUT_VIBE);

    // reset appsync retries counter
    appsyncandmsg_retries_counter = 0;

    // erase cgm and app ago times
    clear_cgm_timeago();

    // check if need to vibrate
    if (AppSyncErrAlert == 100) {
        // APP_LOG(APP_LOG_LEVEL_INFO, "APPSYNC ERROR: VIBRATE");
        alert_handler_cgm(APPSYNC_ERR_VIBE);
        AppSyncErrAlert = 111;
    }
  // NULL out pointers
  null_dict_buffer(&iter);
  null_context_buffer(&context);
      //APP_LOG(APP_LOG_LEVEL_INFO, "4 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

} // end sync_error_callback_cgm

void inbox_dropped_handler_cgm(AppMessageResult appmsg_indrop_error, void *context) {
    // incoming appmessage send back from Pebble app dropped; no data received
    // have never seen handler get called, think because AppSync is always used
    // just set log now to avoid crash, if see log then can go back to old handler
    DictionaryIterator *iter = NULL;
    AppMessageResult inboxdrop_apperr = APP_MSG_OK;
    DictionaryResult inboxdrop_dicterr = DICT_OK;
    // APPMSG IN DROP debug logs
    //APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG IN DROP ERROR");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG IN DROP ERR, CODE: %i RES: %s",
        appmsg_indrop_error, translate_app_error(appmsg_indrop_error));
    sync_error_callback_cgm(inboxdrop_dicterr, inboxdrop_apperr, iter);
} // end inbox_dropped_handler_cgm

void outbox_failed_handler_cgm(DictionaryIterator *failed, AppMessageResult appmsg_outfail_error, void *context) {
    // outgoin

    //appmessage send failed to deliver to Pebble
    // have never seen handler get called, think because AppSync is always used
    // just set log now to avoid crash, if see log then can go back to old handler
    DictionaryIterator *iter = NULL;
    AppMessageResult outboxfail_apperr = APP_MSG_OK;
    DictionaryResult outboxfail_dicterr = DICT_OK;
    // APPMSG OUT FAIL debug logs
    //APP_LOG(APP_LOG_LEVEL_INFO, "APPMSG OUT FAIL ERROR");

    APP_LOG(APP_LOG_LEVEL_DEBUG, "APPMSG OUT FAIL ERR, CODE: %i RES: %s",
        appmsg_outfail_error, translate_app_error(appmsg_outfail_error));
    sync_error_callback_cgm(outboxfail_dicterr, outboxfail_apperr, iter);


} // end outbox_failed_handler_cgm

static void load_icon() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON ARROW FUNCTION START");
    // CONSTANTS
    // ICON ASSIGNMENTS OF ARROW DIRECTIONS
    const char NO_ARROW[] = "0";
    const char DOUBLEUP_ARROW[] = "1";
    const char SINGLEUP_ARROW[] = "2";
    const char UP45_ARROW[] = "3";
    const char FLAT_ARROW[] = "4";
    const char DOWN45_ARROW[] = "5";
    const char SINGLEDOWN_ARROW[] = "6";
    const char DOUBLEDOWN_ARROW[] = "7";
    const char NOTCOMPUTE_ICON[] = "8";
    const char OUTOFRANGE_ICON[] = "9";

    // ARRAY OF SMALL ARROW ICON IMAGES
    const uint8_t SM_ARROW_ICONS[] = {
        RESOURCE_ID_IMAGE_FLAT_SM,            //0
        RESOURCE_ID_IMAGE_UPUP_SM,            //1
        RESOURCE_ID_IMAGE_UP_SM,              //2
        RESOURCE_ID_IMAGE_UP45_SM,            //3
        RESOURCE_ID_IMAGE_DOWN45_SM,          //4
        RESOURCE_ID_IMAGE_DOWN_SM,            //5
        RESOURCE_ID_IMAGE_DOWNDOWN_SM,         //6
        RESOURCE_ID_IMAGE_PIXEL,          //7
        RESOURCE_ID_IMAGE_LOGO             //8
    };

    // INDEX FOR ARRAY OF SMALL ARROW ICON IMAGES
    const uint8_t FLAT_SM_ICON_INDX = 0;
    const uint8_t UPUP_SM_ICON_INDX = 1;
    const uint8_t UP_SM_ICON_INDX = 2;
    const uint8_t UP45_SM_ICON_INDX = 3;
    const uint8_t DOWN45_SM_ICON_INDX = 4;
    const uint8_t DOWN_SM_ICON_INDX = 5;
    const uint8_t DOWNDOWN_SM_ICON_INDX = 6;
    const uint8_t PIXEL_ICON_INDX = 7;
    const uint8_t LOGO_ARROW_ICON_INDX = 8;

    // VARIABLES
    static uint8_t DoubleDownAlert = 100;

    // CODE START
    // set avatar
    //create_update_bitmap(&avatar_bitmap,avatar_layer,RESOURCE_ID_IMAGE_AVATAR);
      //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON code start : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

    // Got an icon value, check data offline condition, clear vibrate flag if needed
    if ( (strcmp(current_bg_delta, "OFF") < 0) || (strcmp(current_bg_delta, "OFF") > 0) ) {
        DataOfflineAlert = 100;
        dataoffline_retries_counter = 0;
    }
    //ARROW LOCATIONS
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ARROW ICON, BEFORE CHECK SPEC VALUE BITMAP");
    // check if special value set
    if (specvalue_alert == 100) {
        // no special value, set arrow
        // check for arrow direction, set proper arrow icon
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD ICON, CURRENT ICON: %s", current_icon);
        if ( (strcmp(current_icon, NO_ARROW) == 0) || (strcmp(current_icon, NOTCOMPUTE_ICON) == 0) || (strcmp(current_icon, OUTOFRANGE_ICON) == 0) ) {
            set_container_image(&icon_bitmap,icon_layer,SM_ARROW_ICONS[PIXEL_ICON_INDX],GPoint(116, 15));
            text_layer_set_background_color(tophalf_layer, top_colour);
            layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            DoubleDownAlert = 100;
        }
        else if (strcmp(current_icon, DOUBLEUP_ARROW) == 0) {

          
#define UPUP PBL_IF_ROUND_ELSE(GPoint(129, 15), GPoint(118, 15))
            set_container_image(&icon_bitmap, icon_layer, SM_ARROW_ICONS[UPUP_SM_ICON_INDX],(UPUP));
            DoubleDownAlert = 100;
            text_layer_set_background_color(tophalf_layer, MED);
            layer_mark_dirty(text_layer_get_layer(tophalf_layer));

        }

        else if (strcmp(current_icon, SINGLEUP_ARROW) == 0) {
#define UP PBL_IF_ROUND_ELSE(GPoint(134, 25), GPoint(118, 20)) 

            set_container_image(&icon_bitmap, icon_layer, SM_ARROW_ICONS[UP_SM_ICON_INDX],(UP));

            text_layer_set_background_color(tophalf_layer, top_colour);
            layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            DoubleDownAlert = 100;
        }
        else if (strcmp(current_icon, UP45_ARROW) == 0) {
#define UP45 PBL_IF_ROUND_ELSE(GPoint(132, 33), GPoint(98, 5)) 
            set_container_image(&icon_bitmap,icon_layer,SM_ARROW_ICONS[UP45_SM_ICON_INDX],(UP45));

            text_layer_set_background_color(tophalf_layer, top_colour);
            layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            DoubleDownAlert = 100;
        }
        else if (strcmp(current_icon, FLAT_ARROW) == 0) {
#define FLAT PBL_IF_ROUND_ELSE(GPoint(122, 22), GPoint(105, 23)) 
            set_container_image(&icon_bitmap,icon_layer,SM_ARROW_ICONS[FLAT_SM_ICON_INDX],(FLAT));

            text_layer_set_background_color(tophalf_layer, top_colour);
            layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            DoubleDownAlert = 100;
        }
        else if (strcmp(current_icon, DOWN45_ARROW) == 0) {
#define DOWN45 PBL_IF_ROUND_ELSE(GPoint(129, 30), GPoint(113, 38)) 
            set_container_image(&icon_bitmap, icon_layer, SM_ARROW_ICONS[DOWN45_SM_ICON_INDX],(DOWN45));

            text_layer_set_background_color(tophalf_layer, top_colour);
            layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            DoubleDownAlert = 100;
        }
        else if (strcmp(current_icon, SINGLEDOWN_ARROW) == 0) {
#define DOWN PBL_IF_ROUND_ELSE(GPoint(133, 32), GPoint(118, 25)) 
            set_container_image(&icon_bitmap,icon_layer,SM_ARROW_ICONS[DOWN_SM_ICON_INDX],(DOWN));

            text_layer_set_background_color(tophalf_layer, top_colour);
            layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            DoubleDownAlert = 100;
        }
        else if (strcmp(current_icon, DOUBLEDOWN_ARROW) == 0) {
            if (DoubleDownAlert == 100) {
                //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD ICON, ICON ARROW: DOUBLE DOWN");
                alert_handler_cgm(DOUBLEDOWN_VIBE);
                DoubleDownAlert = 111;
#define DOUBLEDOWN PBL_IF_ROUND_ELSE(GPoint(129, 23), GPoint(119, 23)) 
                set_container_image(&icon_bitmap,icon_layer,SM_ARROW_ICONS[DOWNDOWN_SM_ICON_INDX],(DOUBLEDOWN));

                text_layer_set_background_color(tophalf_layer, ROUGE);
                layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            }
        }
        else {
            // check for special cases and set icon accordingly
            // check bluetooth
            bt_connected = connection_service_peek_pebble_app_connection();

            // check to see if we are in the loading screen
            if (bt_connected == false) {
                // Bluetooth is out; in the loading screen so set logo
#define LOGO PBL_IF_ROUND_ELSE(GPoint(40, 25), GPoint(25, 25)) 
                set_container_image(&icon_bitmap,icon_layer,SM_ARROW_ICONS[LOGO_ARROW_ICON_INDX],(LOGO));

                text_layer_set_background_color(tophalf_layer, top_colour);
                layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            }
            else {
                // unexpected, set logo icon
//#ifdef PBL_PLATFORM_CHALK
                set_container_image(&icon_bitmap,icon_layer,SM_ARROW_ICONS[LOGO_ARROW_ICON_INDX],(LOGO));

                text_layer_set_background_color(tophalf_layer, top_colour);
                layer_mark_dirty(text_layer_get_layer(tophalf_layer));
            }
            DoubleDownAlert = 100;
        }
    }
}
//LOAD SYMBOL
static void load_symbol() {
  if (strchr(current_symbol, *"E")){
                create_update_bitmap(&symbol_bitmap,symbol_layer,SPECIAL_VALUE_ICONS[LIGHTNING_ICON_INDX]);
                    text_layer_set_text_color(predict_layer, text_colour);
            } else if (strchr(current_symbol, *"L")){
                create_update_bitmap(&symbol_bitmap,symbol_layer,SPECIAL_VALUE_ICONS[LOOP_ICON_INDX]);
                text_layer_set_text_color(predict_layer, text_colour);
            }else if (strchr(current_symbol, *"X")){
                if (LoopOutAlert == 100) {
                    alert_handler_cgm(NOLOOP_VIBE);
                    LoopOutAlert = 111;
                    text_layer_set_text_color(predict_layer, GColorOrange);
                }
                create_update_bitmap(&symbol_bitmap,symbol_layer,SPECIAL_VALUE_ICONS[X_ICON_INDX]);
            } else if (strchr(current_symbol, *"W")){
                if (LoopOutAlert == 100) {
                    alert_handler_cgm(NOLOOP_VIBE);
                    LoopOutAlert = 111;
                    layer_set_hidden(text_layer_get_layer(predict_layer), true);
                    layer_set_hidden(text_layer_get_layer(s_layer), false);
                    layer_set_hidden(predict_circle_layer, true);
                }
                create_update_bitmap(&symbol_bitmap,symbol_layer,SPECIAL_VALUE_ICONS[WARNING_ICON_INDX]);
            }else if (strchr(current_symbol, *"R")){
                create_update_bitmap(&symbol_bitmap,symbol_layer,SPECIAL_VALUE_ICONS[CIRCLE_ICON_INDX]);
            }else{
                create_update_bitmap(&symbol_bitmap,symbol_layer,SPECIAL_VALUE_ICONS[NONE_ICON_INDX]);
            }
}

// forward declarations for animation code
static void load_bg_delta();
static void load_cgmtime();
static void load_apptime();
static void load_rig_battlevel();

// ANIMATION CODE
void destroy_perfectbg_animation(PropertyAnimation **perfectbg_animation) {
    if (*perfectbg_animation == NULL) {
        return;
    }

    if (animation_is_scheduled((Animation*) *perfectbg_animation)) {
        animation_unschedule((Animation*) *perfectbg_animation);
    }

    if (perfectbg_animation != NULL) {
        property_animation_destroy(*perfectbg_animation);
    }
    *perfectbg_animation = NULL;
} // end destroy_perfectbg_animation*/

// PERFECTBG ANIMATION
void perfectbg_animation_started(Animation *animation, void *data) {

    //APP_LOG(APP_LOG_LEVEL_INFO, "PERFECT BG ANIMATE, ANIMATION STARTED ROUTINE");

    // clear out BG and icon
    //text_layer_set_text(bg_layer, " ");
    text_layer_set_text(message_layer, "HIGH 5!\0");

} // end perfectbg_animation_started

void perfectbg_animation_stopped(Animation *animation, bool finished, void *data) {

    //APP_LOG(APP_LOG_LEVEL_INFO, "PERFECT BG ANIMATE, ANIMATION STOPPED ROUTINE");
    // reset bg and icon
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "PERFECT BG ANIMATE, ANIMATION STOPPED, SET TO BG: %s ", last_bg);
    text_layer_set_text(bg_layer, last_bg);
    load_icon();
    load_bg_delta();
    //  destroy_perfectbg_animation(&perfectbg_animation);

} // end perfectbg_animation_stopped

void animate_perfectbg() {

    // CONSTANTS

    // ARRAY OF ICONS FOR PERFECT BG
    const uint8_t PERFECTBG_ICONS[] = {
        RESOURCE_ID_IMAGE_CLUB100,         //0
        RESOURCE_ID_IMAGE_CLUB55           //1
    };

    // INDEX FOR ARRAY OF PERFECT BG ICONS
    static const uint8_t CLUB100_ICON_INDX = 0;
    static const uint8_t CLUB55_ICON_INDX = 1;

    // VARIABLES
    Layer *animate_perfectbg_layer = NULL;

    // for animation
    GRect from_perfectbg_rect = GRect(0,0,0,0);
    GRect to_perfectbg_rect = GRect(0,0,0,0);

    // CODE START

    if (currentBG_isMMOL == 111) {
        create_update_bitmap(&perfectbg_bitmap,perfectbg_layer,PERFECTBG_ICONS[CLUB55_ICON_INDX]);
    }
    else {
        create_update_bitmap(&perfectbg_bitmap,perfectbg_layer,PERFECTBG_ICONS[CLUB100_ICON_INDX]);
    }

    animate_perfectbg_layer = bitmap_layer_get_layer(perfectbg_layer);
#define from_perfectbg_rect PBL_IF_ROUND_ELSE(GRect(144, 88, 100, 47), GRect(144, 94, 100, 52)) 
#define to_perfectbg_rect PBL_IF_ROUND_ELSE(GRect(-185, 88, 100, 47), GRect(-180, 94, 100, 52)) 

    //  destroy_perfectbg_animation(&perfectbg_animation);
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE BG, CREATE FRAME");
    perfectbg_animation = property_animation_create_layer_frame(animate_perfectbg_layer, &from_perfectbg_rect, &to_perfectbg_rect);
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE BG, SET DURATION AND CURVE");
    animation_set_duration((Animation*) perfectbg_animation, PERFECTBG_ANIMATE_SECS*MS_IN_A_SECOND);
    animation_set_curve((Animation*) perfectbg_animation, AnimationCurveLinear);

    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE BG, SET HANDLERS");
    animation_set_handlers((Animation*) perfectbg_animation, (AnimationHandlers) {
        .started = (AnimationStartedHandler) perfectbg_animation_started,
            .stopped = (AnimationStoppedHandler) perfectbg_animation_stopped,
    }, NULL /* callback data */);

    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE BG, SCHEDULE");
    animation_schedule((Animation*) perfectbg_animation);

} //end animate_perfectbg


void destroy_happymsg_animation(PropertyAnimation **happymsg_animation) {
    if (*happymsg_animation == NULL) {
        return;
    }

    if (animation_is_scheduled((Animation*) *happymsg_animation)) {
        animation_unschedule((Animation*) *happymsg_animation);
    }

    if (happymsg_animation != NULL) {
        property_animation_destroy(*happymsg_animation);
    }
    *happymsg_animation = NULL;
} // end destroy_happymsg_animation

// happymsg ANIMATION
void happymsg_animation_started(Animation *animation, void *data) {

    //APP_LOG(APP_LOG_LEVEL_INFO, "HAPPY MSG ANIMATE, ANIMATION STARTED ROUTINE, CLEAR OUT BG DELTA");

    text_layer_set_text(message_layer, current_bg_delta);

} // end happymsg_animation_started

void happymsg_animation_stopped(Animation *animation, bool finished, void *data) {

    //APP_LOG(APP_LOG_LEVEL_INFO, "HAPPY MSG ANIMATE, ANIMATION STOPPED ROUTINE");
    // set BG delta / message layer
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "HAPPY MSG ANIMATE, ANIMATION STOPPED, SET TO BG DELTA);
    //load_bg_delta();
    load_cgmtime();
    load_apptime();
    load_rig_battlevel();
    destroy_happymsg_animation(&happymsg_animation);

} // end happymsg_animation_stopped

void animate_happymsg(char *happymsg_to_display) {

    // CONSTANTS
    const uint8_t HAPPYMSG_BUFFER_SIZE = 30;

    // VARIABLES
    Layer *animate_happymsg_layer = NULL;

    // for animation
    GRect from_happymsg_rect = GRect(0,0,0,0);
    GRect to_happymsg_rect = GRect(0,0,0,0);

    static char animate_happymsg_buffer[30] = {0};

    // CODE START

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "ANIMATE HAPPY MSG, STRING PASSED: %s", happymsg_to_display);
    strncpy(animate_happymsg_buffer, happymsg_to_display, HAPPYMSG_BUFFER_SIZE);
    text_layer_set_text(happymsg_layer, animate_happymsg_buffer);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "ANIMATE HAPPY MSG, MSG IN BUFFER: %s", animate_happymsg_buffer);
    animate_happymsg_layer = text_layer_get_layer(happymsg_layer);
#define from_happymsg_rect PBL_IF_ROUND_ELSE(GRect(144, 98, 150, 38), GRect(144, 104, 150, 43)) 
#define to_happymsg_rect PBL_IF_ROUND_ELSE(GRect(-184, 98, 150, 38), GRect(-180, 104, 150, 43)) 
/*
 from_happymsg_rect = GRect(144, 98, 150, 38);
  to_happymsg_rect = GRect(-184, 98, 150, 38);
#else
    from_happymsg_rect = GRect(144, 104, 150, 43);
    to_happymsg_rect = GRect(-180, 104, 150, 43);
#endif
*/

    destroy_happymsg_animation(&happymsg_animation);
    //APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, CREATE FRAME");
    happymsg_animation = property_animation_create_layer_frame(animate_happymsg_layer, &from_happymsg_rect, &to_happymsg_rect);
    //APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, SET DURATION AND CURVE");
    animation_set_duration((Animation*) happymsg_animation, HAPPYMSG_ANIMATE_SECS*MS_IN_A_SECOND);
    animation_set_curve((Animation*) happymsg_animation, AnimationCurveLinear);

    //APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, SET HANDLERS");
    animation_set_handlers((Animation*) happymsg_animation, (AnimationHandlers) {
        .started = (AnimationStartedHandler) happymsg_animation_started,
            .stopped = (AnimationStoppedHandler) happymsg_animation_stopped,
    }, NULL /* callback data */);

    //APP_LOG(APP_LOG_LEVEL_INFO, "ANIMATE HAPPY MSG, SCHEDULE");
    animation_schedule((Animation*) happymsg_animation);

} //end animate_happymsg



void bg_vibrator (uint16_t BG_BOTTOM_INDX, uint16_t BG_TOP_INDX, uint8_t BG_SNOOZE, uint8_t *bg_overwrite, uint8_t BG_VIBE) {

    // VARIABLES

    uint16_t conv_vibrator_bg = 180;

    conv_vibrator_bg = current_bg;

    // adjust high bg for comparison, if needed
    if ( ((currentBG_isMMOL == 111) && (current_bg >= HIGH_BG_MGDL))
        || ((currentBG_isMMOL == 100) && (current_bg >= HIGH_BG_MMOL)) ) {
            conv_vibrator_bg = current_bg + 1;
    }

    // check BG and vibrate if needed
    //APP_LOG(APP_LOG_LEVEL_INFO, "BG VIBRATOR, CHECK TO SEE IF WE NEED TO VIBRATE");
    if ( ( ((conv_vibrator_bg > BG_BOTTOM_INDX) && (conv_vibrator_bg <= BG_TOP_INDX))
        && ((lastAlertTime == 0) || (lastAlertTime > BG_SNOOZE)) )
        || ( ((conv_vibrator_bg > BG_BOTTOM_INDX) && (conv_vibrator_bg <= BG_TOP_INDX)) && (*bg_overwrite == 100) ) ) {

            //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime SNOOZE VALUE IN: %i", lastAlertTime);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "bg_overwrite IN: %i", *bg_overwrite);

            // send alert and handle a bouncing connection
            if ((lastAlertTime == 0) || (*bg_overwrite == 100)) {
                //APP_LOG(APP_LOG_LEVEL_INFO, "BG VIBRATOR: VIBRATE");
                alert_handler_cgm(BG_VIBE);
                // don't know where we are coming from, so reset last alert time no matter what
                // set to 1 to prevent bouncing connection
                lastAlertTime = 1;
                if (*bg_overwrite == 100) { *bg_overwrite = 111; }
            }

            // if hit snooze, reset snooze counter; will alert next time around
            if (lastAlertTime > BG_SNOOZE) {
                lastAlertTime = 0;
                specvalue_overwrite = 100;
                hypolow_overwrite = 100;
                biglow_overwrite = 100;
                midlow_overwrite = 100;
                low_overwrite = 100;
                midhigh_overwrite = 100;
                bighigh_overwrite = 100;
                //APP_LOG(APP_LOG_LEVEL_INFO, "BG VIBRATOR, OVERWRITE RESET");
            }

            //APP_LOG(APP_LOG_LEVEL_DEBUG, "BG VIBRATOR, lastAlertTime SNOOZE VALUE OUT: %i", lastAlertTime);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "BG VIBRATOR, bg_overwrite OUT: %i", *bg_overwrite);
    }

} // end bg_vibrator

static void load_bg() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, FUNCTION START");
    // CONSTANTS
    //const uint8_t BG_BUFFER_SIZE = 6;June 20

    // ARRAY OF BG CONSTANTS; MGDL
    uint16_t BG_MGDL[] = {
        SPECVALUE_BG_MGDL, //0
        SHOWLOW_BG_MGDL, //1
        HYPOLOW_BG_MGDL, //2
        BIGLOW_BG_MGDL,  //3
        MIDLOW_BG_MGDL,  //4
        LOW_BG_MGDL, //5
        HIGH_BG_MGDL,  //6
        MIDHIGH_BG_MGDL, //7
        BIGHIGH_BG_MGDL, //8
        SHOWHIGH_BG_MGDL //9
    };
    // ARRAY OF BG CONSTANTS; MMOL
    uint16_t BG_MMOL[] = {
        SPECVALUE_BG_MMOL, //0
        SHOWLOW_BG_MMOL, //1
        HYPOLOW_BG_MMOL, //2
        BIGLOW_BG_MMOL,  //3
        MIDLOW_BG_MMOL,  //4
        LOW_BG_MMOL, //5
        HIGH_BG_MMOL,  //6
        MIDHIGH_BG_MMOL, //7
        BIGHIGH_BG_MMOL, //8
        SHOWHIGH_BG_MMOL //9
    };

    // INDEX FOR ARRAYS OF BG CONSTANTS
    const uint8_t SPECVALUE_BG_INDX = 0;
    const uint8_t SHOWLOW_BG_INDX = 1;
    const uint8_t HYPOLOW_BG_INDX = 2;
    const uint8_t BIGLOW_BG_INDX = 3;
    const uint8_t MIDLOW_BG_INDX = 4;
    const uint8_t LOW_BG_INDX = 5;
    const uint8_t HIGH_BG_INDX = 6;
    const uint8_t MIDHIGH_BG_INDX = 7;
    const uint8_t BIGHIGH_BG_INDX = 8;
    const uint8_t SHOWHIGH_BG_INDX = 9;
    // MG/DL SPECIAL VALUE CONSTANTS ACTUAL VALUES
    // mg/dL = mmol / .0555 OR mg/dL = mmol * 18.0182
    const uint8_t SENSOR_NOT_ACTIVE_VALUE_MGDL = 1; // show stop light, ?SN
    const uint8_t MINIMAL_DEVIATION_VALUE_MGDL = 2; // show stop light, ?MD
    const uint8_t NO_ANTENNA_VALUE_MGDL = 3; // show broken antenna, ?NA
    const uint8_t SENSOR_NOT_CALIBRATED_VALUE_MGDL = 5; // show blood drop, ?NC
    const uint8_t STOP_LIGHT_VALUE_MGDL = 6;  // show stop light, ?CD
    const uint8_t HOURGLASS_VALUE_MGDL = 9; // show hourglass, hourglass
    const uint8_t QUESTION_MARKS_VALUE_MGDL = 10; // show ???, ???
    const uint8_t BAD_RF_VALUE_MGDL = 12; // show broken antenna, ?RF

    // MMOL SPECIAL VALUE CONSTANTS ACTUAL VALUES
    // mmol = mg/dL / 18.0182 OR mmol = mg/dL * .0555
    const uint8_t SENSOR_NOT_ACTIVE_VALUE_MMOL = 1; // show stop light, ?SN (.06 -> .1)
    const uint8_t MINIMAL_DEVIATION_VALUE_MMOL = 1; // show stop light, ?MD (.11 -> .1)
    const uint8_t NO_ANTENNA_VALUE_MMOL = 2;  // show broken antenna, ?NA (.17 -> .2)
    const uint8_t SENSOR_NOT_CALIBRATED_VALUE_MMOL = 3; // show blood drop, ?NC (.28 -> .3)
    const uint8_t STOP_LIGHT_VALUE_MMOL = 4;  // show stop light, ?CD (.33 -> .3, set to .4 here)
    const uint8_t HOURGLASS_VALUE_MMOL = 5; // show hourglass, hourglass (.50 -> .5)
    const uint8_t QUESTION_MARKS_VALUE_MMOL = 6;  // show ???, ??? (.56 -> .6)
    const uint8_t BAD_RF_VALUE_MMOL = 7;  // show broken antenna, ?RF (.67 -> .7)

  // ARRAY OF SPECIAL VALUES CONSTANTS; MGDL
    uint8_t SPECVALUE_MGDL[] = {
        SENSOR_NOT_ACTIVE_VALUE_MGDL,  //0
        MINIMAL_DEVIATION_VALUE_MGDL,  //1
        NO_ANTENNA_VALUE_MGDL, //2
        SENSOR_NOT_CALIBRATED_VALUE_MGDL,  //3
        STOP_LIGHT_VALUE_MGDL, //4
        HOURGLASS_VALUE_MGDL,  //5
        QUESTION_MARKS_VALUE_MGDL, //6
        BAD_RF_VALUE_MGDL  //7
    };
    // ARRAY OF SPECIAL VALUES CONSTANTS; MMOL
    uint8_t SPECVALUE_MMOL[] = {
        SENSOR_NOT_ACTIVE_VALUE_MMOL,  //0
        MINIMAL_DEVIATION_VALUE_MMOL,  //1
        NO_ANTENNA_VALUE_MMOL, //2
        SENSOR_NOT_CALIBRATED_VALUE_MMOL,  //3
        STOP_LIGHT_VALUE_MMOL, //4
        HOURGLASS_VALUE_MMOL,  //5
        QUESTION_MARKS_VALUE_MMOL, //6
        BAD_RF_VALUE_MMOL  //7
    };
    // INDEX FOR ARRAYS OF SPECIAL VALUES CONSTANTS
    const uint8_t SENSOR_NOT_ACTIVE_VALUE_INDX = 0;
    const uint8_t MINIMAL_DEVIATION_VALUE_INDX = 1;
    const uint8_t NO_ANTENNA_VALUE_INDX = 2;
    const uint8_t SENSOR_NOT_CALIBRATED_VALUE_INDX = 3;
    const uint8_t STOP_LIGHT_VALUE_INDX = 4;
    const uint8_t HOURGLASS_VALUE_INDX = 5;
    const uint8_t QUESTION_MARKS_VALUE_INDX = 6;
    const uint8_t BAD_RF_VALUE_INDX = 7;

    // VARIABLES

    // pointers to be used to MGDL or MMOL values for parsing
    uint16_t *bg_ptr = NULL;
    uint8_t *specvalue_ptr = NULL;

    // happy message; max message 24 characters
    // DO NOT GO OVER 24 CHARACTERS, INCLUDING SPACES OR YOU WILL CRASH
    // YOU HAVE BEEN WARNED

    char happymsg_buffer42[26] = "THE MEANING OF LIFE?\0";
    char happymsg_buffer73[26] = "GIMME SOME SUGAH\0";
    char happymsg_buffer143[26] = "WE \U0001F499 LOOPING\0";
    //char happymsg_buffer143[26] = "WE <3 U TOO\0";
    char happymsg_buffer109[26] = "LOOP DE LOOP\0";
    char happymsg_buffer222[26] = "T00 SWEET!\0";
    char happymsg_buffer280[26] = "WATCH ME WHIP\0";
    char happymsg_buffer300[26] = "SUGAR IS SWEET \n & SO R U\0";

    // CODE START

    // if special value set, erase anything in the icon field
    if (specvalue_alert == 111) {
         create_update_bitmap(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[NONE_ICON_INDX]); //IF THERE IS SOMETHING WEIRD HAPPENING WITH ERRORS REMOVE THIS
        //  text_layer_set_text(message_layer, current_bg_delta);

    }
    // set special value alert to zero no matter what
    specvalue_alert = 100;

    // see if we're doing MGDL or MMOL; get currentBG_isMMOL value in myBGAtoi
    // convert BG value from string to int

    // FOR TESTING ONLY
    //strncpy(last_bg, "10", BG_BUFFER_SIZE);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, BGATOI IN, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);
    current_bg = myBGAtoi(last_bg);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, BG ATOI OUT, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LAST BG: %s", last_bg);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "CURRENT BG: %i", current_bg);

    if (currentBG_isMMOL == 100) {
        bg_ptr = BG_MGDL;
        specvalue_ptr = SPECVALUE_MGDL;
    }
    else {
        bg_ptr = BG_MMOL;
        specvalue_ptr = SPECVALUE_MMOL;
    }
    // BG parse, check snooze, and set text

    // check for init code or error code
    if ((current_bg <= 0) || (last_bg[0] == '-')) {
        lastAlertTime = 0;

        // check bluetooth
        bt_connected = connection_service_peek_pebble_app_connection();


        if (!bt_connected) {
            //      Bluetooth is out; set BT message
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BG INIT: NO BT, SET NO BT MESSAGE");
            //if (TurnOff_NOBLUETOOTH_Msg == 100) {
            set_message_layer("NO BT\0", "", false, text_colour);

          //text_layer_set_text(message_layer, "NO BT");
            //} // if turnoff nobluetooth msg
        }// if !bluetooth connected
        else {
            // if init code, we will set it right in message layer
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, UNEXPECTED BG: SET ERR ICON");
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, UNEXP BG, CURRENT_BG: %d LAST_BG: %s ", current_bg, last_bg);
            //if (bg_layer != NULL) { text_layer_set_text(bg_layer, "BGE\0");
          if (bg_layer != NULL) {
            //text_layer_set_text(bg_layer, last_bg);
            text_layer_set_text(bg_layer, "ERR\0");
            set_message_layer("NODAT\0", "", false, text_colour);
            //text_layer_set_text(message_layer, "NODAT\0");

            //     create_update_bitmap(&icon_bitmap,icon_layer,SPECIAL_VALUE_ICONS[CIRCLE_ICON_INDX]);
            specvalue_alert = 111;
            }

        } // if current_bg <= 0

    }    else {
        // valid BG

        // check for special value, if special value, then replace icon and blank BG; else send current BG
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BEFORE CREATE SPEC VALUE BITMAP");
        if ((current_bg == specvalue_ptr[NO_ANTENNA_VALUE_INDX]) || (current_bg == specvalue_ptr[BAD_RF_VALUE_INDX])) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET BROKEN ANTENNA");
            if (bg_layer != NULL) { text_layer_set_text(bg_layer, ""); }
            //text_layer_set_text(message_layer, " ");
            set_message_layer("\0", "", false, text_colour);
#define BROKEN_ANTENNA PBL_IF_ROUND_ELSE(GPoint(67, 33), GPoint(51, 35)) 
            set_container_image(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[BROKEN_ANTENNA_ICON_INDX], (BROKEN_ANTENNA));
            layer_mark_dirty(bitmap_layer_get_layer(icon_layer));
            specvalue_alert = 111;
        }

        else if (current_bg == specvalue_ptr[SENSOR_NOT_CALIBRATED_VALUE_INDX]) {
            // APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET BLOOD DROP");
            if (bg_layer != NULL) { text_layer_set_text(bg_layer, ""); }
            set_message_layer("\0", "", false, text_colour);
#define BLOOD_DROP PBL_IF_ROUND_ELSE(GPoint(73, 30), GPoint(58, 30)) 
            set_container_image(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[BLOOD_DROP_ICON_INDX], (BLOOD_DROP));
            layer_mark_dirty(bitmap_layer_get_layer(icon_layer));

            specvalue_alert = 111;
        }
        else if ((current_bg == specvalue_ptr[SENSOR_NOT_ACTIVE_VALUE_INDX]) || (current_bg == specvalue_ptr[MINIMAL_DEVIATION_VALUE_INDX])
                 || (current_bg == specvalue_ptr[STOP_LIGHT_VALUE_INDX])) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET STOP LIGHT");
            if (bg_layer != NULL) { text_layer_set_text(bg_layer, ""); }
            set_message_layer("\0", "", false, text_colour);
#define STOP_LIGHT PBL_IF_ROUND_ELSE(GPoint(79, 33), GPoint(63, 30)) 
            set_container_image(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[STOP_LIGHT_ICON_INDX], (STOP_LIGHT));
            layer_mark_dirty(bitmap_layer_get_layer(icon_layer));

            specvalue_alert = 111;
        }
        else if (current_bg == specvalue_ptr[HOURGLASS_VALUE_INDX]) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET HOUR GLASS");
            if (bg_layer != NULL) { text_layer_set_text(bg_layer, ""); }
            set_message_layer("\0", "", false, text_colour);

#define HOURGLASS PBL_IF_ROUND_ELSE(GPoint(76, 40), GPoint(61, 37)) 
            set_container_image(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[HOURGLASS_ICON_INDX], (HOURGLASS));
            layer_mark_dirty(bitmap_layer_get_layer(icon_layer));

            specvalue_alert = 111;
        }
        else if (current_bg == specvalue_ptr[QUESTION_MARKS_VALUE_INDX]) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, CLEAR TEXT");
            if (bg_layer != NULL) { text_layer_set_text(bg_layer, ""); }
            set_message_layer("\0", "", false, text_colour);


            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, SET BITMAP");
#define QUESTION_MARKS PBL_IF_ROUND_ELSE(GPoint(58, 35), GPoint(43, 35)) 
            set_container_image(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[QUESTION_MARKS_ICON_INDX],(QUESTION_MARKS));

            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, SPECIAL VALUE: SET QUESTION MARKS, DONE");
            specvalue_alert = 111;
            layer_mark_dirty(bitmap_layer_get_layer(icon_layer));
        }
        else if (current_bg < bg_ptr[SPECVALUE_BG_INDX]) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, UNEXPECTED SPECIAL VALUE: SET LOGO ICON");
            if (bg_layer != NULL) { text_layer_set_text(bg_layer, ""); }
            set_message_layer("\0", "", false, text_colour);


#define specialvalue_OFFSET PBL_IF_ROUND_ELSE(GPoint(40, 25), GPoint(25, 25))

            set_container_image(&specialvalue_bitmap,icon_layer,SPECIAL_VALUE_ICONS[LOGOSPECIAL_ICON_INDX],(specialvalue_OFFSET));
            layer_mark_dirty(bitmap_layer_get_layer(icon_layer));

            specvalue_alert = 111;
        } // end special value checks
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, AFTER CREATE SPEC VALUE BITMAP");

        if (specvalue_alert == 100) {
            // we didn't find a special value, so set BG instead
            // arrow icon already set separately
            if (current_bg < bg_ptr[SHOWLOW_BG_INDX]) {
                //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG: SET TO LO");
                if (bg_layer != NULL) { text_layer_set_text(bg_layer, "LO");
                text_layer_set_text_color(bg_layer, ROUGE);} //COLOUR BG LAYERS

            }
            else if (current_bg > bg_ptr[SHOWHIGH_BG_INDX]) {
                //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG: SET TO HI");
                if (bg_layer != NULL) { text_layer_set_text(bg_layer, "HI");
                text_layer_set_text_color(bg_layer, MED);} //COLOUR BG LAYERS}

            }
            //}
            else {
                // else update with current BG
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, SET TO BG: %s ", last_bg);
                if (bg_layer != NULL) { text_layer_set_text(bg_layer, last_bg); }

                if (HardCodeNoAnimations == 100) {
                    if ( ((currentBG_isMMOL == 100) && (current_bg == 100)) || ((currentBG_isMMOL == 111) && (current_bg == 55)) ) {
                        // PERFECT BG CLUB, ANIMATE BG
                        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE PERFECT BG");
                        animate_perfectbg();
                        //layer_set_hidden((Layer *)happymsg_layer, false);JUNE 20

                    } // perfect bg club, animate BG

                    // EVERY TIME YOU DO A NEW MESSAGE, YOU HAVE TO ALLOCATE A NEW HAPPY MSG BUFFER AT THE TOP OF LOAD BG FUNCTION

                    if ( ((currentBG_isMMOL == 100) && (current_bg == 109)) || ((currentBG_isMMOL == 111) && (current_bg == 109)) ) {
                        // ANIMATE HAPPY MSG LAYER
                        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
                        animate_happymsg(happymsg_buffer109);
                    } // animate happy msg layer @ 109 and 6.9

                    if ((currentBG_isMMOL == 100) && (current_bg == 222)) {
                        // ANIMATE HAPPY MSG LAYER
                        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
                        animate_happymsg(happymsg_buffer222);
                    } // animate happy msg layer @ 116

                    if ( ((currentBG_isMMOL == 100) && (current_bg == 280)) || ((currentBG_isMMOL == 111) && (current_bg == 155)) ) {
                        // ANIMATE HAPPY MSG LAYER
                        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
                        animate_happymsg(happymsg_buffer280);
                    } // animate happy msg layer @ 280
                    if ( ((currentBG_isMMOL == 100) && (current_bg == 73)) || ((currentBG_isMMOL == 111) && (current_bg == 36)) ) {
                        // ANIMATE HAPPY MSG LAYER
                        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
                        animate_happymsg(happymsg_buffer73);
                    } // animate happy msg layer @ 88
                    if ( ((currentBG_isMMOL == 100) && (current_bg == 42)) || ((currentBG_isMMOL == 111) && (current_bg == 42)) ) {
                        // ANIMATE HAPPY MSG LAYER
                        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
                        animate_happymsg(happymsg_buffer42);
                    } // animate happy msg layer @ 42

                    if (HardCodeAllAnimations == 111) {
                        // extra animations for those that want them
                        if (((currentBG_isMMOL == 100) && (current_bg == 300))|| ((currentBG_isMMOL == 111) && (current_bg == 160))) {
                            // ANIMATE HAPPY MSG LAYER
                            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
                            animate_happymsg(happymsg_buffer300);
                        } // animate happy msg layer @ 314

                        if (((currentBG_isMMOL == 100) && (current_bg == 143))|| ((currentBG_isMMOL == 111) && (current_bg == 143)))  {
                            // ANIMATE HAPPY MSG LAYER
                            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, ANIMATE HAPPY MSG LAYER");
                            animate_happymsg(happymsg_buffer143);
                        }  // animate happy msg layer @ 143


                    } // HardCodeAllAnimations

                } // HardCodeNoAnimations; end all animation code
      //APP_LOG(APP_LOG_LEVEL_INFO, "6 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            }  } // end bg checks (if special_value_bitmap)

        // see if we're going to use the current bg or the calculated raw bg for vibrations
        if ( ((current_bg > 0) && (current_bg < bg_ptr[SPECVALUE_BG_INDX])) && (HaveCalcRaw == 111) ) {

            current_calc_raw = myBGAtoi(last_calc_raw);

            //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, TurnOffVibrationsCalcRaw: %d", TurnOffVibrationsCalcRaw);

            if (TurnOffVibrationsCalcRaw == 100) {
                // set current_bg to calculated raw so we can vibrate on that instead
                current_bg = current_calc_raw;
                if (currentBG_isMMOL == 100) {
                    bg_ptr = BG_MGDL;
                    specvalue_ptr = SPECVALUE_MGDL;
                }
                else {
                    bg_ptr = BG_MMOL;
                    specvalue_ptr = SPECVALUE_MMOL;
                }
            } // TurnOffVibrationsCalcRaw

        }
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, START VIBRATE, CALC_RAW 3: %d FORMAT CALC RAW 2: %s ", current_calc_raw3, formatted_calc_raw3);

        bg_vibrator (0, bg_ptr[SPECVALUE_BG_INDX], SPECVALUE_SNZ_MIN, &specvalue_overwrite, SPECVALUE_VIBE);
        bg_vibrator (bg_ptr[SPECVALUE_BG_INDX], bg_ptr[HYPOLOW_BG_INDX], HYPOLOW_SNZ_MIN, &hypolow_overwrite, HYPOLOWBG_VIBE);
        bg_vibrator (bg_ptr[HYPOLOW_BG_INDX], bg_ptr[BIGLOW_BG_INDX], BIGLOW_SNZ_MIN, &biglow_overwrite, BIGLOWBG_VIBE);
        bg_vibrator (bg_ptr[BIGLOW_BG_INDX], bg_ptr[MIDLOW_BG_INDX], MIDLOW_SNZ_MIN, &midlow_overwrite, LOWBG_VIBE);
        bg_vibrator (bg_ptr[MIDLOW_BG_INDX], bg_ptr[LOW_BG_INDX], LOW_SNZ_MIN, &low_overwrite, LOWBG_VIBE);
        bg_vibrator (bg_ptr[HIGH_BG_INDX], bg_ptr[MIDHIGH_BG_INDX], HIGH_SNZ_MIN, &high_overwrite, HIGHBG_VIBE);
        bg_vibrator (bg_ptr[MIDHIGH_BG_INDX], bg_ptr[BIGHIGH_BG_INDX], MIDHIGH_SNZ_MIN, &midhigh_overwrite, HIGHBG_VIBE);
        bg_vibrator (bg_ptr[BIGHIGH_BG_INDX], 1000, BIGHIGH_SNZ_MIN, &bighigh_overwrite, BIGHIGHBG_VIBE);

        // else "normal" range or init code
        if ( ((current_bg > bg_ptr[LOW_BG_INDX]) && (current_bg < bg_ptr[HIGH_BG_INDX]))
            || (current_bg <= 0) ) {

                // do nothing; just reset snooze counter
                lastAlertTime = 0;
        } // else if "NORMAL RANGE" BG

    } // else if current bg <= 0
    if (current_bg >= bg_ptr[HIGH_BG_INDX]){
        text_layer_set_text_color(bg_layer, GColorOrange);
        layer_mark_dirty(text_layer_get_layer(bg_layer));
    }
    else if (current_bg <= bg_ptr[LOW_BG_INDX]){
        text_layer_set_text_color(bg_layer, GColorDarkCandyAppleRed );
        layer_mark_dirty(text_layer_get_layer(bg_layer));
    }

    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, FUNCTION OUT");
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG, FUNCTION OUT, SNOOZE VALUE: %d", lastAlertTime);
    else{
        text_layer_set_text_color(bg_layer, GColorMidnightGreen);
        layer_mark_dirty(text_layer_get_layer(bg_layer));
    }
       // APP_LOG(APP_LOG_LEVEL_INFO, "7 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

} // end load_bg

static void set_cgm_timeago (char *timeago_string, int timeago_diff, bool use_timeago_string, char *timeago_label) {

  // CONSTANTS
  const uint8_t TIMEAGO_BUFFER_SIZE = 10;

  // VARIABLES
  static char formatted_cgm_timeago[10] = {0};

  if (use_timeago_string) { text_layer_set_text(cgmtime_layer, (char *)timeago_string); }
  else {
    snprintf(formatted_cgm_timeago, TIMEAGO_BUFFER_SIZE, "%i", timeago_diff);
    strncpy(cgm_label_buffer, timeago_label, sizeof(cgm_label_buffer));
    strcat(formatted_cgm_timeago, cgm_label_buffer);
    text_layer_set_text(cgmtime_layer, formatted_cgm_timeago);
  }

} // end set_cgm_timeago

static void load_cgmtime() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD CGMTIME FUNCTION START");
    // VARIABLES

    static uint32_t cgm_time_offset = 0;
    int cgm_timeago_diff = 0;
    time_t current_temp_time = time(NULL);
    struct tm *current_local_time = localtime(&current_temp_time);
    size_t draw_cgm_time = 0;
    static char cgm_time_text[] = "00:00";

    // CODE START
    // initialize label buffer
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, NEW CGM TIME: %lu", current_cgm_time);

    if (current_cgm_time == 0) {
        // Init code or error code; set text layer & icon to empty value
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIME AGO INIT OR ERROR CODE: %s", cgm_label_buffer);
       clear_cgm_timeago();
    }
  else {

      cgm_time_now = time(NULL);

      if ((init_loading_cgm_timeago == 111) && (PhoneOffAlert == 100)) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD CGMTIME, INIT CGM TIMEAGO SHOW LAST TIME");
        current_temp_time = current_cgm_time;
        current_local_time = localtime(&current_temp_time);

        draw_cgm_time = strftime(cgm_time_text, TIME_TEXTBUFF_SIZE, "%l:%M", current_local_time);

        if (draw_cgm_time != 0) {
          text_layer_set_text(cgmtime_layer, cgm_time_text);
        }


      }

      // display cgm_timeago as now to 5m always, no matter what the difference is by using an offset
      if (stored_cgm_time == current_cgm_time) {
          // stored time is same as incoming time, so display timeago
          current_cgm_timeago = (abs((abs(cgm_time_now - current_cgm_time)) - cgm_time_offset));
      }
      else {
        // got new cgm time, set loading flags and get offset
        if ((stored_cgm_time != 0) && (BluetoothAlert == 100) && (PhoneOffAlert == 100) &&
          (AppSyncErrAlert == 100)) {
          init_loading_cgm_timeago = 100;
        }

        // get offset
        stored_cgm_time = current_cgm_time;
        current_cgm_timeago = 0;
        cgm_time_offset = abs(cgm_time_now - current_cgm_time);

      }

        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIME: %lu", current_cgm_time);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, STORED CGM TIME: %lu", stored_cgm_time);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, TIME NOW IN CGM: %lu", cgm_time_now);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIME OFFSET: %lu", cgm_time_offset);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIMEAGO: %lu", current_cgm_timeago);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, INIT LOADING BOOL: %d", init_loading_cgm_timeago);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, GM TIME AGO LABEL IN: %s", cgm_label_buffer);


        // if not in initial cgm timeago, set rcvr on icon and time label
      if ((init_loading_cgm_timeago == 100) && (BluetoothAlert == 100) && (PhoneOffAlert == 100)) {
        //create_update_bitmap(&cgmicon_bitmap,cgmicon_layer,TIMEAGO_ICONS[RCVRON_ICON_INDX]);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIME: %lu", current_cgm_time);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, STORED CGM TIME: %lu", stored_cgm_time);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, TIME NOW IN CGM: %lu", cgm_time_now);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIME OFFSET: %lu", cgm_time_offset);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CURRENT CGM TIMEAGO: %lu", current_cgm_timeago);

        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, GM TIME AGO LABEL IN: %s", cgm_label_buffer);

        if (current_cgm_timeago < MINUTEAGO) {
          cgm_timeago_diff = 0;
          set_cgm_timeago("now", cgm_timeago_diff, true, "");
          // We've cleared Check Rig, so make sure reset flag is set.
          CGMOffAlert = 100;
        }
        else if (current_cgm_timeago < HOURAGO) {
          cgm_timeago_diff = (current_cgm_timeago / MINUTEAGO);
          set_cgm_timeago("", cgm_timeago_diff, false, "m");

        }
        else if (current_cgm_timeago < DAYAGO) {
          cgm_timeago_diff = (current_cgm_timeago / HOURAGO);
          set_cgm_timeago("", cgm_timeago_diff, false, "h");

        }
        else if (current_cgm_timeago < WEEKAGO) {
          cgm_timeago_diff = (current_cgm_timeago / DAYAGO);
          set_cgm_timeago("", cgm_timeago_diff, false, "d");

        }
        else {
          // clear cgm timeago icon and set init flag
          clear_cgm_timeago();
        }
      }

      // check to see if we need to show receiver off icon
      if ( ((cgm_timeago_diff >= CGMOUT_WAIT_MIN) || ((strcmp(cgm_label_buffer, "") != 0) && (strcmp(cgm_label_buffer, "m") != 0)))
      || ( ( ((current_cgm_timeago < TWOYEARSAGO) && ((current_cgm_timeago / MINUTEAGO) >= CGMOUT_INIT_WAIT_MIN))
          || ((strcmp(cgm_label_buffer, "") != 0) && (strcmp(cgm_label_buffer, "m") != 0)) )
           && (init_loading_cgm_timeago == 111) && (ClearedOutage == 100) && (ClearedBTOutage == 100) ) ) {

          // set receiver off icon
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, SET RCVR OFF ICON, CGM TIMEAGO DIFF: %d", cgm_timeago_diff);
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, SET RCVR OFF ICON, LABEL: %s", cgm_label_buffer);

          // Vibrate if we need to
          if ((BluetoothAlert == 100) && (PhoneOffAlert == 100) && (CGMOffAlert == 100) &&
            (ClearedOutage == 100) && (ClearedBTOutage == 100)) {


            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD CGMTIME, CGM TIMEAGO: VIBRATE");
            alert_handler_cgm(CGMOUT_VIBE);
            CGMOffAlert = 111;
            text_layer_set_text(bg_layer, "OLD\0"); //added August 4
            text_layer_set_text(message_layer, "√RIG\0");

          } // if CGMOffAlert
      } // if CGM_OUT_MIN
        else {
        if ((CGMOffAlert == 111) && (cgm_timeago_diff != 0)) {
          ClearedOutage = 111;
          //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, SET CLEARED OUTAGE: %i ", ClearedOutage);
        }
        // CGM is not out, reset CGMOffAlert
        CGMOffAlert = 100;
      } // else CGM_OUT_MIN

    } // else init code

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD CGMTIME, CGM TIMEAGO LABEL OUT: %s", cgm_label_buffer);
} // end load_cgmtime

static void load_apptime(){
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, READ APP TIME FUNCTION START");
    // VARIABLES
    uint32_t current_app_timeago = 0;
    int app_timeago_diff = 0;

    // CODE START
    draw_date_from_app();

    app_time_now = time(NULL);

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, TIME NOW: %lu", app_time_now);

    current_app_timeago = abs(app_time_now - current_app_time);

//    APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, CURRENT APP TIMEAGO: %lu", current_app_timeago);

    app_timeago_diff = (current_app_timeago / MINUTEAGO);
    if ((current_app_timeago < TWOYEARSAGO) && (app_timeago_diff >= PHONEOUT_WAIT_MIN)) {

        // erase cgm ago times and cgm icon
        text_layer_set_text(cgmtime_layer, "");
        init_loading_cgm_timeago = 111;
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, SET init_loading_cgm_timeago: %i", init_loading_cgm_timeago);
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, CHECK IF HAVE TO VIBRATE");
        // Vibrate if we need to
        if ((BluetoothAlert == 100) && (PhoneOffAlert == 100) &&
            (ClearedOutage == 100) //&& (ClearedBTOutage == 100)
            ) {
                //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, READ APP TIMEAGO: VIBRATE");
                alert_handler_cgm(PHONEOUT_VIBE);
                PhoneOffAlert = 111;
                text_layer_set_text(message_layer, "√PHN\0");
        }
    }
    else {
        // reset PhoneOffAlert
        if (PhoneOffAlert == 111) {
            ClearedOutage = 111;
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD APPTIME, SET CLEARED OUTAGE: %i ", ClearedOutage);
        }
        PhoneOffAlert = 100;
    }
    //} // else init code
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD APPTIME, FUNCTION OUT");
} // end load_apptime

static void load_bg_delta() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "BG DELTA FUNCTION START");
    // CONSTANTS
    //const uint8_t MSGLAYER_BUFFER_SIZE = 14;
    //const uint8_t BGDELTA_LABEL_SIZE = 14;
    //const uint8_t BGDELTA_FORMATTED_SIZE = 14;

    // VARIABLES
    static char formatted_bg_delta[14] = {0};

//    char delta_label_buffer[14] = {0}; //added back for delta

    // CODE START
    // check bluetooth connection
    text_layer_set_text_color(message_layer, text_colour);
    bt_connected = connection_service_peek_pebble_app_connection();

    if ((bt_connected == false) || (BluetoothAlert ==  111)) {
        //  Bluetooth is out; BT message already set, so return
        return;
    }

    // check for CHECK PHONE condition, if true set message
    if ((PhoneOffAlert == 111) && (ClearedOutage == 100) && (ClearedBTOutage == 100)

        && (TurnOff_CHECKPHONE_Msg == 100)) {
            text_layer_set_text(message_layer, "√PHN\0");
           return;
    }

    // check for CHECK CGM condition, if true set message

    if ((CGMOffAlert == 111) && (ClearedOutage == 100) && (ClearedBTOutage == 100)
        && (current_cgm_timeago != 0) && (stored_cgm_time == current_cgm_time) &&
        (TurnOff_CHECKCGM_Msg == 100)) {
            text_layer_set_text(bg_layer, "OLD\0"); //added August 4
            text_layer_set_text(message_layer, "√RIG\0");
            return;
    }

    // Clear out any remaining CHECK RIG condition
    if ((CGMOffAlert == 111) && (current_cgm_timeago == 0) &&
        (TurnOff_CHECKCGM_Msg == 100)) {
            init_loading_cgm_timeago = 100;
            return;
    }
    //START AGAIN HERE WITH MESSAGE LAYER AND SPECIAL VALUE
    // check for special messages; if no string, set no message
    if (strcmp(current_bg_delta, "") == 0) {
        strncpy(formatted_bg_delta, "", sizeof(formatted_bg_delta));
        text_layer_set_text(message_layer, formatted_bg_delta);
        return;
    }
    // check for NO ENDPOINT condition, if true set message
    // put " " (space) in bg field so logo continues to show
    if (strcmp(current_bg_delta, "NOEP") == 0) {
        strncpy(formatted_bg_delta, "NOEP", sizeof(formatted_bg_delta));
        text_layer_set_text(message_layer, formatted_bg_delta);
        // text_layer_set_text(bg_layer, "NOEP");
        specvalue_alert = 100;
        return;
    }

    // check for COMPRESSION (compression low) condition, if true set message
    if (strcmp(current_bg_delta, "PRSS") == 0) {
        strncpy(formatted_bg_delta, "CMP?", sizeof(formatted_bg_delta));
        text_layer_set_text(message_layer, formatted_bg_delta);
        return;
    }
      if (strcmp(current_bg_delta, "LOG ERR") == 0) {
        strncpy(formatted_bg_delta, "LOG ERR", sizeof(formatted_bg_delta));
        text_layer_set_text(message_layer, formatted_bg_delta);
        specvalue_alert = 100;
        return;
    }
  if (strcmp(current_bg_delta, "SVR ERR") == 0) {
        strncpy(formatted_bg_delta, "SVR ERR", sizeof(formatted_bg_delta));
        text_layer_set_text(message_layer, formatted_bg_delta);
        specvalue_alert = 100;
        return;
    }
        if (strcmp(current_bg_delta, "T-OUT") == 0) {
        strncpy(formatted_bg_delta, "T-OUT", sizeof(formatted_bg_delta));
        text_layer_set_text(message_layer, formatted_bg_delta);
        specvalue_alert = 100;
        return;
    }

    // check for DATA OFFLINE condition, if true set message to fix condition
    if (strcmp(current_bg_delta, "OFF") == 0) {
        if (dataoffline_retries_counter >= DATAOFFLINE_RETRIES_MAX) {
            strncpy(formatted_bg_delta, "NODT", sizeof(formatted_bg_delta));
            text_layer_set_text(message_layer, formatted_bg_delta);
            text_layer_set_text(bg_layer, " ");
            if (DataOfflineAlert == 100) {
                //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG DELTA, DATA OFFLINE, VIBRATE");
                alert_handler_cgm(DATAOFFLINE_VIBE);
                DataOfflineAlert = 111;
            } // DataOfflineAlert
            // NOTE: DataOfflineAlert is cleared in load_icon because that means we got a good message again
            // NOTE: dataoffline_retries_counter is cleared in load_icon because that means we got a good message again
        }
        else {
            dataoffline_retries_counter++;
        }
        return;
    } // strcmp "OFF"

    // check if LOADING.., if true set message
    // put " " (space) in bg field so logo continues to show
    if (strcmp(current_bg_delta, "LOOP") == 0) {
        strncpy(formatted_bg_delta, "LOOP", sizeof(formatted_bg_delta));
        text_layer_set_text(message_layer, formatted_bg_delta);
        text_layer_set_text(bg_layer, " ");
        specvalue_alert = 100;
        return;
    }

    // check for zero delta here; if get later then we know we have an error instead
    if (strcmp(current_bg_delta, "0") == 0) {
        strncpy(formatted_bg_delta, "0", sizeof(formatted_bg_delta));
        //text_layer_set_text(message_layer, formatted_bg_delta);
        return;
    }

    if (strcmp(current_bg_delta, "0.0") == 0) {
        strncpy(formatted_bg_delta, "0.0", sizeof(formatted_bg_delta));
        //text_layer_set_text(message_layer, formatted_bg_delta);
        return;
    }

    // check to see if we have MG/DL or MMOL
    // get currentBG_isMMOL in myBGAtoi
    converted_bgDelta = myBGAtoi(current_bg_delta);

    // Bluetooth is good, Phone is good, CGM connection is good, no special message
    // set delta BG message
    // zero here, means we have an error instead; set error message
  //REMOVED AUG1

  /*if (converted_bgDelta == 0) {
        //strncpy(formatted_bg_delta, "ERR", BGDELTA_FORMATTED_SIZE);//WAS ERR
        //text_layer_set_text(message_layer, formatted_bg_delta);
              text_layer_set_text(message_layer, "ZER\0");
        return;
    }*/

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BG DELTA, DELTA STRING: %s", &current_bg_delta[i]);
    if (currentBG_isMMOL == 100) {
        // set mg/dL string
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG DELTA: FOUND MG/DL, SET STRING");
        if (converted_bgDelta >= 100) {
            // bg delta too big, set zero instead
            strncpy(formatted_bg_delta, "0", sizeof(formatted_bg_delta));
        }
        else {
            strncpy(formatted_bg_delta, current_bg_delta, sizeof(formatted_bg_delta));
        }
        //strncpy(delta_label_buffer, " mg/dL", BGDELTA_LABEL_SIZE);
        //strcat(formatted_bg_delta, delta_label_buffer);
    }
    else {
        // set mmol string
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG DELTA: FOUND MMOL, SET STRING");
        if (converted_bgDelta >= 55) {
            // bg delta too big, set zero instead
            strncpy(formatted_bg_delta, "0.0", sizeof(formatted_bg_delta));
        }
        else {
            strncpy(formatted_bg_delta, current_bg_delta, sizeof(formatted_bg_delta));
        }
        //strncpy(delta_label_buffer, " mmol", BGDELTA_LABEL_SIZE);
        //strcat(formatted_bg_delta, delta_label_buffer);
    }
    if (message_layer != NULL) { text_layer_set_text(message_layer, formatted_bg_delta); }
} // end load_bg_delta

//RIG BATTERY
static void load_rig_battlevel() {
    //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, FUNCTION START");

    // VARIABLES
    static char formatted_battlevel[12] = {0};
    static uint8_t LowBatteryAlert = 100;

    //uint8_t current_battlevel = 0;
    static char battlevel_percent[12] = {0};

    //uint8_t current_battlevel = 0;
    //June 20char battlevel_percent[6] = {0};

    // CODE START
    // initialize inverter layer to hide

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, LAST BATTLEVEL: %s", last_battlevel);

    if (strcmp(last_battlevel, " ") == 0) {
        // Init code or no battery, can't do battery; set text layer & icon to empty value
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, NO BATTERY");
        text_layer_set_text(rig_battlevel_layer, "");
        LowBatteryAlert = 100;
        return;
    }

    if (strcmp(last_battlevel, "0") == 0) {
        // Zero battery level; set here, so if we get zero later we know we have an error instead
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, SET STRING");
        text_layer_set_text(rig_battlevel_layer, "0%\0");

        if (LowBatteryAlert == 100) {
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, ZERO BATTERY, VIBRATE");
            alert_handler_cgm(LOWBATTERY_VIBE);
            LowBatteryAlert = 111;
        }
        return;
    }

    current_battlevel = atoi(last_battlevel);

    //APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD BATTLEVEL, CURRENT BATTLEVEL: %i", current_battlevel);
    if ((current_battlevel <= 0) || (current_battlevel > 100) || (last_battlevel[0] == '-')) {
        // got a negative or out of bounds or error battery level
        //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BATTLEVEL, UNKNOWN, ERROR BATTERY");
        text_layer_set_text(rig_battlevel_layer, "ERR\0");

        return;
    }
   // initialize formatted battlevel
    strncpy(formatted_battlevel, " ", sizeof(formatted_battlevel));

    // get current battery level, set battery level text with percent and set gauge line
    if (current_battlevel < 10) { strncpy(formatted_battlevel, " %", sizeof(formatted_battlevel)); }
    else { strncpy(formatted_battlevel, " ", sizeof(formatted_battlevel)); }
    snprintf(battlevel_percent, sizeof(battlevel_percent), "%i%%", current_battlevel);
    strcat(formatted_battlevel, battlevel_percent);
    text_layer_set_text(rig_battlevel_layer, formatted_battlevel);
      //APP_LOG(APP_LOG_LEVEL_INFO, "8 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

}


//NAME CIRCLE update proc
static void name_circle_update_proc(Layer *this_layer, GContext *ctx) {
#ifdef PBL_PLATFORM_CHALK
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(7,25, 44, 44), GOvalScaleModeFillCircle, 100 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    graphics_context_set_fill_color(ctx, text_colour);
    graphics_fill_radial(ctx, GRect(7, 25, 45, 45), GOvalScaleModeFillCircle, 3 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(-2, -2, 44, 44), GOvalScaleModeFillCircle, 100 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    graphics_context_set_fill_color(ctx, text_colour);
    graphics_fill_radial(ctx, GRect(-2, -2, 45, 45), GOvalScaleModeFillCircle, 3 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
#endif
}//END NAME CIRCLE UPDATE PROC

//COB CIRCLE update proc
static void cob_circle_update_proc(Layer *this_layer, GContext *ctx) {
#ifdef PBL_PLATFORM_CHALK
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(15, 127, 42, 30), GOvalScaleModeFillCircle, 105 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    graphics_context_set_fill_color(ctx, text_colour);
    graphics_fill_radial(ctx, GRect(15, 127, 43, 31), GOvalScaleModeFillCircle, 3 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(-3, 40, 42, 38), GOvalScaleModeFillCircle, 100 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    graphics_context_set_fill_color(ctx, text_colour);
    graphics_fill_radial(ctx, GRect(-3, 40, 43, 39), GOvalScaleModeFillCircle, 3 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
#endif
}//END COB CIRCLE UPDATE PROC

static void predict_circle_update_proc(Layer *this_layer, GContext *ctx) {
#ifdef PBL_PLATFORM_CHALK
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(123, 123, 38, 34), GOvalScaleModeFillCircle, 105 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    graphics_context_set_fill_color(ctx, text_colour);
    graphics_fill_radial(ctx, GRect(120,123, 42, 38), GOvalScaleModeFillCircle, 3 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
 #else
  
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(110, 115, 38, 34), GOvalScaleModeFillCircle, 105 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    graphics_context_set_fill_color(ctx, text_colour);
    graphics_fill_radial(ctx, GRect(107, 113, 42, 38), GOvalScaleModeFillCircle, 3 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

   #endif
}//END CIRCLE UPDATE PROC

//CIRCLE update proc
static void circle_update_proc(Layer *this_layer, GContext *ctx) {
#ifdef PBL_PLATFORM_CHALK
      graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(48, 7, 85, 69), GOvalScaleModeFillCircle, 105 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    graphics_context_set_fill_color(ctx, text_colour);
    graphics_fill_radial(ctx, GRect(45, 5, 89, 73), GOvalScaleModeFillCircle, 3 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

  #else
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(34, 2, 82, 81), GOvalScaleModeFillCircle, 105 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    graphics_context_set_fill_color(ctx, text_colour);
    graphics_fill_radial(ctx, GRect(31, 0, 86, 84), GOvalScaleModeFillCircle, 3 /*thickness*/, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
 #endif
}//END CIRCLE UPDATE PROC

//TUPLE
void sync_tuple_changed_callback_cgm(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
//    APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE");
    // VARIABLES
    uint8_t need_to_reset_outage_flag = 100;
    uint8_t get_new_cgm_time = 100;

    // CODE START
    // reset appsync retries counter
    appsyncandmsg_retries_counter = 0;

  // Process all pairs present
  //  while(new_tuple != NULL) {
        //parse key and tuple
        switch (key) {

        case CGM_ICON_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: ICON ARROW");
            strncpy(current_icon, new_tuple->value->cstring, sizeof(current_icon));
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, ICON VALUE: %s ", current_icon);
            load_icon();
            //APP_LOG(APP_LOG_LEVEL_INFO, "icon key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            break; // break for CGM_ICON_KEY

        case CGM_BG_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG CURRENT");
            strncpy(last_bg, new_tuple->value->cstring, sizeof(last_bg));
            // APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, BG VALUE: %s ", last_bg);
            load_bg();
            //text_layer_set_text_color(bg_layer, bg_colour);

            //APP_LOG(APP_LOG_LEVEL_INFO, "bg key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            break; // break for CGM_BG_KEY

        case CGM_TCGM_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ CGM TIME");
            current_cgm_time = new_tuple->value->uint32;
            cgm_time_now = time(NULL);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, CLEARED OUTAGE IN: %i ", ClearedOutage);

            // set up proper CGM time before calling load CGM time
            if ( ((ClearedOutage == 111) //|| (ClearedBTOutage == 111)
                ) && (stored_cgm_time != 0)) {
                    stored_cgm_time = current_cgm_time;
                    current_cgm_timeago = 0;
                    init_loading_cgm_timeago = 111;
                    need_to_reset_outage_flag = 111;
            }
            // get stored cgm time again for bluetooth race condition
            if (get_new_cgm_time == 111) {
                stored_cgm_time = current_cgm_time;
                current_cgm_timeago = 0;
                // reset flag
                get_new_cgm_time = 100;
            }

            // clear CHECK RIG message if still there
            if ((CGMOffAlert == 111) && (need_to_reset_outage_flag = 111) && (stored_cgm_time != current_cgm_time)) {
                load_bg_delta();
            }
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, CURRENT CGM TIME: %lu ", current_cgm_time);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, STORED CGM TIME: %lu ", stored_cgm_time);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, TIME NOW: %lu ", cgm_time_now);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, CLEARED OUTAGE OUT: %i ", ClearedOutage);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, CURRENT CGM TIMEAGO: %lu ", current_cgm_timeago);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, CURRENT CGM TIMEAGO DIFF: %i ", cgm_timeago_diff);

            load_cgmtime();

            // if just cleared an outage, reset flags
            if (need_to_reset_outage_flag == 111) {
                // reset stored cgm_time for bluetooth race condition
                if (ClearedBTOutage == 111) {
                    // just cleared a BT outage, so make sure we are still in init_loading
                    init_loading_cgm_timeago = 111;
                    // set get new CGM time flag
                    get_new_cgm_time = 111;
                    //   }
                    // reset the ClearedOutages flag
                    ClearedOutage = 100;
                    ClearedBTOutage = 100;
                    // reset outage flag
                    need_to_reset_outage_flag = 100;
                }
              //APP_LOG(APP_LOG_LEVEL_INFO, "tcgm key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());
            }

            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ CGM TIME OUT");
            break; // break for CGM_TCGM_KEY

        case CGM_TAPP_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ APP TIME NOW");
            current_app_time = new_tuple->value->uint32;
           // APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, APP TIME VALUE: %lu ", current_app_time);
            load_apptime();
             //APP_LOG(APP_LOG_LEVEL_INFO, "tapp key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());
            break; // break for CGM_TAPP_KEY

        case CGM_DLTA_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG DELTA");
            strncpy(current_bg_delta, new_tuple->value->cstring, sizeof(current_bg_delta));
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, BG DELTA VALUE: %s ", current_bg_delta);
            load_bg_delta();
             //APP_LOG(APP_LOG_LEVEL_INFO, "dlta key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            break; // break for CGM_DLTA_KEY

        case CGM_UBAT_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: UPLOADER BATTERY LEVEL");
            strncpy(last_battlevel, new_tuple->value->cstring, sizeof(last_battlevel));
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, BATTERY LEVEL VALUE: %s ", last_battlevel);
            load_rig_battlevel();
            //APP_LOG(APP_LOG_LEVEL_INFO, "ubat key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            break; // break for CGM_UBAT_KEY

          case CGM_NAME_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: T1D NAME");
            strncpy(current_name, new_tuple->value->cstring, sizeof(current_name));
            text_layer_set_text(t1dname_layer, current_name);

            if ( (strcmp(current_name, " ") == 0) || (strcmp(current_name, "") == 0) ) {
                  layer_set_hidden(name_circle_layer, true);
                  layer_set_hidden(text_layer_get_layer(t1dname_layer), true);
            }else {
                 layer_set_hidden(name_circle_layer, false);
                 layer_set_hidden(text_layer_get_layer(t1dname_layer), false);
            }
            //APP_LOG(APP_LOG_LEVEL_INFO, "name key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());
            break; // break for CGM_NAME_KEY

        case CGM_VALS_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: VALUES");
            strncpy(current_values, new_tuple->value->cstring, sizeof(current_values));
            load_values();
            load_colour();

            break; // break for CGM_VALS_KEY

        case CGM_CLRW_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: CALCULATED RAW");
            text_layer_set_text_color(raw_calc_layer, text_colour);

            strncpy(last_calc_raw, new_tuple->value->cstring, sizeof(last_calc_raw));
            if ( (strcmp(last_calc_raw, "0") == 0) || (strcmp(last_calc_raw, "0.0") == 0) ) {
                strncpy(last_calc_raw, " ", sizeof(last_calc_raw));
                HaveCalcRaw = 100;
            }
            else {
                HaveCalcRaw = 111;
            }
            //text_layer_set_text(s_layer, last_calc_raw);

            //APP_LOG(APP_LOG_LEVEL_INFO, "clrw key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());
            break; // break for CGM_CLRW_KEY

        case CGM_BGSX_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BGS X AXIS");
             //APP_LOG(APP_LOG_LEVEL_INFO, "CGM_BGSX_KEY: %s", new_tuple->value->cstring);
          //APP_LOG(APP_LOG_LEVEL_INFO, "bgsx START key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

          if(bgsx_array)
            {
                free(bgsx_array);
                bgsx_array = NULL;
            }
           //APP_LOG(APP_LOG_LEVEL_INFO, "bgsx AFTER FREE BGSX ARRAY key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            char* nonconst = (char*)malloc(sizeof(char) * new_tuple->length);
            //APP_LOG(APP_LOG_LEVEL_INFO, "bgsx MALLOC key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            strcpy(nonconst, new_tuple->value->cstring);
            //APP_LOG(APP_LOG_LEVEL_INFO, "bgsx STRCPY key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            ProcessingState* state = data_processor_create(nonconst, ',');

            uint8_t num_strings = data_processor_count(state);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "BG num: %i", num_strings);
             //APP_LOG(APP_LOG_LEVEL_INFO, "bgsxNUM STRINGS key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

          bgsx_array = (int*)malloc((num_strings-1)*sizeof(int));
            for (uint8_t n = 0; n < num_strings; n += 1) {
                if (n == 0) {
                    tag_raw = data_processor_get_int(state);
                    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Tag Raw : %i", tag_raw);
             //APP_LOG(APP_LOG_LEVEL_INFO, "bgsx AFTER ARRAY key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

                }
                else {
                    bgsx_array[n-1] = data_processor_get_int(state);
                    //APP_LOG(APP_LOG_LEVEL_DEBUG, "BG Split: %i", bgs[n-1]);
                //APP_LOG(APP_LOG_LEVEL_INFO, "bgsx7 key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

                }
            }
            // number of items in bgs array
            bgsx_array_counter = num_strings - 1;
            if(bgsx_array_counter > 0)
  			      bgsx_array_set = true;
			        free(nonconst);

            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, CGM_BGSX_KEY counter: %d ", bgsx_array_counter);
            //APP_LOG(APP_LOG_LEVEL_INFO, "bgsx key finished : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());
            if(bgty_array)
            {
                free(bgty_array);
                bgty_array = NULL;
            }

          //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG_TIMES Y AXIS");
            char* nonconst1 = (char*)malloc(sizeof(char) * new_tuple->length);
            //strcpy(nonconst1, new_tuple->value->cstring);
             //nonconst1 =  "25, 28, 35, 38, 43, 48, 53, 58, 63, 68, 73, 78, 83, 88, 93, 98, 103, 108, 113, 118";
             nonconst1 =  "23, 28, 33, 38, 43, 48, 53, 58, 63, 68, 73, 78";

            ProcessingState* state_t = data_processor_create(nonconst1, ',');
            uint8_t num_strings_t = data_processor_count(state_t);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "BG_t num: %i", num_strings_t);


            bgty_array = (int*)malloc(num_strings_t*sizeof(int));
            for (uint8_t n = 0; n < num_strings_t; n += 1) {
                bgty_array[n] = data_processor_get_int(state_t);
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "BG_t Split: %i", bgty_array[n]);
            }

            if(num_strings_t > 0)
    			      bgty_array_set = true;

          break;
/*
        case CGM_BGTY_KEY:;
               if(bgty_array)
            {
                free(bgty_array);
                bgty_array = NULL;
            }

          //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BG_TIMES Y AXIS");
            char* nonconst1 = (char*)malloc(sizeof(char) * new_tuple->length);
            //strcpy(nonconst1, new_tuple->value->cstring);
             nonconst1 =  "25, 28, 35, 38, 43, 48, 53, 58, 63, 68, 73, 78, 83, 88, 93, 98, 103, 108, 113, 118";

            ProcessingState* state_t = data_processor_create(nonconst1, ',');
            uint8_t num_strings_t = data_processor_count(state_t);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "BG_t num: %i", num_strings_t);


            bgty_array = (int*)malloc(num_strings_t*sizeof(int));
            for (uint8_t n = 0; n < num_strings_t; n += 1) {
                bgty_array[n] = data_processor_get_int(state_t);
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "BG_t Split: %i", bgty_array[n]);
            }

            if(num_strings_t > 0)
    			      bgty_array_set = true;

//            free(nonconst1);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, CGM_BGTY_KEY: %s ", new_tuple->value->cstring);
            //APP_LOG(APP_LOG_LEVEL_INFO, "bgty key finished : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());
          break;
*/
        case CGM_COB_KEY:;
           // APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: COB: ");
            strncpy(current_cob, new_tuple->value->cstring, sizeof(current_cob));
            text_layer_set_text(cob_layer, current_cob);
            text_layer_set_text_color(cob_layer, text_colour);

            if ( (strcmp(current_cob, " ") == 0) || (strcmp(current_cob, "0") == 0) ) {
               layer_set_hidden(cob_circle_layer, true);
               layer_set_hidden(text_layer_get_layer(cob_layer), true);
            }else {
                  layer_set_hidden(cob_circle_layer, false);
                  layer_set_hidden(text_layer_get_layer(cob_layer), false);
            }
          //APP_LOG(APP_LOG_LEVEL_INFO, "cob key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

          break; // break for CGM_COB_KEY

          case CGM_SYM_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: Loop Symbol");
            strncpy(current_symbol, new_tuple->value->cstring, sizeof(current_symbol));
            load_symbol();

            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, Symbol: %s", current_symbol);
            break; // break for CGM_SYM_KEY
          case CGM_TIME_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: READ APP TIME NOW");

            strncpy(last_ok_time, new_tuple->value->cstring, sizeof(last_ok_time));
            text_layer_set_text(time_layer, last_ok_time);
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE, LAST OK: %s ", last_ok_time);

            break; // break for CGM_TIME_KEY

          case CGM_BASAL_KEY:;
           // APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: BASAL");
            strncpy(current_basal, new_tuple->value->cstring, sizeof(current_basal));
            text_layer_set_text(basal_layer, current_basal);
            //APP_LOG(APP_LOG_LEVEL_INFO, "name key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());
            break; // break for CGM_BASAL_KEY
          
          case CGM_PREDICT_KEY:;
           // APP_LOG(APP_LOG_LEVEL_INFO, "CGM_PREDICT_KEY: %s", new_tuple->value->cstring);
            strncpy(predict, new_tuple->value->cstring, sizeof(predict));
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "SYNC TUPLE,PREDICT: %s ", predict);
            //APP_LOG(APP_LOG_LEVEL_INFO, "icon key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());
            text_layer_set_text(predict_layer, predict);
            if ( (strcmp(predict, " ") == 0) || (strcmp(predict, "X") == 0) ) {
                  layer_set_hidden(predict_circle_layer, true);
                  layer_set_hidden(text_layer_get_layer(predict_layer), true);
            }else {
                 layer_set_hidden(predict_circle_layer, false);
                 layer_set_hidden(text_layer_get_layer(predict_layer), false);
            }
            break; // break for CGM_ICON_KEY
          case CGM_PUMP_KEY:;
            //APP_LOG(APP_LOG_LEVEL_INFO, "SYNC TUPLE: CALCULATED RAW");
            strncpy(pump_status, new_tuple->value->cstring, sizeof(pump_status));
            text_layer_set_text(s_layer, pump_status);
            text_layer_set_background_color(s_layer, chart_colour);

            //APP_LOG(APP_LOG_LEVEL_INFO, "clrw key : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

            break; // break for CGM_CLRW_KEY


        default:
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "new_tuple->value->cstring: %s" ,new_tuple->value->cstring);
            break;
        }  // end switch(key)

	if(bgsx_array_set  && bgty_array_set && chart_layer != NULL)
	{
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "TRYING TO CREATE GRAPH AS DATA EXISTS");
    for(int index = 0; index < bgsx_array_counter; ++index)
      {
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "x: %d, y: %d",bgsx_array[index], bgty_array[index] );
    }
		//clock_refresh(time_now);
    //Layer* s_chart_layer = chart_layer_get_layer(chart_layer);
   // layer_mark_dirty(s_chart_layer);
	  //layer_mark_dirty(chart_layer);
		chart_layer_set_margin(chart_layer, 7);
		chart_layer_set_data(chart_layer, bgty_array, eINT, bgsx_array, eINT, bgsx_array_counter);

		bgsx_array_set = NULL;
		bgty_array_set = NULL;
	}
    //}
    null_tuple_buffer(&new_tuple);
    null_tuple_buffer(&old_tuple);
    null_context_buffer(&context);
        //APP_LOG(APP_LOG_LEVEL_INFO, "9 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

} // end sync_tuple_changed_callback_cgm()

static void send_cmd_cgm(void) {

  // check bluetooth ADDED JUNE 20 V
bt_connected = connection_service_peek_pebble_app_connection(); //was bluetooth_connection_service_peek

        if (bt_connected == false) {
            //      Bluetooth is out; set BT message
            //APP_LOG(APP_LOG_LEVEL_INFO, "LOAD BG, BG INIT: NO BT, SET NO BT MESSAGE");
            if (TurnOff_NOBLUETOOTH_Msg == 100) {
                   text_layer_set_text(message_layer, "NO BT\0");
            } // if turnoff nobluetooth msg
        }
    DictionaryIterator *iter = NULL;
    AppMessageResult sendcmd_openerr = APP_MSG_OK;
    AppMessageResult sendcmd_senderr = APP_MSG_OK;
    DictionaryResult sendcmd_dicterr = DICT_OK;


    //APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD IN, ABOUT TO OPEN APP MSG OUTBOX");
    sendcmd_openerr = app_message_outbox_begin(&iter);

    //APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, CHECK FOR ERROR");
    if (sendcmd_openerr != APP_MSG_OK) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD OPEN ERROR");
        null_dict_buffer(&iter);
        sync_error_callback_cgm(sendcmd_dicterr, sendcmd_openerr, iter);
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD OPEN ERR CODE: %i RES: %s", sendcmd_openerr, translate_app_error(sendcmd_openerr));
        return;
    }

    //APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD, MSG OUTBOX OPEN, NO ERROR, ABOUT TO SEND MSG TO APP");
    sendcmd_senderr = app_message_outbox_send();

    if (sendcmd_senderr != APP_MSG_OK) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "WATCH SENDCMD SEND ERROR");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "WATCH SENDCMD SEND ERR CODE: %i RES: %s", sendcmd_senderr, translate_app_error(sendcmd_senderr));
        null_dict_buffer(&iter);
        sync_error_callback_cgm(sendcmd_dicterr, sendcmd_senderr, iter);

    }
  null_dict_buffer(&iter);

    //APP_LOG(APP_LOG_LEVEL_INFO, "SEND CMD OUT, SENT MSG TO APP");

} // end send_cmd_cgm

void timer_callback_cgm(void *data) {

    //  APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK IN, TIMER POP, ABOUT TO CALL SEND CMD");
    // reset msg timer to NULL
    /*June 25if (timer_cgm != NULL) {
        timer_cgm = NULL;
    }*/
    null_and_cancel_timer (&timer_cgm, false);

    // send message
    send_cmd_cgm();

    //APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, SEND CMD DONE, ABOUT TO REGISTER TIMER");
    // set msg timer
    timer_cgm = app_timer_register((WATCH_MSGSEND_SECS*MS_IN_A_SECOND), timer_callback_cgm, NULL);

    //APP_LOG(APP_LOG_LEVEL_INFO, "TIMER CALLBACK, REGISTER TIMER DONE");

} // end timer_callback_cgm

// format current time from watch

void handle_minute_tick_cgm(struct tm* tick_time_cgm, TimeUnits units_changed_cgm) {

    // VARIABLES
    size_t tick_return_cgm = 0;

    // CODE START

    if (units_changed_cgm & MINUTE_UNIT) {
        //APP_LOG(APP_LOG_LEVEL_INFO, "TICK TIME MINUTE CODE");
        if (timeformat == 0){
            tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%l:%M", tick_time_cgm);
        } else {
            tick_return_cgm = strftime(time_watch_text, TIME_TEXTBUFF_SIZE, "%H:%M", tick_time_cgm);
        }
        if (tick_return_cgm != 0) {
            text_layer_set_text(time_watch_layer, time_watch_text);
        }

        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime IN:  %i", lastAlertTime);
        // increment BG snooze
        ++lastAlertTime;
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "lastAlertTime OUT:  %i", lastAlertTime);
        // check watch battery
        // handle_watch_battery_cgm(battery_state_service_peek()); THIS WAS DUPLICATED REMOVED NOV4
      bt_connected = connection_service_peek_pebble_app_connection ();//June 23
    if ((bt_connected == true) && (BluetoothAlert == 111)) {//June 23
      bt_handler(bt_connected);//June 23
    }
    }

} // end handle_minute_tick_cgm


void window_cgm_add_text_layer (TextLayer **cgm_text_layer, GRect cgm_text_layer_pos, char *cgm_text_font) {

  *cgm_text_layer = text_layer_create(cgm_text_layer_pos);
  text_layer_set_background_color(*cgm_text_layer, GColorClear);
  text_layer_set_font(*cgm_text_layer, fonts_get_system_font(cgm_text_font));
  text_layer_set_text_alignment(*cgm_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer_cgm, text_layer_get_layer(*cgm_text_layer));

} // end window_cgm_add_text_layer

void window_cgm_add_bitmap_layer (BitmapLayer **cgm_bitmap_layer, GRect cgm_bmap_layer_pos, GAlign bmap_align) {

  *cgm_bitmap_layer = bitmap_layer_create(cgm_bmap_layer_pos);
  bitmap_layer_set_alignment(*cgm_bitmap_layer, bmap_align);
  bitmap_layer_set_background_color(*cgm_bitmap_layer, GColorClear);
  bitmap_layer_set_compositing_mode(*cgm_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer_cgm, bitmap_layer_get_layer(*cgm_bitmap_layer));

} // end window_cgm_add_bitmap_layer
//MAIN WINDOW
void window_load_cgm(Window *window_cgm) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD");

    window_layer_cgm = window_get_root_layer(window_cgm);
//    GRect window_bounds_cgm = layer_get_bounds(window_layer_cgm);

    GRect window_bounds = layer_get_bounds(window_layer_cgm);
    // CODE START
  
  
//CHART LAYER
    chart_layer = chart_layer_create((GRect) {
#ifdef PBL_PLATFORM_CHALK
        .origin = { 3, 98},
            .size = { 175, 35 } });
#else
        .origin = { -1, 104},
            .size = { 145, 43 } });
#endif
        //chart_layer_set_plot_colour(chart_layer, plot_colour);
        chart_layer_set_canvas_color(chart_layer, GColorPictonBlue);
        chart_layer_show_points_on_line(chart_layer, true);
        chart_layer_animate(chart_layer, false);
        layer_add_child(window_layer_cgm, chart_layer_get_layer(chart_layer));
        // Click callback
        //    window_set_click_config_provider(window_cgm,(ClickConfigProvider)click_config_provider);

// TIME; CURRENT ACTUAL TIME FROM WATCH
#define TIME_WATCH_LAYER_OFFSET PBL_IF_ROUND_ELSE(GRect(0, 141, 180, 30), GRect(4, 139, 144, 30))
    window_cgm_add_text_layer(&time_watch_layer, TIME_WATCH_LAYER_OFFSET, FONT_KEY_GOTHIC_28_BOLD);
    text_layer_set_text_color(time_watch_layer, GColorWhite);

// DATE
//#define DATE_APP_LAYER_OFFSET PBL_IF_ROUND_ELSE(GRect(56, 130, 50, 22), GRect(-1, 147, 50, 22))
    window_cgm_add_text_layer(&date_app_layer,(PBL_IF_ROUND_ELSE(GRect(56, 130, 50, 22), GRect(-1, 147, 50, 22))), FONT_KEY_GOTHIC_18_BOLD);
    text_layer_set_text_color(date_app_layer, GColorWhite);
    draw_date_from_app();

// TOPHALF
#define TOPHALF_OFFSET PBL_IF_ROUND_ELSE(GRect(0, 0, 200, 68), GRect(0, 0, 145, 77))
    window_cgm_add_text_layer(&tophalf_layer, (TOPHALF_OFFSET), FONT_KEY_GOTHIC_28_BOLD);

//NAME CIRCLE LAYER
    name_circle_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
    layer_add_child(window_layer_cgm, name_circle_layer);
    layer_set_update_proc(name_circle_layer, name_circle_update_proc);
    layer_set_hidden(name_circle_layer, true);

//PREDICT CIRCLE LAYER
    predict_circle_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
    layer_add_child(window_layer_cgm, predict_circle_layer);
    layer_set_update_proc(predict_circle_layer, predict_circle_update_proc);
    layer_set_hidden(predict_circle_layer, true);

//CIRCLE LAYER
    circle_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
    layer_add_child(window_layer_cgm, circle_layer);
    layer_set_update_proc(circle_layer, circle_update_proc);

//COB CIRCLE LAYER
    cob_circle_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
    layer_add_child(window_layer_cgm, cob_circle_layer);
    layer_set_update_proc(cob_circle_layer, cob_circle_update_proc);
    layer_set_hidden(cob_circle_layer, true);

// NOISE
#define NOISE_LAYER_OFFSET PBL_IF_ROUND_ELSE(GRect(71, -15, 40, 28), GRect(54, -16, 40, 28))
    window_cgm_add_text_layer(&noise_layer, (NOISE_LAYER_OFFSET), FONT_KEY_GOTHIC_28_BOLD);

// RIG BATTERY LEVEL
#define RIG_BATTLEVEL_OFFSET PBL_IF_ROUND_ELSE(GRect(66, 78, 50, 50), GRect(52, 81, 45, 50))
    window_cgm_add_text_layer(&rig_battlevel_layer, (RIG_BATTLEVEL_OFFSET), FONT_KEY_GOTHIC_18_BOLD);
    text_layer_set_text_color(rig_battlevel_layer, GColorWhite);

//WATCH BATTERY ICON LAYER
#define BATTERY_GRAPHICS_OFFSET PBL_IF_ROUND_ELSE(GRect(106, 139, 31, 15), GRect(110, 155, 31, 15))
    batteryGraphicsLayer = layer_create((BATTERY_GRAPHICS_OFFSET));
    layer_set_update_proc( batteryGraphicsLayer, batteryGraphicsLayerDraw );
    layer_add_child( window_get_root_layer(window_cgm), batteryGraphicsLayer );
    batteryOutlinePath = gpath_create(&BATTERY_OUTLINE);

// WATCH CHARGING ICON
#define BATTERY_OFFSET PBL_IF_ROUND_ELSE(GRect(106, 139, 14, 6), GRect(110, 155, 21, 9))
    window_cgm_add_bitmap_layer(&battery_layer, (BATTERY_OFFSET), GAlignCenter);

// COB
#define COB_OFFSET PBL_IF_ROUND_ELSE(GRect(16, 130, 40, 28), GRect(-12, 47, 60 , 30))
    window_cgm_add_text_layer(&cob_layer, (COB_OFFSET), FONT_KEY_GOTHIC_18_BOLD);
    layer_set_hidden(text_layer_get_layer(cob_layer), true);

// WATCH BATTERY LEVEL TEXT
#define watch_battlevel_OFFSET PBL_IF_ROUND_ELSE(GRect(110, 157, 31, 15), GRect(110, 150, 50, 22))
    window_cgm_add_text_layer(&watch_battlevel_layer, (watch_battlevel_OFFSET), FONT_KEY_GOTHIC_14_BOLD);
    handle_watch_battery_cgm(battery_state_service_peek());

// T1D NAME/IOB
#define t1dname_OFFSET PBL_IF_ROUND_ELSE(GRect(7,33, 44, 44), GRect(0, 3, 40, 40))
    window_cgm_add_text_layer(&t1dname_layer, (t1dname_OFFSET), FONT_KEY_GOTHIC_24_BOLD);
    text_layer_set_text_color(t1dname_layer, text_colour);

// ICON, ARROW OR SPECIAL VALUE
    window_cgm_add_bitmap_layer(&icon_layer, GRect(33, 3, 135, 144), GAlignLeft);

// RAW CALCULATED
#define raw_calc_OFFSET PBL_IF_ROUND_ELSE(GRect(200, 7, 35, 25), GRect(200, 5, 35, 25))
    window_cgm_add_text_layer(&raw_calc_layer, (raw_calc_OFFSET), FONT_KEY_GOTHIC_24_BOLD);
    text_layer_set_text_color(raw_calc_layer, GColorWhite);

// SYMBOL
#define symbol_OFFSET PBL_IF_ROUND_ELSE(GRect(117, 65, 18, 40), GRect(94, 73, 18, 40))
        window_cgm_add_bitmap_layer(&symbol_layer, (symbol_OFFSET), GAlignCenter);

// BASAL
#define basal_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(3, 73, 60, 25), GRect(-5, 80, 65, 24))
        window_cgm_add_text_layer(&basal_layer, (basal_layer_OFFSET), (PBL_IF_ROUND_ELSE((FONT_KEY_GOTHIC_18_BOLD),(FONT_KEY_GOTHIC_18_BOLD))));
        text_layer_set_text_color(basal_layer, GColorWhite);

// PREDICT
#define predict_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(111, 124, 60, 25), GRect(97, 115, 60, 24))
        window_cgm_add_text_layer(&predict_layer, (predict_layer_OFFSET), (PBL_IF_ROUND_ELSE((FONT_KEY_GOTHIC_24_BOLD),(FONT_KEY_GOTHIC_24_BOLD))));
        //text_layer_set_text_color(predict_layer, text_colour);
        layer_set_hidden(text_layer_get_layer(predict_layer), true);
  
// LOOP TIME AGO
#define time_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(126, 66, 50, 25), GRect(102, 80, 50, 25))
        window_cgm_add_text_layer(&time_layer, (time_layer_OFFSET), (PBL_IF_ROUND_ELSE((FONT_KEY_GOTHIC_24_BOLD),(FONT_KEY_GOTHIC_18_BOLD))));
        text_layer_set_text_color(time_layer, GColorWhite);
        text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);

// BG
#define bg_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(15, 17, 144, 800), GRect(0, 17, 144, 80))
        window_cgm_add_text_layer(&bg_layer, (bg_layer_OFFSET), FONT_KEY_BITHAM_42_BOLD);

// DELTA BG / MESSAGE LAYER
#define message_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(18, 54, 144, 50), GRect(2, 53, 144, 50))
        window_cgm_add_text_layer(&message_layer, (message_layer_OFFSET), FONT_KEY_GOTHIC_24_BOLD);

// PERFECT BG
#define perfectbg_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(-140, 88, 50, 49), GRect(-140, 94, 50, 53))
      window_cgm_add_bitmap_layer(&perfectbg_layer, (perfectbg_layer_OFFSET), GAlignLeft);

// CGM TIME AGO READING
#define cgmtime_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(63, 1, 50, 24), GRect(48, 1, 50, 28))
        window_cgm_add_text_layer(&cgmtime_layer, (cgmtime_layer_OFFSET), FONT_KEY_GOTHIC_24_BOLD);
        text_layer_set_text_color(cgmtime_layer, text_colour);
        //text_layer_set_text_alignment(cgmtime_layer, GTextAlignmentRight);

// HAPPY MSG LAYER
//#define happymsg_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(-180, 98, 175, 38), GRect(-144, 104, 145, 43))
        window_cgm_add_text_layer(&happymsg_layer, (PBL_IF_ROUND_ELSE(GRect(-180, 98, 175, 38), GRect(-144, 104, 145, 43))), FONT_KEY_GOTHIC_18_BOLD);
        text_layer_set_text_color(happymsg_layer, plot_colour);
        text_layer_set_overflow_mode(happymsg_layer, GTextOverflowModeWordWrap);

// PUMP LAYER
#define s_layer_OFFSET PBL_IF_ROUND_ELSE(GRect(0, 98, 175, 38), GRect(0, 104, 145, 43))
        window_cgm_add_text_layer(&s_layer, (s_layer_OFFSET), FONT_KEY_GOTHIC_18_BOLD);
        text_layer_set_overflow_mode(s_layer, GTextOverflowModeWordWrap);
        text_layer_set_text_color(s_layer, text_colour);
        text_layer_set_background_color(s_layer, chart_colour);
        layer_set_hidden(text_layer_get_layer(s_layer), true);



        // put " " (space) in bg field so logo continues to show
        // " " (space) also shows these are init values, not bad or null values
        Tuplet initial_values_cgm[] = {
            TupletCString(CGM_ICON_KEY, " "),
            TupletCString(CGM_BG_KEY, " "),
            TupletInteger(CGM_TCGM_KEY, 0),
            TupletInteger(CGM_TAPP_KEY, 0),
            TupletCString(CGM_DLTA_KEY, "LOOP"),
            TupletCString(CGM_UBAT_KEY, " "),
            TupletCString(CGM_NAME_KEY, " "),
            TupletCString(CGM_VALS_KEY, " "),
            TupletCString(CGM_CLRW_KEY, " "),
            TupletCString(CGM_BGSX_KEY, " "),
            //TupletInteger(CGM_BGTY_KEY, 0),
            TupletCString(CGM_COB_KEY,  " "),
            TupletCString(CGM_SYM_KEY, " "),
            TupletCString(CGM_TIME_KEY, " "),
            TupletCString(CGM_BASAL_KEY, " "),
            TupletCString(CGM_PUMP_KEY, " "),
            TupletCString(CGM_PREDICT_KEY, " "),

        };

        //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, ABOUT TO CALL APP SYNC INIT");
        app_sync_init(&sync_cgm, sync_buffer_cgm, sizeof(sync_buffer_cgm), initial_values_cgm, ARRAY_LENGTH(initial_values_cgm), sync_tuple_changed_callback_cgm, sync_error_callback_cgm, NULL);

        // init timer to null if needed, and register timer
        //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, APP INIT DONE, ABOUT TO REGISTER TIMER");
        /*June 25if (timer_cgm != NULL) {
            timer_cgm = NULL;
        }*/
        null_and_cancel_timer (&timer_cgm, false);

        timer_cgm = app_timer_register((LOADING_MSGSEND_SECS*MS_IN_A_SECOND), timer_callback_cgm, NULL);
        //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW LOAD, TIMER REGISTER DONE");

        bt_handler(connection_service_peek_pebble_app_connection());
        if (bt_connected == true) {
         bt_handler(bt_connected);
  }

} // end window_load_cgm


void window_unload_cgm(Window *window_cgm) {
      app_sync_deinit(&sync_cgm);
        //APP_LOG(APP_LOG_LEVEL_INFO, "unload vefore free bgty_array : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

 if(bgty_array)
            {
                free(bgty_array);
                bgty_array = NULL;
            }
          //APP_LOG(APP_LOG_LEVEL_INFO, "unload after free bgty_array : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

  if(bgsx_array)
            {
                free(bgsx_array);
                bgsx_array = NULL;
            }

  if(chart_layer != NULL)
    {
        chart_layer = NULL;
    }
    //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD IN");
    //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, APP SYNC DEINIT");

    //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY GBITMAPS IF EXIST");
    destroy_null_GBitmap(&icon_bitmap);
    destroy_null_GBitmap(&specialvalue_bitmap);
    destroy_null_GBitmap(&perfectbg_bitmap);
    destroy_null_GBitmap(&battery_bitmap);
    destroy_null_GBitmap(&symbol_bitmap);

    //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY LAYERS IF EXIST");
    destroy_null_Layer(&circle_layer);
    destroy_null_Layer(&cob_circle_layer);
    destroy_null_Layer(&name_circle_layer);
    destroy_null_Layer(&predict_circle_layer);
    destroy_null_Layer(&batteryGraphicsLayer);

    chart_layer_destroy(chart_layer);

    gpath_destroy(batteryOutlinePath);

    //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY BITMAPS IF EXIST");
    destroy_null_BitmapLayer(&icon_layer);
    destroy_null_BitmapLayer(&perfectbg_layer);
    destroy_null_BitmapLayer(&battery_layer);
    destroy_null_BitmapLayer(&symbol_layer);


    //APP_LOG(APP_LOG_LEVEL_INFO, "WINDOW UNLOAD, DESTROY TEXT LAYERS IF EXIST");
    destroy_null_TextLayer(&tophalf_layer);
    destroy_null_TextLayer(&bg_layer);
    destroy_null_TextLayer(&cgmtime_layer);
    destroy_null_TextLayer(&message_layer);
    destroy_null_TextLayer(&rig_battlevel_layer);
    destroy_null_TextLayer(&watch_battlevel_layer);
    destroy_null_TextLayer(&t1dname_layer);
    destroy_null_TextLayer(&time_watch_layer);
    destroy_null_TextLayer(&date_app_layer);
    destroy_null_TextLayer(&happymsg_layer);
    destroy_null_TextLayer(&raw_calc_layer);
    destroy_null_TextLayer(&noise_layer);
    destroy_null_TextLayer(&cob_layer);
    destroy_null_TextLayer(&basal_layer);
    destroy_null_TextLayer(&time_layer);
    destroy_null_TextLayer(&s_layer);
    destroy_null_TextLayer(&predict_layer);


      //APP_LOG(APP_LOG_LEVEL_INFO, "10 : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

    //  APP_LOG(APP_LOG_LEVEL_INFO, "window_cgm_UNLOAD Heap Used: %d, Free: %d ", heap_bytes_used(), heap_bytes_free());
}

static void init_cgm(void) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE IN");
    // subscribe to the tick timer service
    //JUNE 18 Changed from bgsx_array_set = FALSE;

    bgsx_array_set = false;
    bgty_array_set = false;
    //TAP
    accel_tap_service_subscribe(accel_tap_handler);
  
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick_cgm);

    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = bt_handler
    });
    // subscribe to the watch battery state service
    battery_state_service_subscribe(&handle_watch_battery_cgm);

    // init the window pointer to NULL if it needs it
    if (window_cgm != NULL) {
        window_cgm = NULL;
    }

    // create the windows
    window_cgm = window_create();
    //window_set_background_color(window_cgm, bottom_colour);
    //window_set_fullscreen(window_cgm, true);

    window_set_window_handlers(window_cgm, (WindowHandlers) {
        .load = window_load_cgm,
            .unload = window_unload_cgm
    });

    //    window_set_click_config_provider(window_cgm, click_config_provider);

    //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, REGISTER APP MESSAGE ERROR HANDLERS");
    //app_message_register_inbox_dropped(inbox_dropped_handler_cgm);
    //app_message_register_outbox_failed(outbox_failed_handler_cgm);

    //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, ABOUT TO CALL APP MSG OPEN");
     //app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  app_message_open(800, 800);
    //  APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE, APP MSG OPEN DONE");
    if(chart_layer != NULL)
    {
        chart_layer = NULL;
    }

           if(bgty_array)
            {
                free(bgty_array);
                bgty_array = NULL;
            }
           if(bgsx_array)
            {
                free(bgsx_array);
                bgsx_array = NULL;
            }

     const bool animated_cgm = true;
  if (window_cgm != NULL) {
    window_stack_push(window_cgm, animated_cgm);
  }
    //window_stack_push(window_cgm, true);
    //APP_LOG(APP_LOG_LEVEL_INFO, "INIT CODE OUT");
      //APP_LOG(APP_LOG_LEVEL_INFO, "init : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

}  // end init_cgm


static void deinit_cgm(void) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT CODE IN");
    // unsubscribe to the tick timer service
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, UNSUBSCRIBE TICK TIMER");
    tick_timer_service_unsubscribe();
    // unsubscribe to the bluetooth connection service
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, UNSUBSCRIBE BLUETOOTH");
    //bluetooth_connection_service_unsubscribe();
    connection_service_unsubscribe();
    // unsubscribe to the watch battery state service
    battery_state_service_unsubscribe();
    //unsubscribe tap
    accel_tap_service_unsubscribe();

    //FREE MALLOC
    free(bgsx_array);
    free(bgty_array);
    bgsx_array_set = NULL;
    bgty_array_set = NULL;

    // cancel timers if they exist
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL APP TIMER");
    /*if (timer_cgm != NULL) {
        app_timer_cancel(timer_cgm);
        timer_cgm = NULL;
    }*/
      //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL APP TIMER");
    null_and_cancel_timer(&timer_cgm, true);
     //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CANCEL BLUETOOTH TIMER");
    null_and_cancel_timer(&BT_timer, true);

    // destroy the main window if it exists
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CHECK WINDOW POINTER FOR DESTROY");
    if (window_cgm != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, WINDOW POINTER NOT NULL, DESTROY");
    window_destroy(window_cgm);
  }
  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, CHECK WINDOW POINTER FOR NULL");
  if (window_cgm != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT, WINDOW POINTER NOT NULL, SET TO NULL");
    window_cgm = NULL;
  }
      //APP_LOG(APP_LOG_LEVEL_INFO, "deinit : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

  //APP_LOG(APP_LOG_LEVEL_INFO, "DEINIT CODE OUT");
} // end deinit_cgm

int main(void) {
      //APP_LOG(APP_LOG_LEVEL_INFO, "main : Memory Used = %d Free = %d", heap_bytes_used(), heap_bytes_free());

    init_cgm();
    app_event_loop();
    deinit_cgm();

} // end main