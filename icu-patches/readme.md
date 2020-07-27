# Patches for ICU

This folder contains patches that are applied on top of the released version of ICU4C from the upstream `maint/maint-*` branch.

## Approach

Every patch should be thought of as a maintenance burden. We generally want to keep the patch count to a minimum. To that end, every patch should describe its reason for existence.

If an issue has already been fixed upstream and we would obtain the fix by upgrading to a newer version of the library, then a patch file would (generally speaking), not be created, and the
change from upstream would be cherry-picked as-is.

The intent is to only keep patch files for things that will need to be continuously patched, even on newer versions. In other words, the patch files should be for changes that will be re-applied on every version update.

Many of the patches are changes to remove private, internal, or otherwise unwanted APIs from the SDK headers for the Windows OS version of ICU. (The Windows OS version only exposes the public, stable, flat C API surface).

*Any changes should be pursued upstream whenever possible.*

These patches are stored as git ".patch" files under the "patches" folder.

### Patch system

In order to keep things relatively simple, we keep all of the patches in a single folder.

The patch files are *numbered* based on the *relative order* in which they should be applied. This is done using a prefix numbering system. Note that the exact numbering doesn't matter, only the relative ordering of the patches themselves.

For changes that will never be upstreamed we add the prefix "**MSFT-Patch**" to the patch file name, in order to differentiate it from other patches.

For patches that have a corresponding ICU ticket number we include that as a prefix in the patch file name as well (for example: "**ICU-1234**").

These '.patch' files are generated from the output of the `git format-patch --stdout` command.

Patch files should be pruned when a new version of ICU is ingested.

### Applying Changes

To apply/reapply all of the various patches when ingesting a new version of ICU, run the batch script `apply` from this directory.

This script will build a list of all of the patch files under the 'patch' folder and then *interactively* apply the patches.

Below is an example of what the interactive script looks like:

```
  msft-patches> apply.cmd

  Found the following Patch files (in the following order):
     "patches\001-13686.patch"  "patches\002-13687.patch"  "patches\003-test.patch"

  Do you want to proceed with applying the above patch files? [Y,N] Y

  Attempting to apply the patch files...

  Commit Body is:
  --------------------------
  UDataPathIterator adds extra file separator, even if the path already ends with one. [ICU Ticket #13686]
  --------------------------
  Apply? [y]es/[n]o/[e]dit/[v]iew patch/[a]ccept all: y
  Applying: UDataPathIterator adds extra file separator, even if the path already ends with one. [ICU Ticket #13686]
  Using index info to reconstruct a base tree...
  M       ICU/source/common/charstr.cpp
  M       ICU/source/common/charstr.h
  M       ICU/source/common/udata.cpp
  Falling back to patching base and 3-way merge...
  No changes -- Patch already applied.
  Commit Body is:
  --------------------------
  Enable Windows UWP version to use TZ update files (.res files) [ICU Bug #13687]
  --------------------------
  Apply? [y]es/[n]o/[e]dit/[v]iew patch/[a]ccept all: y
  Applying: Enable Windows UWP version to use TZ update files (.res files) [ICU Bug #13687]
  Commit Body is:
  --------------------------
  test patch
  --------------------------
  Apply? [y]es/[n]o/[e]dit/[v]iew patch/[a]ccept all: y
  Applying: test patch
  Using index info to reconstruct a base tree...
  M       ICU/source/common/putil.cpp
  Falling back to patching base and 3-way merge...
  No changes -- Patch already applied.

  msft-patches>
```

### Generating New Patches

Generating a new .patch file works best if you can isolate the "change" as a single git commit.

Generally speaking the steps are:
1. Make whatever changes you need to make.
2. `git add` your changes as needed.
3. `git commit` your changes as a single stand alone commit. Make a note of the **SHA1** hash value for your new commit.
4. Use the following command to create a new .patch file:

    `git format-patch --stdout -1 SHA1 > new-icu.patch`
5. Rename your new patch file so that it is appropriately numbered and descriptive, for example "003-my-new-icu-change.patch".
6. Copy the new patch file under the "patches" folder.
7. Don't forget to `git add` and `git commit` the new patch file as part of your overall change.

### Slicing up a *existing* squash-merge commit:

If you already have a squash-merge committed change that you want to create a patch file from, then you can use this approach to slice up an existing commit. 

General steps:

1. Use `git log` to figure out what the SHA1 hash is just *before* the squash-merge commit.
2. `git rebase -i <SHA1 before the commit>`
3. Mark the expected commit as "edit" in the editor that pops-up, save the file and close.
4. `git reset <SHA1 before the commit>`
5. You should now have all the changes from the squash-merge as *unstaged* changes in your local repo. 
5. `git add <whatever files you want>`
6. Note: Only add the files that you want to be part of the patch.
6. `git commit -m "My ICU patch change"`
7. Make a note of the New SHA1 hash value from your commit.
7. `git format-patch --stdout -1 <New SHA1> > <patch filename>.patch`
8. Rename the new patch file so that it is appropriately numbered and descriptive.
9. Copy the new patch file under the "patches" folder.
10. `git rebase --abort`

At this point you have now isolated the changes from the squash-merge commit into a single patch file and you can create a new branch for adding and committing the patch file.

