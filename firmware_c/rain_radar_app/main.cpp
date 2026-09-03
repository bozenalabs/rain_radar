#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>

#include "battery.hpp"
#include "data_fetching.hpp"
#include "drivers/inky73/inky73.hpp"
#include "drivers/pcf85063a/pcf85063a.hpp"
#include "drivers/psram_display/psram_display.hpp"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"
#include "inky_frame_7.hpp"
#include "persistent_data.hpp"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "pico/types.h"
#include "pimoroni_common.hpp"
#include "rain_radar_common.hpp"
#include "secrets.h"
#include "wifi_setup.hpp"

using namespace pimoroni;

#if PICO_RP2350
enum Colours : uint8_t {
    BLACK = 0,
    BLACK2 = 1,
    YELLOW = 2,
    RED = 3,
    WHITE = 4,
    BLUE = 5,
    GREEN = 6,
    CLEAN = 7
};
#elif PICO_RP2040
enum Colours : uint8_t {
    BLACK = 0,
    WHITE = 1,
    GREEN = 2,
    BLUE = 3,
    RED = 4,
    YELLOW = 5,
    ORANGE = 6,
    CLEAN = 7
};
#else
#error "Unsupported PICO architecture"
#endif

InkyFrame inky_frame;

void draw_error(InkyFrame &graphics, const std::string_view &msg)
{
    graphics.set_pen(Colours::RED);
    graphics.rectangle(Rect(graphics.width / 4, graphics.height * 2 / 3, graphics.width / 2, graphics.height / 4));
    graphics.set_pen(Colours::WHITE);
    graphics.text(msg, Point(graphics.width / 4 + 8, graphics.height * 2 / 3 + 8), graphics.width / 2 - 16, 2);
}

void draw_text_with_background(InkyFrame &graphics, const std::string_view &msg, Point position, uint8_t font_size, Colours text_colour, Colours background_colour)
{
    int text_width = graphics.measure_text(msg, font_size);
    const int text_height = font_size * 9; // approximate height for font size
    const int text_x_padding = 2;
    graphics.set_pen(background_colour);
    graphics.rectangle(Rect(position.x - text_x_padding, position.y - 1, text_width + 2*text_x_padding, text_height));
    graphics.set_pen(text_colour);
    graphics.text(msg, position, graphics.width, font_size);
}

void draw_battery_status(InkyFrame &graphics, const char *status)
{
    int text_width = graphics.measure_text(status, 1);
    draw_text_with_background(graphics, status, Point(graphics.width - text_width - 5, graphics.height - 10), 1, Colours::WHITE, Colours::BLACK);
}

datetime_t dt = {
    .year = 0,
    .month = 0,
    .day = 0,
    .dotw = 0,
    .hour = 0,
    .min = 0,
    .sec = 0,
};

int next_wakeup_min = 10;
int next_wakeup_hour = -1;

int get_mins_until_wakeup(int current_hour, int current_min, int wakeup_hour, int wakeup_min) {
    if (wakeup_hour < 0) {
        int mins_to_wakeup;
        if (wakeup_min < current_min) {
            mins_to_wakeup = (wakeup_min + 60) - current_min;
        } else {
            mins_to_wakeup = wakeup_min - current_min;
        }
        return mins_to_wakeup;
    } else {
        int total_current_mins = current_hour * 60 + current_min;
        int total_wakeup_mins = wakeup_hour * 60 + wakeup_min;
        
        if (total_wakeup_mins < total_current_mins) {
            // Wakeup time is on the next day
            total_wakeup_mins += 24 * 60;
        }
        
        return total_wakeup_mins - total_current_mins;
    }
}

void draw_next_wakeup(InkyFrame &graphics, int hour, int minute)
{
    std::ostringstream oss;
    if (hour >= 0) {
        oss << "Next update at " << std::setfill('0') << std::setw(2) << hour << ":"
            << std::setfill('0') << std::setw(2) << minute;
    } else {
        int mins_to_wakeup = get_mins_until_wakeup(dt.hour, dt.min, hour, minute);
        oss << "Next update in " << mins_to_wakeup << " min";
    }

    int text_width = graphics.measure_text(oss.str(), 1);
    draw_text_with_background(graphics, oss.str(), Point(graphics.width - 60 - text_width, graphics.height - 10), 1, Colours::WHITE, Colours::BLACK);
}

std::tuple<Err, std::string, bool> run_app(persistent::PersistentData &payload)
{
    ResultOr<int8_t> new_preferred_ssid_index = wifi_setup::wifi_connect(inky_frame, payload.wifi_preferred_ssid_index);
    if (!new_preferred_ssid_index.ok())
    {
        return {new_preferred_ssid_index.err, "WiFi connect failed", true};
    }
    int8_t connected_ssid_index = new_preferred_ssid_index.unwrap();
    if (connected_ssid_index != payload.wifi_preferred_ssid_index)
    {
        payload.wifi_preferred_ssid_index = connected_ssid_index;
        printf("New preferred SSID index: %d\n", payload.wifi_preferred_ssid_index);
        persistent::save(&payload);
    }

    inky_frame.set_pen(Colours::RED);
    inky_frame.clear();

    // fetching the image will write to the PSRAM display directly
    ResultOr<data_fetching::ImageHeader> const res = data_fetching::fetch_image(inky_frame, connected_ssid_index);
    if (!res.ok())
    {
        return {res.err, "Image fetch failed", true};
    } 
    data_fetching::ImageHeader image_header = res.unwrap();
    time_t update_ts = image_header.update_ts;
    struct tm *tm_info = gmtime(&update_ts);

    // Convert struct tm to datetime_t
    dt.year  = tm_info->tm_year + 1900; // tm_year is years since 1900
    dt.month = tm_info->tm_mon + 1;     // tm_mon is 0–11
    dt.day   = tm_info->tm_mday;        // 1–31
    dt.dotw  = tm_info->tm_wday;        // 0 = Sunday
    dt.hour  = tm_info->tm_hour;
    dt.min   = tm_info->tm_min;
    dt.sec   = tm_info->tm_sec;
    printf("Image timestamp: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
        dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
    inky_frame.rtc.set_datetime(&dt);
    next_wakeup_hour = image_header.next_wakeup_hours;
    next_wakeup_min = image_header.next_wakeup_minutes;

    if (image_header.draw_extra) {
        // points of interest
        for (const auto &poi : secrets::POINTS_OF_INTEREST_XY)
        {
            inky_frame.set_pen(Colours::WHITE);
            inky_frame.circle(Point(poi[0], poi[1]), 3);
            inky_frame.set_pen(Colours::RED);
            inky_frame.circle(Point(poi[0], poi[1]), 2);
        }
    }

    // Initialize battery monitoring
    // MUST BE INITIALIZED AFTER WIFI SETUP ON PICO W
    Battery battery;
    battery.init();
    const char *status = battery.get_status_string();
    printf("Battery status: %s\n", status);
    printf("%s", battery.is_usb_powered() ? "USB powered\n" : "Battery powered\n");
    if (image_header.draw_battery) {
        draw_battery_status(inky_frame, status);
    }

    return {Err::OK, "", image_header.draw_battery};
}

const uint HOLD_VSYS_EN = 2;

void sleep_for_minutes(InkyFrame &frame, int minutes) {
    printf("Sleeping for %d minute(s)...\n", minutes);

    frame.rtc.unset_alarm();
    frame.rtc.clear_alarm_flag();
    frame.rtc.unset_timer();
    frame.rtc.clear_timer_flag();

    if (minutes > 0) {
        // PCF85063A timer with 1/60Hz tick period supports 1..255 minutes
        uint8_t ticks = static_cast<uint8_t>(std::min(240, minutes));
        frame.rtc.set_timer(ticks, PCF85063A::TIMER_TICK_1_OVER_60HZ);
        frame.rtc.enable_timer_interrupt(true, false);
    }

    // Release the vsys hold pin so that inky can go to sleep on battery
    gpio_put(HOLD_VSYS_EN, false);

    // On USB power the pico won't fall asleep, so manually wait and then reboot
    sleep_ms(std::max(10'000, minutes * 60 * 1000));
    watchdog_reboot(0, 0, 0);
    while (true) {}
}

void sleep_until(InkyFrame &frame, int second, int minute, int hour, int day) {
    frame.rtc.unset_alarm();
    frame.rtc.clear_alarm_flag();
    frame.rtc.unset_timer();
    frame.rtc.clear_timer_flag();

    if (second != -1 || minute != -1 || hour != -1 || day != -1) {
        // Set an alarm to wake inky up at the specified time and day
        frame.rtc.set_alarm(second, minute, hour, day);
        frame.rtc.enable_alarm_interrupt(true);
    }

    int wake_in_minutes = get_mins_until_wakeup(dt.hour, dt.min, hour, minute);

    // Release the vsys hold pin so that inky can go to sleep
    gpio_put(HOLD_VSYS_EN, false);

    // On battery the pico will power off and reboot here
    // On usb power it won't fall asleep so we manually wait and then reboot
    sleep_ms(std::max(10'000, wake_in_minutes * 60 * 1000));
    watchdog_reboot(0, 0, 0);
    while (true) {}
}

int main()
{
    inky_frame.init();
    inky_frame.rtc.unset_alarm();
    inky_frame.rtc.clear_alarm_flag();
    inky_frame.rtc.unset_timer();
    inky_frame.rtc.clear_timer_flag();

    stdio_init_all();
    sleep_ms(100);

    // Read current datetime from RTC
    dt = inky_frame.rtc.get_datetime();

    InkyFrame::WakeUpEvent event = inky_frame.get_wake_up_event();
    printf("Wakeup event: %d\n", event);

    persistent::PersistentData payload = persistent::read();
    printf("Persistent state: preferred_ssid=%d, failure_count=%u\n",
           payload.wifi_preferred_ssid_index, payload.failure_count);

    auto [app_err, app_msg, draw_battery] = run_app(payload);

    if (app_err != Err::OK) {
        // Increment consecutive failure counter and persist
        payload.failure_count++;
        persistent::save(&payload);

        int retry_delay_mins = persistent::get_retry_delay_minutes(payload.failure_count);

        std::ostringstream oss;
        oss << app_msg << " (" << errToString(app_err) << ")\n"
            << "Retry in " << retry_delay_mins << "m (#" << payload.failure_count << ")";
        std::string error_msg = oss.str();
        printf("Error: %s\n", error_msg.c_str());

        draw_error(inky_frame, error_msg);

        if (wifi_setup::is_connected()) {
            wifi_setup::network_deinit(inky_frame);
        }

        inky_frame.update(true);

        printf("Finished with error. Sleeping for %d min...\n", retry_delay_mins);
        sleep_for_minutes(inky_frame, retry_delay_mins);
    } else {
        // Success: reset failure count if it was non-zero
        if (payload.failure_count != 0) {
            printf("Resetting failure_count from %u to 0\n", payload.failure_count);
            payload.failure_count = 0;
            persistent::save(&payload);
        }

        if (draw_battery) {
            draw_next_wakeup(inky_frame, next_wakeup_hour, next_wakeup_min);
        }

        if (wifi_setup::is_connected()) {
            wifi_setup::network_deinit(inky_frame);
        }
        
        inky_frame.update(true);

        printf("Finished successfully. Sleeping until %02d:%02d UTC\n", next_wakeup_hour, next_wakeup_min);
        sleep_until(inky_frame, -1, next_wakeup_min, next_wakeup_hour, -1);
    }

    return 0;
}
