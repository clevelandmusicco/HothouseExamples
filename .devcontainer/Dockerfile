# syntax=docker/dockerfile:1.7

# This Dev Container image is for building Cleveland Music Co. HothouseExamples.
#
# VS Code mounts the user's local HothouseExamples clone into the container.
# Flashing remains the responsibility of the user's host workstation.

FROM mcr.microsoft.com/devcontainers/base:ubuntu

# Avoid interactive package prompts during image builds.
ENV DEBIAN_FRONTEND=noninteractive

# Use the same Arm GNU Embedded Toolchain generation expected by the Daisy
# ecosystem. This Linux host toolchain produces Cortex-M firmware for the
# Daisy Seed; it is not a compiler for the container's own CPU.
ARG ARM_TOOLCHAIN_VERSION=gcc-arm-none-eabi-10-2020-q4-major
ARG ARM_TOOLCHAIN_DIR=/opt/${ARM_TOOLCHAIN_VERSION}

# The downloaded toolchain must match the container architecture, not the
# Daisy Seed. Both variants still emit arm-none-eabi firmware for Cortex-M.
ARG TARGETARCH

# Install only the additional packages needed for this project.
#
#   ca-certificates : HTTPS certificate verification for curl/git
#   curl            : downloads the Arm toolchain archive
#   bzip2           : lets tar extract .tar.bz2 archives
#   git             : initializes HothouseExamples submodules
#   make            : builds libDaisy, DaisySP, and the examples
#   python3         : required for HothouseExamples helper scripts
#   python-is-python3 : makes `python` resolve to Python 3 when scripts expect it
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    bzip2 \
    git \
    make \
    python3 \
    python-is-python3; \
    rm -rf /var/lib/apt/lists/*

# Download the Arm toolchain during image build instead of checking the large
# compressed archive into git.
#
# The MD5 checks here are not cryptographic security; they are a practical guard
# against corrupted or unexpected downloads. The URLs and checksums are pinned
# so users get the same toolchain version across Windows, macOS, and Linux hosts.
RUN set -eux; \
    case "${TARGETARCH}" in \
    amd64) \
    archive="${ARM_TOOLCHAIN_VERSION}-x86_64-linux.tar.bz2"; \
    url="https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/${archive}"; \
    md5="8312c4c91799885f222f663fc81f9a31"; \
    ;; \
    arm64) \
    archive="${ARM_TOOLCHAIN_VERSION}-aarch64-linux.tar.bz2"; \
    url="https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/${archive}"; \
    md5="1c3b8944c026d50362eef1f01f329a8e"; \
    ;; \
    *) \
    echo "Unsupported container architecture: ${TARGETARCH}"; \
    exit 1; \
    ;; \
    esac; \
    curl -fsSL "${url}" -o "/tmp/${archive}"; \
    echo "${md5}  /tmp/${archive}" | md5sum -c -; \
    tar -xjf "/tmp/${archive}" -C /opt; \
    rm "/tmp/${archive}"

# Make arm-none-eabi-gcc, arm-none-eabi-g++, arm-none-eabi-size, etc.
# available in VS Code terminals and build tasks.
ENV PATH="${ARM_TOOLCHAIN_DIR}/bin:${PATH}"

# VS Code will mount the user's local HothouseExamples checkout here.
USER vscode
WORKDIR /workspaces/HothouseExamples