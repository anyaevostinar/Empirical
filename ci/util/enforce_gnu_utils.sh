#!/bin/bash

# adapted from https://superuser.com/a/731431
if [[ "$(basename -- "$0")" == "enforce_gnu_utils.sh" ]]; then
  echo "$0 may not be run, it must be sourced" >&2
  exit 1
fi

for util in "awk" "find" "grep" "sed"; do
  . ./ci/util/enforce_gnu_version.sh "${util}"
done
