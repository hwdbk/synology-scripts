# Incremental encrypted multi-disk backup

Two scripts to 1) generate an index for and 2) copy files to a multi-disk backup disk set. The files can be copied to an encrypted file system on the external disk (using eCryptfs) to prevent misuse in case the disk falls into the wrong hands or is disposed of at some point in time. I use it to create and maintain a standalone, offsite backup of my files once or twice a year or so. Now eCryptfs might not be the best possible encryption mechanism to control access to your files, but it's the only hardware-accellerated encryption that Synology supports and it's better then losing your files completely through human error or a disaster such as theft or fire on the original NAS. It's all about the balance in risk.

`backup_update_generate` starts off a file list as created by `mkfilelist_fast` or `mkfilelist_md5` (which is essentially a file scan of a list of directories/shares you want to backup) and a bunch of disks, empty or (partially) filled by a previous `backup_update_copy`. The idea is that the filelist of each disk is compared against the wanted target filelist, and any differences are calculated with `md5diff` and processed accordingly. This means that:
  - Files that no longer exist in the source file list, will be deleted.
  - New files will be copied.
  - Moved or renamed files are moved/renamed on the backup disks *regardless on which disk they reside* - this is where the efficiency comes in. So, once copied files will not be overwritten or rewritten if they are just moved or renamed.
  - Disappeared files (i.e. deleted files on the source) are deleted on the backup disks and the free space will be reused to write the files from the list of new files, effectively filling up the disks again.

The algorithm depends on `md5sum`, which you'll have to compile for your machine (it's in C++ and only depends on libstdc++ (STL): `g++ -o md5diff md5diff.cpp -lstdc++`).

- `backup_update_generate` generates a (multi-disk) index/todo list of files to copy (`.lst` files). The target disk size can be set for each disk to use in the script (in the array `DS`). Parameters are the `.fst` or `.md5` file of the source directories you want to backup (made with `mkfilelist_fast` or `mkfilelist_md5`) and a list of `.fst` or `.md5` files describing the current contents of the backup set's individual disks (that is, the 'before' status. These files are also created as the output of `backup_update_copy` - i.e. the results of the backup, to be used next time as input state). Typically, you start with a bunch of empty disks if you want to start a whole new backup set. This is reflected by a set of empty filelists for the disks, i.e. empty `.fst` or `.md5` files. These files should be named `<something>_nnn` where `nnn` is the disk sequence number (001, 002 etc.), as above. I use `YYYYMMDD` for `<something>` because that's the convention here. The sequence number is also the index in the `DS` array in the `backup_update_generate` script that describes the size of the disk (in bytes).

`backup_update_generate` does not have an opinion of what should be on which disk - it just needs to know *what* is on the disk(s). `md5diff` will calculate any filesystem differences with the collective disks' content and the target backup fileset it tries to create. This means, for instance, that if a disk got lost/damaged, you can just pass an empty/adjusted `.fst` or `.md5` file for that disk and the files that disappeared with it, will be rewritten on the next pass. You can even move disks around that way, or pre-fill a couple of disks, as long as the input files correctly describe the starting point. Note that there can be non-backup content on the disk, as long as the paths used don't clash and the used disk space is subtracted from the amount listed in the `DS` array. If a disk is unavailable (for instance, off-site) and you can't or don't want to touch its content, pass a `<something>_nnn.fst.locked` for this this as an argument to `backup_update_generate`.

Example output:
```
Executed command: backup_update_generate input.fst src/src_001.fst src/src_002.fst src/src_003.fst src/src_004.fst src/src_005.fst src/src_006.fst src/src_007.fst src/src_008.fst
Create backup set from file list: input.fst
Source disks to use (sorted by disknum):
src/src_001.fst (disk size  2,942,802 MB)
src/src_002.fst (disk size  2,942,802 MB)
src/src_003.fst (disk size  2,942,802 MB)
src/src_004.fst (disk size  2,942,802 MB)
src/src_005.fst (disk size  2,942,802 MB)
src/src_006.fst (disk size  7,925,730 MB)
src/src_007.fst (disk size  2,942,802 MB)
src/src_008.fst (disk size  2,942,802 MB)
Processing differences
# Read 348156 items from 20201208.allsrcdisks.fst
# Read 371017 items from input.fst
# Using file modification dates and file size
# Moved 2 items between 20201208.allsrcdisks.fst and input.fst
# Renamed 2 items between 20201208.allsrcdisks.fst and input.fst
# Modified 6 items between 20201208.allsrcdisks.fst and input.fst
# Deleted 0 items since 20201208.allsrcdisks.fst
# Added 22861 items in input.fst
Assigning differences to each disk: deleted, moved, renamed and modified files
Manage free space on each disk
src_001:  2,942,437 MB in use,        365 MB free -> 20201208_001:  2,942,437 MB in use,        365 MB free;        310 MB to be written
src_002:  2,941,529 MB in use,      1,273 MB free -> 20201208_002:  2,941,529 MB in use,      1,273 MB free;          0 MB to be written
src_003:  2,942,374 MB in use,        428 MB free -> 20201208_003:  2,942,374 MB in use,        428 MB free;          0 MB to be written
src_004:  2,938,466 MB in use,      4,336 MB free -> 20201208_004:  2,941,774 MB in use,      1,028 MB free;    311,917 MB to be written
src_005:  2,935,816 MB in use,      6,986 MB free -> 20201208_005:  2,941,622 MB in use,      1,180 MB free;    121,444 MB to be written
src_006:          0 MB in use,  7,925,730 MB free -> 20201208_006:  3,439,284 MB in use,  4,486,445 MB free;  3,439,284 MB to be written
src_007:  2,939,151 MB in use,      3,650 MB free -> 20201208_007:  2,942,474 MB in use,        327 MB free;      3,986 MB to be written
src_008:  2,941,196 MB in use,      1,606 MB free -> 20201208_008:  2,941,196 MB in use,      1,606 MB free;          0 MB to be written
LEFTOVER LINES: 
none
```
In this particular example of an incremental backup, disk 006 was lost, I bought a larger one (8TB replacing 3TB) and an empty src_006.fst file was passed for it to represent the new empty disk. The script created '+' lines for all lost files (as well as lines for the other updates that occurred in input.fst, for the other disks), so most of the 22861 added items corresponded to '+' lines in 20201208_006.lst. An other possible solution for the lost disk would have been to add a (smaller, say 2TB) empty disk 006, represented by an empty src_006.fst (the disks' free space defined by the DS array in `backup_update_generate`) and adding an extra disk 009 to the set by (in this example) passing an empty src_009.fst. `backup_update_generate` would then easily fill up the 2TB on 006 (as you can see, 3.4TB was needed) and have the rest (1.4TB) overflow to the new disk 009.

- `backup_update_copy` effectuates the change/'todo' list for the specified disk, as calculated by `backup_update_generate`. Take a look in the `.lst` file for the disk to see what will happen: disappeared files will be deleted, then files will be moved and renamed (even if the file had moved to a different share whose file list was collected on generate), then the remaining/resulting free space is filled up again with new files. The destination path for the files is set in the script to /volumeSATA1/satashare1-1 (which is an external disk connected to the 1st eSATA connector on the Synology NAS), but this can be changed if your backup disks are elsewhere. Options: `mountonly` - will only create and/or mount the eCryptfs file system ; `dryrun` - will only display what will happen ; `md5 [prev_md5_file.md5]` - will checksum the copied files, optionally using the results from the previous scan. This option will calculate the md5sum of every file on every disk produced (to keep track of any data corruption and to compare the backup set against the source). For speed, is supports the use of the previous scan's result (using `mkfilelist_md5 -f previous_scan.md5`, see documentation) for the files that weren't touched, and only calculate the md5sum for new/different files that were created during the backup. However, it's good practise to do a full `.md5` scan and S.M.A.R.T. test at least once a year.

Two methods of copying are given in the code: local file copy and rsync pull files from server. This may need some tweaking to suit the situation/file system layout(s). Check the file paths (they're relative to a source, backup destination or rsync module) with the `dryrun` option.

After backup, the result can be cross-checked against the source (`input.fst` in the above example) with `md5diff` by catenating the disks' `.fst` filelists, or using all `.md5` files, for added certainty on the backup's integrity. Any differences (usually files that have moved/disappeared *during* the time the backup was made) will show up there.

`backup_update_generate` does not need to rewrite any file that was once written (as long as it is still required in the backup set), but as a result, a directory may get spread across several disks (PS> a file is never split, of course). After a couple of passes, the disks layout may look a bit messy, but all files are there, which is easy to ascertain and easy to find (with grep) from the backup set's output `.fst` or `.md5` file lists.

# Extras

- `mount_ecryptfolder` is a small helper script that can be used to manually mount encrypted folders. It wraps `mount.ecryptfs` with the mount parameters prefilled with defaults, which also happen to be the parameters used by Synology to mount an encrypted share from the DSM user interface and the above `backup_diskset_copy` and `backup_update_copy` scripts.
