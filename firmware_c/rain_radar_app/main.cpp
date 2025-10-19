#include <cstdio>
#include <math.h>
#include <string>

#include <stdio.h>
#include "drivers/psram_display/psram_display.hpp"
#include "drivers/inky73/inky73.hpp"
#include "inky_frame_7.hpp"
#include "pimoroni_common.hpp"
#include "pico_wireless.hpp"

#include "secrets.h"
#include "network_utils.hpp"

using namespace pimoroni;


uint DISPLAY_WIDTH = 800;
uint DISPLAY_HEIGHT = 480;



static const std::string URL = "https://muse-hub.taile8f45.ts.net/";

void draw_error(InkyFrame &graphics, const std::string_view &msg) {
  graphics.set_pen(Inky73::RED);
  graphics.rectangle(Rect(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT -25, DISPLAY_WIDTH / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(DISPLAY_WIDTH / 2 + 5, DISPLAY_HEIGHT -22), DISPLAY_WIDTH / 2, 2);
}

void draw_lower_left_text(InkyFrame &graphics, const std::string_view &msg) {
  graphics.set_pen(Inky73::BLACK);
  graphics.rectangle(Rect(0, DISPLAY_HEIGHT -25, DISPLAY_WIDTH / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(5, DISPLAY_HEIGHT -22), DISPLAY_WIDTH / 2, 2);
}

int main() {
  stdio_init_all();
  sleep_ms(500);


  InkyFrame inky_frame;
  inky_frame.init();

  printf("Inky Frame init done\n");
  fflush(stdout);
  for (int i = 0; i < 3; i++) {
    printf("Inky Frame init done\n");
    fflush(stdout);
    sleep_ms(1000);
  }

  PicoWireless wireless;


  if(! wifi_connect(wireless, inky_frame) ) {
    printf("Failed to connect to WiFi\n");
    fflush(stdout);

    draw_error(inky_frame, "Failed to connect to WiFi");
    inky_frame.update(true);
    return -1;
  }



  draw_error(inky_frame, "test error");
  draw_lower_left_text(inky_frame, "Pico Rain Radar");
  inky_frame.update(true);

  int i = 0;

  while(true){
   printf("Pico Rain Radar\n");
    printf("Attempt %d to init display\n", i++);
    sleep_ms(1000);
  }




  // while (true) {
  //   while(!inky7.is_pressed(Inky73::BUTTON_A)) {
  //     sleep_ms(10);
  //   }
  //   graphics.set_pen(1);
  //   graphics.clear();

  //   for(int i =0 ; i < 100 ; i++)
  //   {
  //     uint size = 25 + (rand() % 50);
  //     uint x = rand() % graphics.bounds.w;
  //     uint y = rand() % graphics.bounds.h;

  //     graphics.set_pen(0);
  //     graphics.circle(Point(x, y), size);

  //     graphics.set_pen(2+(i%5));
  //     graphics.circle(Point(x, y), size-2);
  //   }
  
  //   gpio_put(LED_PIN, 1);
  //   inky7.update(&graphics);
  //   gpio_put(LED_PIN, 0);
  // }

  printf("Done\n");
  return 0;
}