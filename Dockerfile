# Use a rather old version of ubuntu to ensure compatibility regarding libc
FROM --platform=linux/amd64 debian:9
LABEL description="Build environment for piutools"

# Install build dependencies, multilib to get 32-bit versions
RUN dpkg --add-architecture i386
RUN apt-get -y update 
RUN apt-get install -y g++-multilib
RUN apt-get install -y gcc-multilib
RUN apt-get install -y zip
RUN apt-get install -y cmake
RUN apt-get install -y make
RUN apt-get install -y git
RUN apt-get install -y libc6-dev:i386
RUN apt-get install -y libusb-1.0-0-dev:i386
RUN apt-get install -y libusb-dev:i386
RUN apt-get install -y libasound2-dev:i386
RUN apt-get install -y libx11-dev:i386
RUN apt-get install -y libcurl4-gnutls-dev:i386
RUN apt-get install -y libglu1-mesa-dev:i386
RUN apt-get install -y libgl1:i386

# Delete apt-cache to reduce image size
RUN rm -rf /var/lib/apt/lists/*

# Copy files for building to container
RUN mkdir /piutools
WORKDIR /piutools

# docker build -t piutools_buildenv . 