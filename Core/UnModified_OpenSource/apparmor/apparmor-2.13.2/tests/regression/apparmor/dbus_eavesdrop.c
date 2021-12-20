/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus_service.c  Utility program to attempt to eavesdrop on a bus
 *
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2013 Canonical, Ltd.
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
#include <signal.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dbus_common.h"

DBusConnection *connection = NULL;
DBusError error;
DBusBusType type = DBUS_BUS_SESSION;
const char *address = NULL;
int session_or_system = FALSE;

static void usage(void)
{
	fprintf(stderr, "Usage: dbus_eavesdrop [ADDRESS]\n\n"
		"    ADDRESS\t\t--system, --session (default), or --address=ADDR\n");
}

static int do_eavesdrop(void)
{
	dbus_bus_add_match(connection, "eavesdrop=true,type='method_call'",
			   &error);
	if (dbus_error_is_set(&error)) {
		fprintf(stderr, "FAIL: %s: %s\n", error.name, error.message);
		dbus_error_free(&error);
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int i, rc;

	if (argc < 2) {
		usage();
		rc = 1;
		goto out;
	}

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

			if (address == NULL) {
				fprintf(stderr,
					"FAIL: \"--address=\" requires an ADDRESS\n");
				usage();
				rc = 1;
				goto out;
			} else {
				address = address + 1;
			}
		} else if (!strcmp(arg, "--help")) {
			usage();
			rc = 0;
			goto out;
		} else {
			usage();
			rc = 1;
			goto out;
		}
	}

	if ((session_or_system == FALSE && address == NULL) || i < argc) {
		usage();
		rc = 1;
		goto out;
	}

	if (session_or_system && (address != NULL)) {
		fprintf(stderr,
			"FAIL: \"--address\" may not be used with \"--system\" or \"--session\"\n");
		usage();
		rc = 1;
		goto out;
	}

	dbus_error_init(&error);

	if (address != NULL)
		connection = dbus_connection_open(address, &error);
	else
		connection = dbus_bus_get(type, &error);

	if (connection == NULL) {
		fprintf(stderr,
			"FAIL: Failed to open connection to \"%s\" message bus: %s\n",
			address ? address :
				  ((type == DBUS_BUS_SYSTEM) ? "system" : "session"),
			error.message);
		dbus_error_free(&error);
		rc = 1;
		goto out;
	} else if (address != NULL)
		dbus_bus_register(connection, &error);

	rc = do_eavesdrop();

out:
	if (connection)
		dbus_connection_unref(connection);

	if (rc == 0)
		printf("PASS\n");

	exit(rc);
}
