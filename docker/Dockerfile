#------------------------------------------------------------------------------
# Development environment of EvoComb (Linux arm64): compile, test, debug, profile and
# generate the documentation.
#
# The image only contains the tools: the sources are mounted in /evocomb and the
# build directory is a Docker volume mounted in /build (faster than a bind mount
# on macOS, and it does not mix with the cmake-build-* directories of CLion).
#
# With Docker Compose (recommended, see compose.yaml):
#   docker compose -f docker/compose.yaml build
#   docker compose -f docker/compose.yaml run --rm evocomb
#
# Without Docker Compose (from the root of EvoComb):
#   docker build --platform linux/arm64 -t evocomb-dev docker
#   docker run --rm -it --platform linux/arm64 \
#       --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
#       -v "$PWD":/evocomb -v evocomb-build:/build evocomb-dev
#
# Inside the container:
#   cmake -S /evocomb -B /build/debug -DCMAKE_BUILD_TYPE=Debug   # Ninja and ccache by default
#   cmake --build /build/debug
#   ctest --test-dir /build/debug --output-on-failure
#   valgrind --leak-check=full --error-exitcode=1 /build/debug/bin/unit_tests
#   gdb --args /build/debug/bin/unit_tests --gtest_filter='*NNGGA*'
#   cmake --build /build/debug --target evocomb_doc       # in /evocomb/doc/html
#
# Clang instead of GCC: CC=clang CXX=clang++ cmake -S /evocomb -B /build/clang ...
#------------------------------------------------------------------------------
FROM --platform=linux/arm64 ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

# Name and identifiers of the user (the default ones fit Docker Desktop on macOS)
ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000

RUN apt-get update && apt-get install -y --no-install-recommends \
        # Compilers and build tools
        build-essential \
        g++ \
        clang \
        lld \
        libc++-dev \
        libc++abi-dev \
        cmake \
        ninja-build \
        make \
        ccache \
        pkg-config \
        # Debuggers and profilers
        gdb \
        gdbserver \
        lldb \
        valgrind \
        # Static analysis, formatting and coverage
        clang-tidy \
        clang-format \
        cppcheck \
        lcov \
        gcovr \
        # Documentation
        doxygen \
        graphviz \
        # Utilities (git and the certificates are used by FetchContent for GoogleTest)
        git \
        ca-certificates \
        curl \
        unzip \
        rsync \
        python3 \
        sudo \
        less \
        nano \
        bash-completion \
        locales \
    && rm -rf /var/lib/apt/lists/*

# UTF-8 locale (the sources contain accented characters)
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

# Non-root user with sudo (the "ubuntu" user of the image is replaced to free the UID 1000)
RUN (userdel -r ubuntu 2>/dev/null || true) \
    && groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} --create-home --shell /bin/bash ${USERNAME} \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME} \
    && mkdir -p /evocomb /build \
    && chown ${USERNAME}:${USERNAME} /evocomb /build

# Compiler cache shared by all the build directories (in the build volume)
ENV CCACHE_DIR=/build/.ccache \
    CMAKE_CXX_COMPILER_LAUNCHER=ccache \
    CMAKE_GENERATOR=Ninja

# Allow gdb to auto-load the scripts it finds (pretty printers of libstdc++, .gdbinit of the sources)
RUN echo "set auto-load safe-path /" > /home/${USERNAME}/.gdbinit \
    && chown ${USERNAME}:${USERNAME} /home/${USERNAME}/.gdbinit

USER ${USERNAME}
WORKDIR /evocomb

CMD ["/bin/bash"]
