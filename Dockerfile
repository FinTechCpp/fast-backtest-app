FROM ubuntu:22.04

# Éviter les prompts interactifs
ENV DEBIAN_FRONTEND=noninteractive

# Installer les dépendances MXE
RUN apt-get update && apt-get install -y \
    autoconf \
    automake \
    autopoint \
    bash \
    bison \
    bzip2 \
    flex \
    g++ \
    g++-multilib \
    gettext \
    git \
    gperf \
    intltool \
    libc6-dev-i386 \
    libgdk-pixbuf2.0-dev \
    libltdl-dev \
    libgl-dev \
    libpcre3-dev \
    libssl-dev \
    libtool-bin \
    libxml-parser-perl \
    lzip \
    make \
    openssl \
    p7zip-full \
    patch \
    perl \
    python3 \
    python3-mako \
    python3-pkg-resources \
    ruby \
    sed \
    unzip \
    wget \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Cloner MXE
WORKDIR /opt
RUN git clone https://github.com/mxe/mxe.git

# Compiler MXE avec Qt6 pour Windows 64-bit
WORKDIR /opt/mxe
RUN make MXE_TARGETS='x86_64-w64-mingw32.static' \
    MXE_PLUGIN_DIRS='plugins/examples/host-toolchain/' \
    qtbase qtwidgets qtcharts cmake pkgconf -j$(nproc)

# Ajouter MXE au PATH
ENV PATH="/opt/mxe/usr/bin:${PATH}"

WORKDIR /workspace