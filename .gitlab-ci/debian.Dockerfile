FROM debian:bookworm-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends wget ca-certificates gnupg eatmydata \
   && eatmydata apt-get --no-install-recommends -y install build-essential git wget gcovr locales \
   && eatmydata apt-get -y update \
   && eatmydata apt-get -y dist-upgrade \
   && cd /home/user/app \
   && echo "deb http://deb.debian.org/debian/ experimental main" > /etc/apt/sources.list.d/exp.list \
   && echo "deb http://deb.debian.org/debian/ sid main" > /etc/apt/sources.list.d/sid.list \
   && eatmydata apt-get -y update \
   && eatmydata apt-get --no-install-recommends -y install -t experimental libgtk-3-dev libgtk-4-dev gtk-4-examples dbus-x11 libudev-dev libgnome-desktop-3-dev libdw1\
   && eatmydata apt-get --no-install-recommends -y install -t experimental phoc libfeedback-dev libgtk-4-dev phoc \
   && eatmydata apt-get --no-install-recommends -y build-dep . \
   && eatmydata apt-get clean



