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

#ifndef TICK_SYSLOG_H
#define TICK_SYSLOG_H

#ifdef USE_SYSLOG

#ifndef USE_WIFI
#error "USE_SYSLOG must be used with USE_WIFI"
#endif

#endif

#include <Arduino.h>

void syslog_init(void);
void syslog(String text);


#endif