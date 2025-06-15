FROM registry.gitlab.gnome.org/world/phosh/phosh/debian:v0.0.2025-06-16.2

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && eatmydata apt-get -y update \
   && cd /home/user/app \
   && eatmydata apt-get -y -f install \
   && eatmydata apt-get -y install imagemagick-7.q16 fonts-lato fonts-vlgothic gnome-shell-common gsettings-desktop-schemas feedbackd fonts-cantarell librsvg2-common \
   && eatmydata apt-get -y install python3 python3-junit.xml python3-coloredlogs \
   && eatmydata apt-get clean

# Allow translation files so GTK 3 can set widget direction (not needed in GTK 4)
RUN sed -i "s@path-exclude /usr/share/locale/\*@# path-exclude /usr/share/locale/\*@" /etc/dpkg/dpkg.cfg.d/docker
# Reinstall GTK 3 so translation files are setup
RUN eatmydata apt-get reinstall -y libgtk-3-common
