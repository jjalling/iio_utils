/* Industrialio event test code.
 *
 * Copyright (c) 2011-2012 Lars-Peter Clausen <lars@metafoo.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is primarily intended as an example application.
 * Reads the current buffer setup from sysfs and starts a short capture
 * from the specified device, pretty printing the result after appropriate
 * conversion.
 *
 * Usage:
 *	iio_event_monitor <device_name>
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "iio_utils.h"
#include <linux/iio/events.h>
#include <linux/iio/types.h>

static const char * const iio_chan_type_name_spec[] = {
	[IIO_VOLTAGE] = "voltage",
	[IIO_CURRENT] = "current",
	[IIO_POWER] = "power",
	[IIO_ACCEL] = "accel",
	[IIO_ANGL_VEL] = "anglvel",
	[IIO_MAGN] = "magn",
	[IIO_LIGHT] = "illuminance",
	[IIO_INTENSITY] = "intensity",
	[IIO_PROXIMITY] = "proximity",
	[IIO_TEMP] = "temp",
	[IIO_INCLI] = "incli",
	[IIO_ROT] = "rot",
	[IIO_ANGL] = "angl",
	[IIO_TIMESTAMP] = "timestamp",
	[IIO_CAPACITANCE] = "capacitance",
	[IIO_ALTVOLTAGE] = "altvoltage",
	[IIO_CCT] = "cct",
	[IIO_PRESSURE] = "pressure",
	[IIO_HUMIDITYRELATIVE] = "humidityrelative",
	[IIO_ACTIVITY] = "activity",
	[IIO_STEPS] = "steps",
	[IIO_ENERGY] = "energy",
	[IIO_DISTANCE] = "distance",
	[IIO_VELOCITY] = "velocity",
	[IIO_CONCENTRATION] = "concentration",
	[IIO_RESISTANCE] = "resistance",
#if 0
	[IIO_PH] = "ph",
	[IIO_UVINDEX] = "uvindex",
#endif
};

static const char * const iio_ev_type_text[] = {
	[IIO_EV_TYPE_THRESH] = "thresh",
	[IIO_EV_TYPE_MAG] = "mag",
	[IIO_EV_TYPE_ROC] = "roc",
	[IIO_EV_TYPE_THRESH_ADAPTIVE] = "thresh_adaptive",
	[IIO_EV_TYPE_MAG_ADAPTIVE] = "mag_adaptive",
	[IIO_EV_TYPE_CHANGE] = "change",
};

static const char * const iio_ev_dir_text[] = {
	[IIO_EV_DIR_EITHER] = "either",
	[IIO_EV_DIR_RISING] = "rising",
	[IIO_EV_DIR_FALLING] = "falling"
};

static const char * const iio_modifier_names[] = {
	[IIO_MOD_X] = "x",
	[IIO_MOD_Y] = "y",
	[IIO_MOD_Z] = "z",
	[IIO_MOD_X_AND_Y] = "x&y",
	[IIO_MOD_X_AND_Z] = "x&z",
	[IIO_MOD_Y_AND_Z] = "y&z",
	[IIO_MOD_X_AND_Y_AND_Z] = "x&y&z",
	[IIO_MOD_X_OR_Y] = "x|y",
	[IIO_MOD_X_OR_Z] = "x|z",
	[IIO_MOD_Y_OR_Z] = "y|z",
	[IIO_MOD_X_OR_Y_OR_Z] = "x|y|z",
	[IIO_MOD_LIGHT_BOTH] = "both",
	[IIO_MOD_LIGHT_IR] = "ir",
	[IIO_MOD_ROOT_SUM_SQUARED_X_Y] = "sqrt(x^2+y^2)",
	[IIO_MOD_SUM_SQUARED_X_Y_Z] = "x^2+y^2+z^2",
	[IIO_MOD_LIGHT_CLEAR] = "clear",
	[IIO_MOD_LIGHT_RED] = "red",
	[IIO_MOD_LIGHT_GREEN] = "green",
	[IIO_MOD_LIGHT_BLUE] = "blue",
#if 0
	[IIO_MOD_LIGHT_UV] = "uv",
#endif
	[IIO_MOD_QUATERNION] = "quaternion",
	[IIO_MOD_TEMP_AMBIENT] = "ambient",
	[IIO_MOD_TEMP_OBJECT] = "object",
	[IIO_MOD_NORTH_MAGN] = "from_north_magnetic",
	[IIO_MOD_NORTH_TRUE] = "from_north_true",
	[IIO_MOD_NORTH_MAGN_TILT_COMP] = "from_north_magnetic_tilt_comp",
	[IIO_MOD_NORTH_TRUE_TILT_COMP] = "from_north_true_tilt_comp",
	[IIO_MOD_RUNNING] = "running",
	[IIO_MOD_JOGGING] = "jogging",
	[IIO_MOD_WALKING] = "walking",
	[IIO_MOD_STILL] = "still",
	[IIO_MOD_ROOT_SUM_SQUARED_X_Y_Z] = "sqrt(x^2+y^2+z^2)",
	[IIO_MOD_I] = "i",
	[IIO_MOD_Q] = "q",
	[IIO_MOD_CO2] = "co2",
	[IIO_MOD_VOC] = "voc",
};

static bool event_is_known(struct iio_event_data *event)
{
	enum iio_chan_type type = IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(event->id);
	enum iio_modifier mod = IIO_EVENT_CODE_EXTRACT_MODIFIER(event->id);
	enum iio_event_type ev_type = IIO_EVENT_CODE_EXTRACT_TYPE(event->id);
	enum iio_event_direction dir = IIO_EVENT_CODE_EXTRACT_DIR(event->id);

	switch (type) {
	case IIO_VOLTAGE:
	case IIO_CURRENT:
	case IIO_POWER:
	case IIO_ACCEL:
	case IIO_ANGL_VEL:
	case IIO_MAGN:
	case IIO_LIGHT:
	case IIO_INTENSITY:
	case IIO_PROXIMITY:
	case IIO_TEMP:
	case IIO_INCLI:
	case IIO_ROT:
	case IIO_ANGL:
	case IIO_TIMESTAMP:
	case IIO_CAPACITANCE:
	case IIO_ALTVOLTAGE:
	case IIO_CCT:
	case IIO_PRESSURE:
	case IIO_HUMIDITYRELATIVE:
	case IIO_ACTIVITY:
	case IIO_STEPS:
	case IIO_ENERGY:
	case IIO_DISTANCE:
	case IIO_VELOCITY:
	case IIO_CONCENTRATION:
	case IIO_RESISTANCE:
#if 0
	case IIO_PH:
	case IIO_UVINDEX:
#endif
		break;
	default:
		return false;
	}

	switch (mod) {
	case IIO_NO_MOD:
	case IIO_MOD_X:
	case IIO_MOD_Y:
	case IIO_MOD_Z:
	case IIO_MOD_X_AND_Y:
	case IIO_MOD_X_AND_Z:
	case IIO_MOD_Y_AND_Z:
	case IIO_MOD_X_AND_Y_AND_Z:
	case IIO_MOD_X_OR_Y:
	case IIO_MOD_X_OR_Z:
	case IIO_MOD_Y_OR_Z:
	case IIO_MOD_X_OR_Y_OR_Z:
	case IIO_MOD_LIGHT_BOTH:
	case IIO_MOD_LIGHT_IR:
	case IIO_MOD_ROOT_SUM_SQUARED_X_Y:
	case IIO_MOD_SUM_SQUARED_X_Y_Z:
	case IIO_MOD_LIGHT_CLEAR:
	case IIO_MOD_LIGHT_RED:
	case IIO_MOD_LIGHT_GREEN:
	case IIO_MOD_LIGHT_BLUE:
#if 0
	case IIO_MOD_LIGHT_UV:
#endif
	case IIO_MOD_QUATERNION:
	case IIO_MOD_TEMP_AMBIENT:
	case IIO_MOD_TEMP_OBJECT:
	case IIO_MOD_NORTH_MAGN:
	case IIO_MOD_NORTH_TRUE:
	case IIO_MOD_NORTH_MAGN_TILT_COMP:
	case IIO_MOD_NORTH_TRUE_TILT_COMP:
	case IIO_MOD_RUNNING:
	case IIO_MOD_JOGGING:
	case IIO_MOD_WALKING:
	case IIO_MOD_STILL:
	case IIO_MOD_ROOT_SUM_SQUARED_X_Y_Z:
	case IIO_MOD_I:
	case IIO_MOD_Q:
	case IIO_MOD_CO2:
	case IIO_MOD_VOC:
		break;
	default:
		return false;
	}

	switch (ev_type) {
	case IIO_EV_TYPE_THRESH:
	case IIO_EV_TYPE_MAG:
	case IIO_EV_TYPE_ROC:
	case IIO_EV_TYPE_THRESH_ADAPTIVE:
	case IIO_EV_TYPE_MAG_ADAPTIVE:
	case IIO_EV_TYPE_CHANGE:
		break;
	default:
		return false;
	}

	switch (dir) {
	case IIO_EV_DIR_EITHER:
	case IIO_EV_DIR_RISING:
	case IIO_EV_DIR_FALLING:
	case IIO_EV_DIR_NONE:
		break;
	default:
		return false;
	}

	return true;
}

static void print_event(struct iio_event_data *event)
{
	enum iio_chan_type type = IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(event->id);
	enum iio_modifier mod = IIO_EVENT_CODE_EXTRACT_MODIFIER(event->id);
	enum iio_event_type ev_type = IIO_EVENT_CODE_EXTRACT_TYPE(event->id);
	enum iio_event_direction dir = IIO_EVENT_CODE_EXTRACT_DIR(event->id);
	int chan = IIO_EVENT_CODE_EXTRACT_CHAN(event->id);
	int chan2 = IIO_EVENT_CODE_EXTRACT_CHAN2(event->id);
	bool diff = IIO_EVENT_CODE_EXTRACT_DIFF(event->id);

	if (!event_is_known(event)) {
		fprintf(stderr, "Unknown event: time: %lld, id: %llx\n",
			event->timestamp, event->id);

		return;
	}

	printf("Event: time: %lld, type: %s", event->timestamp,
	       iio_chan_type_name_spec[type]);

	if (mod != IIO_NO_MOD)
		printf("(%s)", iio_modifier_names[mod]);

	if (chan >= 0) {
		printf(", channel: %d", chan);
		if (diff && chan2 >= 0)
			printf("-%d", chan2);
	}

	printf(", evtype: %s", iio_ev_type_text[ev_type]);

	if (dir != IIO_EV_DIR_NONE)
		printf(", direction: %s", iio_ev_dir_text[dir]);

	printf("\n");
}

int main(int argc, char **argv)
{
	struct iio_event_data event;
	const char *device_name;
	char *chrdev_name;
	int ret;
	int dev_num;
	int fd, event_fd;

	if (argc <= 1) {
		fprintf(stderr, "Usage: %s <device_name>\n", argv[0]);
		return -1;
	}

	device_name = argv[1];

	dev_num = find_type_by_name(device_name, "iio:device");
	if (dev_num >= 0) {
		printf("Found IIO device with name %s with device number %d\n",
		       device_name, dev_num);
		ret = asprintf(&chrdev_name, "/dev/iio:device%d", dev_num);
		if (ret < 0)
			return -ENOMEM;
	} else {
		/*
		 * If we can't find an IIO device by name assume device_name is
		 * an IIO chrdev
		 */
		chrdev_name = strdup(device_name);
		if (!chrdev_name)
			return -ENOMEM;
	}

	fd = open(chrdev_name, 0);
	if (fd == -1) {
		ret = -errno;
		fprintf(stderr, "Failed to open %s\n", chrdev_name);
		goto error_free_chrdev_name;
	}

	ret = ioctl(fd, IIO_GET_EVENT_FD_IOCTL, &event_fd);
	if (ret == -1 || event_fd == -1) {
		ret = -errno;
		if (ret == -ENODEV)
			fprintf(stderr,
				"This device does not support events\n");
		else
			fprintf(stderr, "Failed to retrieve event fd\n");
		if (close(fd) == -1)
			perror("Failed to close character device file");

		goto error_free_chrdev_name;
	}

	if (close(fd) == -1)  {
		ret = -errno;
		goto error_free_chrdev_name;
	}

	while (true) {
		ret = read(event_fd, &event, sizeof(event));
		if (ret == -1) {
			if (errno == EAGAIN) {
				fprintf(stderr, "nothing available\n");
				continue;
			} else {
				ret = -errno;
				perror("Failed to read event from device");
				break;
			}
		}

		if (ret != sizeof(event)) {
			fprintf(stderr, "Reading event failed!\n");
			ret = -EIO;
			break;
		}

		print_event(&event);
	}

	if (close(event_fd) == -1)
		perror("Failed to close event file");

error_free_chrdev_name:
	free(chrdev_name);

	return ret;
}
