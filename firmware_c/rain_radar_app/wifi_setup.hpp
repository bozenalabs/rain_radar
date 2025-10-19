#pragma once

#include <cmath>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include "pimoroni_common.hpp"
#include "inky_frame_7.hpp"
#include "rain_radar_common.hpp"

namespace wifi_setup
{

    Err wifi_connect(pimoroni::InkyFrame &inky_frame);
    void network_deinit();
    bool is_connected();

}