#include "lcd.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <ctype.h>
#include <stdarg.h>

// i2c file descriptor.
static int fd = -1;

// char_map can be indexed into to map characters in the range of '0'-'Z' to
// bytes for the LCD.
static const unsigned char char_map[] = {

	0x40, // '-'
	0x00, // '.' (this is a special case)
	0x63, // '/' (but I'm going to render it as a degree symbol)

	0x3f, // '0'
	0x06, // '1'
	0x5b, // '2'
	0x4f, // '3'
	0x66, // '4'
	0x6d, // '5'
	0x7d, // '6'
	0x07, // '7'
	0x7f, // '8'
	0x67, // '9'

	0x00, // Filler characters (for ASCII reasons).
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,

	0x77, // 'A'
	0x7f, // 'B'
	0x39, // 'C'
	0x3f, // 'D'
	0x79, // 'E'
	0x71, // 'F'
	0x3d, // 'G'
	0x76, // 'H'
	0x06, // 'I'
	0x0e, // 'J'
	0x76, // 'K'
	0x38, // 'L'
	0x00, // 'M' (this one's hard)
	0x54, // 'N'
	0x3f, // 'O'
	0x73, // 'P'
	0x00, // 'Q' (this one's hard)
	0x50, // 'R'
	0x6d, // 'S'
	0x78, // 'T'
	0x3e, // 'U'
	0x1c, // 'V'
	0x00, // 'W' (this one's hard)
	0x76, // 'X'
	0x6e, // 'Y'
	0x5b  // 'Z'
};

int lcd_init() {

	// Open the LCD.
	fd = open("/dev/i2c-1", O_RDWR);
	if (fd == -1) goto lcd_init_failure;

	// Set the slave address.
	int status = ioctl(fd, I2C_SLAVE, 0x70);
	if (status == -1) goto lcd_init_failure;

	// Turn on the oscillator.
	char buf[1];
	buf[0] = (0x2 << 4) | 0x1;
	status = write(fd, buf, 1);
	if (status == -1) goto lcd_init_failure;

	// Turn on the display with no blink.
	buf[0] = 0x81;
	status = write(fd, buf, 1);
	if (status == -1) goto lcd_init_failure;

	// Set the brightness.
	int brightness = 11;
	buf[0] = (0xe << 4) | brightness;
	status = write(fd, buf, 1);
	if (status == -1) goto lcd_init_failure;

	return 0;

lcd_init_failure:

	// Clean up.
	if (fd != -1) {
		int err = errno;
		close(fd);
		fd = -1;
		errno = err;
	}
	return -1;

}

int lcd_deinit() {
	if (close(fd) == -1) return -1;
	return 0;
}

int lcd_printf(const char *fmt, ...) {

	// Make sure lcd_init ran successfully.
	if (fd == -1) {
		errno = EIO;
		return -1;
	}

	// Format the string into a buffer.
	char str_buf[6];
	va_list args;
	va_start(args, fmt);
	vsnprintf(str_buf, 6, fmt, args);
	va_end(args);

	// Declare and zero our buffer.
	char buffer[17];
	memset(buffer, 0, 17);

	// Indexes to insert characters into the buffer.
	int indexes[4] = { 1, 3, 7, 9 };

	// Loop over the string and indexes,
	// stopping when we hit a NULL char or when i == 4.
	int i = 0;
	for (char *str = str_buf; *str && i < 4; str++) {

		// We only use uppercase characters.
		char ch = toupper(*str);

		// If it's a decimal, stick it on the previous digit.
		if (ch == '.' && i > 0) {
			buffer[indexes[i-1]] |= 0x80;
			continue;
		}

		// Otherwise if it's outside the range, continue.
		if (ch < '-' || ch > 'Z') continue;

		buffer[indexes[i]] = char_map[ch - '-'];
		i++;

	}

	// Write the buffer out.
	if (write(fd, buffer, 17) == -1) {
		perror("write");
		close(fd);
		return 1;
	}

	return 0;
}

