#include <gpg-error.h>

/* Author: Daniel Kahn Gillmor <dkg@fifthhorseman.net>
 *
 * A simple test program that exercises a tiny piece of libgpg-error
 * (a.k.a. gpgrt).
 */


int main() {
  const char *gpgrt_version;
  gpg_error_t err;

  gpgrt_version = gpgrt_check_version (GPGRT_VERSION);
  gpgrt_printf ("gpgrt version %s\n", gpgrt_version);

  err = gpgrt_chdir ("debian/tests");
  if (err) {
    gpgrt_fprintf (gpgrt_stderr, "gpgrt_chdir failed: %s\n", gpg_strerror (err));
    return 1;
  }
  return 0;
}
