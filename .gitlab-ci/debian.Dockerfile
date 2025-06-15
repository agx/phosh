FROM debian:trixie-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends wget ca-certificates gnupg eatmydata \
   && eatmydata apt-get --no-install-recommends -y install build-essential git wget gcovr locales uncrustify abi-compliance-checker abi-dumper universal-ctags \
   && eatmydata apt-get -y install clang clang-tools lld # For clang build \
   && eatmydata apt-get -y update \
   && eatmydata apt-get -y dist-upgrade \
   && cd /home/user/app \
   && eatmydata apt-get -y update \
   && eatmydata apt-get --no-install-recommends -y build-dep . \
   && eatmydata apt-get clean

# Configure locales for tests
RUN export DEBIAN_FRONTEND=noninteractive && \
   echo 'ar_AE.UTF-8 UTF-8\nde_DE.UTF-8 UTF-8\nen_US.UTF-8 UTF-8\nja_JP.UTF-8 UTF-8\nuk_UA.UTF-8 UTF-8' > /etc/locale.gen && \
   dpkg-reconfigure locales
