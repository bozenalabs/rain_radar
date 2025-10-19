# Dockerfile - Raspberry Pi Pico (RP2040) C/C++ development environment
FROM ubuntu:22.04


ENV DEBIAN_FRONTEND=noninteractive \
    PATH=/usr/bin:$PATH

# Install toolchain and build tools. this will be done as root
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    cmake \
    build-essential \
    gcc-arm-none-eabi \
    gdb-multiarch \
    python3 \
    python3-pip \
    ninja-build \
    ccache \
    libusb-1.0-0-dev \
    pkg-config \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
 && rm -rf /var/lib/apt/lists/*


# Create a non-root developer user and project workspace
RUN useradd -m appuser
USER appuser
RUN mkdir /home/appuser/pico
WORKDIR /home/appuser/pico

RUN git clone --depth=1 --recursive -b master https://github.com/raspberrypi/pico-sdk.git
ENV PICO_SDK_PATH=/home/appuser/pico/pico-sdk
RUN git clone --depth=1 -b master https://github.com/raspberrypi/pico-examples.git
RUN git clone --depth=1 -b main https://github.com/pimoroni/pimoroni-pico.git --recursive
RUN git clone --depth=1 -b main https://github.com/pimoroni/picosystem.git

RUN python3 -m pip install --no-cache-dir --upgrade pip setuptools wheel

# Default behavior: open a shell (override with docker run ... CMD)
CMD ["/bin/bash"]