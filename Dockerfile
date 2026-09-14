# Dockerfile for ezpropkit headless cross-platform firmware builds
FROM python:3.11-slim-bookworm

# Avoid prompts during apt installs
ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1

# Install required build tools and libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    curl \
    build-essential \
    pkg-config \
    libusb-1.0-0-dev \
    udev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install PlatformIO Core
RUN pip install --no-cache-dir -U platformio

# Pre-install the Raspberry Pi platform and Earle Philhower core to warm cache
RUN pio platform install https://github.com/maxgerhardt/platform-raspberrypi.git

WORKDIR /workspace

# Default command compiles the default environment
CMD ["pio", "run"]
