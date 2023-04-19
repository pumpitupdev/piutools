# Docker Runtime Environment For Pump it Up Linux
# Suitable for Exceed->XX
FROM --platform=linux/amd64 ubuntu:18.04
ARG DEBIAN_FRONTEND=noninteractive

# Add 32bit Support 
RUN dpkg --add-architecture i386
RUN apt-get -y update

# Dependencies for glvnd, X11, and Alsa/Pulse.
RUN apt-get install -y -qq --no-install-recommends libglvnd0 libgl1 libglx0 libegl1 libxext6 libx11-6 alsa-utils pulseaudio fuse squashfuse unionfs-fuse

# Dependencies for 32bit Pump Runtime that rely on the OS itself
RUN apt-get install -y 	libstdc++6:i386 libstdc++5:i386 libgl1:i386 libglu1:i386 libasound2:i386 libasound2-plugins:i386

# Optional Debug Dependencies
RUN apt-get install -y strace ltrace gdb

# Delete apt-cache to reduce image size
RUN rm -rf /var/lib/apt/lists/*

# Env vars for the nvidia-container-runtime.
ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES graphics,utility,compute

# Build with docker build -t pumpos_classic -f PumpOS_Classic.Dockerfile .
