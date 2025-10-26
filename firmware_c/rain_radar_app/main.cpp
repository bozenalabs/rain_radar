
#include <cstdio>
#include <math.h>

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "libraries/inky_frame_7/inky_frame_7.hpp"


using namespace pimoroni;

InkyFrame inky_frame;


int main() {
    inky_frame.init();
    inky_frame.rtc.unset_alarm();
    inky_frame.rtc.clear_alarm_flag();
    inky_frame.rtc.unset_timer();
    inky_frame.rtc.clear_timer_flag();

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
