#!/bin/sh

export NOCLIMB=1
export OUT_DIR=/home/coopera/Developer/gear/out/sdkbuild
export PKG=jack2

if [ -z $QNX_VERSION ]; then
    QNX_VERSION=660
fi

setup_dirs() {
    # Set up the package's fetch and build directories
    FETCH_ROOT=$OUT_DIR/fetch
    BUILD_ROOT=$OUT_DIR/build
    HOST_BUILD_ROOT=$BUILD_ROOT/host
    TARGET_BUILD_ROOT=$BUILD_ROOT/target
    INSTALL_ROOT=$OUT_DIR/install
    HOST_INSTALL_ROOT=$INSTALL_ROOT/host
    TARGET_INSTALL_ROOT=$INSTALL_ROOT/target
    STAGING_DIR=$OUT_DIR/staging
    HOST_STAGING_DIR=$STAGING_DIR/host
    TARGET_STAGING_DIR=$STAGING_DIR/target
    ARCHIVE_DIR=$OUT_DIR/archive
    mkdir -p $HOST_INSTALL_ROOT $TARGET_INSTALL_ROOT
}

setup_platform() {

    # Determine the platform we are running on
    case $(uname -s) in
        *MINGW* )
            if [ -z $(which gcc) ]; then
                echo "This script must be run from a MinGW shell."
                exit 1
            fi
            PLATFORM=win32

            # Put the win32 wrapper directory into the path
            WIN32=$ROOT/win32
            QNX_PATH=$WIN32/qnx${QNX_VERSION}
            export PATH=$WIN32:$QNX_PATH:$PATH

            ;;

        *Linux* )
            PLATFORM=linux
            QNX_PATH=/opt/qnx${QNX_VERSION}
            ;;
    esac

    # Aggressively error out (even if the current command is for a host
    # portion of the build)
    if [ ! -d $QNX_PATH ]; then
        echo "Unsupported QNX version $QNX_VERSION"
        exit 1
    fi

    # SDP 6.6.0 has a shell programming construct in its /opt/qnx660-env.sh
    # script that 'set -e' doesn't like. We have to temporarily disable it.
    case "$-" in
        *e* )
            set +e
            . $QNX_PATH/qnx${QNX_VERSION}-env.sh
            set -e
            ;;
        * )
            . $QNX_PATH/qnx${QNX_VERSION}-env.sh
            ;;
    esac
}

# Set up the build environment for either a host- or cross-build
setup_env() {
    local type=$1
    case $type in
        host )
            export PKG_CONFIG_LIBDIR=$HOST_STAGING_DIR/usr/lib/pkgconfig
            if [ $PLATFORM != win32 ]; then
                # Windows pkg-config will auto-guess the sysroot
                export PKG_CONFIG_SYSROOT_DIR=$HOST_STAGING_DIR
            fi
            export CPPFLAGS=-I$HOST_STAGING_DIR/usr/include
            export LDFLAGS=-L$HOST_STAGING_DIR/usr/lib
            export OBJCOPY=objcopy
            export STRIP=strip
            export READELF=readelf
            unset CC
            unset CXX
            unset LD

            case $PLATFORM in
                linux )
                    export CFLAGS="-m32"
                    export CXXFLAGS="-m32"
                    export LDFLAGS="$LDFLAGS -m32"
                    ;;
                * )
                    unset CFLAGS
                    unset CXXFLAGS
            esac
            ;;
        target )
            case $QNX_VERSION in
                platform8 )
                    export QNX_TRIPLET=arm-unknown-nto-qnx8.0.0eabi
                    ;;
                660 )
                    export QNX_TRIPLET=arm-unknown-nto-qnx6.6.0eabi
                    ;;
                * )
                    echo "Unsupported QNX version $QNX_VERSION"
                    exit 1
                    ;;
            esac

            export PKG_CONFIG_LIBDIR=$TARGET_STAGING_DIR/usr/lib/pkgconfig
            if [ $PLATFORM != win32 ]; then
                # Windows pkg-config will auto-guess the sysroot
                export PKG_CONFIG_SYSROOT_DIR=$TARGET_STAGING_DIR
            fi
            export AR=${QNX_TRIPLET}-ar
            export CPPFLAGS=-I$TARGET_STAGING_DIR/usr/include
            export LDFLAGS=-L$TARGET_STAGING_DIR/usr/lib
            
            #export CC="qcc -Vgcc_ntoarmv7le"
            #export CXX="qcc -Vgcc_ntoarmv7le -lang-c++"
            export CC=${QNX_TRIPLET}-gcc
            export CXX=${QNX_TRIPLET}-g++
            export LD=${QNX_TRIPLET}-ld
            export OBJCOPY=${QNX_TRIPLET}-objcopy
            export STRIP=${QNX_TRIPLET}-strip
            export READELF=${QNX_TRIPLET}-readelf
            export CFLAGS="-g -funwind-tables"
            unset CXXFLAGS
            ;;
    esac

    export PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=1
    export PKG_CONFIG_ALLOW_SYSTEM_LIBS=1
    export PATH=$HOST_STAGING_DIR/usr/bin:$PATH

    if [ $PLATFORM = win32 ]; then
        # Windows pkg-config does not supply a pkg.m4 file to aclocal, so we must supply our own
        export ACLOCAL_FLAGS="-I $ROOT/aclocal"
    fi
}

setup_dirs
setup_platform
setup_env target

# Set up all of the directories
FETCH_DIR=$FETCH_ROOT/$PKG
HOST_BUILD_DIR=$HOST_BUILD_ROOT/$PKG
TARGET_BUILD_DIR=$TARGET_BUILD_ROOT/$PKG
HOST_INSTALL_DIR=$HOST_INSTALL_ROOT/$PKG
TARGET_INSTALL_DIR=$TARGET_INSTALL_ROOT/$PKG
mkdir -p $HOST_BUILD_DIR $TARGET_BUILD_DIR $HOST_INSTALL_DIR $TARGET_INSTALL_DIR $HOST_STAGING_DIR $TARGET_STAGING_DIR

exec ./waf \
    --debug\
    --out=${TARGET_BUILD_DIR}\
    --prefix=/usr\
    --destdir=${TARGET_INSTALL_DIR}\
    --dist-target=qnx\
    --doxygen=no\
    --samplerate=yes\
    --sndfile=yes\
    --ioaudio=yes\
    --check-cxx-compiler=qcc\
    --check-c-compiler=qcc\
    --ports-per-application=16\
    configure "$@"
