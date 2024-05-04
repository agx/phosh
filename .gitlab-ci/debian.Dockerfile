FROM debian:trixie-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends wget ca-certificates gnupg eatmydata \
   && eatmydata apt-get --no-install-recommends -y install build-essential git wget gcovr locales uncrustify \
   && eatmydata apt-get -y update \
   && eatmydata apt-get -y dist-upgrade \
   && cd /home/user/app \
   && eatmydata apt-get -y update \
   && eatmydata apt-get --no-install-recommends -y build-dep . \
   && eatmydata apt-get --no-install-recommends -y install python3-lxml python3-colorlog \
   && wget http://ftp.de.debian.org/debian/pool/main/g/gcovr/gcovr_7.2+really-1.1_all.deb \
   && dpkg -i gcovr_7.2+really-1.1_all.deb && rm -f gcovr_7.2+really-1.1_all.deb \\
   && eatmydata apt-get clean

