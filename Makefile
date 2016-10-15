CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -mlongcalls
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld

wifiblinky-0x00000.bin: wifiblinky
	esptool.py elf2image $^

wifiblinky: main.o uart.o

main.o: main.c

uart.o: uart.c

flash: wifiblinky-0x00000.bin
	esptool.py write_flash 0 wifiblinky-0x00000.bin 0x40000 wifiblinky-0x40000.bin

clean:
	rm -f wifiblinky main.o uart.o wifiblinky-0x00000.bin wifiblinky-0x40000.bin
