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
