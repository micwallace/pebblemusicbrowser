#include <pebble.h>

#define INBOX_BUFFER_SIZE (8 * 1024)

Window *window;
Window *album_window;
Window *song_window;
uint8_t s_inboxBuffer[INBOX_BUFFER_SIZE];
/*static DictionaryIterator list_data;
static DictionaryIterator album_data;
static DictionaryIterator song_data;*/
char **artist_data;
char **album_data;
char **song_data;
static MenuLayer *artist_menu_layer;
static MenuLayer *album_menu_layer;
static MenuLayer *song_menu_layer;
int NUM_MENU_SECTIONS = 1;
int NUM_MENU_ICONS =  0;
int NUM_ARTISTS = 0;
int NUM_ALBUMS = 0;
int NUM_SONGS = 0;
char* currentArtist;
char* currentAlbum;
	
// Key values for AppMessage Dictionary
enum {
	ACTION_KEY = 0,	
	DATA_KEY = 1,
	DATA_KEY_2 = 2
};

// Write message to buffer & send
void send_message(char *action, char *param1, char *param2){
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	Tuplet value = TupletCString(ACTION_KEY, action);
	  dict_write_tuplet(iter, &value);
	Tuplet query = TupletCString(DATA_KEY, param1);
	  dict_write_tuplet(iter, &query);
	Tuplet query2 = TupletCString(DATA_KEY_2, param2);
	  dict_write_tuplet(iter, &query2);
	
	dict_write_end(iter);
  AppMessageResult result = app_message_outbox_send();
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Appmessage sent: %d", result); 
}

static char** load_data_from_dictionary(DictionaryIterator *received, char **dest, int size){
	dest = malloc(size * sizeof(char*));
	Tuple *tuple;
	for(int i = 0; i < size; i++) {
		tuple = dict_find(received, i+2);
		if (tuple != NULL){
			dest[i] = malloc(sizeof(tuple->value->cstring));
			dest[i] = tuple->value->cstring;
		}
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "First item: %s", dest[0]); 
	return dest;
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *action;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Appmessage Received!");
	action = dict_find(received, ACTION_KEY);
	if(action) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Action: %s", action->value->cstring); 
		Tuple *count = dict_find(received, DATA_KEY);
		if(strcmp(action->value->cstring, "ARTISTS") == 0) {
			//memcpy(&list_data, received, sizeof(*received));
			NUM_ARTISTS = count->value->int32;
			artist_data = load_data_from_dictionary(received, artist_data, count->value->int32);
			menu_layer_reload_data(artist_menu_layer);
		}
		if(strcmp(action->value->cstring, "ALBUMS") == 0) {
			//memcpy(&album_data, received, sizeof(*received));
			NUM_ALBUMS = count->value->int32;
			album_data = load_data_from_dictionary(received, album_data, count->value->int32);
			menu_layer_reload_data(album_menu_layer);
		}
		if(strcmp(action->value->cstring, "SONGS") == 0) {
			//memcpy(&song_data, received, sizeof(*received));
			NUM_SONGS = count->value->int32;
			song_data = load_data_from_dictionary(received, song_data, count->value->int32);
			menu_layer_reload_data(song_menu_layer);
		}
	}
	action = NULL;
}
	
// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Appmessage Receive failed!"); 
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Appmessage Send failed!"); 
}

static void out_sent_handler(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}
// SONG MENU
static uint16_t song_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t song_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	return NUM_SONGS;
}

static void song_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	//Tuple *item = dict_find(&song_data, cell_index->row+2);
	char *name = song_data[cell_index->row];
	if (name){
		// There is title draw for something more simple than a basic menu item
		#ifdef PBL_RECT
		menu_cell_title_draw(ctx, cell_layer, name);
		#else
		GSize size = layer_get_frame(cell_layer).size;
		graphics_draw_text(ctx, name, fonts_get_system_font(FONT_KEY_GOTHIC_28),
											 GRect(0, 0, size.w, size.h), GTextOverflowModeTrailingEllipsis, 
											 GTextAlignmentCenter, NULL);
		#endif
	}
}

static void song_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu long click!"); 
  //Tuple *item = dict_find(&song_data, cell_index->row+2);
	char *name = song_data[cell_index->row];
	if (name){
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu click item found: %s", name);
		send_message("PLAY", currentArtist, name);
	}
}

#ifdef PBL_ROUND 
static int16_t song_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) { 
  if (menu_layer_is_index_selected(menu_layer, cell_index)) {
    return MENU_CELL_ROUND_FOCUSED_TALL_CELL_HEIGHT;
  } else {
    return MENU_CELL_ROUND_UNFOCUSED_SHORT_CELL_HEIGHT;
  }
}
#endif

static void song_window_load(Window *window) {
	// Now we prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  song_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(song_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = song_menu_get_num_sections_callback,
    .get_num_rows = song_menu_get_num_rows_callback,
    //.get_header_height = PBL_IF_RECT_ELSE(menu_get_header_height_callback, NULL),
    //.draw_header = PBL_IF_RECT_ELSE(menu_draw_header_callback, NULL),
    .draw_row = song_menu_draw_row_callback,
    .select_click = song_menu_select_callback,
		.select_long_click = song_menu_select_callback,
    .get_cell_height = PBL_IF_ROUND_ELSE(song_get_cell_height_callback, NULL),
  });
	// Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(song_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(song_menu_layer));
}
	
static void song_window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(song_menu_layer);
}

// ALBUM MENU
static uint16_t album_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t album_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	return NUM_ALBUMS;
}

static void album_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	//Tuple *item = dict_find(&album_data, cell_index->row+2);
	char *name = album_data[cell_index->row];
	if (name){
		// There is title draw for something more simple than a basic menu item
		#ifdef PBL_RECT
		menu_cell_title_draw(ctx, cell_layer, name);
		#else
		GSize size = layer_get_frame(cell_layer).size;
		graphics_draw_text(ctx, name, fonts_get_system_font(FONT_KEY_GOTHIC_28),
											 GRect(0, 0, size.w, size.h), GTextOverflowModeTrailingEllipsis, 
											 GTextAlignmentCenter, NULL);
		#endif
	}
}

static void album_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu click!"); 
  //Tuple *item = dict_find(&album_data, cell_index->row+2);
	char *name = album_data[cell_index->row];
	if (name){
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu click item found: %s", name);
		song_window = window_create();
		window_set_window_handlers(song_window, (WindowHandlers) {
			.load = song_window_load,
			.unload = song_window_unload,
		});
		currentAlbum = name;
		//memcpy(&currentAlbum, name, sizeof(*name));
		window_stack_push(song_window, true);
		send_message("SONGS", currentArtist, name);
	}
}

static void album_menu_select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu long click!"); 
  //Tuple *item = dict_find(&album_data, cell_index->row+2);
	char *name = album_data[cell_index->row];
	if (name){
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu click item found: %s", name);
		send_message("PLAY", name, "");
	}
}

#ifdef PBL_ROUND 
static int16_t album_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) { 
  if (menu_layer_is_index_selected(menu_layer, cell_index)) {
    return MENU_CELL_ROUND_FOCUSED_TALL_CELL_HEIGHT;
  } else {
    return MENU_CELL_ROUND_UNFOCUSED_SHORT_CELL_HEIGHT;
  }
}
#endif

static void album_window_load(Window *window) {
	// Now we prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  album_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(album_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = album_menu_get_num_sections_callback,
    .get_num_rows = album_menu_get_num_rows_callback,
    //.get_header_height = PBL_IF_RECT_ELSE(menu_get_header_height_callback, NULL),
    //.draw_header = PBL_IF_RECT_ELSE(menu_draw_header_callback, NULL),
    .draw_row = album_menu_draw_row_callback,
    .select_click = album_menu_select_callback,
		.select_long_click = album_menu_select_long_callback,
    .get_cell_height = PBL_IF_ROUND_ELSE(album_get_cell_height_callback, NULL),
  });
	// Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(album_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(album_menu_layer));
}
	
static void album_window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(album_menu_layer);
}

// ARTISTS MENU
static uint16_t artist_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t artist_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	return NUM_ARTISTS;
}

static void artist_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	//Tuple *item = dict_find(&list_data, cell_index->row+2);
	char *item = artist_data[cell_index->row];
	if (item){
		// There is title draw for something more simple than a basic menu item
		#ifdef PBL_RECT
		menu_cell_title_draw(ctx, cell_layer, item);
		#else
		GSize size = layer_get_frame(cell_layer).size;
		graphics_draw_text(ctx, item, fonts_get_system_font(FONT_KEY_GOTHIC_28),
											 GRect(0, 0, size.w, size.h), GTextOverflowModeTrailingEllipsis, 
											 GTextAlignmentCenter, NULL);
		#endif
	}
}

static void artist_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu click!"); 
  //Tuple *item = dict_find(&list_data, cell_index->row+2);
	char *item = artist_data[cell_index->row];
	if (item){
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu click item found: %s", item);
		album_window = window_create();
		window_set_window_handlers(album_window, (WindowHandlers) {
			.load = album_window_load,
			.unload = album_window_unload,
		});
		currentArtist = item;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Current Artist: %s", currentArtist);
		//memcpy(&currentArtist, item, sizeof(*item));
		window_stack_push(album_window, true);
		send_message("ALBUMS", item, "");
	}
}

static void artist_menu_select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu long click!"); 
  //Tuple *item = dict_find(&list_data, cell_index->row+2);
	char *item = artist_data[cell_index->row];
	if (item){
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu click item found: %s", item);
		send_message("PLAY", item, "");
	}
}

#ifdef PBL_ROUND 
static int16_t artist_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) { 
  if (menu_layer_is_index_selected(menu_layer, cell_index)) {
    return MENU_CELL_ROUND_FOCUSED_TALL_CELL_HEIGHT;
  } else {
    return MENU_CELL_ROUND_UNFOCUSED_SHORT_CELL_HEIGHT;
  }
}
#endif

static void main_window_load(Window *window) {
	// Now we prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  artist_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(artist_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = artist_menu_get_num_sections_callback,
    .get_num_rows = artist_menu_get_num_rows_callback,
    //.get_header_height = PBL_IF_RECT_ELSE(menu_get_header_height_callback, NULL),
    //.draw_header = PBL_IF_RECT_ELSE(menu_draw_header_callback, NULL),
    .draw_row = artist_menu_draw_row_callback,
    .select_click = artist_menu_select_callback,
		.select_long_click = artist_menu_select_long_callback,
    .get_cell_height = PBL_IF_ROUND_ELSE(artist_get_cell_height_callback, NULL),
  });
	// Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(artist_menu_layer, window);

  layer_add_child(window_layer, menu_layer_get_layer(artist_menu_layer));
}
	
static void main_window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(artist_menu_layer);
}

void init(void) {
	//dict_write_begin(&list_data, s_inboxBuffer, INBOX_BUFFER_SIZE);
	//dict_write_end(&list_data);
	
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
	window_stack_push(window, true);
	
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	send_message("ARTISTS", "", "");
}

void deinit(void) {
	app_message_deregister_callbacks();
	window_destroy(window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}