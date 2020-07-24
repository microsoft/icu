#!/usr/bin/env bash

source="${BASH_SOURCE[0]}"

# resolve $SOURCE until the file is no longer a symlink
while [[ -h $source ]]; do
  scriptroot="$( cd -P "$( dirname "$source" )" && pwd )"
  source="$(readlink "$source")"

  # if $source was a relative symlink, we need to resolve it relative to the path where the
  # symlink file was located
  [[ $source != /* ]] && source="$scriptroot/$source"
done

usage()
{
  echo "Common settings:"
  echo "  --tracing                  Enable ICU tracing"
  echo "  --help                     Print help and exit (short: -h)"
  echo ""
}

properties=

while [[ $# > 0 ]]; do
  opt="$(echo "${1/#--/-}" | awk '{print tolower($0)}')"
  case "$opt" in
    -help|-h)
      usage
      exit 0
      ;;
    -tracing)
      properties="$properties /p:IcuTracing=true"
      ;;
    *)
  esac
  shift
done

scriptroot="$( cd -P "$( dirname "$source" )" && pwd )"

echo $tracing
"$scriptroot/eng/common/build.sh" --build --restore $properties $@
