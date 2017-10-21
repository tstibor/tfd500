#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "log.h"

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "NA"
#endif

#define DEFAULT_DEVICE "/dev/ttyUSB0"
#define MAX_DEVICE_LENGTH 64
#define MAX_LENGTH 32

#define OPTNCMP(str1, str2)				\
        ((strlen(str1) == strlen(str2)) &&		\
         (strncmp(str1, str2, strlen(str1)) == 0))

#define TFD_WRITE(fd, buf, len)				\
do {							\
	int rc = tfd_write(fd, buf, len);		\
	if (rc)						\
		return rc;				\
} while (0);						\

#define TFD_READ(fd, buf, len)			\
do {						\
	int rc = tfd_read(fd, buf, len);	\
	if (rc)					\
		return rc;			\
} while (0);					\

struct options {
	char o_device[MAX_DEVICE_LENGTH + 1];
	int o_reset;
	int o_clear;
	int o_firmware;
	int o_status;
	int o_gtime;
	int o_stime;
	int o_settings;
	int o_interval;
	int o_mode;
	int o_dump;
	int o_verbose;
};

struct options opt = {
	.o_device = {0},
	.o_reset = 0,
	.o_clear = 0,
	.o_firmware = 0,
	.o_status = 0,
	.o_gtime = 0,
	.o_stime = 0,
	.o_settings = 0,
	.o_interval = 0,
	.o_mode = -1,
	.o_dump = 0,
	.o_verbose = API_MSG_NORMAL
};

typedef enum status {IDLE, BUSY} status_e;
enum mode_e {TEMPERATURE, TEMPERATURE_AND_HUMIDITY};

struct settings_t {
	struct tm start_tm_s;	/* Date and time of recorded data. */
	uint32_t nrec;		/* Number of records. */
	mode_t mode;		/* Temperature, temperature and humidity. */
	uint16_t interval;	/* 10 sec, 60 sec, 300 sec (5 min). */
};

void trim(char *str)
{
	char *p = strchr(str, '\r');
	if (p != NULL)
		*p = '\0';
}

char *mode_as_str(const mode_t mode)
{
	if (mode == TEMPERATURE)
		return("temperature");
	return("temperature and humidity");
}

void usage(const char *cmd_name, const int rc)
{
	fprintf(stdout, "usage: %s [options]\n"
		"\t-s, --status\n"
		"\t-r, --reset\n"
		"\t-c, --clear\n"
		"\t-f, --firmware\n"
		"\t-g, --gtime\n"
		"\t-t, --stime\n"
		"\t-e, --settings\n"
		"\t-u, --dump\n"
		"\t-h, --help\n"
		"\t-d, --device <string> [default: /dev/ttyUSB0]\n"
		"\t-i, --interval {10, 60, 300} secs\n"
		"\t-m, --mode {0, 1}, 0: temperature, 1: temperature and humidity\n"
		"\t-v, --verbose {error, warn, message, info, debug} [default: message]\n"
		"version: %s © 2017 by Thomas Stibor <thomas@stibor.net>\n",
		cmd_name, PACKAGE_VERSION);
	exit(rc);
}

int parseopts(int argc, char *argv[])
{
	struct option long_opts[] = {
		{"device",   required_argument, 0, 'd'},
		{"status",   no_argument,       0, 's'},
		{"reset",    no_argument,       0, 'r'},
		{"clear",    no_argument,       0, 'c'},
		{"firmware", no_argument,       0, 'f'},
		{"gtime",    no_argument,       0, 'g'},
		{"stime",    no_argument,       0, 't'},
		{"settings", no_argument,       0, 'e'},
		{"interval", required_argument, 0, 'i'},
		{"mode",     required_argument, 0, 'm'},
		{"dump",     no_argument,       0, 'u'},
		{"verbose",  required_argument, 0, 'v'},
		{"help",     no_argument,       0, 'h'},
		{0, 0, 0, 0}
	};
	int c;
	while ((c = getopt_long(argc, argv, "d:srcfgtei:m:uv:h",
				long_opts, NULL)) != -1) {
		switch (c) {
		case 'd': {
			strncpy(opt.o_device, optarg, MAX_DEVICE_LENGTH);
			break;
		}
		case 's': {
			opt.o_status = 1;
			break;
		}
		case 'r': {
			opt.o_reset = 1;
			break;
		}
		case 'c': {
			opt.o_clear = 1;
			break;
		}
		case 'f': {
			opt.o_firmware = 1;
			break;
		}
		case 'g': {
			opt.o_gtime = 1;
			break;
		}
		case 't': {
			opt.o_stime = 1;
			break;
		}
		case 'e': {
			opt.o_settings = 1;
			break;
		}
		case 'i': {
			opt.o_interval = atoi(optarg);
			break;
		}
		case 'm': {
			opt.o_mode = atoi(optarg);
			break;
		}
		case 'u': {
			opt.o_dump = 1;
			break;
		}
		case 'v': {
			if (OPTNCMP("error", optarg))
				opt.o_verbose = API_MSG_ERROR;
			else if (OPTNCMP("warn", optarg))
				opt.o_verbose = API_MSG_WARN;
			else if (OPTNCMP("message", optarg))
				opt.o_verbose = API_MSG_NORMAL;
			else if (OPTNCMP("info", optarg))
				opt.o_verbose = API_MSG_INFO;
			else if (OPTNCMP("debug", optarg))
				opt.o_verbose = API_MSG_DEBUG;
			else {
				fprintf(stdout, "wrong argument for -v, "
					"--verbose '%s'\n", optarg);
				usage(argv[0], -1);
			}
			api_msg_set_level(opt.o_verbose);
			break;
		}
		case 'h': {
			usage(argv[0], 0);
			break;
		}
		case 0: {
			break;
		}
		default:
			return -EINVAL;
		}
	}

	if (!opt.o_device[0])
		strncpy(opt.o_device, DEFAULT_DEVICE, MAX_DEVICE_LENGTH);

	if (argc < 2)
		usage(argv[0], -1);

	return 0;
}

int open_device(const char *device)
{
	int fd = -1;
	int rc;

	fd = open(device, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		C_ERROR(errno, "open '%s'", device);
		return -errno;
	}
	struct termios termios_s;
	memset(&termios_s, 0, sizeof(termios_s));
	rc = tcgetattr(fd, &termios_s);
	if (rc) {
		C_ERROR(errno, "tcgetattr");
		close(fd);
		return -errno;
	}

	rc = cfsetispeed(&termios_s, B115200);
	if (rc) {
		C_ERROR(errno, "tcgetattr");
		close(fd);
		return -errno;
	}
	rc = cfsetospeed(&termios_s, B115200);
	if (rc) {
		C_ERROR(errno, "tcgetattr");
		close(fd);
		return -errno;
	}

	termios_s.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);
	termios_s.c_oflag &= ~(OPOST);
	termios_s.c_iflag &= ~(INLCR | ICRNL | IXON | IXOFF | IXANY | IMAXBEL);
	termios_s.c_cflag &= ~(CSIZE | PARENB);
	termios_s.c_cflag |= CS8;
	termios_s.c_cc[VMIN]  = 0;
	termios_s.c_cc[VTIME] = 1;

	rc = tcsetattr(fd, TCSANOW, &termios_s);
	if (rc) {
		C_ERROR(errno, "tcsetattr");
		close(fd);
		return -errno;
	}

	return fd;
}

int tfd_write(const int fd, const char *buf, const size_t len)
{
	ssize_t s = 0;
	for (size_t l = 0; l < len; l++) {
		s = write(fd, &buf[l], 1);
		C_DEBUG("[fd=%d,s=%d] write ('%d','%c')",
			fd, s, buf[l], buf[l]);
		if (s < 1) {
			C_ERROR(errno, "write");
			return -errno;
		}
	}

	return 0;
}

int tfd_read(const int fd, char *buf, const size_t len)
{
	ssize_t s = 0;
	char b = 0;
	size_t c = 0;

	do {
		if (len > 0 && c++ == len)
			break;

		s = read(fd, &b, 1);
		C_DEBUG("[fd=%d,s=%d] read ('%d','%c')", fd, s, b, b);
		if (s < 0) {
			C_ERROR(errno, "read");
			return -errno;
		}
		*buf++ = b;
	} while (s != 0 && b != '\n');

	return 0;
}

int tfd_reset(const int fd)
{
	return tfd_write(fd, "X", 1);
}

int tfd_clear(const int fd)
{
	return tfd_write(fd, "R", 1);
}

int tfd_version(const int fd, char *version)
{
	char data[MAX_LENGTH] = {0};

	TFD_WRITE(fd, "v", 1);
	TFD_READ(fd, data, 0);

	trim((char *)data);
	strncpy(version, (char *)data, MAX_LENGTH);

	return 0;
}

int tfd_status(const int fd, status_e *status)
{
	char data = 0;

	TFD_WRITE(fd, "a", 1);
	TFD_READ(fd, &data, 1);
	if (data != 'a')
		return -EINVAL;

	TFD_READ(fd, &data, 1);

	*status = data == '1' ? BUSY : IDLE;

	return 0;
}

int tfd_gtime(const int fd, struct tm *tm_s)
{
	char *data = calloc(MAX_LENGTH, sizeof(char));
	if (!data) {
		C_ERROR(errno, "calloc");
		return -errno;
	}

	TFD_WRITE(fd, "o", 1);
	TFD_READ(fd, data, 25);
	if (data[0] != 'o') {
		if (data)
			free(data);
		return -EINVAL;
	}
	strptime(data + 8, "%d.%m.%y %H:%M:%S", tm_s);

	if (data)
		free(data);

	return 0;
}

int tfd_stime(const int fd)
{
	time_t cur_time_t;
	struct tm *tm_s = NULL;

	time (&cur_time_t);
	tm_s = localtime(&cur_time_t);

	char time_data[MAX_LENGTH] = {0};
	strftime(time_data, MAX_LENGTH, "%02d.%02m.%02y %02H:%02M:%02S", tm_s);

	TFD_WRITE(fd, "T", 1);
	TFD_WRITE(fd, time_data, strlen(time_data));

	char resp = 0;
	TFD_READ(fd, &resp, 1);
	if (resp != 'T')
		return -EINVAL;

	return 0;
}

int tfd_settings(const int fd, struct settings_t *settings)
{
	char *time_data = calloc(MAX_LENGTH, sizeof(char));
	if (!time_data) {
		C_ERROR(errno, "calloc");
		return -errno;
	}

	TFD_WRITE(fd, "d", 1);
	TFD_READ(fd, (char *)time_data, 25);
	if (time_data[0] != 'd') {
		if (time_data)
			free(time_data);
		return -EINVAL;
	}

	char nrec[7] = {0};
	snprintf(nrec, 7, "%s", time_data + 1);
	settings->nrec = atoi(nrec);

	strptime(time_data + 8, "%d.%m.%y %H:%M:%S", &settings->start_tm_s);

	if (time_data)
		free(time_data);

	char *remain_data = calloc(MAX_LENGTH, sizeof(char));
	if (!remain_data) {
		C_ERROR(errno, "calloc");
		return -errno;
	}
	TFD_WRITE(fd, "o", 1);
	TFD_READ(fd, remain_data, 25);
	if (remain_data[0] != 'o') {
		if (remain_data)
			free(remain_data);
		return -EINVAL;
	}

	char mode = 0;
	snprintf(&mode, 2, "%s", remain_data + 2);
	settings->mode = mode == '0' ? TEMPERATURE : TEMPERATURE_AND_HUMIDITY;

	char interval = 0;
	snprintf(&interval, 2, "%s", remain_data + 5);
	if (interval == '0')
		settings->interval = 10;
	else if (interval == '1')
		settings->interval = 60;
	else if (interval == '2')
		settings->interval = 300;
	else {
		if (remain_data)
			free(remain_data);
		return -EINVAL;
	}

	if (remain_data)
		free(remain_data);

	return 0;
}

int tfd_interval(const int fd, const uint16_t interval)
{
	char data;

	switch (interval) {
	case 10: {
		data = '0';
		break;
	}
	case 60: {
		data = '1';
		break;
	}
	case 300: {
		data = '2';
		break;
	}
	default:
		return -EINVAL;
	}

	TFD_WRITE(fd, "I", 1);
	TFD_WRITE(fd, &data, 1);

	TFD_READ(fd, &data, 1);
	if (data != 'I')
		return -EINVAL;

	return 0;
}

int tfd_mode(const int fd, const int mode)
{
	if (!(mode == 0 || mode == 1))
		return -EINVAL;

	char data;
	data = mode == 0 ? '0' : '1';
	TFD_WRITE(fd, "C", 1);
	TFD_WRITE(fd, &data, 1);

	TFD_READ(fd, &data, 1);
	if (data != 'C')
		return -EINVAL;

	return 0;
}

int tfd_dump(const int fd)
{
	int rc;
	struct settings_t settings;
	memset(&settings, 0, sizeof(settings));

	rc = tfd_settings(fd, &settings);
	if (rc) {
		C_ERROR(EINVAL, "tfd_settings");
		return -EINVAL;
	}
	time_t start_time = timegm(&settings.start_tm_s);

	char cmd[5]		  = {0};
	uint32_t	 lineno	  = 0;
	char str_time[MAX_LENGTH] = {0};
	struct tm	*tm_s	  = NULL;
	unsigned char	*data	  = calloc(256, sizeof(unsigned char));
	if (!data)
		return -errno;

	const uint32_t	nblocks	      = settings.nrec / (float)85;
	const uint32_t	remain_blocks = settings.nrec % 85;
	uint16_t	I	      = 85 * 3;

	for (uint32_t b = 0; b <= nblocks; b++) {
		snprintf(cmd, 6, "F%04d", b);
		TFD_WRITE(fd, cmd, 5);

		char resp = 0;
		TFD_READ(fd, &resp, 1);
		if (resp != 'F') {
			if (data)
				free(data);
			return -EINVAL;
		}

		if (b == nblocks)
			I = remain_blocks * 3;

		memset(data, 0, sizeof(unsigned char) * (I + 1));
		TFD_READ(fd, (char *)data, I + 1);
		for (uint16_t i = 0; i < I; i += 3) {
			uint16_t temp = data[i] << 8 | data[i + 1];
			tm_s = gmtime(&start_time);
			strftime(str_time, MAX_LENGTH, "%d.%m.%Y %H:%M:%S",
				 tm_s);
			fprintf(stdout, "%d;%s;%.1f;%d\n",
				lineno++, str_time, temp / 10.0, data[i + 2]);
			start_time += settings.interval;
		}
	}

	if (data)
		free(data);

	return 0;
}

int main(int argc, char *argv[])
{
	int rc;

	api_msg_set_level(opt.o_verbose);
	rc = parseopts(argc, argv);
	if (rc) {
		C_WARN("try '%s --help' for more information", argv[0]);
		return -1;
	}

	int fd = -1;
	fd = open_device(opt.o_device);
	if (fd < 0)
		return -1;

	if (opt.o_reset) {
		rc = tfd_reset(fd);
		if (rc)
			C_WARN("factory reset failed");
		else
			fprintf(stdout, "factory reset was successful\n");
	}
	if (opt.o_clear) {
		rc = tfd_clear(fd);
		if (rc)
			C_WARN("clearing flash memory failed");
		else
			fprintf(stdout, "flash memory successfully cleared\n");
	}
	if (opt.o_firmware) {
		char ver[MAX_LENGTH] = {0};
		rc = tfd_version(fd, ver);
		if (rc)
			C_WARN("get firmware version failed");
		else
			fprintf(stdout, "%s\n", ver);
	}
	if (opt.o_status) {
		status_e status;
		rc = tfd_status(fd, &status);
		if (rc)
			C_WARN("status failed");
		else
			fprintf(stdout, "%s\n",
				status == IDLE ? "idle" : "busy");
	}
	if (opt.o_gtime) {
		struct tm tm_s = {0};
		rc = tfd_gtime(fd, &tm_s);
		if (rc)
			C_WARN("get time failed");
		else
			fprintf(stdout, "%s", asctime(&tm_s));
	}
	if (opt.o_stime) {
		rc = tfd_stime(fd);
		if (rc)
			C_WARN("set time failed");
		else
			fprintf(stdout, "set time was successful\n");
	}
	if (opt.o_settings) {
		status_e status;
		rc = tfd_status(fd, &status);
		if (rc)
			C_WARN("status failed");

		struct settings_t settings;
		memset(&settings, 0, sizeof(settings));
		rc = tfd_settings(fd, &settings);
		if (rc)
			C_WARN("settings failed");
		else {
			fprintf(stdout,
				"status     : %s\n"
				"start      : %s"
				"interval   : %d secs\n"
				"mode       : %s\n"
				"num records: %d\n",
				status == IDLE ? "not recording" : "recording",
				asctime(&settings.start_tm_s),
				settings.interval,
				mode_as_str(settings.mode),
				settings.nrec);
		}
	}
	if (opt.o_interval) {
		rc = tfd_interval(fd, opt.o_interval);
		if (rc)
			C_WARN("set interval failed");
		else
			fprintf(stdout, "set interval was successful\n");
 	}
	if (opt.o_mode != -1) {
		rc = tfd_mode(fd, opt.o_mode);
		if (rc)
			C_WARN("set mode failed");
		else
			fprintf(stdout, "set mode was successful\n");
	}
	if (opt.o_dump) {
		rc = tfd_dump(fd);
		if (rc)
			C_WARN("dump failed");
	}

	close(fd);
	return rc;
}
