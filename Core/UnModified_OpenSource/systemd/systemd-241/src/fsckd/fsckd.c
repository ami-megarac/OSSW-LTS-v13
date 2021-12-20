/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2015 Canonical

  Author:
    Didier Roche <didrocks@ubuntu.com>

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <getopt.h>
#include <errno.h>
#include <libintl.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "sd-daemon.h"
#include "build.h"
#include "def.h"
#include "sd-event.h"
#include "log.h"
#include "list.h"
#include "macro.h"
#include "socket-util.h"
#include "fd-util.h"
#include "string-util.h"
#include "io-util.h"
#include "util.h"
#include "alloc-util.h"
#include "locale-util.h"

#define FSCKD_SOCKET_PATH "/run/systemd/fsck.progress"
#define IDLE_TIME_SECONDS 30
#define PLYMOUTH_REQUEST_KEY "K\2\2\3"
#define CLIENTS_MAX 128

struct Manager;

typedef struct Client {
        struct Manager *manager;
        char *device_name;
        /* device id refers to "fd <fd>" until it gets a name as "device_name" */
        char *device_id;

        pid_t fsck_pid;
        FILE *fsck_f;

        size_t cur;
        size_t max;
        int pass;

        double percent;

        bool cancelled;
        bool bad_input;

        sd_event_source *event_source;

        LIST_FIELDS(struct Client, clients);
} Client;

typedef struct Manager {
        sd_event *event;

        LIST_HEAD(Client, clients);
        unsigned n_clients;

        size_t clear;

        int connection_fd;
        sd_event_source *connection_event_source;

        bool show_status_console;

        double percent;
        int numdevices;

        int plymouth_fd;
        sd_event_source *plymouth_event_source;
        bool plymouth_cancel_sent;

        bool cancel_requested;
} Manager;

static void client_free(Client *c);
static void manager_free(Manager *m);

DEFINE_TRIVIAL_CLEANUP_FUNC(Client*, client_free);
DEFINE_TRIVIAL_CLEANUP_FUNC(Manager*, manager_free);

static int manager_write_console(Manager *m, const char *message) {
        _cleanup_fclose_ FILE *console = NULL;
        int l;
        size_t j;

        assert(m);

        if (!m->show_status_console)
                return 0;

        /* Reduce the SAK window by opening and closing console on every request */
        console = fopen("/dev/console", "we");
        if (!console)
                return -errno;

        if (message) {
                fprintf(console, "\r%s\r%n", message, &l);
                if (m->clear  < (size_t)l)
                        m->clear = (size_t)l;
        } else {
                fputc('\r', console);
                for (j = 0; j < m->clear; j++)
                        fputc(' ', console);
                fputc('\r', console);
        }
        fflush(console);

        return 0;
}

static double compute_percent(int pass, size_t cur, size_t max) {
        /* Values stolen from e2fsck */

        static const double pass_table[] = {
                0, 70, 90, 92, 95, 100
        };

        if (pass <= 0)
                return 0.0;

        if ((unsigned) pass >= ELEMENTSOF(pass_table) || max == 0)
                return 100.0;

        return pass_table[pass-1] +
                (pass_table[pass] - pass_table[pass-1]) *
                (double) cur / max;
}

static int client_request_cancel(Client *c) {
        assert(c);

        if (c->cancelled)
                return 0;

        log_info("Request to cancel fsck for %s from fsckd", c->device_id);
        if (kill(c->fsck_pid, SIGTERM) < 0) {
                /* ignore the error and consider that cancel was sent if fsck just exited */
                if (errno != ESRCH)
                        return log_error_errno(errno, "Cannot send cancel to fsck for %s: %m", c->device_id);
        }

        c->cancelled = true;
        return 1;
}

static void client_free(Client *c) {
        assert(c);

        if (c->manager) {
                LIST_REMOVE(clients, c->manager->clients, c);
                c->manager->n_clients--;
        }

        sd_event_source_unref(c->event_source);
        fclose(c->fsck_f);
        if (c->device_name)
                free(c->device_name);
        if (c->device_id)
                free(c->device_id);
        free(c);
}

static void manager_disconnect_plymouth(Manager *m) {
        assert(m);

        m->plymouth_event_source = sd_event_source_unref(m->plymouth_event_source);
        m->plymouth_fd = safe_close(m->plymouth_fd);
        m->plymouth_cancel_sent = false;
}

static int manager_plymouth_feedback_handler(sd_event_source *s, int fd, uint32_t revents, void *userdata) {
        Manager *m = userdata;
        Client *current;
        char buffer[6];
        ssize_t l;

        assert(m);

        l = read(m->plymouth_fd, buffer, sizeof(buffer));
        if (l < 0) {
                log_warning_errno(errno, "Got error while reading from plymouth: %m");
                manager_disconnect_plymouth(m);
                return -errno;
        }
        if (l == 0) {
                manager_disconnect_plymouth(m);
                return 0;
        }

        if (l > 1 && buffer[0] == '\15')
                log_error("Message update to plymouth wasn't delivered successfully");

        /* the only answer support type we requested is a key interruption */
        if (l > 2 && buffer[0] == '\2' && buffer[5] == '\3') {
                m->cancel_requested = true;

                /* cancel all connected clients */
                LIST_FOREACH(clients, current, m->clients)
                        client_request_cancel(current);
        }

        return 0;
}

static int manager_connect_plymouth(Manager *m) {
        union sockaddr_union sa = PLYMOUTH_SOCKET;
        int r;

        if (!plymouth_running())
                return 0;

        /* try to connect or reconnect if sending a message */
        if (m->plymouth_fd >= 0)
                return 1;

        m->plymouth_fd = socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0);
        if (m->plymouth_fd < 0)
                return log_warning_errno(errno, "Connection to plymouth socket failed: %m");

        if (connect(m->plymouth_fd, &sa.sa, offsetof(struct sockaddr_un, sun_path) + 1 + strlen(sa.un.sun_path+1)) < 0) {
                r = log_warning_errno(errno, "Couldn't connect to plymouth: %m");
                goto fail;
        }

        r = sd_event_add_io(m->event, &m->plymouth_event_source, m->plymouth_fd, EPOLLIN, manager_plymouth_feedback_handler, m);
        if (r < 0) {
                log_warning_errno(r, "Can't listen to plymouth socket: %m");
                goto fail;
        }

        return 1;

fail:
        manager_disconnect_plymouth(m);
        return r;
}

static int plymouth_send_message(int plymouth_fd, const char *message, bool update) {
        _cleanup_free_ char *packet = NULL;
        int n;
        char mode = 'M';

        if (update)
                mode = 'U';

        if (asprintf(&packet, "%c\002%c%s%n", mode, (int) (strlen(message) + 1), message, &n) < 0)
                return log_oom();

        return loop_write(plymouth_fd, packet, n + 1, true);
}

static int manager_send_plymouth_message(Manager *m, const char *message) {
        const char *plymouth_cancel_message = NULL, *l10n_cancel_message = NULL;
        int r;

        r = manager_connect_plymouth(m);
        if (r < 0)
                return r;
        /* 0 means that plymouth isn't running, do not send any message yet */
        else if (r == 0)
                return 0;

        if (!m->plymouth_cancel_sent) {

                /* Indicate to plymouth that we listen to Ctrl+C */
                r = loop_write(m->plymouth_fd, PLYMOUTH_REQUEST_KEY, sizeof(PLYMOUTH_REQUEST_KEY), true);
                if (r < 0)
                        return log_warning_errno(r, "Can't send to plymouth cancel key: %m");

                m->plymouth_cancel_sent = true;

                l10n_cancel_message = _("Press Ctrl+C to cancel all filesystem checks in progress");
                plymouth_cancel_message = strjoina("fsckd-cancel-msg:", l10n_cancel_message);

                r = plymouth_send_message(m->plymouth_fd, plymouth_cancel_message, false);
                if (r < 0)
                        log_warning_errno(r, "Can't send filesystem cancel message to plymouth: %m");

        } else if (m->numdevices == 0) {

                m->plymouth_cancel_sent = false;

                r = plymouth_send_message(m->plymouth_fd, "", false);
                if (r < 0)
                        log_warning_errno(r, "Can't clear plymouth filesystem cancel message: %m");
        }

        r = plymouth_send_message(m->plymouth_fd,  message, true);
        if (r < 0)
                return log_warning_errno(r, "Couldn't send \"%s\" to plymouth: %m", message);

        return 0;
}

static int manager_update_global_progress(Manager *m) {
        Client *current = NULL;
        _cleanup_free_ char *console_message = NULL;
        _cleanup_free_ char *fsck_message = NULL;
        int current_numdevices = 0, r;
        double current_percent = 100;

        /* get the overall percentage */
        LIST_FOREACH(clients, current, m->clients) {
                current_numdevices++;

                /* right now, we only keep the minimum % of all fsckd processes. We could in the future trying to be
                   linear, but max changes and corresponds to the pass. We have all the informations into fsckd
                   already if we can treat that in a smarter way. */
                current_percent = MIN(current_percent, current->percent);
        }

        /* update if there is anything user-visible to update */
        if (fabs(current_percent - m->percent) > 0.001 || current_numdevices != m->numdevices) {
                m->numdevices = current_numdevices;
                m->percent = current_percent;

                if (asprintf(&console_message,
                             ngettext("Checking in progress on %d disk (%3.1f%% complete)",
                                      "Checking in progress on %d disks (%3.1f%% complete)", m->numdevices),
                                      m->numdevices, m->percent) < 0)
                        return -ENOMEM;

                if (asprintf(&fsck_message, "fsckd:%d:%3.1f:%s", m->numdevices, m->percent, console_message) < 0)
                        return -ENOMEM;

                r = manager_write_console(m, console_message);
                if (r < 0)
                        return r;

                /* try to connect to plymouth and send message */
                r = manager_send_plymouth_message(m, fsck_message);
                if (r < 0)
                        return r;
        }
        return 0;
}

static int client_progress_handler(sd_event_source *s, int fd, uint32_t revents, void *userdata) {
        Client *client = userdata;
        char line[LINE_MAX];
        Manager *m;

        assert(client);
        m = client->manager;

        /* check first if we need to cancel this client */
        if (m->cancel_requested)
                client_request_cancel(client);

        while (fgets(line, sizeof(line), client->fsck_f) != NULL) {
                int pass;
                size_t cur, max;
                _cleanup_free_ char *device = NULL, *old_device_id = NULL;

                if (sscanf(line, "%i %zu %zu %ms", &pass, &cur, &max, &device) == 4) {
                        if (!client->device_name) {
                                client->device_name = strdup(device);
                                if (!client->device_name) {
                                        log_oom();
                                        continue;
                                }
                                old_device_id = client->device_id;
                                client->device_id = strdup(device);
                                if (!client->device_id) {
                                        log_oom();
                                        client->device_id = old_device_id;
                                        old_device_id = NULL;
                                        continue;
                                }
                        }
                        client->pass = pass;
                        client->cur = cur;
                        client->max = max;
                        client->bad_input = false;
                        client->percent = compute_percent(client->pass, client->cur, client->max);
                        log_debug("Getting progress for %s (%zu, %zu, %d) : %3.1f%%", client->device_id,
                                  client->cur, client->max, client->pass, client->percent);
                } else {
                        if (errno == ENOMEM) {
                                log_oom();
                                continue;
                        }

                        /* if previous input was already garbage, kick it off from progress report */
                        if (client->bad_input) {
                                log_warning("Closing connection on incorrect input of fsck connection for %s", client->device_id);
                                client_free(client);
                                manager_update_global_progress(m);
                                return 0;
                        }
                        client->bad_input = true;
                }

        }

        if (feof(client->fsck_f)) {
                log_debug("Fsck client %s disconnected", client->device_id);
                client_free(client);
        }

        manager_update_global_progress(m);
        return 0;
}

static int manager_new_connection_handler(sd_event_source *s, int fd, uint32_t revents, void *userdata) {
        _cleanup_(client_freep) Client *c = NULL;
        _cleanup_close_ int new_fsck_fd = -1;
        _cleanup_fclose_ FILE *new_fsck_f = NULL;
        struct ucred ucred = {};
        Manager *m = userdata;
        int r;

        assert(m);

        /* Initialize and list new clients */
        new_fsck_fd = accept4(m->connection_fd, NULL, NULL, SOCK_CLOEXEC|SOCK_NONBLOCK);
        if (new_fsck_fd < 0) {
                log_error_errno(errno, "Couldn't accept a new connection: %m");
                return 0;
        }

        if (m->n_clients >= CLIENTS_MAX) {
                log_error("Too many clients, refusing connection.");
                return 0;
        }


        new_fsck_f = fdopen(new_fsck_fd, "r");
        if (!new_fsck_f) {
                log_error_errno(errno, "Couldn't fdopen new connection for fd %d: %m", new_fsck_fd);
                return 0;
        }
        new_fsck_fd = -1;

        r = getpeercred(fileno(new_fsck_f), &ucred);
        if (r < 0) {
                log_error_errno(r, "Couldn't get credentials for fsck: %m");
                return 0;
        }

        c = new0(Client, 1);
        if (!c) {
                log_oom();
                return 0;
        }

        c->fsck_pid = ucred.pid;
        c->fsck_f = new_fsck_f;
        new_fsck_f = NULL;

        if (asprintf(&(c->device_id), "fd %d", fileno(c->fsck_f)) < 0) {
                log_oom();
                return 0;
        }

        r = sd_event_add_io(m->event, &c->event_source, fileno(c->fsck_f), EPOLLIN, client_progress_handler, c);
        if (r < 0) {
                log_oom();
                return 0;
        }

        LIST_PREPEND(clients, m->clients, c);
        m->n_clients++;
        c->manager = m;

        log_debug("New fsck client connected: %s", c->device_id);

        /* only request the client to cancel now in case the request is dropped by the client (chance to recancel) */
        if (m->cancel_requested)
                client_request_cancel(c);

        c = NULL;
        return 0;
}

static void manager_free(Manager *m) {
        if (!m)
                return;

        /* clear last line */
        manager_write_console(m, NULL);

        sd_event_source_unref(m->connection_event_source);
        safe_close(m->connection_fd);

        while (m->clients)
                client_free(m->clients);

        manager_disconnect_plymouth(m);

        sd_event_unref(m->event);

        free(m);
}

static int manager_new(Manager **ret, int fd) {
        _cleanup_(manager_freep) Manager *m = NULL;
        int r;

        assert(ret);

        m = new0(Manager, 1);
        if (!m)
                return -ENOMEM;

        m->plymouth_fd = -1;
        m->connection_fd = fd;
        m->percent = 100;

        r = sd_event_default(&m->event);
        if (r < 0)
                return r;

        if (access("/run/systemd/show-status", F_OK) >= 0)
                m->show_status_console = true;

        r = sd_event_add_io(m->event, &m->connection_event_source, fd, EPOLLIN, manager_new_connection_handler, m);
        if (r < 0)
                return r;

        *ret = m;
        m = NULL;

        return 0;
}

static int run_event_loop_with_timeout(Manager *m, usec_t timeout) {
        int r, code;
        sd_event *e = m->event;

        assert(e);

        for (;;) {
                r = sd_event_get_state(e);
                if (r < 0)
                        return r;
                if (r == SD_EVENT_FINISHED)
                        break;

                r = sd_event_run(e, timeout);
                if (r < 0)
                        return r;

                /* Exit if we reached the idle timeout and no more clients are
                   connected. If there is still an fsck process running but
                   simply slow to send us progress updates, exiting would mean
                   that this fsck process receives SIGPIPE resulting in an
                   aborted file system check. */
                if (r == 0 && m->n_clients == 0) {
                        sd_event_exit(e, 0);
                        break;
                }
        }

        r = sd_event_get_exit_code(e, &code);
        if (r < 0)
                return r;

        return code;
}

static void help(void) {
        printf("%s [OPTIONS...]\n\n"
               "Capture fsck progress and forward one stream to plymouth\n\n"
               "  -h --help             Show this help\n"
               "     --version          Show package version\n",
               program_invocation_short_name);
}

static int parse_argv(int argc, char *argv[]) {

        enum {
                ARG_VERSION = 0x100,
                ARG_ROOT,
        };

        static const struct option options[] = {
                { "help",      no_argument,       NULL, 'h'           },
                { "version",   no_argument,       NULL, ARG_VERSION   },
                {}
        };

        int c;

        assert(argc >= 0);
        assert(argv);

        while ((c = getopt_long(argc, argv, "hv", options, NULL)) >= 0)
                switch (c) {

                case 'h':
                        help();
                        return 0;

                case ARG_VERSION:
                        puts("systemd " GIT_VERSION);
                        puts(SYSTEMD_FEATURES);
                        return 0;

                case '?':
                        return -EINVAL;

                default:
                        assert_not_reached("Unhandled option");
                }

        if (optind < argc) {
                log_error("Extraneous arguments");
                return -EINVAL;
        }

        return 1;
}

int main(int argc, char *argv[]) {
        _cleanup_(manager_freep) Manager *m = NULL;
        int fd = -1;
        int r, n;

        log_set_target(LOG_TARGET_AUTO);
        log_parse_environment();
        log_open();
        init_gettext();

        r = parse_argv(argc, argv);
        if (r <= 0)
                goto finish;

        n = sd_listen_fds(0);
        if (n > 1) {
                log_error("Too many file descriptors received.");
                r = -EINVAL;
                goto finish;
        } else if (n == 1)
                fd = SD_LISTEN_FDS_START + 0;
        else {
                fd = make_socket_fd(LOG_DEBUG, FSCKD_SOCKET_PATH, SOCK_STREAM, SOCK_CLOEXEC);
                if (fd < 0) {
                        r = log_error_errno(fd, "Couldn't create listening socket fd on %s: %m", FSCKD_SOCKET_PATH);
                        goto finish;
                }
        }

        r = manager_new(&m, fd);
        if (r < 0) {
                log_error_errno(r, "Failed to allocate manager: %m");
                goto finish;
        }

        r = run_event_loop_with_timeout(m, IDLE_TIME_SECONDS * USEC_PER_SEC);
        if (r < 0) {
                log_error_errno(r, "Failed to run event loop: %m");
                goto finish;
        }

        sd_event_get_exit_code(m->event, &r);

finish:
        return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
