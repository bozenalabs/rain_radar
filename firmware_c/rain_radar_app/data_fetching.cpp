#include "data_fetching.hpp"

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
#include "pico/platform.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"

#include "pico/async_context.h"
#include "http_client_util.hpp"
#include "rain_radar_common.hpp"
#include "wifi_setup.hpp"
#include "psram_display.hpp"
#include "inky_frame_7.hpp"
#include "pico/util/datetime.h"
#include "pico/types.h"

#define HOST "muse-hub.taile8f45.ts.net"

namespace data_fetching
{
    // Parse HTTP date string like "Mon, 27 Oct 2025 21:09:46 GMT" to datetime_t
    bool parse_http_date(const char *date_str, datetime_t *dt)
    {
        // Month names for parsing
        const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

        char day_name[4], month_name[4];
        int day, year, hour, min, sec;

        // Parse format: "Mon, 27 Oct 2025 21:09:46 GMT"
        int parsed = sscanf(date_str, "Date: %3s, %d %3s %d %d:%d:%d GMT",
                            day_name, &day, month_name, &year, &hour, &min, &sec);

        if (parsed != 7)
        {
            printf("Failed to parse date string\n");
            return false;
        }

        // Find month number (1-12)
        int month = 0;
        for (int i = 0; i < 12; i++)
        {
            if (strncmp(month_name, months[i], 3) == 0)
            {
                month = i + 1;
                break;
            }
        }

        if (month == 0)
        {
            printf("Invalid month in date string: %s\n", month_name);
            return false;
        }

        // Fill datetime_t structure
        dt->year = year;
        dt->month = month;
        dt->day = day;
        dt->hour = hour;
        dt->min = min;
        dt->sec = sec;
        dt->dotw = 0; // Day of week - we could calculate this but it's not critical

        printf("Parsed date: %04d-%02d-%02d %02d:%02d:%02d\n",
               dt->year, dt->month, dt->day, dt->hour, dt->min, dt->sec);

        return true;
    }

    // Parse simple "key=value" body
    // bool parseBody(const char *body, size_t body_len, ImageInfo &info)
    // {
    //     const char *line = body;
    //     const char *end = body + body_len;

    //     while (line < end)
    //     {
    //         const char *nextLine = (const char *)memchr(line, '\n', end - line);
    //         if (!nextLine)
    //             nextLine = end;

    //         // find '='
    //         const char *eq = (const char *)memchr(line, '=', nextLine - line);
    //         if (eq)
    //         {
    //             std::string_view key(line, eq - line);
    //             std::string_view value(eq + 1, nextLine - eq - 1);

    //             printf("Key: '%.*s' Value: '%.*s'\n", (int)key.size(), key.data(), (int)value.size(), value.data());

    //             if (key == "local_time")
    //             {
    //                 info.update_ts = strtol(value.data(), nullptr, 10);
    //             }
    //             // else if (key == "text")
    //             // {
    //             //     const size_t textBufSize = sizeof(info.image_text);
    //             //     size_t n = value.copy(info.image_text, textBufSize - 1);
    //             //     info.image_text[n] = '\0';
    //             // }
    //         }

    //         line = nextLine + 1;
    //     }

    //     return true;
    // }

    // err_t image_info_callback_fn(void *_image_info, __unused struct altcp_pcb *conn, struct pbuf *p, err_t err)
    // {
    //     if (err != ERR_OK || p == NULL)
    //     {
    //         printf("Error in image_info_callback_fn: %d\n", err);
    //         return err;
    //     }

    //     // TODO: handle pbuf chains
    //     size_t body_len = p->len;

    //     ImageInfo *info = (ImageInfo *)_image_info;
    //     parseBody((char *)p->payload, body_len, *info);

    //     return ERR_OK;
    // }

    struct ImageWriterHelper
    {
        datetime_t server_datetime;
        pimoroni::PSRamDisplay &psram_display;
        size_t const max_address_write;
        size_t offset = 0;
        Err result;

        ImageWriterHelper(pimoroni::InkyFrame &inky_frame)
            : server_datetime({0})
            , psram_display(inky_frame.ramDisplay)
            , max_address_write(inky_frame.width * inky_frame.height)
            , offset(0)
            , result(Err::OK)
        {
        }
    };

    err_t datetime_header_parser(__unused httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, __unused u32_t content_len)
    {
        printf("\nheaders %u\n", hdr_len);
        ImageWriterHelper *info = (ImageWriterHelper *)arg;

        const char *header_buffer = (const char *)hdr->payload;
        size_t header_buffer_len = hdr->len;

        // Look for Date header
        const char *date_header = "Date: ";
        const char *date_start = strnstr(header_buffer, date_header, header_buffer_len);
        // date_start might not be null terminate!
        if (date_start)
        {
            char safe_buffer[64];
            size_t copy_len = strnlen(date_start, sizeof(safe_buffer) - 1);
            memcpy(safe_buffer, date_start, copy_len);
            safe_buffer[copy_len] = '\0';
            printf("Found Date header: %s\n", safe_buffer);

            // Parse the date directly - sscanf will stop at the end of the valid format
            if (parse_http_date(safe_buffer, &info->server_datetime))
            {
                printf("Successfully parsed server datetime\n");
            }
            else
            {
                printf("Failed to parse server datetime\n");
            }
        }
        else
        {
            printf("No Date header found\n");
        }

        return ERR_OK;
    }

    // ResultOr<ImageInfo> fetch_image_info(int8_t connected_ssid_index)
    // {
    //     if (!wifi_setup::is_connected())
    //     {
    //         printf("Not connected to WiFi!\n");
    //         return Err::NO_CONNECTION;
    //     }

    //     http_client_util::http_req_t req = {0};
    //     req.hostname = HOST;
    //     std::string url_str = "/" + std::to_string(connected_ssid_index) + "/image_info.txt";
    //     req.url = url_str.c_str();
    //     printf("Requesting URL: %s from %s\n", req.url, req.hostname);

    //     ImageInfo info;
    //     info.update_ts = 0;
    //     info.has_server_datetime = false;
    //     req.callback_arg = &info;

    //     req.headers_fn = datetime_header_parser;
    //     req.recv_fn = image_info_callback_fn;
    //     /* No CA certificate checking */
    //     struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
    //     assert(tls_config);
    //     req.tls_config = tls_config; // setting tls_config enables https
    //     int result = http_client_util::http_client_request_sync(cyw43_arch_async_context(), &req);
    //     altcp_tls_free_config(tls_config);

    //     return result ? Err::ERROR : ResultOr(info);
    // }

    err_t image_data_callback_fn(void *_arg, __unused struct altcp_pcb *conn, struct pbuf *p, err_t err)
    {
        if (err != ERR_OK || p == NULL)
        {
            printf("Error in image_data_callback_fn: %d\n", err);
            return err;
        }

        ImageWriterHelper *image_writer = (ImageWriterHelper *)_arg;

        // TODO: handle pbuf chains
        size_t body_len = p->len;
        // printf("Received image data chunk of %u bytes\n", body_len);

        // Ive had to modify PSRamDisplay to make the write function and pointToAddress public
        size_t offset = image_writer->offset;
        size_t new_offset = offset + body_len;
        if (new_offset > image_writer->max_address_write)
        {
            printf("Image data exceeds display size\n");
            return ERR_BUF;
        }
        image_writer->psram_display.write_span(offset, body_len, (const uint8_t *)p->payload);
        image_writer->offset = new_offset;

        // https://forums.raspberrypi.com/viewtopic.php?t=385648
        altcp_recved(conn, body_len);
        pbuf_free(p);

        return ERR_OK;
    }

    void result_fn(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
    {
        // httpc_result is already passed as req->result.
        // set arg to result
        ImageWriterHelper *image_writer = (ImageWriterHelper *)arg;
        image_writer->result = httpStatusToErr(srv_res);
    }

    ResultOr<datetime_t> fetch_image(pimoroni::InkyFrame &inky_frame, int8_t connected_ssid_index)
    {
        printf("Fetching image for SSID index %d\n", connected_ssid_index);

        if (!wifi_setup::is_connected())
        {
            printf("Not connected to WiFi!\n");
            return Err::NO_CONNECTION;
        }

        http_client_util::http_req_t req = {0};
        req.hostname = HOST;
        std::string url_str = "/" + std::to_string(connected_ssid_index) + "/quantized.bin";
        req.url = url_str.c_str();
        printf("Requesting URL: %s from %s\n", req.url, req.hostname);

        ImageWriterHelper image_writer(inky_frame);

        req.callback_arg = &image_writer;

        req.headers_fn = datetime_header_parser;
        req.recv_fn = image_data_callback_fn;
        /* No CA certificate checking */
        struct altcp_tls_config *tls_config = altcp_tls_create_config_client(NULL, 0);
        assert(tls_config);
        req.tls_config = tls_config; // setting tls_config enables https

        req.result_fn = result_fn;

        int result = http_client_util::http_client_request_sync(cyw43_arch_async_context(), &req);
        altcp_tls_free_config(tls_config);

        if (image_writer.result != Err::OK)
        {
            return image_writer.result;
        }

        if (result)
        {
            return Err::ERROR;
        }

        if (image_writer.server_datetime.year == 0)
        {
            printf("No valid server datetime received\n");
            return Err::COULDNT_PARSE_DATE;
        }
        return ResultOr<datetime_t>(image_writer.server_datetime);
    }

}