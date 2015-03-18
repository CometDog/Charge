#include "pebble.h"

#ifdef PBL_COLOR
  #include "gcolor_definitions.h" // Allows the user of color
#endif

static Window *s_main_window; // Main window
static TextLayer *s_hour_label, *s_minute_label, *s_date_label; // Layers for time, month, and date
static Layer *s_solid_layer, *s_time_layer, *s_battery_layer; // Background layers
static GFont s_time_font, s_date_font; // Fonts

// Buffers
static char s_date_buffer[] = "XXX XXX";
static char s_hour_buffer[3];
static char s_minute_buffer[3];

// Makes text uppercase when called
char *upcase(char *str)
{
    for (int i = 0; str[i] != 0; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 0x20;
        }
    }

    return str;
}

// Update background when called
static void update_bg(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite); // Set the fill color
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone); // Fill the screen
}

// Handles updating time
static void update_time() {
  // Get time and structure
  time_t temp = time(NULL); 
  struct tm *t = localtime(&temp);

  // Write time as 24h or 12h format onto the buffer
  if(clock_is_24h_style() == true) { // 24h format
    strftime(s_hour_buffer, sizeof(s_hour_buffer), "%H", t);
  } else { // 12h format
    strftime(s_hour_buffer, sizeof(s_hour_buffer), "%I", t);
  }
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", t); // Minutes

  // Display on Time Layer
  text_layer_set_text(s_hour_label, s_hour_buffer);
  text_layer_set_text(s_minute_label, s_minute_buffer);

  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %d", t); // Write month and date to buffers
  
  upcase(s_date_buffer); // Set date to uppercase

  text_layer_set_text(s_date_label, s_date_buffer); // Display month and date on their respective layers
}

static void update_battery(Layer *layer, GContext *ctx) {
  int bat = battery_state_service_peek().charge_percent; // Get battery percentage
  
 // Set battery color based on screen types
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorGreen);
  #else
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, GPoint(104, 128), (bat + (bat / 20))); // Outline for b&w screens
    graphics_context_set_fill_color(ctx, GColorWhite); 
  #endif
  graphics_fill_circle(ctx, GPoint(104, 128), battery_state_service_peek().charge_percent); // Fill circle
}

// Update time when called
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// Loads the layers onto the main window
static void main_window_load(Window *window) {
  
  // Creates window_layer as root and sets its bounds
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the fonts
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPEN_SANS_26));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPEN_SANS_18));
  
  // Create background the layers
  s_solid_layer = layer_create(bounds);
  s_battery_layer = layer_create(bounds);
  s_time_layer = layer_create(bounds);
  
  // Update background layers
  layer_set_update_proc(s_solid_layer, update_bg);
  layer_set_update_proc(s_battery_layer, update_battery);
  
  // Create the labels
  s_hour_label = text_layer_create(GRect(72,72,40,30));
  s_minute_label = text_layer_create(GRect(72,97,40,30));
  s_date_label = text_layer_create(GRect(55,122,70,30));
  
  // Set background and text colors
  text_layer_set_background_color(s_hour_label, GColorClear);
  text_layer_set_text_color(s_hour_label, GColorBlack);
  text_layer_set_background_color(s_minute_label, GColorClear);
  text_layer_set_text_color(s_minute_label, GColorBlack);
  text_layer_set_background_color(s_date_label, GColorClear);
  text_layer_set_text_color(s_date_label, GColorBlack);
  
  // Set fonts
  text_layer_set_font(s_hour_label, s_time_font);
  text_layer_set_font(s_minute_label, s_time_font);
  text_layer_set_font(s_date_label, s_date_font);
  
  // Avoid blank screen in case updating time fails
  text_layer_set_text(s_hour_label, "00");
  text_layer_set_text(s_minute_label, "00");
  text_layer_set_text(s_date_label, "ERROR");
  
  // Align text
  text_layer_set_text_alignment(s_hour_label, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_minute_label, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_date_label, GTextAlignmentCenter);
  
  // Apply layers to screen
  layer_add_child(window_layer, s_solid_layer); 
  layer_add_child(window_layer, s_battery_layer);
  layer_add_child(window_layer, s_time_layer);
  layer_add_child(s_time_layer, text_layer_get_layer(s_hour_label));
  layer_add_child(s_time_layer, text_layer_get_layer(s_minute_label));
  layer_add_child(window_layer, text_layer_get_layer(s_date_label));
  
  // Display the time immediately
  update_time();
}

// Unloads the layers on the main window
static void main_window_unload(Window *window) {
  // Unload the fonts
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  
  // Destroy the background layers
  layer_destroy(s_solid_layer);
  layer_destroy(s_time_layer);
  layer_destroy(s_battery_layer);
  
  // Destroy the labels
  text_layer_destroy(s_hour_label);
  text_layer_destroy(s_date_label);
  text_layer_destroy(s_minute_label);
}
  
// Initializes the main window
static void init() {
  s_main_window = window_create(); // Create the main window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true); // Show the main window. Animations = true.
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler); // Update time every minute
}

// Deinitializes the main window
static void deinit() {
  window_destroy(s_main_window); // Destroy the main window
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}