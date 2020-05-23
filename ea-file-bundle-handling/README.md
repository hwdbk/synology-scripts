# ea-file-bundle-handling

This directory contains scripts to manage files on a Synology share in conjunction with their extended attributes' sidecar files. For every file stored on a Synology share, Synology may maintain a `@SynoEAStream` and/or `@SynoResource` file. Which file is present (one, both or none) depends on the file attributes as set by the Finder when copying/creating the file, or added later by the Finder. For extensive information about the gory details on what is stored where and how (and gory they are...), see the separate directory on extended attributes. The general structure on disk (the native Synology ext4 or btrfs file system) is:

- `share/directory/file`
- `share/directory/@eaDir/file@SynoEAStream`
- `share/directory/@eaDir/file@SynoResource`

where file could also be a directory:

- `share/directory/dir/`
- `share/directory/@eaDir/dir@SynoEAStream`
- `share/directory/@eaDir/dir@SynoResource`

The scripts handle these files as a file bundle when they are moved/renamed (`mv_with_ea`), deleted (`rm_with_ea`, `rmdir_with_ea`), check if a directory is truly empty (`isemptydir_with_ea`) or create a hard link in a different place (`ln_with_ea`). There is also a script that gets rid of all extraneous `@Syno` files (`cleanup_SynoFiles`).

`mv_with_ea`

The `mv_with_ea` moves or renames the file or directory to a new location or name and moves/renames the `@Syno` files with it in a consistent manner. The only thing that is not updated instantly is the corresponding FinderInfo data in the cnid.db database of the afpd, but no worry, I've found that that is updated as soon as the file is exposed to the Finder of a connected Mac OS client and the `@Syno` files are queried.

`rm_with_ea`, `rmdir_with_ea`

These scripts work sort of as expected, I guess.

`isemptydir_with_ea`

Script to test if a directory is empty, if the `@eaDir` subdirectory is taken into account (i.e. is ignored - with an `@eaDir` directory present, the directory would never be empty, even without any regular files or directories in it).

`ln_with_ea`

The `ln_with_ea` script creates a hard link of a file in a different directory on the same file system. It works the same as the `ln` command (without the `-s` because that's for sissies) but also hardlinks (or even backlinks) its associated `@Syno` files if they are present. This allows the hard linked file to share the extended attributes (xattr) of the file it was created from (note: as opposed to `ln -s`, with `ln` there is no such thing as an original file, only an _originating_ file). The great thing about it is that, after hard linking a file, the extended attributes can be changed on the linked file as well as the originating file.

There's one optimisation added to the mix: the (back)linking of the `@SynoEAStream` file only takes place if it contains a 'non-bogus' extended attribute. This is maintained in `xattrs.lst`. This filter was added after I discovered that Apple stores a whole lot of bogus information in the extended attributes (`com.apple.quarantine` is an annoying one, but `com.apple.lastuseddate#PS` is probably the worst). For instance, if you want to link the attribute describing 'the file was downloaded from URL', you should add `com.apple.metadata:kMDItemWhereFroms` and possibly `com.apple.metadata:kMDItemDownloadedDate` to the `xattr.lst`. If you don't want this, just edit line 84 in the script.

The most useful xattr for me is the **`com.apple.metadata:_kMDItemUserTags`**, which is the xattr in which the user tags are stored (both the 'old-style' labels (Finder colours) and the custom Tags. See the directory with tools for handling extended attributes and Finder tags.

`cleanup_SynoFiles`

After using the Synology NAS for a while, I found out that it stored literally thousands of files that don't really contribute in a functional way. These files were sometimes left behind after an incomplete cleanup or delete, but are
mostly created through Finder extended attributes and resource forks (actually, that's where the pollution started a
long time ago). It results in every regular file having *at least* two additional sidecar files associated with it, and sometimes more. It does slow down the system, hence this script to get rid of them.

The script scans for and cleans a directory for any extraneous `@Syno` sidecar files. With 'extraneous', I mean:
- the 'stray' `@SynoEAStream` and `@SynoResource` files, i.e. files that don't have a parent (regular) file.
- any subdirectories *inside* the `@eaDir` directories (these are used by Synology to store previews, so if you have
Photo Station, Audio Station or Video Station running, you will not want this), `SYNO_DTIME` delete timestamps (left behind after restoring files from the `#recycle` bin), and other paraphernalia).
- the 'bogus' `@SynoEAStream` and `@SynoResource` files ('bogus' means the files don't have any of the meaningful xattrs listed in xattrs.lst, as discussed above - this might need some tweaking to your own taste as I can't determine which xattrs you find useful).
- when deleting, it also cleans up any empty `@eaDir` directories that are left behind.

When used with `-l`, it just lists the extraneous files on stdout, and does not delete anything.
