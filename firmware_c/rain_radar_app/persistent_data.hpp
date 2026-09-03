#pragma once

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "pico/stdlib.h"
#include "hardware/flash.h" // for flash erasing and writing
#include "hardware/sync.h"  // for interrupts

#define FLASH_TARGET_OFFSET (1800 * 1024) // 1.8MB into flash, safely above firmware binary

namespace persistent
{
    constexpr uint32_t PERSISTENT_MAGIC = 0x52414452; // 'RADR'

    struct PersistentData
    {
        uint32_t magic;
        int8_t wifi_preferred_ssid_index; // index into the known SSIDs array
        uint16_t failure_count;           // consecutive failure count for exponential backoff
        uint8_t reserved[9];              // reserved for future expansion
    };

    inline int get_retry_delay_minutes(uint16_t failure_count)
    {
        // Retry schedule:
        // Fail 1: 1 min
        // Fail 2: 1 min
        // Fail 3: 1 min
        // Fail 4: 2 min
        // Fail 5: 4 min
        // Fail 6: 8 min
        // Fail 7: 16 min
        // Fail 8: 32 min
        // Fail 9: 64 min
        // Fail 10: 128 min
        // Fail 11+: 240 min (~4 hours max)
        if (failure_count <= 3)
        {
            return 1;
        }
        int shift = failure_count - 3;
        if (shift > 7)
        {
            return 240; // Capped at 4 hours (240 minutes)
        }
        int delay = 1 << shift; // 2, 4, 8, 16, 32, 64, 128
        return std::min(240, delay);
    }

    inline void save(PersistentData *myData)
    {
        if (!myData)
            return;

        myData->magic = PERSISTENT_MAGIC;

        // Flash programming buffer must be aligned to FLASH_PAGE_SIZE (256 bytes)
        uint8_t page_buffer[FLASH_PAGE_SIZE];
        memset(page_buffer, 0xFF, sizeof(page_buffer));
        memcpy(page_buffer, myData, sizeof(*myData));

        printf("Programming flash persistent data (failures=%u, ssid_idx=%d)...\n",
               myData->failure_count, myData->wifi_preferred_ssid_index);

        uint32_t interrupts = save_and_disable_interrupts();
        flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);             // erase 4KB sector
        flash_range_program(FLASH_TARGET_OFFSET, page_buffer, FLASH_PAGE_SIZE); // write 256B page
        restore_interrupts(interrupts);

        printf("Flash programming done.\n");
    }

    inline PersistentData read()
    {
        PersistentData myData{
            .magic = PERSISTENT_MAGIC,
            .wifi_preferred_ssid_index = 0,
            .failure_count = 0,
            .reserved = {0}
        };

        const PersistentData *flash_data = (const PersistentData *)(XIP_BASE + FLASH_TARGET_OFFSET);
        if (flash_data->magic == PERSISTENT_MAGIC)
        {
            memcpy(&myData, flash_data, sizeof(myData));
        }
        else
        {
            printf("Flash data uninitialized (magic 0x%08lX != 0x%08lX), using defaults.\n",
                   (unsigned long)flash_data->magic, (unsigned long)PERSISTENT_MAGIC);
        }

        return myData;
    }

} // namespace persistent
