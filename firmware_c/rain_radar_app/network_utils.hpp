#pragma once

#include <cmath>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include "pico_wireless.hpp"
#include "pimoroni_common.hpp"
#include "secrets.h"
#include "inky_frame_7.hpp"

using namespace pimoroni;

bool wifi_connect(InkyFrame &inky_frame);
void network_deinit();