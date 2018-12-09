#include "temp.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

int get_sensor_filename(char *buf, int buf_cap) {

	// Open the directory.
	DIR *dir = opendir("/sys/bus/w1/devices");
	if (dir == NULL) {
		perror("opendir");
		return 1;
	}

	// Loop over all items in the directory.
	int found = 0;
	struct dirent *entry;
	while (1) {

		// Get next item.
		entry = readdir(dir);
		if (entry == NULL) break; // Done.

		// What we're looking for is a symbolic link..
		if (entry->d_type != DT_LNK) continue;

		// ..with a name starting with "28-".
		if (strncmp("28-", entry->d_name, 3) != 0) continue;

		// Copy into the output buffer.
		snprintf(buf, buf_cap, "/sys/bus/w1/devices/%s/w1_slave", entry->d_name);
		found = 1;

	}

	if (!found) fprintf(stderr, "Couldn't find device!\n");

	if (closedir(dir) == -1) {
		perror("closedir");
		return 1;
	}

	return found ? 0 : 1;

}

double read_temp(char *filename) {

	// Open the file.
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		perror("fopen");
		return 0.0;
	}

	// Read both lines at once (and null-terminate).
	char buf[128];
	int len = fread(buf, 1, sizeof(buf)-1, file);
	buf[len] = '\0';

	// Check for "NO".
	if (strstr(buf, "NO")) {
		fprintf(stderr, "Read YES not NO!\n");
		fclose(file);
		return 0.0;
	}

	// Read the temperature.
	char *temp_str = strstr(buf, "t=");
	if (temp_str == NULL) {
		fprintf(stderr, "Couldn't find t=!\n");
		fclose(file);
		return 0.0;
	}
	int temp;
	if (sscanf(temp_str, "t=%d\n", &temp) == EOF) {
		fprintf(stderr, "Couldn't parse temperature value %s!\n", temp_str);
		fclose(file);
		return 0.0;
	}

	// Close.
	if (fclose(file) == EOF) {
		perror("fclose");
		return 0.0;
	}

	double celcius = temp / 1000.0;
	double farenheit = celcius*9.0/5.0 + 32.0;
	return farenheit;

}

