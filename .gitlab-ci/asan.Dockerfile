FROM registry.gitlab.gnome.org/world/phosh/phosh/debian:v0.0.2024-05-08

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && eatmydata apt-get -y update \
   && cd /home/user/app \
   && echo "deb http://deb.debian.org/debian-debug/ trixie-debug main" > /etc/apt/sources.list.d/debug.list \
   && eatmydata apt-get update \
   && eatmydata apt-get -y install libgtk-3-0t64-dbgsym libglib2.0-0t64-dbgsym \
   && eatmydata apt-get clean
