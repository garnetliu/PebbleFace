#include <pebble.h>
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS
};

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_battery_layer, *s_weather_layer, *s_bluetooth_layer;

static GFont s_clock_font;

static int s_battery_level, s_battery_charging;

//static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  //layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
  
  //if(!connected) {
  //  vibes_double_pulse();
  //}
//}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);

  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }

}
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


static void update_battery() {
  // Write the current battery level into a buffer
  static char battery_buffer[] = "pebble:~$ battery\n100% battery low";
  if (s_battery_charging) { //since there is no resolution between 70% and 100% while charging...
    if (s_battery_level > 60) {
      snprintf(battery_buffer, sizeof(battery_buffer), "pebble:~$ battery\ncharging");
    } else {
      snprintf(battery_buffer, sizeof(battery_buffer), "pebble:~$ battery\n%d%% charging", s_battery_level);
    }
  } else {
      if (s_battery_level > 20) {
        snprintf(battery_buffer, sizeof(battery_buffer), "pebble:~$ battery\n%d%%", s_battery_level);
      } else {
        snprintf(battery_buffer, sizeof(battery_buffer), "pebble:~$ battery\n%d%% battery low", s_battery_level);
      }
  }
  
  // Display this battery on the TextLayer
  text_layer_set_text(s_battery_layer, battery_buffer);
}

static void battery_callback(BatteryChargeState state) {  
  // Record the new battery level
  s_battery_level = state.charge_percent;
  s_battery_charging = state.is_charging;
  
  // Update meter
  update_battery();
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char time_buffer[] = "pebble:~$ time\n00:00 AM";
  strftime(time_buffer, sizeof(time_buffer), clock_is_24h_style() ? "pebble:~$ time\n%H:%M" : "pebble:~$ time\n%I:%M %p", tick_time);

  // Write date
  static char date_text[] = "pebble:~$ date\n00/00/00";
  strftime(date_text, sizeof(date_text), "pebble:~$ date\n%x", tick_time);;
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, time_buffer);
  text_layer_set_text(s_date_layer, date_text);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 60 minutes
  if(tick_time->tm_min % 60 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }

}


static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  int occupied = 30, margin = 3;
  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, occupied));
  s_date_layer = text_layer_create(GRect(0, occupied + margin, bounds.size.w, occupied * 2 + margin));
  s_weather_layer = text_layer_create(GRect(0, occupied * 2 + margin, bounds.size.w, occupied * 3 + margin * 2));
  s_battery_layer = text_layer_create(GRect(0, occupied * 3 + margin * 2, bounds.size.w, occupied * 4 + margin * 3));

  // create GFont
  s_clock_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MENLO_REGULAR_14));
  
  // time layer settings
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_clock_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  
  // date layer settings
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_clock_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  
  // battery layer settings
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_font(s_battery_layer, s_clock_font);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
  
  // Style the text
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
  text_layer_set_font(s_weather_layer, s_clock_font);
  text_layer_set_text(s_weather_layer, "pebble:~$ weather\nloading...");

  // add to window
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_weather_layer);
  
  // Unload font
  fonts_unload_custom_font(s_clock_font);
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Set window background to black
  window_set_background_color(s_main_window, GColorBlack);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  
  // Make sure the status is displayed from the start
  update_time();
  update_battery();
} 

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}