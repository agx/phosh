FROM registry.gitlab.gnome.org/world/phosh/phosh/debian:v0.0.2024-05-08

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && eatmydata apt-get -y update \
   && cd /home/user/app \
   && eatmydata apt-get -y -f install \
   && eatmydata apt-get -y install imagemagick-6.q16 fonts-lato fonts-vlgothic gnome-shell-common gsettings-desktop-schemas feedbackd fonts-cantarell librsvg2-common \
   && eatmydata apt-get -y install python3 python3-junit.xml python3-coloredlogs \
   && eatmydata apt-get clean
