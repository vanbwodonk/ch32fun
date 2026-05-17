#pragma once

#include <stdint.h>
#include "ch32fun.h"
#include "usbcdc_usb.h"
#include "usbcdc_config.h"
#include "usbfs_compat.h"

#define USBFSD USBFS
#define __NOP() __asm__("nop")

#define WCH_USBCDC_EP0_SIZE 8
#define WCH_USBCDC_EP1_SIZE 8
#define WCH_USBCDC_EP2_SIZE 64

extern unsigned char wch_usbcdc_EP0_buffer[];
extern unsigned char wch_usbcdc_EP1_buffer[];
extern unsigned char wch_usbcdc_EP2_buffer[];

extern const USB_DEV_DESCR wch_usbcdc_DevDescr;
typedef struct __attribute__((packed)) {
  USB_CFG_DESCR config;
  USB_IAD_DESCR association;
  USB_ITF_DESCR interface0;
  unsigned char functional[19];
  USB_ENDP_DESCR ep1IN;
  USB_ITF_DESCR interface1;
  USB_ENDP_DESCR ep2OUT;
  USB_ENDP_DESCR ep2IN;
} WCH_USBCDC_CFG_DESCR_CDC;
extern const WCH_USBCDC_CFG_DESCR_CDC wch_usbcdc_CfgDescr;

extern const USB_STR_DESCR wch_usbcdc_LangDescr;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bString[WCH_USBCDC_MANUF_LEN];
} WCH_USBCDC_MANUF_DESCR;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bString[WCH_USBCDC_PROD_LEN];
} WCH_USBCDC_PROD_DESCR;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bString[WCH_USBCDC_SERIAL_LEN];
} WCH_USBCDC_SER_DESCR;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bString[WCH_USBCDC_INTERF_LEN];
} WCH_USBCDC_INTERF_DESCR;

extern WCH_USBCDC_MANUF_DESCR wch_usbcdc_ManufDescr;
extern WCH_USBCDC_PROD_DESCR wch_usbcdc_ProdDescr;
extern WCH_USBCDC_SER_DESCR wch_usbcdc_SerDescr;
extern WCH_USBCDC_INTERF_DESCR wch_usbcdc_InterfDescr;

extern volatile uint8_t  USB_SetupReq, USB_SetupTyp, USB_Config, USB_Addr, USB_ENUM_OK;
extern volatile uint16_t USB_SetupLen;
extern const uint8_t*    USB_pDescr;

extern volatile uint8_t CDC_writePointer;
extern volatile uint8_t CDC_writeBusyFlag;

void CDC_init(void);
uint8_t CDC_available(void);
uint8_t CDC_ready(void);
uint8_t CDC_connected(void);
uint32_t CDC_baud(void);
void CDC_check_bootloader(void);
void BOOT_now(void);
void CDC_flush(void);
void CDC_write(char c);
void CDC_write_buf(const char *data, int len);
char CDC_read(void);
void CDC_EP_init(void);
uint8_t CDC_control(void);
void CDC_EP0_OUT(void);
void CDC_EP2_IN(void);
void CDC_EP2_OUT(void);

void USB_init(void);
void USB_EP0_copyDescr(uint8_t len);

typedef struct __attribute__((packed)) {
    uint8_t  bRequestType;
    uint8_t  bRequest;
    uint8_t  wValueL;
    uint8_t  wValueH;
    uint8_t  wIndexL;
    uint8_t  wIndexH;
    uint8_t  wLengthL;
    uint8_t  wLengthH;
} USB_SETUP_REQ, *PUSB_SETUP_REQ;

#define USB_SetupBuf ((PUSB_SETUP_REQ)wch_usbcdc_EP0_buffer)

void generate_unique_serial_descriptor(void);
void generate_manufacturer_descriptor(void);
void generate_product_descriptor(void);
void generate_interface_descriptor(void);
void generate_all_string_descriptors(void);

void uint32_to_hex_string(uint32_t value, char* output);
void string_to_utf16le_descriptor(const char* source, uint16_t* dest, int max_len);
