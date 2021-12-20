/*
 * Copyright 2014 Canonical Ltd.
 *
 * The libapparmor library is licensed under the terms of the GNU
 * Lesser General Public License, version 2.1. Please see the file
 * COPYING.LGPL.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AA_PRIVATE_H
#define _AA_PRIVATE_H 1

#include <dirent.h>
#include <stdbool.h>
#include <sys/apparmor_private.h>

#define autofree __attribute((cleanup(_aa_autofree)))
#define autoclose __attribute((cleanup(_aa_autoclose)))
#define autofclose __attribute((cleanup(_aa_autofclose)))
#define unused __attribute__ ((unused))

#define asprintf _aa_asprintf

#if ENABLE_DEBUG_OUTPUT

#define PERROR(fmt, args...)	print_error(true, "libapparmor", fmt, ## args)
#define PDEBUG(fmt, args...)	print_debug("libapparmor: " fmt, ## args)

#else /* ENABLE_DEBUG_OUTPUT */

#define PERROR(fmt, args...)	print_error(false, "libapparmor", fmt, ## args)
#define PDEBUG(fmt, args...)	/* do nothing */

#endif /* ENABLE_DEBUG_OUTPUT */

#define MY_TEST(statement, error)               \
	if (!(statement)) {                     \
		fprintf(stderr, "FAIL: %s\n", error); \
		rc = 1; \
	}

void print_error(bool honor_env_var, const char *ident, const char *fmt, ...);
void print_debug(const char *fmt, ...);

void atomic_inc(unsigned int *v);
bool atomic_dec_and_test(unsigned int *v);

int _aa_dirat_for_each2(int dirfd, const char *name, void *data,
			int (* cb)(int, const struct dirent *, void *));

#endif /* _AA_PRIVATE_H */
