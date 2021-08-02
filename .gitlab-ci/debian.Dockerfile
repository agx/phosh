FROM debian:bullseye-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends wget ca-certificates gnupg eatmydata \
   && echo "deb http://deb.debian.org/debian/ experimental main" > /etc/apt/sources.list.d/exp.list \
   && eatmydata apt-get -y update \
   && eatmydata apt-get --no-install-recommends -y install libhandy-1-dev/experimental libhandy-1-0/experimental gir1.2-handy-1/experimental libgladeui-common/experimental \
   && cd /home/user/app \
   && eatmydata apt-get --no-install-recommends -y build-dep . \
   && eatmydata apt-get --no-install-recommends -y install build-essential git wget gcovr \
   && eatmydata apt-get clean \
   && eatmydata dpkg --force-depends --remove lcov
