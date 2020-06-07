# File tracker

Scripts to scan your file system and create a 'fingerprint' of each file so it can be tracked for
modification, deletion, renaming or moving. There's two kind of fingerprints: a 'fast' one (just scanning the file system,
which should not take more than a couple of minutes, depending on how many files you have) and a full data scan (using
md5sum, which is more robust but can take a lot of time).

- `mkfilelist_fast` prints on stdout a list of file 'fingerprints' consisting of
modification date_time, file size, path and filename. Although not 100% bullet-proof (there may be 2 files with the same size,
same modification date and yet have different contents), in practical situations it is more than enough to do a quick scan.

- `mkfilelist_md5` prints on stdout a list of true file 'fingerprints' consisting of
modification date_time, file size, md5sum, path and filename. Using the md5sum achieves two things: first, it reads all file data
from disk ensuring that any disk corruption gets detected (producing I/O errors) and secondly, it produces the the MD5 hash of
the data as the data fingerprint, which will be associated with the file regardless its modification date and will stay the
same as long as the file is not modified/tampered with.

The output of `mkfilelist_fast` is the same as `mkfilelist_md5` except that the md5sum field is not there in the tab-separated
lines. This allows fast scans and full scans to be used together (i.e. a full scan can be used as a fast scan, but obviously
not the other way around). Which brings me to the program to check/process these files.

- `md5diff`
