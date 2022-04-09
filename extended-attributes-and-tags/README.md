# Extended attributes and tags (as seen from the Synology side)

This directory contains a toolset to retrieve Mac OS X -created extended attributes natively from the Synology ext4 or btrfs file system. For a collection
of excellent articles on what extended attributes are and how/what for they are used in Mac OS X, read
https://eclecticlight.co/2020/10/24/theres-more-to-files-than-data-extended-attributes/ and it's related click-through Related articles at the bottom.
Of course, Synology doesn't use unix' native xattr tool to store these. Not by a long shot...

I was triggered on this by new funcionality in the Finder that allowed a user not only to tag a file with Labels (the 'Finder colours' red, orange, yellow, green, blue, purple and gray), but also custom tags. Then I found out that these were stored in the Apple-proprietary `com.apple.metadata:_kMDItemUserTags` extended attribute and that jdberry wrote a cool tool to manage them from the (Mac OS X Terminal) command line:

https://github.com/jdberry/tag

The scripts provided here attempt to do the same natively from the Synology side of the afpd tether. It works in two stages:
- the `get_attr` script retrieves the extended attribute raw data from a file's associated `@SynoEAStream` file. This can be used for any extended attribute, even your own.
- the `tag` script interprets this raw data and translates it according to the Apple data structures laid out for the `com.apple.metadata:_kMDItemUserTags` extended attribute. It prints the tags on separate lines, just like jdberry's `tag -gN` would do (`--garrulous --no-name`).
- the `listtags` script calls `tag` for all files (and directories, recursively) passed on the commandline. It prints the tags as comma-separated list on one line per file, just like jdberry's `tag -lR` would do, but skipping files that have no tags and printing the full path. Useful to keep a backup copy of all tags, or to check/track tags when making changes to the file system.
- the script `check_filename_lengths` checks if the file names used are not too long. This is an issue because on any file system, the maximum file name length is limited; if you want to use extended attributes on top, the file name has to have room for the added suffix `@SynoResource` or `@SynoEAStream`, limiting the effective max. file name length to 13 less than the hard limit.
  - Using eCryptfs, Synology _does_ allow you to set extended attributes on filenames with length between 131 and 143, however, it uses an undocumented mangling algorithm to keep the `@SynoResource` and/or `@SynoEAStream` file name at 143 characters exactly:

```
File names of 120-130 characters
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890@SynoEAStream
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901@SynoEAStream
12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123@SynoEAStream
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234@SynoEAStream
12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456@SynoEAStream
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567@SynoEAStream
12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789@SynoEAStream
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890@SynoEAStream    <- these are exactly 143 chrs total

File names with 131-143 characters
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789_@SynoEAStream    <- mangled @Syno file names
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789r@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789i@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789a@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789(@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789$@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789.@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789}@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789x@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789m@SynoEAStream
123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789c@SynoEAStream
1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567894@SynoEAStream
```
This is a problem because with a mangled last part, the tools here to read and interpret the `@SynoEAStream` file won't be able to find the right associated file.

  - When _not_ using eCryptfs, but ext4, the same thing happens, but at much longer lengths: the file name length limit in ext4, for instance, is 255, and the usable file name length for files with extended attributes, and not mangling the file names of the @Syno files, is 13 less, i.e. 242.

Finally, extended attributes (and hence, tags) are retrieved read-only. That is because the structure of the `@SynoEAStream` file is undocumented and after reverse-engineering already turned out to be quite complicated just to retrieve the binary data, let alone add or modify extended attributes to that file.
