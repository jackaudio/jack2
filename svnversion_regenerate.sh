#!/bin/bash

#set -x

if test $# -ne 1 -a $# -ne 2
then
  echo "Usage: "`basename "$0"`" <file> [define_name]"
  exit 1
fi

OUTPUT_FILE="`pwd`/${1}"
TEMP_FILE="${OUTPUT_FILE}.tmp"

#echo svnversion...
#pwd
#echo $OUTPUT_FILE
#echo $TEMP_FILE

OLDPWD=`pwd`
cd ..

if test $# -eq 2
then
  DEFINE=${2}
else
  DEFINE=SVN_VERSION
fi

if test -d .svn
then
  REV=`svnversion 2> /dev/null`
else
  if test -d .git
  then
    git status >/dev/null # updates dirty state
    REV=`git show | grep '^ *git-svn-id:' | sed 's/.*@\([0-9]*\) .*/\1/'`
    if test ${REV}
    then
      test -z "$(git diff-index --name-only HEAD)" || REV="${REV}M"
    else
      REV=0+`git rev-parse HEAD`
      test -z "$(git diff-index --name-only HEAD)" || REV="${REV}-dirty"
    fi
  fi
fi

if test -z ${REV}
then
  REV="unknown"
fi

echo "#define ${DEFINE} \"${REV}\"" > ${TEMP_FILE}
if test ! -f ${OUTPUT_FILE}
then
  echo "Generated ${OUTPUT_FILE} (${REV})"
  cp ${TEMP_FILE} ${OUTPUT_FILE}
  if test $? -ne 0; then exit 1; fi
else
  if ! cmp -s ${OUTPUT_FILE} ${TEMP_FILE}
  then echo "Regenerated ${OUTPUT_FILE} (${REV})"
    cp ${TEMP_FILE} ${OUTPUT_FILE}
    if test $? -ne 0; then exit 1; fi
  fi
fi

cd "${OLDPWD}"

rm ${TEMP_FILE}

exit $?
