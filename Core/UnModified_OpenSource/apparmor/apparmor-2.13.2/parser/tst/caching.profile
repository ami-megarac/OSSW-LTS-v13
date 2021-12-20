# Simple example profile for caching tests

/bin/pingy {
  capability net_raw,
  capability setuid,
  network inet raw,

  /bin/ping mixr,
  /etc/modules.conf r,
}
