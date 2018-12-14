#include "web.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

// Helper function to open a file, write to it, and close it.
static int open_write_close(const char *filename, const char *str_to_write) {
	int fd = open(filename, O_RDWR);
	if (fd == -1) return -1;
	if (write(fd, str_to_write, strlen(str_to_write)) == -1) {
		int err = errno;
		close(fd);
		errno = err;
		return -1;
	}
	return close(fd);
}

static int http_serve_nothing(int fd) {
	return dprintf(
		fd,
		"HTTP/1.1 200 OK\r\n"
		"\r\n"
	);
}

static int http_serve_file(int fd, const char *filename) {

	// Open the file.
	int file_fd = open("index.html", O_RDONLY);
	if (file_fd == -1) return -1;

	// Get its size.
	struct stat buf;
	if (fstat(file_fd, &buf) == -1) {
		close(file_fd);
		return -1;
	}

	// Write headers.
	int err = dprintf(
		fd,
		"HTTP/1.1 200 OK\r\n"
		"Server: ECE471\r\n"
		"Content-Length: %ld\r\n"
		"Content-Type: text/html\r\n"
		"\r\n",
		buf.st_size
	);
	if (err == -1) {
		close(file_fd);
		return -1;
	}

	return sendfile(fd, file_fd, NULL, buf.st_size);

}

// Global variables for the HTTP server.
static int socket_fd;

// This procedure handles incoming HTTP requests and whatever.
static void *web_proc(void *arg) {

	int toggle = 0;
	static char buf[512];

	while (1) {

		// Accept a connection.
		int fd = accept(socket_fd, NULL, NULL);
		if (fd == -1) {
			perror("web_proc");
			return NULL;
		}

		// Read the request.
		int count = read(fd, buf, sizeof(buf)-1);
		if (count == -1) {
			perror("web_proc");
			close(fd);
			return NULL;
		}
		buf[count] = 0; // Null-terminate the string.

		printf(buf);

		// If it's looking to blend, let's blend.
		if (strstr(buf, "blend") != NULL) {

			// Toggle GPIO21.
			// Due to pull-up / pull-down nonsense,
			// we need to set it to an input to turn off.
			if (toggle) {
				if (open_write_close("/sys/class/gpio/gpio21/direction", "out") == -1) {
					perror("web_proc");
					return NULL;
				}
				if (open_write_close("/sys/class/gpio/gpio21/value", "1") == -1) {
					perror("web_proc");
					return NULL;
				}
			} else {
				open_write_close("/sys/class/gpio/gpio21/direction", "in");
			}
			toggle = !toggle;

			// Respond back with nothing.
			if (http_serve_nothing(fd) == -1) {
				perror("http_serve_nothing");
				close(fd);
				return NULL;
			}

		} else {

			// Otherwise, serve the HTML file.
			if (http_serve_file(fd, "index.html") == -1) {
				perror("http_serve_file");
				close(fd);
				return NULL;
			}

		}

		// Close the connection.
		sleep(1);
		if (close(fd) == -1) {
			perror("web_proc");
			return NULL;
		}

	}

	return NULL;

	/*
	int toggle = 0;
	while (1) {
		sleep(1);
	}
	return NULL;
	*/
}

int web_init() {

	// Initialize the TCP socket.
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		perror("web_init");
		return -1;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(8080);
	if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("web_init");
		close(socket_fd);
		return -1;
	}
	if (listen(socket_fd, 32) == -1) {
		close(socket_fd);
		return -1;
	}

	// Initialize the GPIO.
	if (open_write_close("/sys/class/gpio/export", "21") == -1) {
		perror("web_init");
		close(socket_fd);
		return -1;
	}
	if (open_write_close("/sys/class/gpio/gpio21/direction", "in") == -1) {
		perror("web_init");
		close(socket_fd);
		open_write_close("/sys/class/gpio/unexport", "21");
		return -1;
	}

	// Create the thread. We don't care about the ID, just throw it in the trash.
	pthread_t thread;
	if (pthread_create(&thread, NULL, web_proc, NULL) != 0) {
		perror("web_init");
		close(socket_fd);
		open_write_close("/sys/class/gpio/unexport", "21");
		return 1;
	}

	return 0;

}

int web_deinit() {
	if (open_write_close("/sys/class/gpio/unexport", "21") == -1) {
		perror("web_deinit");
		close(socket_fd);
		return -1;
	}
	if (close(socket_fd) == -1) {
		perror("web_deinit");
		return -1;
	}
	return 0;
}

