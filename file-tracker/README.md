# File tracker

Scripts to scan your file system and create a 'fingerprint' of each file so it can be tracked for
modification, deletion, renaming or moving. There's two kind of fingerprints: a 'fast' one (just scanning the file system,
which should not take more than a couple of minutes, depending on how many files you have) and a full data scan (using
md5sum, which is more robust but can take a lot of time).

- `mkfilelist_fast` prints on stdout a list of file 'fingerprints' consisting of
modification date_time, file size, path and filename. Although not 100% bullet-proof (there may be 2 files with the same size,
same modification date and yet have different contents), in practical situations it is more than enough to do a quick scan).

Example output: `20010330_150816\t3069\t\t./Standards/tv-anytime/SP004v11_schema.xsd`

- `mkfilelist_md5` prints on stdout a list of true file 'fingerprints' consisting of
modification date_time, file size, md5sum, path and filename. Using the md5sum achieves two things: first, it reads all file data
from disk ensuring that any disk corruption gets detected (producing I/O errors) and secondly, it produces the the MD5 hash of
the data as the data fingerprint, which will be associated with the file regardless its modification date and will stay the
same as long as the file is not modified/tampered with.

Example output: `20010330_150816\t3069\te94f17c8c2124c759216dcb98ff864f0\t./Standards/tv-anytime/SP004v11_schema.xsd`

Scanning an entire filesystem can take a lot of time. Usually, it's not necessary (or desired) to run a full scan often; only new/changed files are of particular interest. When used with `-f previous_scan.md5`, `mkfilelist_md5` will use the results of a previous scan for files that haven't changed (as far as their modification date, file size and file path are concerned; it can't vouch for its contents - to check that, you'd need to do a full (re)scan) and insert the previous result's md5sum in the output file rather than reading the entire file and calculating the md5sum. This saves a lot of time but doesn't validate the file's contents, obviously. To indicate that the/a md5sum value was reused, a dash (`-`) is appended to the md5sum. When the process is repeated multiple generations of reused md5sums are indicated by the number of dashes.

The output of `mkfilelist_fast` is the same as `mkfilelist_md5` except that the md5sum field is not there in the tab-separated
lines. This allows fast scans and full scans to be used together (i.e. a full scan can be used as a fast scan, but obviously
not the other way around). Which brings me to the program to check/process these files.

- `md5diff` is the C++ program (sorry, this algorithm is too complex and compute-intensive to run as a script) to read and compare the lists produced by `mkfilelist_md5` and/or `mkfilelist_fast`. It works as a diff, reading a left side file and right side file and then comparing the differences. It can also read a listing produced natively by `md5sum`, but then the fingerprint lacks the modification date and file size. It skips input lines starting with `#` so the input can have comment lines.

`md5diff` output format:

lines starting with -: deleted files ('wiped' list)

  format: `-\tmoddate\tsize\t[md5sum]\tpath\n`
  
lines starting with =: files that moved to a different directory, but file name stayed the same ('moved' list)

  format: `=\tmoddate\tsize\t[md5sum]\tpath\t->\tnewpath\n`
  
lines starting with >: files that were renamed (possibly to a different directory) ('renamed' list)

  format: `>\tmoddate\tsize\t[md5sum]\tpath\t->\tnewpath\n`
  
lines starting with *: files that were modified ('modified' list)

  format: `*\tmoddate\tsize\t->\tnewmoddate\tnewsize\t[md5sum]\tpath\n`

lines starting with +: files that were added ('new files' list)

  format: `+\tmoddate\tsize\t[md5sum]\tpath\n`

The output is checked/filtered easily by using grep, e.g. `md5diff oldfs.md5 newfs.md5 | grep ^-` will filter the list of deleted files, etc.
