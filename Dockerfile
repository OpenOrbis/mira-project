FROM ubuntu:focal

# Version to build
ENV MIRA_PLATFORM MIRA_PLATFORM_ORBIS_BSD_672

# Install key
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -

# Install dependencies
RUN apt-get -qq update && \
    apt-get install -qqy --install-recommends \
        ca-certificates \
        autoconf automake cmake dpkg-dev file git make patch \
        libc-dev libc++-dev libgcc-10-dev libstdc++-10-dev  \
        dirmngr gnupg2 lbzip2 wget xz-utils libtinfo5 clang-13 lldb-13 lld-13 git && \
    rm -rf /var/lib/apt/lists/*

# TODO: Prep OOSDK Toolchain

# Build Mira
RUN export MIRA_PLATFORM=${MIRA_PLATFORM} && mkdir /build && cd /build && \
    git clone https://github.com/kiwidoggie/mira-project.git && \
    /usr/local/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -DMIRA_PLATFORM=${MIRA_PLATFORM} -H/build/mira-project -B/build/mira-project/build -G "Unix Makefiles" \
    /usr/local/bin/cmake --build /build/mira-project --config Debug --target clean -j 34 --