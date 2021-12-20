/usr/bin/pulseaudio {

  ^/usr/lib/pulseaudio/pulse/gconf-helper {
    signal receive set=term peer=/usr/bin/pulseaudio,

  }
}
