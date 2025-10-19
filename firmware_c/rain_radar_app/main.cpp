
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
#include "inky_frame_7.hpp"
#include "network_utils.hpp"
#include "pico_wireless.hpp"
#include "pico/stdlib.h"
#include "pimoroni_common.hpp"
#include "secrets.h"
#include "pico/cyw43_arch.h"

using namespace pimoroni;



static const std::string URL = "https://muse-hub.taile8f45.ts.net/";

void draw_error(InkyFrame &graphics, const std::string_view &msg) {
  graphics.set_pen(Inky73::RED);
  graphics.rectangle(Rect(graphics.width / 2, graphics.height -25, graphics.width / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(graphics.width / 2 + 5, graphics.height -22), graphics.width / 2, 2);
}

void draw_lower_left_text(InkyFrame &graphics, const std::string_view &msg) {
  graphics.set_pen(Inky73::BLACK);
  graphics.rectangle(Rect(0, graphics.height -25, graphics.width / 2, 25));
  graphics.set_pen(Inky73::WHITE);
  graphics.text(msg, Point(5, graphics.height -22), graphics.width / 2, 2);
}


int main() {
    stdio_init_all();
    printf("Starting Pico W Wi-Fi example...\n");

  if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
  printf("failed to initialise\n");
  return 1;
  }
  printf("initialised\n");

  cyw43_arch_enable_sta_mode();
  
  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
    while(true) {
      printf("failed to connect\n");
      sleep_ms(1000);
    }
  }
  printf("connected\n");


    // // Optional: print IP address
    // uint8_t ip[4];
    // cyw43_arch_get_ip_address(ip);
    // printf("IP Address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

    // Keep running so Wi-Fi driver stays active
    while (true) {
            printf("Connected!\n");
            sleep_ms(1000);
    }

    cyw43_arch_deinit();
    return 0;

  stdio_init_all();
  PicoWireless wireless;
  wireless.init();
  sleep_ms(1000);


  InkyFrame inky_frame;
  inky_frame.init();


  for (int i = 0; i < 3; i++) {
    printf("Inky Frame init done %d\n", i);
    fflush(stdout);
    sleep_ms(1000);
  }

    printf("firmware version Nina %s\n", wireless.get_fw_version());
    uint8_t* mac = wireless.get_mac_address();
    printf("mac address ");
    for(uint i = 0; i < WL_MAC_ADDR_LENGTH; i++) {
      printf("%d:", mac[i]);
    }
    printf("\n");
        fflush(stdout);


        while (true) {
        int networks = wireless.get_scan_networks();
        printf("Found %d network(s)\n", networks);

        for(auto network = 0; network < networks; network++) {
            std::string ssid = wireless.get_ssid_networks(network);
            wl_enc_type enc_type = wireless.get_enc_type_networks(network);
            switch(enc_type) {
                case ENC_TYPE_WEP:
                    printf("%s (WEP)\n", ssid.c_str());
                    break;
                case ENC_TYPE_TKIP:
                    printf("%s (TKIP)\n", ssid.c_str());
                    break;
                case ENC_TYPE_CCMP:
                    printf("%s (CCMP)\n", ssid.c_str());
                    break;
                case ENC_TYPE_UNKNOWN:
                    printf("%s (UNKNOWN)\n", ssid.c_str());
                    break;
                default:
                    printf("%s\n", ssid.c_str());
                    break;
            }
        }

        sleep_ms(5000);
    }



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