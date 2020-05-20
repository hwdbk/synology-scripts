# ea-file-bundle-handling

This directory contains scripts to manage files on a Synology share in conjunction with their attributes sidecar files. For every file stored on a Synology share, Synology may maintain a `@SynoEAStream` and/or `@SynoResource` file. Which file is present (one, both or none) depends on the file attributes as set by the Finder when copying/creating the file, or added later by the Finder. The general structure is:

- `share/directory/file`
- `share/directory/@eaDir/file@SynoEAStream`
- `share/directory/@eaDir/file@SynoResource`

where file could also be a directory:

- `share/directory/dir/`
- `share/directory/@eaDir/dir@SynoEAStream`
- `share/directory/@eaDir/dir@SynoResource`

The scripts handle these files as a file bundle when they are moved/renamed (`mv_with_ea`), deleted (`rm_with_ea`, `rmdir_with_ea`), check if a directory is truly empty (`isemptydir_with_ea`) or create a hard link in a different place (`ln_with_ea`).

# `ln_with_ea`
The `ln_with_ea` script creates a hard link of a file in a different directory on the same file system. It works the same as the `ln` command (without the `-s` because that's for sissies) but also hardlinks (or even backlinks) its associated `@Syno` files if they are present. This allows the hard linked file to share the extended attributes (xattr) of the file it was created from (note: as opposed to `ln -s`, with `ln` there is no such thing as an original file, only an _originating_ file). The great thing about it is that, after hard linking a file, the extended attributes can be changed on the linked file as well as the originating file.

There's one optimisation added to the mix: the (back)linking of the `@SynoEAStream` file only takes place if it contains a 'non-bogus' extended attribute. This is maintained in `xattrs.lst`. This filter was added after I discovered that Apple stores a whole lot of bogus information in the extended attributes (`com.apple.quarantine` is an annoying one, but `com.apple.lastuseddate#PS` is probably the worst). For instance, if you want to link the attribute describing 'the file was downloaded from URL', you should add `com.apple.metadata:kMDItemWhereFroms` and possibly `com.apple.metadata:kMDItemDownloadedDate` to the `xattr.lst`. If you don't want this, just edit line 84 in the script.

The most useful xattr for me is the `**com.apple.metadata:_kMDItemUserTags**`, which is the xattr in which the user tags are stored (both the 'old-style' labels (Finder colours) and the custom Tags. See the directory with tools for handling extended attributes.
