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

## Projects

| Project | What it does | Status |
|---------|-------------|--------|
| `blink/` | PB12 + PC18/PC19 toggle, USB CDC debug output | Working |
| `i2c_oled_remap/` | I2C1 remap to PC18/PC19, SSD1306 OLED | Needs HW test |
| `i2c_scan_remap/` | I2C1 remap to PC18/PC19, address scanner | Needs HW test |
| `usbcdc_libs/` | Shared USB CDC C library | Working |
| `usbcdc/` | Hello World via USB serial every 1s | Working |
| `usbcdc_loopback/` | Echo loopback via USB serial | Working |
| `jump_bootloader/` | Jump to USB bootloader from software via USB CDC command | Working |
| `tim2_encoder/` | TIM2 encoder mode on PA0(CH1)/PA1(CH2), USB CDC debug | Experimenting |
| `exti_encoder/` | EXTI-based quadrature decoder on PA0/PA1, USB CDC debug | Experimenting |


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

## TIM2 Encoder Mode — `tim2_encoder/`

CH32X035 timers support quadrature encoder mode via the `SMS` bits in `SMCFGR`:

| SMS | Mode | Count edges |
|-----|------|------------|
| 001 | Encoder 1 | TI1 edges, TI2 = direction |
| 010 | Encoder 2 | TI2 edges, TI1 = direction |
| 011 | Encoder 3 | Both TI1 + TI2 edges (4x resolution) |

### Default TIM2 pins (no remap):
- TIM2_CH1 = **PA0** (available on UFQFPN20)
- TIM2_CH2 = **PA1** (available on UFQFPN20)

### Register setup (from `tim2_encoder/tim2_encoder.c`):
```c
RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
funPinMode(PA0, GPIO_CFGLR_IN_PUPD);
funPinMode(PA1, GPIO_CFGLR_IN_PUPD);
TIM2->CTLR1 = 0;     // disable
TIM2->CHCTLR1 =      // CC1S=01 (TI1→CH1), CC2S=01 (TI2→CH2)
  (TIM2->CHCTLR1 & ~TIM_CC1S) | 0x0001 |
  (TIM2->CHCTLR1 & ~TIM_CC2S) | 0x0100;
TIM2->CHCTLR1 |= (0xF << 4) | (0xF << 12);  // IC1F=IC2F=0xF filter
TIM2->SMCFGR = (TIM2->SMCFGR & ~TIM_SMS) | 3;  // encoder mode 3
TIM2->ATRLR = 0xFFFF;  // max range
TIM2->CNT = 0x8000;    // center for signed reading
TIM2->CTLR1 |= TIM_CEN;  // enable
// Read: (int16_t)(TIM2->CNT - 0x8000)
```

### Gotchas:
- `funPinMode` for CH32X03x uses a **macro** that shifts `mode` directly into CFGLR. Use `GPIO_CFGLR_IN_PUPD = 8` (clean 4-bit value), NOT `GPIO_Mode_IPU = 0x48` (extra bits bleed into adjacent pin nibbles).
- ch32fun's `mini_vsnprintf` does NOT support the `+` format flag. Use `%d` not `%+d`.
- Add IC1F/IC2F input filter (0xF = max) for mechanical encoder debounce. For heavy bounce, also use `IC1PSC` prescaler.

## UFQFPN20 Pin Availability

Available: PA0-2, PA9-14, PB0-1, PB10-12 (some), PC0-3, PC14-19
Not available in this package: PB2-9, PB13-21, PA15-23
Debug: PC18(SWDIO) + PC19(SWCLK) — locked by SDI
USB: PC16(D-) + PC17(D+)
