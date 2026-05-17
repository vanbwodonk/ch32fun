# CH32X035 Project Notes

## Pin Critical Facts

### PC18/PC19 — SDI Debug Lock
- **Cannot be used as GPIO.** The WCH Serial Debug Interface (SDI) permanently owns PC18 (SWDIO) and PC19 (SWCLK). Even with `AFIO_PCFR1_SWJ_CFG_DISABLE` (which controls ARM SWJ, not the proprietary SDI) the pins stay locked.
- **`AFIO->PCFR1` bits [26:24] = `0x04000000` (SWJ_CFG_DISABLE) has no effect** on the SDI. This is a known issue: [openwch/ch32x035#8](https://github.com/openwch/ch32x035/issues/8) — no resolution from WCH.
- **Alternate function remapping (I2C1, USART, etc.) should still work** because the peripheral's pin control overrides SDI at the hardware level. GPIO does not.
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

## UFQFPN20 Pin Availability

Available: PA0-2, PA9-14, PB0-1, PB10-12 (some), PC0-3, PC14-19
Not available in this package: PB2-9, PB13-21, PA15-23
Debug: PC18(SWDIO) + PC19(SWCLK) — locked by SDI
USB: PC16(D-) + PC17(D+)
