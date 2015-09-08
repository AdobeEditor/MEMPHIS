#include <pebble.h>
#include "num2text.h"
#define KEY_SKIN_SELECT 0
  
////////

static Layer *s_path1_layer;

// GPath describes the shape
static GPath *s_path1;
static GPathInfo PATH_INFO1 = {
  .num_points = 4,
  .points = (GPoint[]) { {-5, 98}, {145, 98}, {145, 168}, {-5, 168} }
};

static void layer1_update_proc(Layer *layer, GContext *ctx) {
  // Set the color using RGB values
  graphics_context_set_fill_color(ctx, GColorWhite);

  // Draw the filled shape in above color
  gpath_draw_filled(ctx, s_path1);
}
  
static GBitmap *s_squiggle_bitmap;
static BitmapLayer *s_bitmap_layer;
static Layer *s_path2_layer;

static void inbox_recieved_handler(DictionaryIterator *iter, void *context) {
	Tuple *skin_select_t = dict_find(iter, KEY_SKIN_SELECT);
}

if (skin_select_t) {
	if (skin_select_t == 'yellow-black')
		int skin_select = skin_select_t->value;
		persist_write_int(KEY_SKIN_SELECT, skin_select);
		int activeSkin = 'RESOURCE_ID_CHARLIEBROWN_COLOR_IMAGE'
			
	if (skin_select_t == 'lavender-pink')
		int skin_select = skin_select_t->value;
		persist_write_int(KEY_SKIN_SELECT, skin_select);
		int activeSkin = 'RESOURCE_ID_FLASHY_COLOR_IMAGE'
	
	if (skin_select_t == 'yellow-pink')
		int skin_select = skin_select_t->value;
		persist_write_int(KEY_SKIN_SELECT, skin_select);
		int activeSkin = 'RESOURCE_ID_SQUIGGLE_COLOR_IMAGE'
}


////////

typedef enum {
  MOVING_IN,
  IN_FRAME,
  PREPARE_TO_MOVE,
  MOVING_OUT
} SlideState;

typedef struct {
  TextLayer *label;
  SlideState state; // animation state
  char *next_string; // what to say in the next phase of animation
  bool unchanged_font;

  int left_pos;
  int right_pos;
  int still_pos;

  int movement_delay;
  int delay_count;
} SlidingRow;

typedef struct {
  TextLayer *demo_label;
  SlidingRow rows[3];
  int last_hour;
  int last_minute;

  GFont helvetica30_bold;
  GFont helvetica20_light;

  Window *window;
  Animation *animation;


  struct SlidingTextRenderState {
    // double buffered string storage
    char hours[2][32];
    uint8_t next_hours;
    char first_minutes[2][32];
    char second_minutes[2][32];
    uint8_t next_minutes;

    struct SlidingTextRenderDemoTime {
      int secs;
      int mins;
      int hour;
    } demo_time;

  } render_state;

} SlidingTextData;

SlidingTextData *s_data;
  
static void init_sliding_row(SlidingTextData *data, SlidingRow *row, GRect pos, GFont font,
        int delay) {
  row->label = text_layer_create(pos);

  text_layer_set_background_color(row->label, GColorWhite);
  text_layer_set_text_color(row->label, GColorBlack);
  if (font) {
    text_layer_set_font(row->label, font);
    row->unchanged_font = true;
  } else {
    row->unchanged_font = false;
  }

  row->state = IN_FRAME;
  row->next_string = NULL;

  row->left_pos = -pos.size.w;
  row->right_pos = pos.size.w;
  row->still_pos = pos.origin.x;

  row->movement_delay = delay;
  row->delay_count = 0;

  data->last_hour = -1;
  data->last_minute = -1;
}

////////

// GPath describes the shape
static GPath *s_path2;
static GPathInfo PATH_INFO2 = {
  .num_points = 4,
  .points = (GPoint[]) { {-5, 94}, {145, 94}, {145, 97}, {-5, 97} }
};

static void layer2_update_proc(Layer *layer, GContext *ctx) {
  // Set the color using RGB values
  graphics_context_set_fill_color(ctx, GColorBlack);

  // Draw the filled shape in above color
  gpath_draw_filled(ctx, s_path2);
}

////////

static void slide_in_text(SlidingTextData *data, SlidingRow *row, char* new_text) {
  (void) data;

  const char *old_text = text_layer_get_text(row->label);
  if (old_text) {
    row->next_string = new_text;
    row->state = PREPARE_TO_MOVE;
  } else {
    text_layer_set_text(row->label, new_text);
    GRect frame = layer_get_frame(text_layer_get_layer(row->label));
    frame.origin.x = row->right_pos;
    layer_set_frame(text_layer_get_layer(row->label), frame);
    row->state = MOVING_IN;
  }
}


static bool update_sliding_row(SlidingTextData *data, SlidingRow *row) {
  (void) data;

  GRect frame = layer_get_frame(text_layer_get_layer(row->label));
  bool something_changed = true;
  switch (row->state) {
    case PREPARE_TO_MOVE:
      frame.origin.x = row->still_pos;
      row->delay_count++;
      if (row->delay_count > row->movement_delay) {
        row->state = MOVING_OUT;
        row->delay_count = 0;
      }
    break;

    case MOVING_IN: {
      int speed = abs(frame.origin.x - row->still_pos) / 3 + 1;
      frame.origin.x -= speed;
      if (frame.origin.x <= row->still_pos) {
        frame.origin.x = row->still_pos;
        row->state = IN_FRAME;
      }
    }
    break;

    case MOVING_OUT: {
      int speed = abs(frame.origin.x - row->still_pos) / 3 + 1;
      frame.origin.x -= speed;

      if (frame.origin.x <= row->left_pos) {
        frame.origin.x = row->right_pos;
        row->state = MOVING_IN;
        text_layer_set_text(row->label, row->next_string);
        row->next_string = NULL;
      }
    }
    break;

    case IN_FRAME:
    default:
      something_changed = false;
      break;
  }
  if (something_changed) {
    layer_set_frame(text_layer_get_layer(row->label), frame);
  }
  return something_changed;
}

static void animation_update(struct Animation *animation, const AnimationProgress time_normalized) {
  SlidingTextData *data = s_data;

  struct SlidingTextRenderState *rs = &data->render_state;

  time_t now = time(NULL);
  struct tm t = *localtime(&now);

  bool something_changed = false;

  if (data->last_minute != t.tm_min) {
    something_changed = true;

    minute_to_formal_words(t.tm_min, rs->first_minutes[rs->next_minutes], rs->second_minutes[rs->next_minutes]);
    if(data->last_hour != t.tm_hour || t.tm_min <= 20
       || t.tm_min/10 != data->last_minute/10) {
      slide_in_text(data, &data->rows[1], rs->first_minutes[rs->next_minutes]);
    } else {
      // The tens line didn't change, so swap to the correct buffer but don't animate
      text_layer_set_text(data->rows[1].label, rs->first_minutes[rs->next_minutes]);
    }
    slide_in_text(data, &data->rows[2], rs->second_minutes[rs->next_minutes]);
    rs->next_minutes = rs->next_minutes ? 0 : 1;
    data->last_minute = t.tm_min;
  }

  if (data->last_hour != t.tm_hour) {
    hour_to_12h_word(t.tm_hour, rs->hours[rs->next_hours]);
    slide_in_text(data, &data->rows[0], rs->hours[rs->next_hours]);
    rs->next_hours = rs->next_hours ? 0 : 1;
    data->last_hour = t.tm_hour;
  }

  for (size_t i = 0; i < ARRAY_LENGTH(data->rows); ++i) {
    something_changed = update_sliding_row(data, &data->rows[i]) || something_changed;
  }

  if (!something_changed) {
    animation_unschedule(data->animation);
  }
}

static void make_animation() {
  s_data->animation = animation_create();
  animation_set_duration(s_data->animation, ANIMATION_DURATION_INFINITE);
                  // the animation will stop itself
  static const struct AnimationImplementation s_animation_implementation = {
    .update = animation_update,
  };
  animation_set_implementation(s_data->animation, &s_animation_implementation);
  animation_schedule(s_data->animation);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  make_animation();
}

static void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  free(s_data);
}

static void handle_init() {
  SlidingTextData *data = (SlidingTextData*)malloc(sizeof(SlidingTextData));
  s_data = data;

  data->render_state.next_hours = 0;
  data->render_state.next_minutes = 0;
  data->render_state.demo_time.secs = 0;
  data->render_state.demo_time.mins = 0;
  data->render_state.demo_time.hour = 0;

  data->window = window_create();

  window_set_background_color(data->window, GColorWhite);

  data->helvetica30_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELVETICA_BOLD_30));
  data->helvetica20_light = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELVETICA_LIGHT_20));

  Layer *window_layer = window_get_root_layer(data->window);
  GRect layer_frame = layer_get_frame(window_layer);
  const int16_t width = layer_frame.size.w;

  ////////
  
  s_squiggle_bitmap = gbitmap_create_with_resource(activeSkin);
  s_bitmap_layer = bitmap_layer_create(layer_frame);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_squiggle_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  
  s_path1 = gpath_create(&PATH_INFO1);

  s_path1_layer = layer_create(layer_frame);
  layer_set_update_proc(s_path1_layer, layer1_update_proc);
  layer_add_child(window_layer, s_path1_layer);
  
  s_path2 = gpath_create(&PATH_INFO2);

  s_path2_layer = layer_create(layer_frame);
  layer_set_update_proc(s_path2_layer, layer2_update_proc);
  layer_add_child(window_layer, s_path2_layer);
  
  if (persist_read_int(KEY_SKIN_SELECT)) {
	  int skin_select = persist_read_int(KEY_SKIN_SELECT)
	  if (skin_select == 'yellow-black')
	  	int activeSkin = 'RESOURCE_ID_CHARLIEBROWN_COLOR_IMAGE'
			
	  if (skin_select == 'lavender-pink')
	  	int activeSkin = 'RESOURCE_ID_FLASHY_COLOR_IMAGE'
	
	  if (skin_select == 'yellow-pink')
	  	int activeSkin = 'RESOURCE_ID_SQUIGGLE_COLOR_IMAGE'
  }
  
  ////////
  
  init_sliding_row(data, &data->rows[1], GRect(8, 135, width, 30), data->helvetica20_light, 3);
  layer_add_child(window_layer, text_layer_get_layer(data->rows[1].label));
  
  init_sliding_row(data, &data->rows[0], GRect(8, 100, width, 40), data->helvetica30_bold, 6);
  layer_add_child(window_layer, text_layer_get_layer(data->rows[0].label));

  init_sliding_row(data, &data->rows[2], GRect(8, 170, width, 0), data->helvetica20_light, 0);
  layer_add_child(window_layer, text_layer_get_layer(data->rows[2].label));

  GFont norm14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  data->demo_label = text_layer_create(GRect(0, -3, 100, 20));
  text_layer_set_background_color(data->demo_label, GColorWhite);
  text_layer_set_text_color(data->demo_label, GColorBlack);
  text_layer_set_font(data->demo_label, norm14);
  text_layer_set_text(data->demo_label, "demo mode");
  layer_add_child(window_layer, text_layer_get_layer(data->demo_label));

  layer_set_hidden(text_layer_get_layer(data->demo_label), true);
  layer_mark_dirty(window_layer);

  make_animation();

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  const bool animated = true;
  window_stack_push(data->window, animated);
  
  
  ////////
  
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}



int main(void) {
  handle_init();

  app_event_loop();

  handle_deinit();
}

//// Credit is owed to those who developed the 'Sliding Text' watchface as it was used to create the foundation for this watchface.
