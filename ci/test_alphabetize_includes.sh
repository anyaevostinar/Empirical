#!/bin/bash

# enforce use of GNU version of coreutils
. ./ci/util/enforce_gnu_utils.sh

# refuse to continue if uncommitted changes are present
. ./ci/util/enforce_git_status.sh

SOURCE_HASH=$( find -path ./third-party -prune -false -o -type f | sort | xargs cat | sha1sum )

./ci/impl/alphabetize_includes.sh

if [ "${SOURCE_HASH}" == "$( find -path ./third-party -prune -false -o -type f | sort | xargs cat | sha1sum )" ];
then
  exit 0 # success
else
  echo "unalphabetized includes detected, run ./ci/impl/alphabetize_includes.sh locally to find & fix"
  exit 1 # failure
fi
