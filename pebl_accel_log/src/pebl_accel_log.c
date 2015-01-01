#include <pebble.h>

// set these to mess with stuff
// #define SAMPLE_RATE ACCEL_SAMPLING_25HZ
#define SAMPLE_RATE ACCEL_SAMPLING_100HZ
#define BUFFER_SAMPLE_RATE 25
#define SAMPLE_ELEMENTS 3
#define BUFFER_T int16_t
#define BYTE_ARRAY
#define RAW       // use just x,y,z, not time step and vibrate

// derived constants
#define SAMPLE_BATCH (SAMPLE_RATE>25?25:SAMPLE_RATE) // TX 1/sec, but max 25
#define DOWNSAMPLE_BY (SAMPLE_RATE / BUFFER_SAMPLE_RATE)
#define DOWNSAMPLE (DOWNSAMPLE_BY > 1)
#define BUFFER_LEN (BUFFER_SAMPLE_RATE * SAMPLE_ELEMENTS)

static Window *window;
static TextLayer *text_layer;
DataLoggingSessionRef data_log;
static BUFFER_T data_buff[BUFFER_LEN];

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}

uint16_t count = 0;
#ifdef RAW
static void accel_raw_data_handler(AccelRawData *data, uint32_t num_samples, uint64_t timestamp) {
#else
static void accel_data_handler(AccelData *data, uint32_t num_samples) {
#endif

#if DOWNSAMPLE
  // crude low-pass filter; ideally have an IIR with history that persists
  // across invocations (or at least add FIR coeffs in the inner loop)
  int32_t x_avg = 0, y_avg = 0, z_avg = 0;
  static uint8_t buff_idx = 0;
  for (uint8_t i = 0; i < SAMPLE_BATCH; i += DOWNSAMPLE_BY) {
    x_avg = 0; y_avg = 0; z_avg = 0;
    for (uint8_t j = 0; j < DOWNSAMPLE_BY; j++) {
      x_avg += data[i+j].x;
      y_avg += data[i+j].y;
      z_avg += data[i+j].z;
    }
    data_buff[buff_idx++] = (BUFFER_T) (x_avg / DOWNSAMPLE_BY);
    data_buff[buff_idx++] = (BUFFER_T) (y_avg / DOWNSAMPLE_BY);
    data_buff[buff_idx++] = (BUFFER_T) (z_avg / DOWNSAMPLE_BY);
  }

  // if we're downsampling, the buffer will only be few (and thus ready
  // to transmit) every few invocations
  if (buff_idx < (BUFFER_LEN - 1)) {
    return;
  }
  buff_idx = 0;
#endif

  //Show the data
  static char s_buffer[128];
  snprintf(s_buffer, sizeof(s_buffer), 
    "Nsamples=%d\nX,Y,Z\n0 %d,%d,%d\n1 %d,%d,%d\n2 %d,%d,%d",
    (int)num_samples,
    data[0].x, data[0].y, data[0].z,
  #if DOWNSAMPLE
    data_buff[0], data_buff[1], data_buff[2],
  #else
    data[1].x, data[1].y, data[1].z,
  #endif
    data[2].x, data[2].y, data[2].z
  );
  text_layer_set_text(text_layer, s_buffer);

#ifdef BYTE_ARRAY
  #if DOWNSAMPLE
  DataLoggingResult r = data_logging_log(data_log, &data_buff[0], 1);
  #else
  DataLoggingResult r = data_logging_log(data_log, data, 1);
  #endif
#else
  #if DOWNSAMPLE
  DataLoggingResult r = data_logging_log(data_log, &data_buff[0], BUFFER_LEN);
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

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

// static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
//     snprintf(time_formatted, 6, "%02d:%02d", tick_time->tm_hour, tick_time->tm_min);
//     text_layer_set_text(text_layer, time_formatted);
// }

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create(GRect(5, 0, bounds.size.w - 10, bounds.size.h));
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(text_layer, "No data yet.");
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

  // init_dlog();
  data_log = data_logging_create(
  /* tag */          0x6,
#ifdef BYTE_ARRAY
  /* DataLogType */  DATA_LOGGING_BYTE_ARRAY,
  /* length */       sizeof(BUFFER_T)*BUFFER_LEN,
#else
  /* DataLogType */   DATA_LOGGING_INT,
  /* length */        2,
#endif
  // /* resume */       true );
  /* resume */       false );

  // tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
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
// #endif
  // tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Started: %lu", time(NULL));

  app_event_loop();
  deinit();
}
