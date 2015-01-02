
// NOTES
// -Sending byte arrays results in almost no data being sent on iOS; for
//  some reason sending individual bytes as int8_t seems to work though
//    -although I'm not positive everything is getting through here either
// -This currently sends accelerometer values as int8_ts at 20Hz; this 
//  means we're sending 60 bytes/sec
//    -there's *maybe* a *tiny* amount of info loss from this relative to
//    full bit depth at 50Hz+, but since the ADC is only 10 bits to begin
//    with and this still produces different values over time when left
//    at rest on a stationary object, it's unlikely we're really losing
//    anything but noise (and we're saving tons of BLE transmits)

#include <pebble.h>

// ================================================================
// Constants
// ================================================================

// set these to mess with stuff
// #define SAMPLE_RATE ACCEL_SAMPLING_25HZ
#define SAMPLE_RATE ACCEL_SAMPLING_100HZ  // *not* 100, just a flag
#define SAMPLE_BATCH 20     // must be a multiple of DOWNSAMPLE_BY
#define BUFFER_SAMPLE_RATE 20
#define SAMPLE_ELEMENTS 3
// #define BUFFER_T int16_t
#define BUFFER_T int8_t
#define BUFFER_QUANTIZE_SHIFT 4
#define BUFFER_QUANTIZE_MASK 0xff
// #define BYTE_ARRAY   //use byte array instead of ints; doesn't work for no reason
#define RAW       // use just x,y,z, not time step and vibrate
// #define SHOW_ACCEL //otherwise show time

// below breaks cuz SAMPLE_RATE is a flag, not Hz
// #define SAMPLE_BATCH (SAMPLE_RATE>25?25:SAMPLE_RATE) // TX 1/sec, but max 25

// derived constants
#define DOWNSAMPLE_BY (100 / BUFFER_SAMPLE_RATE)  //assumes 100Hz raw sampling
#define DOWNSAMPLE (DOWNSAMPLE_BY > 1)
#define BUFFER_LEN (BUFFER_SAMPLE_RATE * SAMPLE_ELEMENTS)
#define BUFFER_END_PADDING 3
#define BUFFER_PAD_VALUE (-128)
#define BUFFER_LEN_WITH_PADDING (BUFFER_LEN + BUFFER_END_PADDING)

// ================================================================
// Static vars
// ================================================================

static Window *window;
static TextLayer *text_layer;
static char s_buffer[128];
static char time_buffer[128];
static uint32_t totalBytes = 0;
static DataLoggingSessionRef data_log;
static BUFFER_T data_buff[BUFFER_LEN_WITH_PADDING];

static int shouldShowAccelData = 0;
static int showingAccelData = 0;

// ================================================================
// Utility funcs
// ================================================================

static inline BUFFER_T quantize(int16_t x) {
  int16_t shifted = x >> BUFFER_QUANTIZE_SHIFT;
  int16_t masked = shifted & BUFFER_QUANTIZE_MASK;
  return (BUFFER_T) masked;
}

// ================================================================
// Displaying crap
// ================================================================

static uint8_t justStarted = 1;
static void show_time() {
  if (showingAccelData || justStarted) {
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
    text_layer_set_text(text_layer, time_buffer);
    justStarted = 0;
  }
  showingAccelData = 0;
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  snprintf(time_buffer, 16, "\n%02d:%02d", tick_time->tm_hour, tick_time->tm_min);
  if (! shouldShowAccelData) {
    show_time();
  }
}

static void show_accel_data(AccelRawData* data) {
  if (! showingAccelData) {
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  }
  showingAccelData = 1;

  snprintf(s_buffer, sizeof(s_buffer),
    "SampRate=%d\n  X, Y, Z\n0 %d,%d,%d\n1 %d,%d,%d\n2 %d,%d,%d\nbytes: %ld",
    (int)BUFFER_SAMPLE_RATE,

  #if DOWNSAMPLE
    data_buff[0],
    data_buff[1],
    data_buff[2],
    data_buff[3],
    data_buff[4],
    data_buff[5],
    data_buff[6],
    data_buff[7],
    data_buff[8],
  #else
    data[0].x, data[0].y, data[0].z,
    data[1].x, data[1].y, data[1].z,
    data[2].x, data[2].y, data[2].z,
  #endif
    totalBytes
  );
  text_layer_set_text(text_layer, s_buffer);
}

// ================================================================
// UI Callbacks
// ================================================================

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (shouldShowAccelData) {
    shouldShowAccelData = 0;
    show_time();
  } else {
    shouldShowAccelData = 1;
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // text_layer_set_text(text_layer, "up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // text_layer_set_text(text_layer, "down");
}

// ================================================================
// Accelerometer callback
// ================================================================
// this is complicated because it's flexible wrt sampling rates, buffer
// sizes, etc; also because I unrolled the loops to do FIR filters with
// various numbers of taps

#ifdef RAW
static void accel_raw_data_handler(AccelRawData *data, uint32_t num_samples, uint64_t timestamp) {
#else
static void accel_data_handler(AccelData *data, uint32_t num_samples) {
#endif

  //-------------------------------
  // downsample if necessary
  //-------------------------------
#if DOWNSAMPLE
  // simple IIR filters; ideally use IIR
  static int32_t x_avg, y_avg, z_avg;
  static uint8_t buff_idx = 0;
  for (uint16_t i = 0; i < num_samples; i += DOWNSAMPLE_BY) {
    x_avg = 0; y_avg = 0; z_avg = 0;
  #if DOWNSAMPLE_BY == 5
    static int8_t shifts[] = {5,2,1,2,5}; // 1/32, 1/4, 1/2, 1/4, 1/32
    // it's basically identical to the coeffs for:
    //  scipy.signal.firwin(5, cutoff = 0.2, window = "hamming")

    x_avg +=  data[i+0].x >> shifts[0];
    x_avg += (data[i+1].x >> shifts[1]) - (data[i+1].x >> 6);
    x_avg += (data[i+2].x >> shifts[2]) - (data[i+2].x >> 5);
    x_avg += (data[i+3].x >> shifts[3]) - (data[i+3].x >> 6);
    x_avg +=  data[i+4].x >> shifts[4];

    y_avg +=  data[i+0].y >> shifts[0];
    y_avg += (data[i+1].y >> shifts[1]) - (data[i+1].y >> 6);
    y_avg += (data[i+2].y >> shifts[2]) - (data[i+2].y >> 5);
    y_avg += (data[i+3].y >> shifts[3]) - (data[i+3].y >> 6);
    y_avg +=  data[i+4].y >> shifts[4];

    x_avg +=  data[i+0].z >> shifts[0];
    z_avg += (data[i+1].z >> shifts[1]) - (data[i+1].z >> 6);
    z_avg += (data[i+2].z >> shifts[2]) - (data[i+2].z >> 5);
    z_avg += (data[i+3].z >> shifts[3]) - (data[i+3].z >> 6);
    z_avg +=  data[i+4].z >> shifts[4];

  #elif DOWNSAMPLE_BY == 4
    static int8_t shifts[] = {5,1,1,5}; // 1/32, 1/2, 1/2, 1/32
    // note that we subtract off the small values so the total
    // magnitude is 1; this is also closer to the FIR coeffs you
    // actually get if you do a filter with wc of .25 and hamming window
    // it's basically identical to the coeffs for:
    //  scipy.signal.firwin(4, cutoff = 0.34695, window = "hamming")

    x_avg +=  data[i+0].x >> shifts[0];
    x_avg += (data[i+1].x >> shifts[1]) - (data[i+1].x >> shifts[0]);
    x_avg += (data[i+2].x >> shifts[2]) - (data[i+2].x >> shifts[3]);
    x_avg +=  data[i+3].x >> shifts[0];

    y_avg +=  data[i+0].y >> shifts[0];
    y_avg += (data[i+1].y >> shifts[1]) - (data[i+1].y >> shifts[0]);
    y_avg += (data[i+2].y >> shifts[2]) - (data[i+2].y >> shifts[3]);
    y_avg +=  data[i+3].y >> shifts[0];

    z_avg +=  data[i+0].z >> shifts[0];
    z_avg += (data[i+1].z >> shifts[1]) - (data[i+1].z >> shifts[0]);
    z_avg += (data[i+2].z >> shifts[2]) - (data[i+2].z >> shifts[3]);
    z_avg +=  data[i+3].z >> shifts[0];

  #elif DOWNSAMPLE_BY == 2
    static int8_t shifts[] = {1,1}; // 1/2, 1/2
    for (uint8_t j = 0; j < DOWNSAMPLE_BY; j++) {
      x_avg += data[i+j].x >> shifts[j];
      y_avg += data[i+j].y >> shifts[j];
      z_avg += data[i+j].z >> shifts[j];
    }
  #else
    for (uint8_t j = 0; j < DOWNSAMPLE_BY; j++) {
      x_avg += data[i+j].x;
      y_avg += data[i+j].y;
      z_avg += data[i+j].z;
    }
    x_avg /= DOWNSAMPLE_BY;
    y_avg /= DOWNSAMPLE_BY;
    z_avg /= DOWNSAMPLE_BY;

  #endif  //DOWNSAMPLE_BY

    data_buff[buff_idx++] = quantize(x_avg);
    data_buff[buff_idx++] = quantize(y_avg);
    data_buff[buff_idx++] = quantize(z_avg);

  } //for each sample

  // if we're downsampling, the buffer will only be full (and thus ready
  // to transmit) every few invocations, since the max number of samples
  // is capped at 25 (and we use 20); eg, 20 raw samples -> 4 buff samples
  // snprintf(s_buffer, 64, "downsample: buff idx: %d\nnumSamples: %ld\nbuffLen: %d", 
  //   buff_idx, num_samples, BUFFER_LEN);
  if (buff_idx < BUFFER_LEN) {
    return;
  }
  //append some marker values so we know where this block ended; if 
  //we don't have this, we have no way of recovering if an element gets
  //lost
  for (uint8_t i = 0; i < BUFFER_END_PADDING; i++) {
    data_buff[buff_idx++] = BUFFER_PAD_VALUE;
  }
  buff_idx = 0;
  totalBytes += BUFFER_LEN * sizeof(BUFFER_T);
#else
  totalBytes += num_samples * sizeof(BUFFER_T) * SAMPLE_ELEMENTS;
#endif  // DOWNSAMPLE

  //-------------------------------
  // print data
  //-------------------------------
  if (shouldShowAccelData) {
    show_accel_data(data);
  }

  //-------------------------------
  // log data
  //-------------------------------
#ifdef BYTE_ARRAY
  #if DOWNSAMPLE
  DataLoggingResult r = data_logging_log(data_log, &data_buff[0], 1);
  // DataLoggingResult r = data_logging_log(data_log, &data_buff[0], BUFFER_LEN);
  #else
  DataLoggingResult r = data_logging_log(data_log, data, 1);
  #endif
#else
  #if DOWNSAMPLE
  DataLoggingResult r = data_logging_log(data_log, &data_buff[0], BUFFER_LEN_WITH_PADDING);
  #else
  DataLoggingResult r = data_logging_log(data_log, data, 3 * num_samples);
  #endif
#endif

  if (r != DATA_LOGGING_SUCCESS) {
    switch (r) {
      case DATA_LOGGING_BUSY:
        snprintf(s_buffer, 48, "data logging busy!");
        break;
      case DATA_LOGGING_FULL:
        snprintf(s_buffer, 48, "data logging full!");
        break;
      case DATA_LOGGING_NOT_FOUND:
        snprintf(s_buffer, 48, "data logging not found!");
        break;
      case DATA_LOGGING_CLOSED:
        snprintf(s_buffer, 48, "data logging closed!");
        break;
      case DATA_LOGGING_INVALID_PARAMS:
        // snprintf(s_buffer, 64, "data logging\ninvalid params!");
        snprintf(s_buffer, 128, "data logging\ninvalid params"
            "\ndata_log null?=%d\nn=%d, data=%p", 
          data_log == NULL,
          (int) num_samples,
          data);
        break;
      default:
        snprintf(s_buffer, 64, "data logging\nunkown error!");
    }
    // snprintf(s_buffer, 32, "data logging failure!");
  }
}

// ================================================================
// Initialization / Destruction
// ================================================================
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create(GRect(5, 0, bounds.size.w - 10, bounds.size.h));
  text_layer_set_overflow_mode(text_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
  // app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED); //faster tx, more power

  data_log = data_logging_create(
    0x5,   //arbitrary "tag" to identify data log
#ifdef BYTE_ARRAY
    DATA_LOGGING_BYTE_ARRAY,      // log type; ios dislikes this one 
    sizeof(BUFFER_T) * BUFFER_LEN_WITH_PADDING,  // size of each log "item"
#else
    DATA_LOGGING_INT,             // log type
    sizeof(BUFFER_T),             // size of each item
#endif
    true );                       // append to old logs with same tag
    // false );                   // overwrite old logs with same tag

#ifdef RAW
  accel_raw_data_service_subscribe(SAMPLE_BATCH, &accel_raw_data_handler);
#else
  accel_data_service_subscribe(SAMPLE_BATCH, &accel_data_handler);
#endif
  accel_service_set_sampling_rate(SAMPLE_RATE);
}

static void deinit(void) {
  data_logging_finish(data_log);
// #ifdef RAW
  // accel_raw_data_service_unsubscribe();  //not a thing, apparently
// #else
  accel_data_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

// ================================================================
// Main
// ================================================================

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Started: %lu", time(NULL));

  app_event_loop();
  deinit();
}
