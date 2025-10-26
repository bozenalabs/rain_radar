
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

    inky_frame.sleep_until(-1, 1,   -1, -1);

    return 0;
}
