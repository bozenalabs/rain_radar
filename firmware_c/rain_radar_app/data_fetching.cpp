
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include <string>

#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"

#include "pico/async_context.h"
#include "http_client_util.hpp"
#include "rain_radar_common.hpp"
#include "wifi_setup.hpp"


#define HOST "muse-hub.taile8f45.ts.net"
#define URL_REQUEST "/image_info.txt"

Result test_fetch() {

    if(!wifi_setup::is_connected()) {
        printf("Not connected to WiFi!\n");
        return Result::NO_CONNECTION;
    }


    http_client_util::EXAMPLE_HTTP_REQUEST_T req1 = {0};
    req1.hostname = HOST;
    req1.url = URL_REQUEST;
    req1.headers_fn = http_client_util::http_client_header_print_fn;
    req1.recv_fn = http_client_util::http_client_receive_print_fn;
    /* No CA certificate checking */
    struct altcp_tls_config * tls_config = altcp_tls_create_config_client(NULL, 0);
    assert(tls_config);
    req1.tls_config = tls_config; // setting tls_config enables https
    int result = http_client_util::http_client_request_sync(cyw43_arch_async_context(), &req1);
    altcp_tls_free_config(tls_config);

    return result ? Result::ERROR : Result::OK;
}