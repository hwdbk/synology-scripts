# Extended attributes and tags (as seen from the Synology side)

This directory contains a toolset to retrieve Mac OS X -created extended attributes natively from the Synology ext4 or btrfs file system. And, of course, Synology doesn't use unix' native xattr tool to store these. Not by a long shot...

I was triggered on this by new funcionality in the Finder that allowed a user not only to tag a file with Labels (the 'Finder colours' red, orange, yellow, green, blue, purple and gray), but also custom tags. Then I found out that these were stored in the Apple-proprietary `com.apple.metadata:_kMDItemUserTags` extended attribute and that jdberry wrote a cool tool to manage them from the (Mac OS X Terminal) command line:

https://github.com/jdberry/tag

These scripts attempt to do the same natively from the Synology side of the afpd tether. It works in two stages:
- the `get_attr` script retrieves the extended attribute raw data from a file's associated `@SynoEAStream` file. This can be used for any extended attribute, even your own.
- the `tag` script interprets this raw data and translates it according to the Apple data structures laid out for the `com.apple.metadata:_kMDItemUserTags` extended attribute.

Currently, extended attributes (and hence, tags) are retrieved read-only. That is because the structure of the `@SynoEAStream` file is already quite complicated just to retrieve the binary data, let alone add or modify extended attributes to that file.
