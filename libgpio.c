/*
 * Copyright (C) 2020 GlobalLogic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/Log.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include "libgpio.h"

#define MAX_PATH_LENGTH 24

static int libgpio_open_fd(int chip, int line, int flags, int data)
{
	char path[MAX_PATH_LENGTH];
	struct gpiohandle_request req;
	int fd, ret;

	ret = snprintf(path, MAX_PATH_LENGTH, "/dev/gpiochip%d", chip);
	if (ret < 0) {
		ALOGE("%s: error determinig path", __func__);
		return -EINVAL;
	}

	fd = open(path, 0);
	if (fd < 0) {
		ALOGE("%s: error opening file %s %s (%d)", __func__, path,
				strerror(errno), errno);
		return -ENOENT;
	}

	req.lineoffsets[0] = line;
	req.flags = flags;
	if (flags & GPIOHANDLE_REQUEST_OUTPUT) {
		req.default_values[0] = data;
	}
	strcpy(req.consumer_label, "LIBGPIO");
	req.lines = 1;

	ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if (ret < 0) {
		ALOGE("%s: error opening line handle %s (%d)", __func__,
				strerror(errno), errno);
		return -errno;
	}
	return req.fd;
}

/**
 * Sets GPIO line to output mode with given value
 *
 * @param chip gpiochip number in sysfs
 * @param line offset of selected pin in a chip
 * @return 0 on success, negative value on error
 */

int libgpio_set_value(int chip, int line, enum GPIO_STATE value)
{
	int fd = libgpio_open_fd(chip, line, GPIOHANDLE_REQUEST_OUTPUT, value);
	if (fd <  0) {
		return fd;
	}
	close(fd);
	return 0;
}

/**
 * Sets GPIO line to input mode and read value
 *
 * @param chip gpiochip number in sysfs
 * @param line offset of selected pin in a chip
 * @return 0 if input is low, 1 if input is high,
 * negative value on error
 */
int libgpio_get_value(int chip, int line)
{
	struct gpiohandle_data data;
	int ret;
	int fd = libgpio_open_fd(chip, line, GPIOHANDLE_REQUEST_INPUT, 0);
	if (fd <  0) {
		return fd;
	}

	ret = ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);

	close(fd);
	if (ret < 0) {
		ALOGE("%s: error reading data from fd %d. %s(%d)", __func__, fd,
				strerror(errno), errno);
		return -errno;

	}

	return data.values[0];
}
