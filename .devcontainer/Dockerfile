FROM mcr.microsoft.com/devcontainers/typescript-node:22

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    ninja-build \
    git \
    pkg-config \
    ccache \
    clang-18 \
    lld-18 \
    lldb-18 \
    clangd-18 \
    llvm-18 \
    llvm-18-dev \
    valgrind \
    gdb \
    curl \
    libasound2-dev \
    libjack-jackd2-dev \
    libfreetype6-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libcurl4-openssl-dev \
    libwebkit2gtk-4.1-dev \
    mesa-common-dev \
    lv2-dev \
    && rm -rf /var/lib/apt/lists/*


# Aktuelles CMake direkt von Kitware installieren
ENV CMAKE_VERSION=4.1.2
WORKDIR /tmp
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh \
    && chmod +x cmake-${CMAKE_VERSION}-linux-x86_64.sh \
    && ./cmake-${CMAKE_VERSION}-linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm cmake-${CMAKE_VERSION}-linux-x86_64.sh

# Claude CLI installieren
RUN npm install -g @anthropic-ai/claude-code

# CCache konfigurieren
ENV CCACHE_DIR=/workspace/.ccache
ENV PATH="/usr/lib/ccache:${PATH}"

USER node
WORKDIR /workspace
