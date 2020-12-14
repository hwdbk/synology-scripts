# Encrypted multi-disk backup

Simple scripts to generate an index for and copy files to a multi-disk backup disk set. The files are copied to an encrypted file system on the external disk (using eCryptfs) to prevent misuse in case the disk falls into the wrong hands or is disposed of at some point in time. I use it to create and maintain a standalone, offsite backup of my files once or twice a year or so. Now eCryptfs might not be the best possible encryption mechanism to control access to your files, but it's better then losing your files completely through human error or a disaster such as theft or fire on the original NAS. It's all about the balance in risk.

- `backup_diskset_generate` generates a (multi-disk) index/todo list of files to copy. The target disk size is 3TB and fixed (making that flexible would be the first obvious thing to do in extending this script but I didn't get round that yet) but can be changed in the script (the size, that is, not the flexibility of using mixed size disks). Multiple input paths can be specified that will be used for sourcing the files and the script will 'cut' the list into chunks that will (just) fit one disk, and then move on to the next disk. The backup disk set is named after the date (e.g. 20200605); disks are numbered 001, 002 etc. Skipping to the next disk can be forced by using the keyword '`newdisk`' in the input paths.

- `backup_diskset_copy` copies the files as specified by the generated index/todo list to the external disk. The destination path for the files is set in the script to /volumeSATA1/satashare1-1 (which is an external disk connected to the 1st eSATA connector on the Synology NAS), but this can be changed if your backup disks are elsewhere. Options: `mountonly` - will only create and/or mount the eCryptfs file system ; `dryrun` - will only display what will happen ; `nomd5` - will not checksum the copied files.

- `backup_diskset_rename` renames an existing backup disk (or rather the directory on it) to a new backup disk set name. This is useful to make incremental backups and allows existing files to be reused (not rewritten again) after some time if they are already on the backup disk (and haven't changed).

There is a much more efficient alternative to this: `backup_update_generate` and `backup_update_copy`. These scripts start off a file list as created by `mkfilelist_fast` (which is essentially a file scan of a list of directories/shares you want to backup) and a bunch of disks, empty or filled by a previous `backup_diskset_copy` or `backup_update_copy`. The idea is that the file list of each disk is compared against the wanted target file list, and any differences are calculated with `md5diff` and processed accordingly. This means that files that no longer exist in the source file list, will be deleted, new files will be copied and (this is where the efficiency comes in), moved or renamed files are moved/renamed on the backup disks *regardless on which disk they reside*. So, once copied files will not be overwritten or rewritten if they are just moved or renamed. Disappeared files (i.e. deleted files on the source) files are deleted on the backup disks and the free space that is returned, will be reused to write the files from the list of new files, effectively filling up the disks again. The algorithm depends on `md5sum`, which you'll have to compile for your machine (it's in C++).

- `backup_update_generate` generates a (multi-disk) index/todo list of files to copy (`.lst` files). The target disk size can be set for each disk to use in the script (in the array `DS`). Parameters are the `.fst` file of the source directories you want to backup (made with `mkfilelist_fast`) and a list of `.fst` files describing the current contents of the backup set's individual disks (that is, the 'before' status. These files are created as output of `backup_diskset_copy` and `backup_update_copy` - i.e. the results of the previous backup). Typically, you can start with a bunch of empty disks (=empty .fst files) if you want to start a whole new backup set. These files should be named `<something>_nnn` where `nnn` is the disk sequence number (001, 002 etc.), as above. I use `YYYYMMDD` for `<something>` because that's the convention here.

`backup_update_generate` does not have an opinion of what should be on which disk - it just needs to know *what* is on the disk(s). `md5diff` will calculate any filesystem differences with the collective disks' content and the target backup fileset it tries to create. This means, for instance, that if a disk got lost/damaged, you can just pass an empty/adjusted .fst file for that disk and the files that disappeared with it, will be rewritten on the next pass.

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
src_001:  2,942,437 MB in use,        365 MB free -> 20201208_001:  2,942,437 MB in use,        365 MB free
src_002:  2,941,529 MB in use,      1,273 MB free -> 20201208_002:  2,941,529 MB in use,      1,273 MB free
src_003:  2,942,374 MB in use,        428 MB free -> 20201208_003:  2,942,374 MB in use,        428 MB free
src_004:  2,938,466 MB in use,      4,336 MB free -> 20201208_004:  2,941,774 MB in use,      1,028 MB free
src_005:  2,935,816 MB in use,      6,986 MB free -> 20201208_005:  2,941,622 MB in use,      1,180 MB free
src_006:          0 MB in use,  7,925,730 MB free -> 20201208_006:  3,439,284 MB in use,  4,486,445 MB free
src_007:  2,939,151 MB in use,      3,650 MB free -> 20201208_007:  2,942,474 MB in use,        327 MB free
src_008:  2,941,196 MB in use,      1,606 MB free -> 20201208_008:  2,941,196 MB in use,      1,606 MB free
LEFTOVER LINES: 
none
```
In this particular example of an incremental backup, disk 006 was lost, I bought a larger one (8TB replacing 3TB) and an empty .fst file was passed for it to represent the new empty disk. The script created '+' lines for all lost files as well (as the other regular updates that occurred in input.fst for the other disks), so most of the 22861 added items corresponded to '+' lines in 20201208_006.lst. An other solution for the lost disk would be to add a (smaller) empty disk 006, represented by an empty src_006.fst (the disks' free space defined by the DS array in `backup_update_generate`) and adding an extra disk 009 to the set by (in this example) passing an empty src_009.fst.

- `backup_update_copy` effectuates the change/'todo' list for the specified disk, as calculated by `backup_update_generate`. Take a look in the `.lst` file for the disk to see what will happen: disappeared files will be deleted, then files will be moved and renamed (even if the file had moved to a different share whose file list was collected on generate), then the remaining/resulting free space is filled up again with new files. `backup_update_copy` has an option to calculate the md5sum of every file on every disk produced (to keep track of any data corruption and to compare the backup set against the source). For speed, is supports the use of the previous scan's result (`mkfilelist_md5 -f previous_scan.md5`) for the files that weren't touched, and only calculate the md5sum for new/different files that were created during the backup.

Now there's something to say to both methods: `backup_diskset_generate` will create an ordered disk set and you get to decide which files are written out in which order (on which disks, to a certain extent), but updating basically rewrites the entire sequence and pushes data forward across the disks as you data set increases in size. Files get rewritten on the next disk if they wouldn't no longer fit on the disk they originally were backed up on. `backup_update_generate`, on the other hand, 100% reuses any file ever written (as long as it is still required in the backup set), but as a result, a directory may get spread across several disks (PS> a file is never split, of course). After a couple of passes, the disks layout may look a bit messy, but all files are there, and easy to find (with grep) from the backup set's output `.fst` or `.md5` file lists.

The first method is ideal if you expect to be wanting to restore a specific directory regularly. The latter is ideal to regularly and fastly update the disk set for offline storage and, if push comes to shove, you don't mind popping in a couple of disks to retrieve the contents of a specific directory. You can't have it all...
