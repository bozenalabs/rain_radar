
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <string>

#include "drivers/inky73/inky73.hpp"
#include "drivers/psram_display/psram_display.hpp"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"
#include "inky_frame_7.hpp"
#include "wifi_setup.hpp"
#include "pico/stdlib.h"
#include "pimoroni_common.hpp"
#include "secrets.h"
#include "data_fetching.hpp"
#include "rain_radar_common.hpp"
#include "persistent_data.hpp"

using namespace pimoroni;

void draw_error(InkyFrame &graphics, const std::string_view &msg)
{

    graphics.set_pen(Inky73::RED);
    graphics.rectangle(Rect(graphics.width / 2, graphics.height - 25, graphics.width / 2, 25));
    graphics.set_pen(Inky73::WHITE);
    graphics.text(msg, Point(graphics.width / 2 + 5, graphics.height - 22), graphics.width / 2, 2);
}

void draw_lower_left_text(InkyFrame &graphics, const std::string_view &msg)
{
    graphics.set_pen(Inky73::BLACK);
    graphics.rectangle(Rect(0, graphics.height - 25, graphics.width, 25));
    graphics.set_pen(Inky73::WHITE);
    graphics.text(msg, Point(5, graphics.height - 22), graphics.width / 2, 2);
}


int main()
{
    stdio_init_all();
    sleep_ms(2000);

    InkyFrame inky_frame;
    inky_frame.init();

    InkyFrame::WakeUpEvent event = inky_frame.get_wake_up_event();
    printf("Wakup event: %d\n", event);

    PersistentData persistent_data = read_persistent_data();
    printf("Persistent data: mode=%d\n", persistent_data.mode);
    persistent_data.mode = (persistent_data.mode + 1) % 5;
    printf("New persistent data: mode=%d\n", persistent_data.mode);
    write_persistent_data(&persistent_data);


    auto on_error = [&](const std::string_view &msg)
    {
        printf("Error: %.*s\n", (int)msg.size(), msg.data());
        draw_error(inky_frame, msg);
        inky_frame.update(true);
    };

    if (wifi_setup::wifi_connect(inky_frame) != Err::OK)
    {
        on_error("WiFi connect failed");
        return -1;
    }

    ResultOr<data_fetching::ImageInfo> info = data_fetching::fetch_image_info();
    if (!info.ok())
    {
        on_error("Image info fetch failed");
        return -1;
    }

    Err result = data_fetching::fetch_image(inky_frame);
    if (result != Err::OK)
    {
        on_error("Image fetch failed");
        return -1;
    }

    draw_lower_left_text(inky_frame, info.unwrap().image_text);
    inky_frame.update(true);

    wifi_setup::network_deinit(inky_frame);
    constexpr int SLEEP_AFTER_UPDATE_MINUTES = 1;
    printf("Done, sleeping for %d\n", SLEEP_AFTER_UPDATE_MINUTES);
    inky_frame.sleep(SLEEP_AFTER_UPDATE_MINUTES);
    // watchdog_reboot(0, 0, 0);
    return 0;
}