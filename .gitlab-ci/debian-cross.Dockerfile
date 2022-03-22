FROM debian:bookworm-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && dpkg --add-architecture i386 \
   && echo "deb [arch=i386] http://deb.debian.org/debian/ testing main" > /etc/apt/sources.list.d/i386.list \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends wget ca-certificates eatmydata gnupg \
   && cd /home/user/app \
   && DEB_BUILD_PROFILES=nodoc,nocheck eatmydata apt-get -y --no-install-recommends -a i386 build-dep . \
   && eatmydata apt-get -y install --no-install-recommends git wget crossbuild-essential-i386 \
   && eatmydata apt-get clean \
   && rm -rf /var/lib/apt/lists/*
