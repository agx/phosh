FROM debian:bullseye-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && dpkg --add-architecture i386 \
   && echo "deb [arch=i386] http://deb.debian.org/debian/ testing main" > /etc/apt/sources.list.d/i386.list \
   && echo "deb [arch=amd64 arch=i386] http://deb.debian.org/debian/ experimental main" >> /etc/apt/sources.list.d/exp.list \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends eatmydata \
   && eatmydata apt-get -y -o APT::Immediate-Configure=false install libhandy-1-dev:i386/experimental libhandy-1-0:i386/experimental gir1.2-handy-1:i386/experimental libgladeui-common/experimental \
   && cd /home/user/app \
   && DEB_BUILD_PROFILES=nodoc,nocheck eatmydata apt-get -y --no-install-recommends -a i386 -o APT::Immediate-Configure=false build-dep . \
   && eatmydata apt-get -y install --no-install-recommends git wget crossbuild-essential-i386 \
   && eatmydata apt-get clean \
   && rm -rf /var/lib/apt/lists/*
