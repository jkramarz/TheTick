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

#ifdef USE_MDNS_RESPONDER

#ifndef USE_WIFI
#error "USE_MDNS_RESPONDER must be used with USE_WIFI"
#endif

#include <ESPmDNS.h>
#include <Arduino.h>
#include "tick_utils.h"
#include "tick_mdns_responder.h"

void mdns_responder_init(void) {
  if (MDNS.begin(mDNShost)) {
    output_debug_string("Open http://" + String(mDNShost) + ".local/");
  } else {
    output_debug_string("Error setting up MDNS responder!");
  }

  MDNS.addService("http", "tcp", 80);
  output_debug_string("Configured MDNS service");
}

#else

void mdns_responder_init(void) {}

#endif