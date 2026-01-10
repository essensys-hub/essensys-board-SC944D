# Use a lightweight Debian base
FROM debian:bookworm-slim

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
# gcc-m68k-linux-gnu: Cross-compiler for Motorola 68000 family (includes ColdFire support)
# make, cmake: Build tools
# git: Version control
# srecord: For manipulating S19/HEX files (commonly used in embedded)
# build-essential: General build utilities
RUN apt-get update && apt-get install -y \
    gcc-m68k-linux-gnu \
    g++-m68k-linux-gnu \
    build-essential \
    make \
    cmake \
    git \
    srecord \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /workspace

# Default command can be observing the version to verify installation
CMD ["m68k-linux-gnu-gcc", "--version"]
