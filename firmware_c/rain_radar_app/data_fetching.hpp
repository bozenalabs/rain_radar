#pragma once

#include "rain_radar_common.hpp"
#include <string>
#include "inky_frame_7.hpp"
#include "pico/types.h"

namespace data_fetching
{

    // struct ImageInfo
    // {
    //     int64_t update_ts;
    //     datetime_t server_datetime;
    //     bool has_server_datetime;
    //     // char image_text[64];
    // };

    // ResultOr<ImageInfo> fetch_image_info(int8_t connected_ssid_index);
    ResultOr<datetime_t> fetch_image(pimoroni::InkyFrame &inky_frame, int8_t connected_ssid_index);
    
}
