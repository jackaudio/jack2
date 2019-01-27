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
  # force installation of gcc-6 if required
  if [ "${CC}" == "gcc-6" ]; then
    brew install gcc@6
  fi
  # force installation of gcc-7 if required
  if [ "${CC}" == "gcc-7" ]; then
    brew install gcc@7
  fi
  # force installation of gcc-8 if required
  if [ "${CC}" == "gcc-8" ]; then
    brew install gcc@8
  fi
fi

if [ "$TRAVIS_OS_NAME" == "linux" ]; then
  # autotools, automake, make are present in the trusty image
  sudo apt-get install -y \
    doxygen \
    libffado-dev \
    libsamplerate-dev \
    libsndfile-dev \
    libasound2-dev \
    libdb-dev \
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
    portaudio19-dev \
    locate

# remove everything that jack will provide
# (it can not be a dependency for the build)
# these files were dragged in by the above apt-get install of dependency packages
  sudo rm -rf /usr/lib/x86_64-linux-gnu/libjack*
  sudo rm -rf /usr/include/jack*
  sudo rm -rf /usr/share/doc/libjack*
  sudo rm -rf /var/lib/dpkg/info/libjack*
  sudo rm -rf /usr/lib/x86_64-linux-gnu/pkgconfig/jack.pc
# when these files aren't deleted: jackd will behave strange after install.
# one symptom: unknown option character l

  sudo updatedb
  echo "found these files with 'jack' in name after installing dependencies and clean up:"
  echo "========================================================================="
  locate jack | grep -v /home/travis/build
  echo "========================================================================="

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
