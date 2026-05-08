#include "config.h"
#include <string.h>
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/flash.h"

// ================= HAL / Radio =================
PicoHal* hal = new PicoHal(SPI_PORT, SPI_MISO, SPI_MOSI, SPI_SCK);
LR2021 radio = new Module(hal, RFM_NSS, RFM_IRQ, RFM_RST, RFM_BUSY);
LoRaWANNode node(&radio, &Region, subBand);

// ==================== CONFIG ====================
#define FLASH_TARGET_OFFSET     (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define FLASH_EMULATE_EEPROM_SIZE 4096
#define MAGIC_VALUE             0x5AA55AA5
#define JOIN_RETRY_DELAY_MS     30000

static constexpr uint32_t LORAWAN_STATE_LEN =
    4 + RADIOLIB_LORAWAN_NONCES_BUF_SIZE + RADIOLIB_LORAWAN_SESSION_BUF_SIZE;

// Keep the persisted LoRaWAN state inside one flash sector so each save can
// erase/program exactly one sector and restore by reading a simple linear blob.
static_assert(LORAWAN_STATE_LEN <= FLASH_EMULATE_EEPROM_SIZE,
              "LoRaWAN state does not fit in emulated EEPROM flash sector");
static_assert(FLASH_EMULATE_EEPROM_SIZE == FLASH_SECTOR_SIZE,
              "Emulated EEPROM area must be exactly one flash sector");

// ==================== FLASH HELPERS ====================
static bool flash_erase(void) {
    uint32_t addr = FLASH_TARGET_OFFSET;
    // Flash erase/program cannot run directly from XIP flash while interrupts
    // might execute flash code. flash_safe_execute enters the SDK safe zone.
    int rc = flash_safe_execute([](void* p) {
        flash_range_erase(*(uint32_t*)p, FLASH_EMULATE_EEPROM_SIZE);
    }, &addr, UINT32_MAX);
    if (rc != PICO_OK) {
        printf("[NVS] flash erase failed: %d\n", rc);
        return false;
    }
    return true;
}

static bool flash_write(uint32_t offset, const uint8_t* data) {
    uint32_t addr = FLASH_TARGET_OFFSET + offset;
    uintptr_t params[] = { addr, (uintptr_t)data };
    // The Pico SDK requires flash writes to be page-sized and page-aligned.
    int rc = flash_safe_execute([](void* p) {
        uintptr_t* args = (uintptr_t*)p;
        flash_range_program(args[0], (const uint8_t*)args[1], FLASH_PAGE_SIZE);
    }, params, UINT32_MAX);
    if (rc != PICO_OK) {
        printf("[NVS] flash program failed at %lu: %d\n", offset, rc);
        return false;
    }
    return true;
}

// ==================== CORE EEPROM EMULATION ====================
uint8_t eeprom_read(uint32_t addr) {
    return *(const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET + addr);
}

bool eeprom_write_all(const uint8_t* full_data, uint32_t total_len) {
    if (total_len > FLASH_EMULATE_EEPROM_SIZE) {
        printf("[NVS] Write too large: %lu\n", total_len);
        return false;
    }

    // Program the whole emulated EEPROM sector after erasing it. Unused bytes
    // are kept at 0xFF, which is the erased flash state.
    uint8_t buf[FLASH_EMULATE_EEPROM_SIZE];
    memset(buf, 0xFF, FLASH_EMULATE_EEPROM_SIZE);
    memcpy(buf, full_data, total_len);

    if (!flash_erase()) {
        return false;
    }

    for (uint32_t i = 0; i < FLASH_EMULATE_EEPROM_SIZE; i += FLASH_PAGE_SIZE) {
        if (!flash_write(i, buf + i)) {
            return false;
        }
    }

    return true;
}

// =====================================================
// SAVE / RESTORE
// =====================================================
bool saveLoRaWANState() {
    uint8_t flash_buf[LORAWAN_STATE_LEN] = {0};

    // Layout:
    //   [magic][RadioLib nonces buffer][RadioLib session buffer]
    // The RadioLib buffers include their own signatures/checksums.
    uint32_t magic = MAGIC_VALUE;
    memcpy(flash_buf, &magic, 4);
    memcpy(flash_buf + 4, node.getBufferNonces(), RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    memcpy(flash_buf + 4 + RADIOLIB_LORAWAN_NONCES_BUF_SIZE,
           node.getBufferSession(),
           RADIOLIB_LORAWAN_SESSION_BUF_SIZE);

    if (!eeprom_write_all(flash_buf, LORAWAN_STATE_LEN)) {
        return false;
    }

    // Read back immediately so flash safety/configuration problems show up in
    // the same boot instead of only after the next reset.
    for (uint32_t i = 0; i < LORAWAN_STATE_LEN; i++) {
        if (eeprom_read(i) != flash_buf[i]) {
            printf("[NVS] Verify failed at %lu\n", i);
            return false;
        }
    }

    printf("[NVS] State saved\n");
    return true;
}

bool restoreLoRaWANState() {
    uint32_t magic = 0;
    uint8_t* m = (uint8_t*)&magic;
    for (int i = 0; i < 4; i++) {
        m[i] = eeprom_read(i);
    }

    if (magic != MAGIC_VALUE) {
        printf("[NVS] No saved session\n");
        return false;
    }

    // Nonces must be restored before the session buffer because RadioLib checks
    // that both buffers belong together.
    uint8_t nonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    for (size_t i = 0; i < RADIOLIB_LORAWAN_NONCES_BUF_SIZE; i++) {
        nonces[i] = eeprom_read(4 + i);
    }

    int16_t state = node.setBufferNonces(nonces);
    if (state != RADIOLIB_ERR_NONE) {
        printf("[NVS] Nonces restore failed: %d\n", state);
        return false;
    }

    uint8_t session[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
    uint32_t session_start = 4 + RADIOLIB_LORAWAN_NONCES_BUF_SIZE;
    for (size_t i = 0; i < RADIOLIB_LORAWAN_SESSION_BUF_SIZE; i++) {
        session[i] = eeprom_read(session_start + i);
    }

    state = node.setBufferSession(session);
    if (state != RADIOLIB_ERR_NONE) {
        printf("[NVS] Session restore failed: %d\n", state);
        return false;
    }

    printf("[NVS] Session restored\n");
    return true;
}

// =====================================================
// Utilities
// =====================================================
void printHex(const uint8_t* data, size_t len) {
    printf("[HEX]: ");
    for (size_t i = 0; i < len; i++)
        printf("%02X ", data[i]);
    printf("\n");
}

void printAscii(const uint8_t* data, size_t len) {
    printf("[ASCII]: ");
    for (size_t i = 0; i < len; i++) {
        printf("%c", (data[i] >= 32 && data[i] <= 126) ? data[i] : '.');
    }
    printf("\n");
}

void joinLoRaWAN(bool restored) {
    for (;;) {
        LoRaWANJoinEvent_t joinEvent;
        absolute_time_t started = get_absolute_time();

        printf("[LoRaWAN] Activating OTAA...\n");
        int16_t state = node.activateOTAA(&joinEvent);
        int64_t elapsed_ms = absolute_time_diff_us(started, get_absolute_time()) / 1000;

        printf("[LoRaWAN] activateOTAA returned %d after %lld ms\n", state, elapsed_ms);
        printf("[LoRaWAN] DevNonce=%u JoinNonce=%lu NewSession=%d\n",
               joinEvent.devNonce,
               joinEvent.joinNonce,
               joinEvent.newSession ? 1 : 0);

        if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
            saveLoRaWANState();
            printf("[LoRaWAN] %s OK!\n", restored ? "Restored" : "Joined");
            return;
        }

        // Even failed OTAA attempts can advance DevNonce after JoinRequest TX.
        // Save it to avoid reusing DevNonce after a reset.
        saveLoRaWANState();
        printf("[LoRaWAN] Join failed: %d, retry in %lu ms\n", state, (uint32_t)JOIN_RETRY_DELAY_MS);
        sleep_ms(JOIN_RETRY_DELAY_MS);
        restored = false;
    }
}

// =====================================================
// Main
// =====================================================
int main() {
    stdio_init_all();
    sleep_ms(2000);

    printf("[LR2021] Initializing... ");
    radio.irqDioNum = 11;
    radio.XTAL = true;

    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        printf("failed, code %d\n", state);
        return 1;
    }
    printf("success!\n");
    printf("[NVS] Flash offset: 0x%08lX, size: %u\n",
           (uint32_t)FLASH_TARGET_OFFSET,
           (unsigned)FLASH_EMULATE_EEPROM_SIZE);

    state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        printf("OTAA init failed: %d\n", state);
        while (1) sleep_ms(10);
    }

    bool restored = restoreLoRaWANState();

    joinLoRaWAN(restored);

    for (;;) {
        printf("[Uplink] Sending...\n");

        uint8_t tx[3] = {
            (uint8_t)radio.random(100),
            (uint8_t)(radio.random(2000) >> 8),
            (uint8_t)(radio.random(2000) & 0xFF)
        };

        uint8_t rx[64];
        size_t rxLen = sizeof(rx);
        int16_t res = node.sendReceive(tx, sizeof(tx), 1, rx, &rxLen);

        if (res < 0) {
            printf("TX error: %d\n", res);
        } else {
            // Uplink frame counters change after a successful send. Persist the
            // updated session so a reset does not roll counters backwards.
            saveLoRaWANState();
        }

        if (res > 0) {
            printf("[Downlink] Received!\n");
            printf("RX win: %d\n", res);
            printf("Len: %u\n", (unsigned)rxLen);
            printHex(rx, rxLen);
            printAscii(rx, rxLen);
        } else {
            printf("[Downlink] None\n");
        }

        printf("Next in %lus\n\n", uplinkIntervalSeconds);
        sleep_ms(uplinkIntervalSeconds * 1000UL);
    }

    return 0;
}
