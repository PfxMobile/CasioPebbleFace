#include <pebble.h>
#include <pebble-effect-layer/pebble-effect-layer.h>
#define DEBUG 0

enum ConfigKeys {
	JS_READY=0,
	C_INV=1,
	C_VIBR=2,
	C_DATEFMT=3,
	C_WEATHER=4,
	C_UNITS=5,
	C_CKEY=6,
	C_VIBR_BT=7,
	C_SHOWSEC=8,
	C_BATT_DGT=9,
	C_BATT_SHOW=10,
	C_COND_SHOW=11,
	W_TIME=90,
	W_TEMP=91,
	W_ICON=92,
	W_COND=93,
  C_CALENDAR=94,
  C_SUBJ=95,
  C_HOURS=96
};

typedef struct {
	bool inv;
	uint8_t showsec, showbatt;
	bool vibr, vibr_bt, battdgt;
	uint16_t datefmt;
	bool isdst, isAM;
	bool weather, cond;
	bool isunit;
	uint32_t cityid;
	uint32_t w_time;
	int16_t w_temp;  
	char w_icon[2], w_cond[50],c_subj[80],c_hours[10];
	bool w_UpdateRetry;
  bool c_UpdateRetry;
  bool msg_busy;  
	bool s_Charging;
  bool ready;
  bool isNight;
} __attribute__((__packed__)) CfgDta_t;

static CfgDta_t CfgData = {
	.inv = false,
	.showsec = 0,
	.showbatt = 100,
	.vibr = false,
	.vibr_bt = false,
	.battdgt = true,	
	.datefmt = 1,
	.isdst = false,
	.isAM = false,
	.weather = true,
	.cond = false,
	.isunit = false,
	.cityid = 625144,
	.w_time = 0,
	.w_temp = 0,
	.w_icon = " ",
	.w_cond = "",
	.w_UpdateRetry = false,
  .c_UpdateRetry=false,
	.s_Charging = false,  
  .c_subj = "",
  .c_hours = "",
  .ready = false,
  .msg_busy = false,
  .isNight = false
};

Window *window, *sec_window;
Layer *background_layer; 
TextLayer *ddmm_layer, *yyyy_layer, *hhmm_layer, *ss_layer, *wd_layer;
BitmapLayer *radio_layer, *connection_layer,*battery_layer;
InverterLayer *inv_layer, *sec_inv_layer;

static GBitmap *background, *radio,*connection, *batteryAll, *weekdayAll, *batteryAkt;
static GFont digitS, digitM, digitL, WeatherF, arial9, arial12;

char ddmmBuffer[] = "00-00", yyyyBuffer[] = "0000", hhmmBuffer[] = "00:00", ssBuffer[] = "00", wdBuffer[] = "XXXX";
static uint8_t aktBatt, aktBattAnim;
static AppTimer *timer_weather, *timer_batt;
int dayNum=0;
HealthValue stepsValue = 0;
static char s_status[256];
//-----------------------------------------------------------------------------------------------------------------------
static void append_status(const char * message, AppMessageResult result) {  
  
  if(result==APP_MSG_BUSY){
    CfgData.msg_busy = true;    
    layer_set_hidden(bitmap_layer_get_layer(connection_layer), false);  
  }else{
    CfgData.msg_busy = false;      
    layer_set_hidden(bitmap_layer_get_layer(connection_layer), true);  
  }
  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Status message:%s result:%d",s_status,result);  
}
//-----------------------------------------------------------------------------------------------------------------------
char *upcase(char *str) {
    for (int i = 0; str[i] != 0; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 'a' - 'A';
        }
    }
    return str;
}
//-----------------------------------------------------------------------------------------------------------------------
static void update_all()
{
	if (window_stack_get_top_window() == sec_window)
	{
		Layer *window_layer = window_get_root_layer(window);
		layer_remove_from_parent(text_layer_get_layer(ss_layer));
		if (CfgData.showsec > 0)
			layer_add_child(window_layer, text_layer_get_layer(ss_layer));	
		layer_remove_from_parent(inverter_layer_get_layer(inv_layer));
		if (CfgData.inv)
			layer_add_child(window_layer, inverter_layer_get_layer(inv_layer));
		window_stack_pop(false);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void secwnd_update_proc(Layer *layer, GContext *ctx) 
{
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Second Windows Layer Update");
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, GRect(114, 66, 25, 18), 0, GCornerNone);
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackBattery(void *data) 
{
	if (CfgData.s_Charging)
	{
		int nImage = 10 - (aktBattAnim / 10);
		
		bitmap_layer_set_bitmap(battery_layer, NULL);
		gbitmap_destroy(batteryAkt);
		batteryAkt = gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, 10*nImage, 20, 10));
		bitmap_layer_set_bitmap(battery_layer, batteryAkt);
		update_all();

		aktBattAnim += 10;
		if (aktBattAnim > 100)
			aktBattAnim = aktBatt;
		timer_batt = app_timer_register(1000, timerCallbackBattery, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
void battery_state_service_handler(BatteryChargeState charge_state) 
{
	int nImage = 0;
	aktBatt = charge_state.charge_percent;
	
	if (charge_state.is_charging)
	{
		if (!CfgData.s_Charging)
		{
			nImage = 10;
			CfgData.s_Charging = true;
			aktBattAnim = aktBatt;
			timer_batt = app_timer_register(1000, timerCallbackBattery, NULL);
		}
	}
	else
	{
		nImage = 10 - (aktBatt / 10);
		CfgData.s_Charging = false;
	}
	
	bitmap_layer_set_bitmap(battery_layer, NULL);
	gbitmap_destroy(batteryAkt);
	batteryAkt = gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, 10*nImage, 20, 10));
	bitmap_layer_set_bitmap(battery_layer, batteryAkt);
	layer_set_hidden(bitmap_layer_get_layer(battery_layer), !CfgData.s_Charging && (CfgData.battdgt || aktBatt > CfgData.showbatt));	
	update_all();
}
//-----------------------------------------------------------------------------------------------------------------------
void bluetooth_connection_handler(bool connected)
{  
	layer_set_hidden(bitmap_layer_get_layer(radio_layer), connected != true); 
	update_all();
		
	if (!connected && CfgData.vibr_bt)
		vibes_enqueue_custom_pattern( (VibePattern) {
			.durations = (uint32_t []) {100, 100, 100, 100, 400, 400, 100, 100, 100},
				.num_segments = 9
		});
}
//-----------------------------------------------------------------------------------------------------------------------
static void handle_health(HealthEventType event, void *context) {
		if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "update_health : %d", event);		
		if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
			stepsValue = health_service_sum_today(HealthMetricStepCount);
			update_all();
		}
}
//-----------------------------------------------------------------------------------------------------------------------
static void background_layer_update_callback(Layer *layer, GContext* ctx) 
{
	if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Background Layer Update");
	graphics_context_set_text_color(ctx, GColorBlack);

	//Background
	GSize bg_size = gbitmap_get_bounds(background).size;
	graphics_draw_bitmap_in_rect(ctx, background, GRect(0, 0, bg_size.w, bg_size.h));

	//DD
	GBitmap *bmpTmp = gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, 115, 12, 5));
	graphics_draw_bitmap_in_rect(ctx, bmpTmp, GRect(CfgData.datefmt != 3 ? 17 : 48, 20, 12, 5));
	gbitmap_destroy(bmpTmp);
	
	//MM
	bmpTmp = gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, 120, 12, 5));
	graphics_draw_bitmap_in_rect(ctx, bmpTmp, GRect(CfgData.datefmt != 3 ? 48 : 17, 20, 12, 5));
	gbitmap_destroy(bmpTmp);
	
  //STEP
	bmpTmp = gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, 125, 21, 5));
	graphics_draw_bitmap_in_rect(ctx, bmpTmp, GRect(100, 20, 21, 5));
	gbitmap_destroy(bmpTmp);
	//DST
	if (CfgData.isdst)
	{
		bmpTmp = gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, 110, 12, 5));
		graphics_draw_bitmap_in_rect(ctx, bmpTmp, GRect(116, 103, 12, 5));
		gbitmap_destroy(bmpTmp);
	}
	
	//Weather
	GRect rc = GRect(110, 82, 34, 34);
	if (CfgData.weather)
	{
    if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Background Layer Update: Weather");
		if (CfgData.w_time != 0)
		{
      if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Background Layer Update: Weather CfgData.w_time != 0");
			char sTemp[] = "-999";
			snprintf(sTemp, sizeof(sTemp), "%d", (int16_t)((double)CfgData.w_temp * (CfgData.isunit ? 1.8 : 1) + (CfgData.isunit ? 32 : 0))); //Â°C or Â°F?
			graphics_draw_text(ctx, sTemp, digitS, GRect(83, 60 - (CfgData.cond ? 2 : 0), 50, 32), GTextOverflowModeFill, GTextAlignmentRight, NULL);
			graphics_draw_text(ctx, !CfgData.isunit ? "_" : "`", WeatherF, GRect(134, 56 - (CfgData.cond ? 2 : 0), 18, 32), GTextOverflowModeFill, GTextAlignmentCenter, NULL);

			GSize sz = graphics_text_layout_get_content_size(CfgData.w_icon, WeatherF, rc, GTextOverflowModeFill, GTextAlignmentCenter);
      if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Background Layer Update: Get icon sz h:%d w:%d:",sz.w,sz.h);
      if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Background Layer Update: Get icon coordinates x:%d y:%d:",rc.origin.x+rc.size.w/2-sz.w/2,rc.origin.y+rc.size.h/2-sz.h/2);
      int defaultIconSize = 28;
			// graphics_draw_text(ctx, CfgData.w_icon, WeatherF, GRect(rc.origin.x+rc.size.w/2-sz.w/2, rc.origin.y+rc.size.h/2-sz.h/2, sz.w, sz.h), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      graphics_draw_text(ctx, CfgData.w_icon, WeatherF, GRect(rc.origin.x+rc.size.w/2-defaultIconSize/2, rc.origin.y+rc.size.h/2-defaultIconSize/2, defaultIconSize, defaultIconSize), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
			
			if (CfgData.cond)
			{
				//sz = graphics_text_layout_get_content_size(CfgData.w_cond, arial9, GRect(5, 65, 108, 10), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
				graphics_draw_text(ctx, CfgData.w_cond, arial9, GRect(5, 66, 108, 10), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
			}
		}
		else
		{
      if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Background Layer Update: Weather ELSE");
			GSize sz = graphics_text_layout_get_content_size("h", WeatherF, rc, GTextOverflowModeFill, GTextAlignmentCenter);
			graphics_draw_text(ctx, "h", WeatherF, GRect(rc.origin.x+rc.size.w/2-sz.w/2, rc.origin.y+rc.size.h/2-sz.h/2, sz.w, sz.h), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
		}
	}
	
	//AM/PM
	if(!clock_is_24h_style())
	{
		bmpTmp = gbitmap_create_as_sub_bitmap(batteryAll, GRect(CfgData.isAM ? 0 : 4, 125, 4, 5));
		graphics_draw_bitmap_in_rect(ctx, bmpTmp, GRect(6, 65, 4, 5));
		gbitmap_destroy(bmpTmp);
	}
	
	//Battery as percent
	if(!CfgData.s_Charging && CfgData.battdgt && aktBatt <= CfgData.showbatt)
	{
		char sTemp[] = "100%";
		snprintf(sTemp, sizeof(sTemp), "%d%%", aktBatt);
		graphics_draw_text(ctx, sTemp, arial9, GRect(115, 155, 25, 10), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
	}
  // Steps
  char stepsTemp[] = "0000";	
  snprintf(stepsTemp, sizeof(stepsTemp), "%d", (int) stepsValue);  
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "Steps: %s", stepsTemp);
  //text_layer_set_text(yyyy_layer, stepsTemp);
  graphics_draw_text(ctx, stepsTemp, digitS, GRect(78, 22, 60, 32), GTextOverflowModeFill, GTextAlignmentRight, NULL);				
  
  
  //Calendar
	if(strlen(CfgData.c_subj)>0)
	{
		char calendTemp[] = "Simple Sample Calendar event in 24 hours.";
		snprintf(calendTemp, sizeof(calendTemp), "%s", CfgData.c_subj);
    if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "Caledar draw %s", calendTemp);
		graphics_draw_text(ctx, calendTemp, arial9, GRect(10, 56, 145, 10), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	}
  
  // WEEK_DAY IMAGE  
  bmpTmp = gbitmap_create_as_sub_bitmap(weekdayAll, GRect(0, dayNum*19 + dayNum, 33, 19));
	graphics_draw_bitmap_in_rect(ctx, bmpTmp, GRect(10, 142, 33, 19));
	gbitmap_destroy(bmpTmp);  
}
//-----------------------------------------------------------------------------------------------------------------------
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	uint8_t seconds = tick_time->tm_sec;

	if (CfgData.showsec > 0 && seconds % CfgData.showsec == 0)
	{
		strftime(ssBuffer, sizeof(ssBuffer), "%S", tick_time);
		text_layer_set_text(ss_layer, ssBuffer);
	}

	if(seconds == 0 || units_changed == MINUTE_UNIT)
	{
		if(clock_is_24h_style())
			strftime(hhmmBuffer, sizeof(hhmmBuffer), "%H:%M", tick_time);
		else
		{
			CfgData.isAM = tick_time->tm_hour < 12;
			strftime(hhmmBuffer, sizeof(hhmmBuffer), "%I:%M", tick_time);
		}
		
		//strcpy(hhmmBuffer, "88:88");
		text_layer_set_text(hhmm_layer, hhmmBuffer);
		
		strftime(ddmmBuffer, sizeof(ddmmBuffer), 
			CfgData.datefmt == 1 ? "%d-%m" : 
			CfgData.datefmt == 2 ? "%d/%m" : 
			CfgData.datefmt == 3 ? "%m/%d" : "%d.%m", tick_time);
		//snprintf(ddmmBuffer, sizeof(ddmmBuffer), "%d", rc.origin.x);
		text_layer_set_text(ddmm_layer, ddmmBuffer);
		
		//strftime(wdBuffer, sizeof(wdBuffer), "%a", tick_time);
		//strcpy(wdBuffer, "sáb");
		//upcase(wdBuffer);
		//text_layer_set_text(wd_layer, wdBuffer);    
    // WEEK_DAY IMAGE
    strftime(wdBuffer, sizeof(wdBuffer), "%u", tick_time);
    dayNum = atoi(wdBuffer)-1;
    if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "WeekDay: %s", wdBuffer);
    
		//strftime(yyyyBuffer, sizeof(yyyyBuffer), "%Y", tick_time);
		//text_layer_set_text(yyyy_layer, yyyyBuffer);

		//Check DST at 4h at morning
		if ((tick_time->tm_hour == 4 && tick_time->tm_min == 0) || units_changed == MINUTE_UNIT)
		{
			CfgData.isdst = (tick_time->tm_isdst > 0);
			update_all();
		}
		//Check Night 
    CfgData.isNight = (tick_time->tm_hour >= 1 && tick_time->tm_hour <= 5);			    
    if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Night tm_hour %d",tick_time->tm_hour); 
		//Hourly vibrate
		if (CfgData.vibr && tick_time->tm_min == 0)
			vibes_double_pulse();
		
		//Update Background Layer
		update_all();
	}
	else if (CfgData.showsec > 0 && !window_stack_contains_window(sec_window))
	{
		Layer *sec_window_layer = window_get_root_layer(sec_window);
		layer_remove_from_parent(text_layer_get_layer(ss_layer));
		layer_add_child(sec_window_layer, text_layer_get_layer(ss_layer));	
		layer_remove_from_parent(inverter_layer_get_layer(sec_inv_layer));
		if (CfgData.inv)
			layer_add_child(sec_window_layer, inverter_layer_get_layer(sec_inv_layer));
		window_stack_push(sec_window, false);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static bool update_weather() 
{
	strcpy(CfgData.w_icon, "h");
	strcpy(CfgData.w_cond, "Updating...");
  if(CfgData.isNight){
    if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Night mode ON - no updates");   
    return false;
  }  
	update_all();	
	DictionaryIterator *iter;
	AppMessageResult begin_result = app_message_outbox_begin(&iter);
  if (begin_result == APP_MSG_OK) {
    append_status("update_weather success", APP_MSG_OK);
  } else {
    append_status("update_weather failed", begin_result);
    return false;
  }
	
	Tuplet val_ckey = TupletInteger(C_CKEY, CfgData.cityid);
	dict_write_tuplet(iter, &val_ckey);
	dict_write_end(iter);

	app_message_outbox_send();
	if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Send message with data: c_ckey=%d", (int)CfgData.cityid);
	return true;
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackWeather(void *data) 
{
	if (CfgData.w_UpdateRetry && !layer_get_hidden(bitmap_layer_get_layer(radio_layer)))
	{
		update_weather();    
		timer_weather = app_timer_register(30000, timerCallbackWeather, NULL); //Try again in 30 sec
	}
	else
	{
		CfgData.w_UpdateRetry = true;
		timer_weather = app_timer_register(60000*60, timerCallbackWeather, NULL); //1h static update
	}
}

//-----------------------------------------------------------------------------------------------------------------------
static void update_configuration(void)
{
    if (persist_exists(C_INV))
		CfgData.inv = persist_read_bool(C_INV);
	
    if (persist_exists(C_SHOWSEC))
		CfgData.showsec = persist_read_int(C_SHOWSEC);
	
    if (persist_exists(C_BATT_DGT))
		CfgData.battdgt = persist_read_bool(C_BATT_DGT);
	
    if (persist_exists(C_BATT_SHOW))
		CfgData.showbatt = persist_read_int(C_BATT_SHOW);
	
   if (persist_exists(C_VIBR))
		CfgData.vibr = persist_read_bool(C_VIBR);
	
    if (persist_exists(C_VIBR_BT))
		CfgData.vibr_bt = persist_read_bool(C_VIBR_BT);
	
    if (persist_exists(C_DATEFMT))
		CfgData.datefmt = (int16_t)persist_read_int(C_DATEFMT);
	
    if (persist_exists(C_WEATHER))
		CfgData.weather = persist_read_bool(C_WEATHER);
	
    if (persist_exists(C_COND_SHOW))
		CfgData.cond = persist_read_bool(C_COND_SHOW);
	
    if (persist_exists(C_UNITS))
		CfgData.isunit = persist_read_bool(C_UNITS);
	
    if (persist_exists(C_CKEY))
		CfgData.cityid = persist_read_int(C_CKEY);
	
    if (persist_exists(W_TIME))
		CfgData.w_time = persist_read_int(W_TIME);
	
    if (persist_exists(W_TEMP))
		CfgData.w_temp = persist_read_int(W_TEMP);

    if (persist_exists(W_ICON))
		persist_read_string(W_ICON, CfgData.w_icon, sizeof(CfgData.w_icon));

	if (persist_exists(W_COND))
		persist_read_string(W_COND, CfgData.w_cond, sizeof(CfgData.w_cond));
  
  if (persist_exists(C_SUBJ))
    	persist_read_string(C_SUBJ, CfgData.c_subj, sizeof(CfgData.c_subj));
  
  if (persist_exists(C_HOURS))
		persist_read_string(C_HOURS, CfgData.c_hours, sizeof(CfgData.c_hours));

	if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Curr Conf #1: inv:%d, showsec:%d, battdgt:%d, showbatt:%d, vibr:%d, datefmt:%d, weather:%d", CfgData.inv, CfgData.showsec, CfgData.battdgt, CfgData.showbatt, CfgData.vibr, CfgData.datefmt, CfgData.weather);
	if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Curr Conf #2: isunit:%d, cityid:%d", CfgData.isunit, (int)CfgData.cityid);
	if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Weather Data: w_time:%d, w_temp:%d, w_icon:%s, w_cond:%s", (int)CfgData.w_time, CfgData.w_temp, CfgData.w_icon, CfgData.w_cond);
  if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Calendar Data: calendar :c_subj%s, c_hours:%s", CfgData.c_subj, CfgData.c_hours);
	
	Layer *window_layer = window_get_root_layer(window);

	//Inverter Layer
	layer_remove_from_parent(inverter_layer_get_layer(inv_layer));
	if (CfgData.inv)
		layer_add_child(window_layer, inverter_layer_get_layer(inv_layer));
	
	//Second Layer
	layer_remove_from_parent(text_layer_get_layer(ss_layer));
	if (CfgData.showsec > 0)
		layer_add_child(window_layer, text_layer_get_layer(ss_layer));	
	
	//Move Hour/Minute layer on condition
	GRect rc = layer_get_frame(text_layer_get_layer(hhmm_layer));
	rc.origin.y = CfgData.cond ? 61 : 53;
	layer_set_frame(text_layer_get_layer(hhmm_layer), rc);
	
	//Get a time structure so that it doesn't start blank
	time_t tmAkt = time(NULL);
	struct tm *t = localtime(&tmAkt);

	//Manually call the tick handler when the window is loading
	tick_handler(t, MINUTE_UNIT);

	//Set Battery state
	BatteryChargeState btchg = battery_state_service_peek();
	battery_state_service_handler(btchg);    
	
	//Set Bluetooth state
	bool connected = bluetooth_connection_service_peek();
	bluetooth_connection_handler(connected);
	
	//Update Weather
	if (CfgData.w_UpdateRetry && CfgData.weather)
	{
		if (CfgData.w_time == 0 || (tmAkt - CfgData.w_time) > 60*60)
			timer_weather = app_timer_register(100, timerCallbackWeather, NULL);
		else
			timer_weather = app_timer_register((60*60-(tmAkt-CfgData.w_time))*1000, timerCallbackWeather, NULL);
	}    
}
//-----------------------------------------------------------------------------------------------------------------------
void in_received_handler(DictionaryIterator *received, void *ctx)
{
	if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received Data: ");
    time_t tmAkt = time(NULL);
	
	Tuple *akt_tuple = dict_read_first(received);
    while (akt_tuple)
    {
		int intVal = atoi(akt_tuple->value->cstring);
        if (DEBUG) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "KEY %d=%d/%s/%d", (int16_t)akt_tuple->key, akt_tuple->value->int16 ,akt_tuple->value->cstring, intVal);

		switch (akt_tuple->key)	{
		case JS_READY:
			CfgData.w_UpdateRetry = true;      
      CfgData.ready = true;            
			break;
		case C_INV:
			persist_write_bool(C_INV, strcmp(akt_tuple->value->cstring, "yes") == 0);
			break;
		case C_SHOWSEC:
			persist_write_int(C_SHOWSEC, 
				strcmp(akt_tuple->value->cstring, "nev") == 0 ? 0 : 
				strcmp(akt_tuple->value->cstring, "05s") == 0 ? 5 : 
				strcmp(akt_tuple->value->cstring, "10s") == 0 ? 10 : 
				strcmp(akt_tuple->value->cstring, "15s") == 0 ? 15 : 
				strcmp(akt_tuple->value->cstring, "30s") == 0 ? 30 : 1);
			break;
		case C_BATT_DGT:
			persist_write_bool(C_BATT_DGT, strcmp(akt_tuple->value->cstring, "yes") == 0);
			break;
		case C_BATT_SHOW:
			persist_write_int(C_BATT_SHOW, intVal);
			break;
		case C_VIBR:
			persist_write_bool(C_VIBR, strcmp(akt_tuple->value->cstring, "yes") == 0);
			break;
		case C_VIBR_BT:
			persist_write_bool(C_VIBR_BT, strcmp(akt_tuple->value->cstring, "yes") == 0);
			break;
		case C_DATEFMT:
			persist_write_int(C_DATEFMT, 
				strcmp(akt_tuple->value->cstring, "fra") == 0 ? 1 : 
				strcmp(akt_tuple->value->cstring, "eng") == 0 ? 2 : 
				strcmp(akt_tuple->value->cstring, "usa") == 0 ? 3 : 0);
			break;
		case C_WEATHER:
			persist_write_bool(C_WEATHER, strcmp(akt_tuple->value->cstring, "yes") == 0);
			break;
		case C_UNITS:
			persist_write_bool(C_UNITS, strcmp(akt_tuple->value->cstring, "f") == 0);
			break;
		case C_COND_SHOW:
			persist_write_bool(C_COND_SHOW, strcmp(akt_tuple->value->cstring, "yes") == 0);
			break;
		case C_CKEY:
			persist_write_int(C_CKEY, intVal);
			if ((int)CfgData.cityid != intVal) //City Changed, force reload weather
			{
				CfgData.w_UpdateRetry = true;
				persist_write_int(W_TIME, 0);
			}
			break;
		case W_TEMP:
			persist_write_int(W_TIME, tmAkt);
			persist_write_int(W_TEMP, akt_tuple->value->int16);
			CfgData.w_UpdateRetry = false; //Update successful, usual update wait time
			break;
		case W_ICON:
			persist_write_string(W_ICON, akt_tuple->value->cstring);
			break;
		case W_COND:
			persist_write_string(W_COND, akt_tuple->value->cstring);
			break;
    case C_SUBJ:      
      CfgData.c_UpdateRetry = false; //Update successful, usual update wait time
			persist_write_string(C_SUBJ, akt_tuple->value->cstring);
      break;
    case C_HOURS:      
      persist_write_string(C_HOURS, akt_tuple->value->cstring);			
			break;
		}
		
		akt_tuple = dict_read_next(received);
	}
	
    update_configuration();
}
//-----------------------------------------------------------------------------------------------------------------------
void in_dropped_handler(AppMessageResult reason, void *ctx)
{
    if (DEBUG) app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Message dropped, reason code %d",
            reason);
}
//-----------------------------------------------------------------------------------------------------------------------
void window_load(Window *window)
{
	background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	batteryAll = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERIES);
  weekdayAll = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WEEKDAY_32_139);
	radio = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RADIO);
  connection = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECTION);
	
	digitS = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_25));
	digitM = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_CASIO_DOTTED_22));
	digitL = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_CASIO_66));
 	WeatherF = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_32));
	arial9 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ARIAL_BOLD_9));
  arial12 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ARIAL_BOLD_11));
 
	Layer *window_layer = window_get_root_layer(window);

	//Init Background
	background_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(background_layer, background_layer_update_callback);
	layer_add_child(window_layer, background_layer);

	//DAY+MONTH layer
	ddmm_layer = text_layer_create(GRect(2, 22, 70, 32));
	text_layer_set_background_color(ddmm_layer, GColorClear);
	text_layer_set_text_color(ddmm_layer, GColorBlack);
	text_layer_set_text_alignment(ddmm_layer, GTextAlignmentCenter);
	text_layer_set_font(ddmm_layer, digitS);
	layer_add_child(window_layer, text_layer_get_layer(ddmm_layer));
        
	//YEAR layer
	//yyyy_layer = text_layer_create(GRect(82, 5, 60, 32));
	//text_layer_set_background_color(yyyy_layer, GColorClear);
	//text_layer_set_text_color(yyyy_layer, GColorBlack);
	//text_layer_set_text_alignment(yyyy_layer, GTextAlignmentCenter);
	//text_layer_set_font(yyyy_layer, digitS);
	//layer_add_child(window_layer, text_layer_get_layer(yyyy_layer));
        
	//HOUR+MINUTE layer
	hhmm_layer = text_layer_create(GRect(8, 53, 110, 75));
	text_layer_set_background_color(hhmm_layer, GColorClear);
	text_layer_set_text_color(hhmm_layer, GColorBlack);
	text_layer_set_text_alignment(hhmm_layer, GTextAlignmentLeft);
	text_layer_set_overflow_mode(hhmm_layer, GTextOverflowModeFill);
	text_layer_set_font(hhmm_layer, digitL);
	layer_add_child(window_layer, text_layer_get_layer(hhmm_layer));
        
	//SECOND layer
	ss_layer = text_layer_create(GRect(111, 58, 30, 30));
	text_layer_set_background_color(ss_layer, GColorClear);
	text_layer_set_text_color(ss_layer, GColorBlack);
	text_layer_set_text_alignment(ss_layer, GTextAlignmentCenter);
	text_layer_set_font(ss_layer, digitS);
        
	//Init battery
	battery_layer = bitmap_layer_create(GRect(116, 155, 20, 10)); 
	bitmap_layer_set_background_color(battery_layer, GColorClear);
	layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));	
	
	//WEEKDAY layer
	//wd_layer = text_layer_create(GRect(0, 138, 84, 40));
	//text_layer_set_background_color(wd_layer, GColorClear);
	//text_layer_set_text_color(wd_layer, GColorBlack);
	//text_layer_set_text_alignment(wd_layer, GTextAlignmentCenter);
	//text_layer_set_font(wd_layer, digitM);
	//layer_add_child(window_layer, text_layer_get_layer(wd_layer));
        
	//Init bluetooth radio
	radio_layer = bitmap_layer_create(GRect(113, 125, 31, 33));
	bitmap_layer_set_background_color(radio_layer, GColorClear);
	bitmap_layer_set_bitmap(radio_layer, radio);
	bitmap_layer_set_compositing_mode(radio_layer, GCompOpSet);
	layer_add_child(window_layer, bitmap_layer_get_layer(radio_layer));
  
  //Init connection 
	connection_layer = bitmap_layer_create(GRect(65, 135, 31, 31));
	bitmap_layer_set_background_color(connection_layer, GColorClear);
	bitmap_layer_set_bitmap(connection_layer, connection);
	bitmap_layer_set_compositing_mode(connection_layer, GCompOpSet);
	layer_add_child(window_layer, bitmap_layer_get_layer(connection_layer));
	layer_set_hidden(bitmap_layer_get_layer(connection_layer), true);  
  
	//Init inverter_layer
	inv_layer = inverter_layer_create(GRect(0, 0, 144, 168));
	sec_inv_layer = inverter_layer_create(GRect(114, 66, 25, 18));

	//Update Configuration
	update_configuration();
}
//-----------------------------------------------------------------------------------------------------------------------
void window_unload(Window *window)
{
	//Destroy text layers
	text_layer_destroy(ddmm_layer);
	text_layer_destroy(yyyy_layer);
	text_layer_destroy(hhmm_layer);
	text_layer_destroy(ss_layer);
	text_layer_destroy(wd_layer);
	
	//Unload Fonts
	fonts_unload_custom_font(digitS);
	fonts_unload_custom_font(digitM);
	fonts_unload_custom_font(digitL);
	fonts_unload_custom_font(WeatherF);
	fonts_unload_custom_font(arial9);
	
	//Destroy GBitmaps
	gbitmap_destroy(batteryAkt);
	gbitmap_destroy(batteryAll);
	gbitmap_destroy(radio);
	gbitmap_destroy(background);

	//Destroy BitmapLayers
	bitmap_layer_destroy(battery_layer);
	bitmap_layer_destroy(radio_layer);
	layer_destroy(background_layer);
	
	//Destroy Inverter Layer
	inverter_layer_destroy(inv_layer);
	inverter_layer_destroy(sec_inv_layer);
}
//-----------------------------------------------------------------------------------------------------------------------
void handle_init(void) 
{
	char* sLocale = setlocale(LC_TIME, ""), sLang[3];
	if (strncmp(sLocale, "en", 2) == 0)
		strcpy(sLang, "en");
	else if (strncmp(sLocale, "de", 2) == 0)
		strcpy(sLang, "de");
	else if (strncmp(sLocale, "es", 2) == 0)
		strcpy(sLang, "es");
	else if (strncmp(sLocale, "fr", 2) == 0)
		strcpy(sLang, "fr");
  else if (strncmp(sLocale, "ru", 2) == 0)
		strcpy(sLang, "ru");
	if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "Time locale is set to: %s/%s", sLocale, sLang);

	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
    window_stack_push(window, true);
	
	sec_window = window_create();
	window_set_background_color(sec_window, GColorClear);
	Layer *sec_window_layer = window_get_root_layer(sec_window);
	layer_set_update_proc(sec_window_layer, secwnd_update_proc);

	//Subscribe services
	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler)tick_handler);
	battery_state_service_subscribe(&battery_state_service_handler);
	bluetooth_connection_service_subscribe(&bluetooth_connection_handler);
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "Subscribe Health");
  #if defined(PBL_HEALTH)
  // Attempt to subscribe 
  if(!health_service_events_subscribe(handle_health, NULL)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  } else {
    if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "Health available!");
    // health_service_set_heart_rate_sample_period(0);
  }
  #else
  APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  #endif 


	//Subscribe messages
	app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_open(128, 128);
	
	if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
}
//-----------------------------------------------------------------------------------------------------------------------
void handle_deinit(void) 
{
	app_timer_cancel(timer_weather);
	app_timer_cancel(timer_batt);  
	app_message_deregister_callbacks();
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
  health_service_events_unsubscribe();
  
	window_destroy(sec_window);
	window_destroy(window);
}
//-----------------------------------------------------------------------------------------------------------------------
int main(void) 
{
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}
//-----------------------------------------------------------------------------------------------------------------------