// Copyright (C) 2024, 2025 Jakub "lenwe" Kramarz
// This file is part of The Tick.
// 
// The Tick is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// The Tick is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License

#include <Arduino.h>
#include "tick_default_config.h"

#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "tick_wifi.h"
#include "tick_utils.h"
#include "tick_lcd.h"
#include "tick_syslog.h"
#include "tick_ota.h"
#include "tick_osdp.h"
#include "tick_ble.h"
#include "tick_http.h"
#include "tick_mdns_responder.h"
#include "tick_heartbeat.h"
#include "tick_reset.h"

// byte incoming_byte = 0;
unsigned long config_reset_millis = 30000;
byte reset_pin_state = 1;

unsigned long last_aux_change = 0;
byte last_aux = 1;
byte expect_aux = 2;

#include "tick_wiegand_reader.h"
#include "tick_clockanddata_reader.h"

void detachInterrupts(void) {
  noInterrupts();
  switch(current_tick_mode){
    #ifdef USE_WIEGAND
    case tick_mode_wiegand:
      detachInterrupt(digitalPinToInterrupt(wiegand_pin_d1));
      detachInterrupt(digitalPinToInterrupt(wiegand_pin_d0));
      break;
    #endif
    #ifdef USE_CLOCKANDDATA
    case tick_mode_clockanddata:
      detachInterrupt(digitalPinToInterrupt(clockanddata_pin_clock));
      break;
    #endif
  }
  interrupts();
}

void attachInterrupts(void) {
  noInterrupts();
  switch(current_tick_mode){
    #ifdef USE_WIEGAND
    case tick_mode_wiegand:
      attachInterrupt(digitalPinToInterrupt(wiegand_pin_d0), reader1_wiegand_trigger, FALLING);
      attachInterrupt(digitalPinToInterrupt(wiegand_pin_d1), reader1_wiegand_trigger, FALLING);
      break;
    #endif
    #ifdef USE_CLOCKANDDATA
    case tick_mode_clockanddata:
      attachInterrupt(digitalPinToInterrupt(clockanddata_pin_clock), clockanddata_trigger, FALLING);
      break;
    #endif
  }
  interrupts();
}

void IRAM_ATTR auxChange(void) {
  volatile byte new_value = digitalRead(pin_aux);
  if (new_value == expect_aux) {
    last_aux = new_value;
    expect_aux = 2;
    return;
  }
  if (new_value != last_aux) {
    if (millis() - last_aux_change > 10) {
      last_aux_change = millis();
      last_aux = new_value;
      append_log("aux", "changed to "+String(new_value));
    }
  }
}

void output_debug_string(String s){
  DBG_OUTPUT_PORT.println(s);
  display_temporary_message(s, 5000);
}


void jamming_enable(void){
  switch(current_tick_mode){
    #ifdef USE_WIEGAND
    case tick_mode_wiegand:
      wiegand_drainD0();
      break;
    #endif
    #ifdef USE_CLOCKANDDATA
    case tick_mode_clockanddata:
      clockanddata_drain_clock();
      break;
    #endif
    #ifdef USE_OSDP
    case tick_mode_osdp_cp:
    case tick_mode_osdp_pd:
      osdp_drain();
      break;
    #endif
  }
}

void jamming_disable(void){
  switch(current_tick_mode){
    #ifdef USE_WIEGAND
    case tick_mode_wiegand:
      wiegand_restoreD0();
      break;
    #endif
    #ifdef USE_CLOCKANDDATA
    case tick_mode_clockanddata:
      clockanddata_restore_clock();
      break;
    #endif
    #ifdef USE_OSDP
    case tick_mode_osdp_cp:
    case tick_mode_osdp_pd:
      osdp_restore();
      break;
    #endif
  }
}

void append_log(String facility, String text) {
  File file = SPIFFS.open(LOG_FILE, "a");
  if (file) {
    String log_line = String(getBootCount()) + "; " + String(millis()) + "; " + facility + "; " + text;
    file.println(log_line);
    DBG_OUTPUT_PORT.println("Appending to log: " + String(millis()) + " " + text);
    file.close();
  } else {
    DBG_OUTPUT_PORT.println("Failed opening log file.");
  }
}

void showAddress()
{
  display_line(2, false, String("IP: ") + WiFi.localIP().toString());
}

void IRAM_ATTR resetConfig(void) {
  static unsigned long last_event = 0;
  unsigned long now = millis();

  if (now < 15000 || now > 60000) {
    return;
  }

  if (now - last_event > 50 && now - last_event < 500) {
    reset_button_counter++;
  } else {
    reset_button_counter=0;
  }
  last_event = millis();
}
void setup() {
  heartbeat_init();



  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");



  display_init();

  output_debug_string("Chip ID: 0x" + String(getChipID(), HEX));

  dhcp_hostname = String(HOSTNAME);
  dhcp_hostname += String(getChipID(), HEX);
  output_debug_string("Hostname: " + dhcp_hostname);
  display_line(0, false, dhcp_hostname);

  delay(1000);

  if (!SPIFFS.begin()) {
    output_debug_string(F("Failed to mount SPIFFS"));
    delay(1000);
    return;
  } else {
    DBG_OUTPUT_PORT.println(F("SPIFFS mount suceeded"));
  }

  // If a log.txt exists, use ap_ssid=TheTick-<chipid> instead of the default TheTick-config
  // A config file will take precedence over this
  if (SPIFFS.exists(LOG_FILE)) {
    #ifdef USE_WIFI
    dhcp_hostname.toCharArray(ap_ssid, sizeof(ap_ssid));
    #endif
  }
  append_log("boot", "Starting up!");

  if(!loadConfig("/config.default")){
    output_debug_string(F("No default configuration."));
    delay(1000);
    return;
  }
  if(!loadConfig("/config.txt")){
    output_debug_string(F("No configuration. Using defaults."));
  }

  // Inputs
  pinMode(wiegand_pin_d0, INPUT);
  digitalWrite(wiegand_pin_d0, HIGH);
  pinMode(wiegand_pin_d1, INPUT);
  digitalWrite(wiegand_pin_d1, HIGH);
  pinMode(pin_aux, INPUT);
  digitalWrite(pin_aux, LOW);
  pinMode(pin_reset, INPUT);
  digitalWrite(pin_reset, HIGH);
  osdp_disable_transceiver();

  switch(current_tick_mode){
    #ifdef USE_WIEGAND
    case tick_mode_wiegand:
      pinMode(wiegand_pin_d0, INPUT);
      digitalWrite(wiegand_pin_d0, HIGH);
      pinMode(wiegand_pin_d1, INPUT);
      digitalWrite(wiegand_pin_d1, HIGH);
      break;
    #endif
    #ifdef USE_CLOCKANDDATA
    case tick_mode_clockanddata:
      pinMode(clockanddata_pin_clock, INPUT);
      digitalWrite(clockanddata_pin_clock, HIGH);
      pinMode(clockanddata_pin_data, INPUT);
      digitalWrite(clockanddata_pin_data, HIGH);
      break;
    #endif
    #ifdef USE_OSDP
    case tick_mode_osdp_cp:
    case tick_mode_osdp_pd:
      osdp_init();
      break;
    #endif
  }

  display_line(1, false, String("mode: ") + modeToString(current_tick_mode, true));

  wifi_init();
  syslog_init();
  ota_init();
  ble_init();
  http_init();
  mdns_responder_init();

  // Input interrupts
  attachInterrupts();
  attachInterrupt(digitalPinToInterrupt(pin_reset), resetConfig, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(pin_aux), auxChange, CHANGE);

  showAddress();
}

void card_read_handler(String s){
  // for some reason, this function is very fragile.
  // if everything blows up, it is because I made some changes here
  String card_id = s;

  output_debug_string(card_id);

  ble_card_read(card_id.c_str());

  if(strcasecmp(card_id.c_str(), DoS_id) == 0) {
    jamming_enable();
    append_log("dos", String("enabled by control card ") + card_id);
  } else {
    append_log(modeToString(current_tick_mode), card_id);
  }
}

void transmit_id(String sendValue, unsigned long bitcount){
  switch(current_tick_mode){
    #ifdef USE_WIEGAND
    case tick_mode_wiegand:
      wiegand_transmit_id(sendValue, bitcount);
      break;
    #endif
    #ifdef USE_CLOCKANDDATA
    case tick_mode_clockanddata:
      clockanddata_transmit_id(sendValue, bitcount);
      break;
    #endif
    #ifdef USE_OSDP
    case tick_mode_osdp_pd:
      osdp_transmit_id(sendValue, bitcount);
      break;
    #endif
  }
  append_log("tx", sendValue);
}

void loop()
{
  display_loop();
  heartbeat_loop();
  reset_loop();

  switch(current_tick_mode){
    #ifdef USE_WIEGAND
    case tick_mode_wiegand:
      wiegand_loop();
      break;
    #endif
    #ifdef USE_CLOCKANDDATA
    case tick_mode_clockanddata:
      clockanddata_loop();
      break;
    #endif
    #ifdef USE_OSDP
    case tick_mode_osdp_cp:
    case tick_mode_osdp_pd:
      osdp_loop();
      break;
    #endif
  }

  http_loop();
  ble_loop();
  ota_loop();
}