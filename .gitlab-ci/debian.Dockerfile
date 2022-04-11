FROM debian:bookworm-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends wget ca-certificates gnupg eatmydata \
   && eatmydata apt-get -y update \
   && cd /home/user/app \
   && eatmydata apt-get --no-install-recommends -y build-dep . \
   && eatmydata apt-get --no-install-recommends -y install build-essential git wget gcovr locales \
   && echo "deb http://deb.debian.org/debian/ sid main" >> /etc/apt/sources.list.d/sid.list \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends phoc \
   && eatmydata apt-get clean



