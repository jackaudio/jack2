#/bin/sh
# This script eases updating the waf build system. Make sure wget and gpg are
# installed and run:
# $ ./waf-update.sh <version>
# This script will deploy the build system in source form as to ease downtream
# maintainence at Debian/Ubuntu. For example see [1] and the pages it links to.
# [1] https://wiki.debian.org/UnpackWaf

script=`basename ${0}`

msg() {
    echo "${script}: ${@}"
}

die() {
    msg "${@}"
    exit 1
}

test $# -ne 1 && die "usage: ${script} <version>"
version=${1}

homepage=https://waf.io

# Fetch the pubkey from the main homepage.
pubkey=tnagy1024.asc
if ! test -e ${pubkey}
then
    wget ${homepage}/${pubkey} || die "wget failed"
fi

# The key can also be found at freehackers.
pubkey2=${pubkey}.2
if ! test -e ${pubkey2}
then
    wget http://freehackers.org/~tnagy/${pubkey} -O ${pubkey2} || die "wget failed"
fi

# Make sure the keys are identic.
cmp ${pubkey} ${pubkey2}
status=$?
if test $status -eq 0
then
    msg "${pubkey} and ${pubkey2} match"
elif test $status -eq 1
then
    die "${pubkey} and ${pubkey2} did not match"
else
    die "cmp failed"
fi

# Now that the keys match, it is likely that this key is the real key so import
# it.
gpg --import ${pubkey} || die "could not import public key"

waf_dir=waf-${version}
waf_tar=${waf_dir}.tar.bz2
waf_asc=${waf_tar}.asc

if ! test -e ${waf_tar}
then
    wget ${homepage}/${waf_tar} || die "wget failed"
fi

if ! test -e ${waf_asc}
then
    wget ${homepage}/${waf_asc} || die "wget failed"
fi

gpg --verify ${waf_asc}
if test $? -eq 0
then
    msg "tarball verification succeeded"
else
    die "tarball verificatin failed"
fi

if test -d ${waf_dir}
then
    rm -rf ${waf_dir} || die "could not remove old waf directory"
fi

tar xjf ${waf_tar} || die "could not unpack tarball"

# Remove the old waflib directory and replace it with the new.
git rm -rf waflib || die "could not remove old waflib directory"
cp -R ${waf_dir}/waflib . || die "could not copy waflib directory"
git add waflib || die "could not add waflib to version control"

# Copy the waf-light script to the toplevel directory and rename it to waf so
# that the usual ./waf [args] can be used.
cp ${waf_dir}/waf-light waf || die "could not copy waf-light"

exit 0
