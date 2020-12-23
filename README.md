# synology-scripts
A collection of (bash) scripts that I've developed over the years to make the use of a Synology NAS more productive (and fun). Some scripts are very Synology-specific (such as the ones dealing with extended attributes) but others are quite generic and may be of use to other 'nix users as well.

There are scripts for:
- **extended-attributes-and-tags**: retrieving and managing extended attributes that are set from Mac OS, such as custom tags and labels and user xattrs, and access them natively from within the Synology NAS.
- **ea-file-bundle-handling**: managing files with these extended attributes without creating inconsistencies in the synology file system.
- **linked-collections** generating and managing directories with hard linked files based on custom criteria, effectively creating collections or repositories from other (source) directories, such as music playlists or movie genres (well, that's what I use it for; your mileage may vary).
- **mac-nfd-conversion**: convert file name character representation between native/normal UTF-8 and MacOSX NFD (normalization form decomposed) representation.
- **file-tracker**: generating checksum files that allow you to keep track of your files, wherever they go.
- **encrypted-multi-disk-backup**: create a backup spanning multiple disks (if necessary), where each disk contains the data in an encrypted file system. The script generates an index and log which allows you to use the disk(s) individually to restore data (as long as you remember the passphrase).

In order to use these, you'll need to be able to execute these scripts from the command line, which you can do from an ssh session or from the task scheduler. Some commands only work properly when run with the appropriate permissions. It's up to the user if this implies root privileges or less.
