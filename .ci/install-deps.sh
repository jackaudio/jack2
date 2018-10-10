#!/usr/bin/env bash

set -euo pipefail

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
  brew install --c++11 \
    pkg-config \
    aften \
    libsamplerate \
    libsndfile \
    opus \
    readline \
    doxygen
fi

if [ "$TRAVIS_OS_NAME" == "linux" ]; then
  # autotools, automake, make are present in the trusty image
  sudo apt-get install -y \
    doxygen \
    libffado-dev \
    libsamplerate-dev \
    libsndfile-dev \
    libasound2-dev \
    systemd-services \
    systemd \
    libsystemd-journal-dev \
    libsystemd-login-dev \
    libsystemd-id128-dev \
    libsystemd-daemon-dev \
    libpam-systemd \
    libdbus-1-dev \
    libeigen3-dev \
    libopus-dev \
    libportaudio2
  # force installation of gcc-6 if required
  if [ "${CC}" == "gcc-6" ]; then
    sudo apt-get install gcc-6 g++-6
  fi
  # force installation of gcc-7 if required
  if [ "${CC}" == "gcc-7" ]; then
    sudo apt-get install gcc-7 g++-7
  fi
  # force installation of gcc-8 if required
  if [ "${CC}" == "gcc-8" ]; then
    sudo apt-get install gcc-8 g++-8
  fi
  # force installation of clang-3.8 if required
  if [ "${CC}" == "clang-3.8" ]; then
    sudo apt-get install clang-3.8
  fi
fi

exit 0
