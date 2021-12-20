#! /bin/sh

# https://bugs.debian.org/cgi-bin/pkgreport.cgi?tag=gcc-pr66145;users=debian-gcc@lists.debian.org

vendor=Debian
if dpkg-vendor --derives-from Ubuntu; then
    vendor=Ubuntu
fi

if [ "$vendor" = Debian ]; then
    :
    pkgs=$(echo '
antlr
libaqsis1
libassimp3
blockattack
boo
libboost-date-time1.54.0
libboost-date-time1.55.0
libcpprest2.4
printer-driver-brlaser
c++-annotations
clustalx
libdavix0
libdballe6
dff
libdiet-sed2.8
libdiet-client2.8
libdiet-admin2.8
digikam-private-libs
emscripten
ergo
fceux
flush
libfreefem++
freeorion
fslview
fwbuilder
libgazebo5
libgetfem4++
libgmsh2
gnote
gnudatalanguage
python-healpy
innoextract
libinsighttoolkit4.7
libdap17
libdapclient6
libdapserver7
libkolabxml1
libpqxx-4.0
libreoffice-core
librime1
libwibble-dev
lightspark
libmarisa0
mira-assembler
mongodb
mongodb-server
ncbi-blast+
libogre-1.8.0
libogre-1.9.0
openscad
libopenwalnut1
passepartout
pdf2djvu
photoprint
plastimatch
plee-the-bear
povray
powertop
psi4
python3-taglib
realtimebattle
ruby-passenger
libapache2-mod-passenger
schroot
sqlitebrowser
tecnoballz
wesnoth-1.12-core
widelands
libwreport2
xflr5
libxmltooling6')
else
    pkgs=$(echo '
antlr
libaqsis1
libassimp3
blockattack
boo
libboost-date-time1.55.0
libcpprest2.2
printer-driver-brlaser
c++-annotations
chromium-browser
clustalx
libdavix0
libdballe6
dff
libdiet-sed2.8
libdiet-client2.8
libdiet-admin2.8
libkgeomap2
libmediawiki1
libkvkontakte1
emscripten
ergo
fceux
flush
libfreefem++
freeorion
fslview
fwbuilder
libgazebo5
libgetfem4++
libgmsh2
gnote
gnudatalanguage
python-healpy
innoextract
libinsighttoolkit4.6
libdap17
libdapclient6
libdapserver7
libkolabxml1
libpqxx-4.0
libreoffice-core
librime1
libwibble-dev
lightspark
libmarisa0
mira-assembler
mongodb
mongodb-server
ncbi-blast+
libogre-1.8.0
libogre-1.9.0
openscad
libopenwalnut1
passepartout
pdf2djvu
photoprint
plastimatch
plee-the-bear
povray
powertop
psi4
python3-taglib
realtimebattle
ruby-passenger
libapache2-mod-passenger
sqlitebrowser
tecnoballz
wesnoth-1.12-core
widelands
libwreport2
xflr5
libxmltooling6')
fi

fn=debian/libstdc++-breaks.$vendor
rm -f $fn
echo $pkgs
for p in $pkgs; do
    #echo $p
    if ! apt-cache show --no-all-versions $p >/dev/null; then
	echo "not found: $p"
    fi
    v=$(apt-cache show --no-all-versions $p | awk '/^Version/ {print $2}')
    case "$p" in
    libboost-date-time*)
      echo "$p," >> $fn
      ;;
    *)
      echo "$p (<= $v)," >> $fn
    esac
done
