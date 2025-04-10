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

#ifndef TICK_CLOCKANDDATA_READER_H
#define TICK_CLOCKANDDATA_READER_H

#include <Arduino.h>

#ifdef USE_CLOCKANDDATA

void clockanddata_drain_clock(void);
void clockanddata_restore_clock(void);

void IRAM_ATTR clockanddata_trigger(void);

void clockanddata_transmit_id(String sendValue, unsigned long bitcount);
void clockanddata_loop(void);

#endif
#endif