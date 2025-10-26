
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
#include "battery.hpp"


using namespace pimoroni;

InkyFrame inky_frame;
void draw_error(InkyFrame &graphics, const std::string_view &msg)
{

    graphics.set_pen(Inky73::RED);
    graphics.rectangle(Rect(graphics.width / 3, graphics.height * 2 / 3, graphics.width / 3, graphics.height / 4));
    graphics.set_pen(Inky73::WHITE);
    graphics.text(msg, Point(graphics.width / 3 + 5, graphics.height * 2 / 3 + 5), graphics.width / 3 - 5, 2);
}

void draw_lower_left_text(InkyFrame &graphics, const std::string_view &msg)
{
    // graphics.set_pen(Inky73::BLACK);
    // graphics.rectangle(Rect(0, graphics.height - 25, graphics.width, 25));
    graphics.set_pen(Inky73::WHITE);
    graphics.text(msg, Point(5, graphics.height - 17), graphics.width / 2, 2);
}

void draw_battery_status(InkyFrame &graphics, Battery &battery)
{
    const char* status = battery.get_status_string();
    graphics.set_pen(Inky73::WHITE);
    
    // Measure text width to right-align it
    int text_width = graphics.measure_text(status, 1);
    int x_position = graphics.width - text_width - 5;  // 5 pixels from right edge
    
    graphics.text(status, Point(x_position, 5), graphics.width, 1);  // Small text at top
}


int main() {
    inky_frame.init();
    inky_frame.rtc.unset_alarm();
    inky_frame.rtc.clear_alarm_flag();
    inky_frame.rtc.unset_timer();
    inky_frame.rtc.clear_timer_flag();


    stdio_init_all();
    sleep_ms(2000);

    InkyFrame::WakeUpEvent event = inky_frame.get_wake_up_event();
    printf("Wakup event: %d\n", event);

    datetime_t dt = {
        .year = 0,
        .month = 0,
        .day = 0,
        .dotw = 0,
        .hour = 0,
        .min = 0,
        .sec = 0,
    };
    inky_frame.rtc.set_datetime(&dt);




    persistent::PersistentData payload = persistent::read();

    auto on_error = [&](const std::string_view &msg, Err err)
    {
        std::string error_msg = std::string(msg) + " (" + std::string(errToString(err)) + ")";
        printf("Error: %s\n", error_msg.c_str());
        draw_error(inky_frame, error_msg);
        inky_frame.update(true);
    };

    ResultOr<int8_t> new_preferred_ssid_index = wifi_setup::wifi_connect(inky_frame, payload.wifi_preferred_ssid_index);
    if (!new_preferred_ssid_index.ok())
    {
        on_error("WiFi connect failed", new_preferred_ssid_index.result);
        return -1;
    }
    int8_t connected_ssid_index = new_preferred_ssid_index.unwrap();
    if (connected_ssid_index != payload.wifi_preferred_ssid_index)
    {
        payload.wifi_preferred_ssid_index = connected_ssid_index;
        printf("New preferred SSID index: %d\n", payload.wifi_preferred_ssid_index);
        persistent::save(&payload);
    }

    // Initialize battery monitoring
    // MUST BE INITIALIZED AFTER WIFI SETUP ON PICO W
    // for some reason it needs cyw43_arch_init() to have been called first
    Battery battery;
    battery.init();
    printf("Battery status: %s\n", battery.get_status_string());
    printf("%s", battery.is_usb_powered() ? "USB powered\n" : "Battery powered\n");

    inky_frame.set_pen(Inky73::GREEN);
    inky_frame.clear();

    // fetching the image will write to the PSRAM display directly
    Err result = data_fetching::fetch_image(inky_frame, connected_ssid_index);
    if (result != Err::OK)
    {
        on_error("Image fetch failed", result);
        return -1;
    }

    // points of interest
    for (const auto &poi : secrets::POINTS_OF_INTEREST_XY)
    {
        inky_frame.set_pen(Inky73::WHITE);
        inky_frame.circle(Point(poi[0], poi[1]), 3);
        inky_frame.set_pen(Inky73::RED);
        inky_frame.circle(Point(poi[0], poi[1]), 2);
    }

    // draw_battery_status(inky_frame, battery);

    // wifi_setup::network_deinit(inky_frame);

    inky_frame.update(true);



    inky_frame.led(InkyFrame::LED_ACTIVITY, 100);
    sleep_ms(500);
    inky_frame.led(InkyFrame::LED_ACTIVITY, 0);
    sleep_ms(500);

    inky_frame.led(InkyFrame::LED_ACTIVITY, 100);
    sleep_ms(500);
    inky_frame.led(InkyFrame::LED_ACTIVITY, 0);
    sleep_ms(500);

    inky_frame.led(InkyFrame::LED_ACTIVITY, 100);
    sleep_ms(500);
    inky_frame.led(InkyFrame::LED_ACTIVITY, 0);

    printf("done!\n");

    inky_frame.sleep_until(-1, 1, -1, -1);

    return 0;
}
