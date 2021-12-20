/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus_message.c  Utility program to send messages from the command line
 *
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * Originally dbus-send.c from the dbus package. It has been heavily modified
 * to work within the regression test framework.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dbus_common.h"

DBusConnection *connection;
DBusError error;
DBusBusType type = DBUS_BUS_SESSION;
const char *type_str = NULL;
const char *name = NULL;
int message_type = DBUS_MESSAGE_TYPE_INVALID;
const char *address = NULL;
int session_or_system = FALSE;
int log_fd = -1;

static void usage(int ecode)
{
	char *prefix = ecode ? "FAIL: " : "";

	fprintf(stderr,
		"%6sUsage: dbus_unrequested_reply [ADDRESS] --name=NAME --type=TYPE\n"
		"    ADDRESS\t\t--system, --session (default), or --address=ADDR\n"
		"    NAME\t\tthe message destination\n"
		"    TYPE\t\tmethod_return or error\n",
		prefix);
	exit(ecode);
}

static int do_unrequested_reply(void)
{
	DBusMessage *message;

	if (message_type == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		message = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_RETURN);

		if (message) {
			dbus_message_set_no_reply(message, TRUE);

			/* Make up an invalid reply_serial */
			if (!dbus_message_set_reply_serial(message,
							   123456789)) {
				fprintf(stderr,
					"FAIL: Couldn't set reply_serial\n");
				return 1;
			}
		}
	} else if (message_type == DBUS_MESSAGE_TYPE_ERROR) {
		message = dbus_message_new(DBUS_MESSAGE_TYPE_ERROR);

		if (message) {
			dbus_message_set_no_reply(message, TRUE);

			/* Make up an invalid reply_serial */
			if (!dbus_message_set_reply_serial(message,
							   123456789)) {
				fprintf(stderr,
					"FAIL: Couldn't set reply_serial\n");
				return 1;
			}

			/* Make up an error */
			if (!dbus_message_set_error_name(message,
					DBUS_ERROR_PROPERTY_READ_ONLY)) {
				fprintf(stderr,
					"FAIL: Couldn't set error name\n");
				return 1;
			}
		}
	} else {
		fprintf(stderr, "FAIL: Internal error, unknown message type\n");
		return 1;
	}

	if (message == NULL) {
		fprintf(stderr, "FAIL: Couldn't allocate D-Bus message\n");
		return 1;
	}

	if (!dbus_message_set_destination(message, name)) {
		fprintf(stderr, "FAIL: Not enough memory\n");
		return 1;
	}

	log_message(log_fd, "sent ", message);
	dbus_connection_send(connection, message, NULL);
	dbus_connection_flush(connection);

	dbus_message_unref(message);

	return 0;
}

int main(int argc, char *argv[])
{
	int i, rc;

	if (argc < 3)
		usage(1);

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "--system") == 0) {
			type = DBUS_BUS_SYSTEM;
			session_or_system = TRUE;
		} else if (strcmp(arg, "--session") == 0) {
			type = DBUS_BUS_SESSION;
			session_or_system = TRUE;
		} else if (strstr(arg, "--address") == arg) {
			address = strchr(arg, '=');

			if (!address || !++address) {
				fprintf(stderr,
					"FAIL: \"--address=\" requires an ADDRESS\n");
				usage(1);
			}
		} else if (strstr(arg, "--name=") == arg) {
			name = strchr(arg, '=');

			if (!name || !++name) {
				fprintf(stderr,
					"FAIL: \"--name=\" requires a NAME\n");
				usage(1);
			}
		} else if (strstr(arg, "--type=") == arg) {
			type_str = strchr(arg, '=');

			if (!type_str || !++type_str) {
				fprintf(stderr,
					"FAIL: \"--type=\" requires a TYPE\n");
				usage(1);
			}
		} else if (strstr(arg, "--log=") == arg) {
			char *path = strchr(arg, '=');

			if (!path || !++path) {
				fprintf(stderr,
					"FAIL: \"--log=\" requires a PATH\n");
				usage(1);
			}

			log_fd = open(path, O_CREAT | O_TRUNC | O_WRONLY,
				      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
				      S_IROTH | S_IWOTH);
			if (log_fd < 0) {
				fprintf(stderr,
					"FAIL: Couldn't open log file \"%s\": %m\n",
					path);
				exit(1);
			}
		} else if (!strcmp(arg, "--help"))
			usage(0);
		else if (arg[0] == '-')
			usage(1);
		else
			usage(1);
	}

	if (!name)
		usage(1);

	if (!type_str) {
		usage(1);
	} else {
		message_type = dbus_message_type_from_string(type_str);
		if (message_type != DBUS_MESSAGE_TYPE_METHOD_RETURN &&
		    message_type != DBUS_MESSAGE_TYPE_ERROR) {
			fprintf(stderr,
				"FAIL: Message type \"%s\" is not supported\n",
				type_str);
			exit(1);
		}
	}

	if (session_or_system && address != NULL) {
		fprintf(stderr,
			"FAIL: \"--address\" may not be used with \"--system\" or \"--session\"\n");
		usage(1);
	}

	dbus_error_init(&error);

	if (address != NULL)
		connection = dbus_connection_open(address, &error);
	else
		connection = dbus_bus_get(type, &error);

	if (connection == NULL) {
		fprintf(stderr,
			"FAIL: Failed to open connection to \"%s\" message bus: %s\n",
			(address !=
			 NULL) ? address : ((type ==
					     DBUS_BUS_SYSTEM) ? "system" :
					    "session"), error.message);
		dbus_error_free(&error);
		exit(1);
	} else if (address != NULL)
		dbus_bus_register(connection, &error);

	rc = do_unrequested_reply();
	dbus_connection_unref(connection);
	if (rc == 0)
		printf("PASS\n");

	exit(rc);
}
