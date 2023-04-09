FROM debian:bookworm-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends wget ca-certificates gnupg eatmydata \
   && echo "deb http://deb.debian.org/debian/ experimental main" > /etc/apt/sources.list.d/exp.list \
   && eatmydata apt-get -y update \
   && eatmydata apt-get -y dist-upgrade \
   && cd /home/user/app \
   && eatmydata apt-get --no-install-recommends -y build-dep . \
   && eatmydata apt-get --no-install-recommends -y install build-essential git wget gcovr locales \
   && eatmydata apt-get --no-install-recommends -y install -t experimental phoc \
   && eatmydata apt-get clean



