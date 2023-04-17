#!/bin/bash
set +x
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# ----------------------------------------------------------------------
#  This script is used to apply patches to ICU.
# ----------------------------------------------------------------------

# Look for files under the "patches" folder that end in ".patch".
for i in ${SCRIPT_DIR}/patches/*.patch;
  do patch_file_list="$patch_file_list $i"
done

echo
echo "Found the following Patch files (in the following order):"
echo " $patch_file_list"
echo

# Ask the user if they want to apply the patches.
[[ "$(read -e -p 'Do you want to proceed with applying the above patch files? [Y,N]'; echo $REPLY)" == [Yy]* ]] && echo "Attempting to apply the patch files..." && echo && git am -3 -i --keep-cr --whitespace=nowarn $patch_file_list || exit 1

# Notes on the 'git am' options:
#  -3
#        When the patch does not apply cleanly, fall back on 3-way merge.
#  -i
#        Run interactively.
#  --keep-cr
#        Prevent stripping CR at the end of lines. 
#  --whitespace=nowarn 
#        Turns off the trailing whitespace warning.

