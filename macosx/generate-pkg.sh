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

sed -e "s|@CURDIR@|${PWD}|" package.xml.in > package.xml

productbuild \
	--distribution package.xml \
	--identifier org.jackaudio.jack2 \
	--package-path "${PWD}" \
	--version ${VERSION} \
	jack2-osx-${VERSION}.pkg

rm package.xml

# ---------------------------------------------------------------------------------------------------------------------
