FROM ubuntu:24.04

# The Android build environment for scripts/android/build.py --ci-container: one pinned toolchain, so an APK
# does not depend on what the machine building it has installed. The image next door (scripts/linux/Dockerfile)
# builds the desktop app; this one holds only what an arm64 Android build needs: a host clang for the translator
# and the disc extractor, CMake and ninja, a JDK 17, and the Android SDK with one platform, one build-tools and
# one NDK.
#
# The target is arm64 Android, but every tool here runs on x86-64, so the image is x86-64 whatever the host is;
# scripts/android/build.py passes --platform linux/amd64 (Docker Desktop emulates it on Apple silicon, which
# works and is slow).

ARG DEBIAN_FRONTEND=noninteractive

# Pinned so the image is the toolchain: bump one of these and the diff says which part of the toolchain moved.
# cmdline-tools 19.0 (the sdkmanager), build-tools 36.0.0, platform android-36 (SDL's Java calls
# Display.getHighestHdrSdrRatio(), an API 36 method, so javac needs at least this platform), NDK r29 (the version
# docs/ANDROID.md's bring-up used; the builder needs r27 or newer).
ARG CMDLINE_TOOLS_ZIP=commandlinetools-linux-13114758_latest.zip
ARG CMDLINE_TOOLS_SHA256=7ec965280a073311c339e571cd5de778b9975026cfcbe79f2b1cdcb1e15317ee
ARG ANDROID_PLATFORM=android-36
ARG ANDROID_BUILD_TOOLS=36.0.0
ARG ANDROID_NDK=29.0.14206865

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential ca-certificates curl git python3 unzip \
        cmake ninja-build clang lld \
        openjdk-17-jdk-headless \
    && rm -rf /var/lib/apt/lists/*

ENV ANDROID_HOME=/opt/android-sdk \
    ANDROID_SDK_ROOT=/opt/android-sdk \
    JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64 \
    PATH=/opt/android-sdk/cmdline-tools/latest/bin:$PATH

# The sdkmanager, checked against its published hash before it is unpacked.
RUN mkdir -p ${ANDROID_HOME}/cmdline-tools \
    && curl -fsSL -o /tmp/cmdline-tools.zip "https://dl.google.com/android/repository/${CMDLINE_TOOLS_ZIP}" \
    && echo "${CMDLINE_TOOLS_SHA256}  /tmp/cmdline-tools.zip" | sha256sum -c - \
    && unzip -q /tmp/cmdline-tools.zip -d ${ANDROID_HOME}/cmdline-tools \
    && mv ${ANDROID_HOME}/cmdline-tools/cmdline-tools ${ANDROID_HOME}/cmdline-tools/latest \
    && rm /tmp/cmdline-tools.zip

# The SDK packages and the licences they need, accepted here once: a build run has no home of its own
# (scripts/android/build.py runs the container with HOME=/tmp) and must not stop to ask.
RUN yes | sdkmanager --sdk_root="${ANDROID_HOME}" --licenses > /dev/null \
    && sdkmanager --sdk_root="${ANDROID_HOME}" \
        "platforms;${ANDROID_PLATFORM}" \
        "build-tools;${ANDROID_BUILD_TOOLS}" \
        "ndk;${ANDROID_NDK}" \
    && rm -rf ${ANDROID_HOME}/.temp /root/.android /root/.java

# What the builder's step 1 looks for, checked while the image builds rather than on the first APK.
RUN clang --version && cmake --version && ninja --version && javac -version \
    && ls "${ANDROID_HOME}/ndk/${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" \
          "${ANDROID_HOME}/platforms/${ANDROID_PLATFORM}/android.jar" \
          "${ANDROID_HOME}/build-tools/${ANDROID_BUILD_TOOLS}/aapt2" \
          "${ANDROID_HOME}/build-tools/${ANDROID_BUILD_TOOLS}/d8" \
          "${ANDROID_HOME}/build-tools/${ANDROID_BUILD_TOOLS}/apksigner"

WORKDIR /work
