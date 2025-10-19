
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

constexpr int HOLD_VSYS_EN = 2;
void inky_sleep(InkyFrame &frame, int wake_in_minutes)
{
    frame.rtc.clear_timer_flag();
    if (wake_in_minutes != -1)
    {
        frame.rtc.set_timer(wake_in_minutes, PCF85063A::TIMER_TICK_1_OVER_60HZ);
        frame.rtc.enable_timer_interrupt(true, false);
    }

    gpio_put(HOLD_VSYS_EN, false);
    printf("Sleeping for %d minutes\n", wake_in_minutes);
    stdio_flush();
    sleep_ms(std::max(10'000, wake_in_minutes * 60 * 1000));
    stdio_flush();
    printf("Waking up\n");
    watchdog_reboot(0, 0, 0);
    while (true)
    {
    }
}

int main()
{

    stdio_init_all();
    sleep_ms(500);

    InkyFrame inky_frame;
    inky_frame.init();

    InkyFrame::WakeUpEvent event = inky_frame.get_wake_up_event();
    printf("Wakup event: %d\n", event);

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

    inky_frame.update(true);

    wifi_setup::network_deinit(inky_frame);
    const int SLEEP_AFTER_UPDATE_MINUTES = 1;
    printf("Done, sleeping for %d\n", SLEEP_AFTER_UPDATE_MINUTES);
    inky_sleep(inky_frame, SLEEP_AFTER_UPDATE_MINUTES);
    printf("here\n");

    return 0;
}