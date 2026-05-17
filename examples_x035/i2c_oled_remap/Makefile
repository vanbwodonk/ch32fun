all : flash

TARGET:=i2c_oled_remap
TARGET_MCU:=CH32X035
ADDITIONAL_C_FILES:=../usbcdc_libs/usbcdc_cdc.c ../usbcdc_libs/usbcdc_descr.c ../usbcdc_libs/usbcdc_handler.c
include ../../ch32fun/ch32fun.mk

CFLAGS+=-I../usbcdc_libs

flash : cv_flash
clean : cv_clean
