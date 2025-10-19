#pragma once

#include "rain_radar_common.hpp"
#include <string>
#include "inky_frame_7.hpp"

namespace data_fetching
{

    struct ImageInfo
    {
        int64_t update_ts;
        char image_text[64];
    };

    ResultOr<ImageInfo> fetch_image_info();
    Err fetch_image(pimoroni::InkyFrame &inky_frame, int8_t connected_ssid_index);

}
