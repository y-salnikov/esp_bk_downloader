CC = xtensa-lx106-elf-gcc
CFLAGS = -I. -mlongcalls -Os
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -lc -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld -Os
TARGET= bk_dl
$(TARGET)-0x00000.bin: $(TARGET)
	esptool.py elf2image $^

$(TARGET): $(TARGET).o

$(TARGET).o: $(TARGET).c

flash: $(TARGET)-0x00000.bin
	esptool.py write_flash 0 $(TARGET)-0x00000.bin 0x10000 $(TARGET)-0x10000.bin

clean:
	rm -f $(TARGET) $(TARGET).o $(TARGET)-0x00000.bin $(TARGET)-0x10000.bin
