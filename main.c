#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <dirent.h>
#include <signal.h>
#include "lcd.h"
#include "temp.h"
#include "web.h"

// Signal handler stuff.
volatile sig_atomic_t stop = 0;
void sighandler(int signum) {
	stop = 1;
}
// init_sighandler initializes the signal handler to capture SIGTERM and SIGINT.
// Returns 0 on success.
// On error, returns -1 and sets errno appropriately.
static int init_sighandler() {
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = sighandler;
	if (sigaction(SIGTERM, &act, NULL) == -1) return -1;
	if (sigaction(SIGINT,  &act, NULL) == -1) return -1;
	return 0;
}

int main(int argc, char **argv) {

	// Initialize the signal handler.
	if (init_sighandler() == -1) {
		perror("sigaction");
		return 1;
	}

	// Launch the web server thread.
	if (web_init() != 0) return 1;

	// Get the sensor filename.
	static char buf[256];
	if (get_sensor_filename(buf, sizeof(buf)) != 0) return 1;

	// Initialize the LCD.
	if (lcd_init() == -1) {
		perror("lcd_init");
		return 1;
	}

	// Read and print the temperature in a loop.
	while(!stop) {

		double temp = read_temp(buf);

		// Report an error on invalid temperatures.
		if (temp < -99.9 || temp > 999) {
			lcd_printf("err");
			sleep(1);
			continue;
		}

		// Otherwise, print according to the assignment.
		// Note that I coded '/' to be the degree symbol.
		if (temp >= 0.0 && temp <= 99.9) {
			lcd_printf("%.1f/", temp);
		} else {
			lcd_printf("%.0f/", temp);
		}

		sleep(1);
	}

	// Clean up.
	if (web_deinit() == -1) {
		perror("web_deinit");
		lcd_deinit();
		return 0;
	}
	if (lcd_deinit() == -1) {
		perror("lcd_deinit");
		return 1;
	}

	return 0;

}
