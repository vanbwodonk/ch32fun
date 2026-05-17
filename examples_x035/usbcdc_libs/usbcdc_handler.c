#include "usbcdc_internal.h"

volatile uint8_t  USB_SetupReq, USB_SetupTyp, USB_Config, USB_Addr, USB_ENUM_OK;
volatile uint16_t USB_SetupLen;
const uint8_t*    USB_pDescr;

static inline void USB_EP_init(void) {
  USBFSD->UEP0_DMA    = (uint32_t)wch_usbcdc_EP0_buffer;
  USBFSD->UEP0_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_T_RES_NAK;
  USBFSD->UEP0_TX_LEN = 0;

  CDC_EP_init();

  USB_ENUM_OK = 0;
  USB_Config  = 0;
  USB_Addr    = 0;
}

void USB_init(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC;
    RCC->AHBPCENR |= RCC_USBFS;

    for(volatile int i = 0; i < 5000; i++) __NOP();

    GPIOC->CFGXR &= ~(0xF << 0);
    GPIOC->CFGXR |= (4 << 0);   /* PC16 input floating */
    GPIOC->CFGXR &= ~(0xF << 4);
    GPIOC->CFGXR |= (8 << 4);   /* PC17 input pull-up */
    GPIOC->BSXR = 1 << 1;

    AFIO->CTLR = (AFIO->CTLR & ~(UDP_PUE_MASK | UDM_PUE_MASK))
               | USB_PHY_V33 | UDP_PUE_1K5 | USB_IOEN;

    for(volatile int i = 0; i < 20000; i++) __NOP();

    generate_all_string_descriptors();

    USBFSD->BASE_CTRL = 0x00;
    for(volatile int i = 0; i < 5000; i++) __NOP();

    USB_EP_init();

    USBFSD->DEV_ADDR = 0x00;
    USBFSD->INT_FG = 0xff;

    USBFSD->UDEV_CTRL = USBFS_UD_PD_DIS | USBFS_UD_PORT_EN;

    USBFSD->BASE_CTRL = USBFS_UC_DEV_PU_EN | USBFS_UC_INT_BUSY | USBFS_UC_DMA_EN;

    for(volatile int i = 0; i < 100000; i++) __NOP();

    USBFSD->INT_EN = USBFS_UIE_SUSPEND | USBFS_UIE_BUS_RST | USBFS_UIE_TRANSFER;
    NVIC_EnableIRQ(USBFS_IRQn);
}

void USB_EP0_copyDescr(uint8_t len) {
  uint8_t* tgt = wch_usbcdc_EP0_buffer;
  while(len--) *tgt++ = *USB_pDescr++;
}

static inline void USB_EP0_SETUP(void) {
  uint8_t len = 0;
  USB_SetupLen = ((uint16_t)USB_SetupBuf->wLengthH<<8) | (USB_SetupBuf->wLengthL);
  USB_SetupReq = USB_SetupBuf->bRequest;
  USB_SetupTyp = USB_SetupBuf->bRequestType;

  if((USB_SetupTyp & 0x60) == 0x00) {
    switch(USB_SetupReq) {
      case 0x06:
        switch(USB_SetupBuf->wValueH) {
          case USB_DESCR_TYP_DEVICE:
            USB_pDescr = (const uint8_t*)&wch_usbcdc_DevDescr; len = sizeof(wch_usbcdc_DevDescr); break;
          case USB_DESCR_TYP_CONFIG:
            USB_pDescr = (const uint8_t*)&wch_usbcdc_CfgDescr; len = sizeof(wch_usbcdc_CfgDescr); break;
          case USB_DESCR_TYP_STRING: {
            switch(USB_SetupBuf->wValueL) {
              case 0:   USB_pDescr = (const uint8_t*)&wch_usbcdc_LangDescr; break;
              case 1:   USB_pDescr = (const uint8_t*)&wch_usbcdc_ManufDescr; break;
              case 2:   USB_pDescr = (const uint8_t*)&wch_usbcdc_ProdDescr; break;
              case 3:   USB_pDescr = (const uint8_t*)&wch_usbcdc_SerDescr; break;
              case 4:   USB_pDescr = (const uint8_t*)&wch_usbcdc_InterfDescr; break;
              default:  USB_pDescr = (const uint8_t*)&wch_usbcdc_SerDescr; break;
            }
            len = ((const uint8_t*)USB_pDescr)[0];
            break;
          }
          default: len = 0xff; break;
        }
        if(len != 0xff) {
          if(USB_SetupLen > len) USB_SetupLen = len;
          len = USB_SetupLen >= WCH_USBCDC_EP0_SIZE ? WCH_USBCDC_EP0_SIZE : USB_SetupLen;
          USB_EP0_copyDescr(len);
        }
        break;
      case 0x05:
        USB_Addr = USB_SetupBuf->wValueL; break;
      case 0x08:
        wch_usbcdc_EP0_buffer[0] = USB_Config; if(USB_SetupLen > 1) USB_SetupLen = 1; len = USB_SetupLen; break;
      case 0x09:
        USB_Config  = USB_SetupBuf->wValueL; USB_ENUM_OK = 1; break;
      case 0x00:
        wch_usbcdc_EP0_buffer[0] = 0x00; wch_usbcdc_EP0_buffer[1] = 0x00; if(USB_SetupLen > 2) USB_SetupLen = 2; len = USB_SetupLen; break;
      default:
        len = 0xff; break;
    }
  } else if((USB_SetupTyp & 0x60) == 0x20) {
    len = CDC_control();
  } else {
    len = 0xff;
  }

  if(len == 0xff) {
    USB_SetupReq = 0xff;
    USBFSD->UEP0_CTRL_H = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_STALL | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_STALL;
  } else {
    USB_SetupLen -= len;
    USBFSD->UEP0_TX_LEN = len;
    USBFSD->UEP0_CTRL_H = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_ACK;
  }
}

static inline void USB_EP0_IN(void) {
  uint8_t len;
  if((USB_SetupTyp & 0x60) == 0x20) { return; }
  switch(USB_SetupReq) {
    case 0x06:
      len = USB_SetupLen >= WCH_USBCDC_EP0_SIZE ? WCH_USBCDC_EP0_SIZE : USB_SetupLen;
      USB_EP0_copyDescr(len);
      USB_SetupLen -= len;
      USBFSD->UEP0_TX_LEN = len;
      USBFSD->UEP0_CTRL_H ^= USBFS_UEP_T_TOG;
      break;
    case 0x05:
      USBFSD->DEV_ADDR    = (USBFSD->DEV_ADDR & USBFS_UDA_GP_BIT) | USB_Addr;
      USBFSD->UEP0_CTRL_H = USBFS_UEP_T_RES_NAK | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_ACK;
      break;
    default:
      USBFSD->UEP0_CTRL_H = USBFS_UEP_T_RES_NAK | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_ACK;
      break;
  }
}

static inline void USB_EP0_OUT(void) {
  if((USB_SetupTyp & 0x60) == 0x20) { CDC_EP0_OUT(); return; }
  USBFSD->UEP0_CTRL_H = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK | USBFS_UEP_R_RES_ACK;
}

void USBFS_IRQHandler(void) __attribute__((interrupt));
void USBFS_IRQHandler(void) {
  uint8_t intflag = USBFSD->INT_FG;
  uint8_t intst   = USBFSD->INT_ST;
  if(intflag & USBFS_UIF_TRANSFER) {
    uint8_t callIndex = intst & USBFS_UIS_ENDP_MASK;
    switch(intst & USBFS_UIS_TOKEN_MASK) {
      case USBFS_UIS_TOKEN_SETUP: USB_EP0_SETUP(); break;
      case USBFS_UIS_TOKEN_IN:
        switch(callIndex) { case 0: USB_EP0_IN(); break; case 2: CDC_EP2_IN(); break; default: break; }
        break;
      case USBFS_UIS_TOKEN_OUT:
        switch(callIndex) { case 0: USB_EP0_OUT(); break; case 2: CDC_EP2_OUT(); break; default: break; }
        break;
    }
    USBFSD->INT_FG = USBFS_UIF_TRANSFER;
  }
  if(intflag & USBFS_UIF_SUSPEND) { USBFSD->INT_FG = USBFS_UIF_SUSPEND; }
  if(intflag & USBFS_UIF_BUS_RST) {
    USB_EP_init();
    USBFSD->DEV_ADDR = 0; USBFSD->INT_FG = 0xff;
  }
}
