#!/bin/bash

set -e

cd $(dirname ${0})

# ---------------------------------------------------------------------------------------------------------------------

installed_prefix="${1}"

if [ -z "${installed_prefix}" ]; then
    echo "usage: ${0} <installed_prefix>"
    exit 1
fi

# ---------------------------------------------------------------------------------------------------------------------

VERSION=$(cat ../wscript | awk 'sub("VERSION=","")' | tr -d "'")

rm -f jack2-osx-root.pkg
rm -f jack2-osx-${VERSION}.pkg
rm -f package.xml

# ---------------------------------------------------------------------------------------------------------------------

pkgbuild \
    --identifier org.jackaudio.jack2 \
    --install-location "/usr/local/" \
    --root "${installed_prefix}/" \
    jack2-osx-root.pkg

# ---------------------------------------------------------------------------------------------------------------------

# https://developer.apple.com/library/content/documentation/DeveloperTools/Reference/DistributionDefinitionRef/Chapters/Distribution_XML_Ref.html

pushd "${installed_prefix}"
mkdir -p share/jack2
touch share/jack2/jack2-osx-files.txt
find -sL . -type f | awk 'sub("./","/usr/local/")' > share/jack2/jack2-osx-files.txt
popd

sed -e "s|@CURDIR@|${PWD}|" package.xml.in > package.xml
cat package-welcome.txt.in "${installed_prefix}/share/jack2/jack2-osx-files.txt" > package-welcome.txt

productbuild \
    --distribution package.xml \
    --identifier org.jackaudio.jack2 \
    --package-path "${PWD}" \
    --version ${VERSION} \
    jack2-osx-${VERSION}.pkg

rm jack2-osx-root.pkg package.xml package-welcome.txt

# ---------------------------------------------------------------------------------------------------------------------
