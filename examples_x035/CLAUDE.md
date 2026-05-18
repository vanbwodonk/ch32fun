# CH32X035 Project Notes

## Pin Critical Facts

### PC18/PC19 — SDI Debug Lock
- **CAN be used as GPIO** by writing `AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_SWJ_CFG) | AFIO_PCFR1_SWJ_CFG_DISABLE;` (same as Arduino core's `GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE)`). Confirmed working in `blink/`.
- Use **GPIOC->CFGXR** (not CFGLR/CFGHR) for config: each nibble = `CNF[1:0] | MODE[1:0]`. Value `0x3` = Out_PP 50MHz.
- Use **GPIOC->BSXR** (not BSHR) for set/reset: bits 0-7 = set, bits 16-23 = reset. PC18 = bit 2/18, PC19 = bit 3/19.
- `funDigitalWrite`/`funPinMode` ch32fun macros only support pins 0-15; use CFGXR/BSXR directly for pins 16+.
- **Working I2C on UFQFPN20**: bit-bang I2C on any free GPIO (PA5/PA6 in `i2c_oled_static`). Hardware I2C1 default pins PA10/PA11 are not available in UFQFPN20 package.
- CFGXR config for extended pins PC16-PC23: each nibble = `MODE[1:0] | CNF[3:2]`. Value `0x01` = Out_PP 10MHz, `0x03` = Out_PP 50MHz, `0xB` = AF_PP 50MHz, `0xF` = AF_OD 50MHz.
- BSXR register for set/reset of PC16-PC23: bits 0-7 = set, bits 16-23 = reset. PC18 = bit 2/18, PC19 = bit 3/19.
- **Workaround for I2C on UFQFPN20**: use I2C1 remap 3 (PC18=SDA, PC19=SCL) via AFIO_PCFR1 bits [4:2] = 011. I2C signals should appear even if no OLED is connected.

### PC16/PC17 — USB Pins
- PC16 = D-, PC17 = D+. Configured as input floating (PC16) and input pull-up (PC17) with `AFIO->CTLR` for PHY and pull-up.
- USB works regardless of the PC18/PC19 SDI lock — separate pins.

## USB CDC Serial Library — `usbcdc_libs/`

A C port of [CH32X035_USBSerial](https://github.com/jobitjoseph/CH32X035_USBSerial) (Arduino library), converted from C++ to pure C.

### Files
```
usbcdc_libs/
├── usbcdc_cdc.c         # CDC endpoint handlers, read/write/flush
├── usbcdc_config.h      # VID/PID (0x16C0/0x27DD), strings, UID serial
├── usbcdc_descr.c       # USB descriptors, string generation from UID
├── usbcdc_handler.c     # USB init, ISR, EP0 setup/control
├── usbcdc_internal.h    # Shared declarations, register compat defines
├── usbcdc_usb.h         # USB descriptor structs
└── usbfs_compat.h       # USBFS register bit defines
```

### Key API
```c
void CDC_init(void);                    // Initialize USB + CDC
uint8_t CDC_available(void);            // Bytes available to read
uint8_t CDC_connected(void);            // DTR set (host opened port)
char CDC_read(void);                    // Read one byte (blocks)
void CDC_write(char c);                 // Write one byte (flushes)
void CDC_write_buf(const char *d, int l); // Write buffer (non-blocking, overwrites pending)
```

### Important: `CDC_write()` vs `CDC_write_buf()`
- `CDC_write()` is blocking — spins on `CDC_writeBusyFlag`. Used for interactive echo.
- `CDC_write_buf()` is **non-blocking** — directly overwrites the EP2 buffer and re-arms TX. Used for periodic output.
- Both set TX to ACK and rely on the host sending an IN token. If the host doesn't poll (picocom vs minicom), `CDC_write()` blocks forever. `CDC_write_buf()` always overwrites regardless.

### How to add to any project
```makefile
ADDITIONAL_C_FILES:=../usbcdc_libs/usbcdc_cdc.c ../usbcdc_libs/usbcdc_descr.c ../usbcdc_libs/usbcdc_handler.c
CFLAGS+=-I../usbcdc_libs
```
```c
#include "../usbcdc_libs/usbcdc_internal.h"
// ...
CDC_init();
while (!CDC_connected());
CDC_write_buf("hello\r\n", 7);
```

## Projects Created Today

| Project | What it does |
|---------|-------------|
| `blink/` | PB12 + PC18/PC19 toggle, USB CDC debug output |
| `i2c_oled_remap/` | I2C1 remap to PC18/PC19, USB CDC debug |
| `usbcdc_libs/` | Shared USB CDC C library |
| `usbcdc/` | Hello World via USB serial every 1s |
| `usbcdc_loopback/` | Echo loopback via USB serial |
| `jump_bootloader/` | Jump to USB bootloader from software via USB CDC command |

## Bootloader Entry — WORKING

**Status: CONFIRMED WORKING** on CH32X035F8U6. The wagiminator `BOOT_now()` method works when combined with pulling D+ low before the reset.

### Working sequence (from `jump_bootloader/`):

```c
#define NVIC_RESETSYS  ((uint32_t)0x00000080)  // bit 7, NOT bit 3!

void BOOT_now(void) {
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
  FLASH->BOOT_MODEKEYR = FLASH_KEY1;
  FLASH->BOOT_MODEKEYR = FLASH_KEY2;
  FLASH->STATR |= FLASH_STATR_BOOT_MODE;   // 0x4000 (bit 14)
  FLASH->CTLR  |= FLASH_CTLR_LOCK;
  RCC->RSTSCKR |= RCC_RMVF;
  NVIC->CFGR = NVIC_RESETSYS | NVIC_KEY3;  // system reset
}

// Before calling BOOT_now():
// 1. Pull D+ (PC17) LOW for ~200ms (resets host USB state)
// 2. Release D+ back to input floating
// 3. Call BOOT_now()
```

### Critical details:
- `FLASH_STATR_BOOT_MODE` = **0x4000** (bit 14), NOT 0x400 (bit 10)
- `NVIC_RESETSYS` = **0x80** (bit 7), NOT 0x08 (bit 3)
- D+ must be pulled low then released before the reset
- After reset, chip enumerates as WCH bootloader via USB
- Flash with: `wchisp flash firmware.bin`

### Makefile integration (from `jump_bootloader/Makefile`):
```makefile
all : jump_bootloader flash

TARGET:=jump_bootloader
TARGET_MCU:=CH32X035
ADDITIONAL_C_FILES:=../usbcdc_libs/usbcdc_cdc.c ../usbcdc_libs/usbcdc_descr.c ../usbcdc_libs/usbcdc_handler.c
include ../../ch32fun/ch32fun.mk

CFLAGS+=-I../usbcdc_libs

jump_bootloader : $(TARGET).bin
	python -c "import serial; serial.Serial('/dev/ttyACM0', 1200).close()"
	@sleep 3
flash : cv_flash
clean : cv_clean
```

This triggers the 1200 baud bootloader sequence before flashing via WCH-Link. The `jump_bootloader` target builds the firmware, triggers bootloader entry (1200 baud + DTR low), waits 3 seconds, then `flash` runs `cv_flash` (minichlink over SDI).

### Manual trigger:
```sh
python -c "import serial; serial.Serial('/dev/ttyACM0', 1200).close()"
```

After bootloader entry, flash with `wchisp flash firmware.bin` (USB bootloader) or `minichlink -w firmware.bin -b` (WCH-Link/SDI).

## Arduino Core USART4 Bug

The CH32X035 Arduino core (`uart.c`) has **two issues** preventing USART4 from working:

1. **`HAVE_HWSERIAL4` not defined** in `variant_CH32X035G8U.h` — `Serial4` object is never instantiated.
2. **Naming mismatch**: CH32X035 headers use `USART4_BASE`/`USART4`/`RCC_APB1Periph_USART4`, but `uart.c` checks for `UART4_BASE`/`UART4`/`RCC_APB1Periph_UART4`. The USART4 init block is silently compiled out.

**Fix in `platformio.ini` build_flags:**
```ini
build_flags =
    -DHAVE_HWSERIAL4
    -DUART4_BASE=USART4_BASE
    -DUART4=USART4
    -DRCC_APB1Periph_UART4=RCC_APB1Periph_USART4
    -DUART4_IRQn=USART4_IRQn
```

Default USART4 pins (no remap needed): PB0(TX), PB1(RX).

## UFQFPN20 Pin Availability

Available: PA0-2, PA9-14, PB0-1, PB10-12 (some), PC0-3, PC14-19
Not available in this package: PB2-9, PB13-21, PA15-23
Debug: PC18(SWDIO) + PC19(SWCLK) — locked by SDI
USB: PC16(D-) + PC17(D+)
