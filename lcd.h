#ifndef LCD_H
#define LCD_H

// These functions return 0 on success and -1 (with errno set appropriately) on
// failure.
int lcd_init();
int lcd_deinit();

// lcd_write writes up to four characters of formatted string to the LCD.
// Only characters in the range '0'-'z' will be printed.
// Returns 0 on success.
// On error, returns -1 and sets errno appropriately.
int lcd_printf(const char *fmt, ...);

#endif

