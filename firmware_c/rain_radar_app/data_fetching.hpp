#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include <string>

static const std::string URL = "http://muse-hub.taile8f45.ts.net/";
static const std::string PORT = "443";


void test_fetch() {
    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);

    if(link_status != CYW43_LINK_UP) {
        printf("Not connected to WiFi!\n");
        return;
    }


}