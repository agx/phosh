FROM registry.gitlab.gnome.org/world/phosh/phosh/debian:v0.0.20230607

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && eatmydata apt-get -y update \
   && cd /home/user/app \
   && echo "deb http://deb.debian.org/debian-debug/ bookworm-debug main" > /etc/apt/sources.list.d/debug.list \
   && echo "deb http://deb.debian.org/debian-debug/ sid-debug main" > /etc/apt/sources.list.d/debug.list \
   && echo "deb http://deb.debian.org/debian-debug/ experimental-debug main" > /etc/apt/sources.list.d/debug.list \
   && eatmydata apt-get update \
   && eatmydata apt-get -y install -t experimental libgtk-3-0-dbgsym libglib2.0-0-dbgsym \
   && eatmydata apt-get clean
