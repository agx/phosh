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

