#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "funconfig.h"
#include "ch32fun.h"

#define FUSB_CONFIG_EPS       4
#define FUSB_5V_OPERATION     0
#define FUSB_HID_INTERFACES   0
#define FUSB_HID_USER_REPORTS 0
#define FUSB_CURSED_TURBO_DMA 0

#include "usb_defines.h"

#ifndef CDC_GET_LINE_CODING
#define CDC_GET_LINE_CODING   0x21
#endif
#ifndef CDC_SET_LINE_CODING
#define CDC_SET_LINE_CODING   0x20
#endif
#ifndef CDC_SET_CTL_LINE_STATE
#define CDC_SET_CTL_LINE_STATE 0x22
#endif

typedef struct {
  uint32_t dwDTERate;
  uint8_t  bCharFormat;
  uint8_t  bParityType;
  uint8_t  bDataBits;
} __attribute__((packed)) LineCoding;

#define EP_DATA_IN  0x82
#define EP_DATA_OUT 0x02
#define EP_CMD      0x81

static const uint8_t device_descriptor[] = {
  18, 1, 0x00, 0x02, 0x02, 0x00, 0x00, 64,
  0x09, 0x12, 0x35, 0xd0, 0x03, 0x00, 1, 2, 3, 1
};

#define USB_DEV_CLASS_COMM 0x02
#define USB_DEV_SUBCLASS_ACM 0x02
#define USB_DEV_PROTOCOL_AT 0x01

static const uint8_t config_descriptor[] = {
  0x09, 0x02, 0x43, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,

  0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,

  0x05, 0x24, 0x00, 0x10, 0x01,
  0x05, 0x24, 0x01, 0x00, 0x00,
  0x04, 0x24, 0x02, 0x02,
  0x05, 0x24, 0x06, 0x00, 0x01,

  0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x01,

  0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,

  0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
  0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00,
};

#define STR_MANUFACTURER u"ch32fun"
#define STR_PRODUCT      u"USB CDC Serial"
#define STR_SERIAL       u"X0350001"

struct usb_string_descriptor_struct { uint8_t bLength; uint8_t bDescriptorType; uint16_t wString[]; };

const static struct usb_string_descriptor_struct string0 __attribute__((section(".rodata"))) = { 4, 3, {0x0409} };
const static struct usb_string_descriptor_struct string1 __attribute__((section(".rodata"))) = { sizeof(STR_MANUFACTURER), 3, STR_MANUFACTURER };
const static struct usb_string_descriptor_struct string2 __attribute__((section(".rodata"))) = { sizeof(STR_PRODUCT), 3, STR_PRODUCT };
const static struct usb_string_descriptor_struct string3 __attribute__((section(".rodata"))) = { sizeof(STR_SERIAL), 3, STR_SERIAL };

const static struct descriptor_list_struct {
  uint32_t lIndexValue;
  const uint8_t *addr;
  uint8_t length;
} descriptor_list[] = {
  {0x00000100, device_descriptor, sizeof(device_descriptor)},
  {0x00000200, config_descriptor, sizeof(config_descriptor)},
  {0x00000300, (const uint8_t *)&string0, 4},
  {0x04090301, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
  {0x04090302, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},
  {0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)},
};
#define DESCRIPTOR_LIST_ENTRIES (sizeof(descriptor_list)/sizeof(struct descriptor_list_struct))

#endif
