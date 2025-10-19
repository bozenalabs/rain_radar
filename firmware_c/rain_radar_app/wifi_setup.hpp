#pragma once

#include "pimoroni_common.hpp"
#include "inky_frame_7.hpp"
#include "rain_radar_common.hpp"

namespace wifi_setup
{

    ResultOr<int8_t> wifi_connect(pimoroni::InkyFrame &inky_frame, int8_t preferred_ssid_index);
    void network_deinit(pimoroni::InkyFrame &inky_frame);
    bool is_connected();

}