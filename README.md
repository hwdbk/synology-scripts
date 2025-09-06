# synology-scripts
[UPDATE] As of August 2025, I will no longer be maintaining this code set, simply because I've moved away from Synology. Instead, I've started to port the same code set to a generic linux distribution (Ubuntu in my case, but they should work on any Linux distribution), giving the same functionality without the Synology quirks, and for me a better and supportable future. [END UPDATE]

A collection of (bash) scripts (and some C++ code) that I've developed over the years for a Synology NAS.
These scripts are very Synology-specific (such as the ones dealing with extended attributes, stored in @eaDir/*@Syno* sidecar files)
For the more generic scripts, look in the **linux-nas-scripts** repository instead.
I'm currently running DSM7 and, as far as these scripts were concerned, I've found no problems when upgrading from DSM6.2 to DSM7.0.

[NOTE ON COMPILING CODE FOR SYNOLOGY] Since Spring 2022, when I found it no longer safe to use Entware binaries (g++, in this particular case), I have not been able to compile the C++ programs. Any changes since then, I have not been able to test/verify. I strongly recommend to compile the code in a safe Docker container or on a standalone linux distribution that has the same kernel version as Synology uses for your model (and you'll find out this is pretty ancient). The resulting binaries _should_ work.

There are scripts for:
- **extended-attributes-and-tags**: retrieving and managing extended attributes that are set from Mac OS, such as custom tags and labels and user xattrs, and access them natively from within the Synology NAS.
- **ea-file-bundle-handling**: managing files with these extended attributes without creating inconsistencies in the synology file system.
- **linked-collections** [moved to  **linux-nas-scripts**]
- **mac-nfd-conversion**: convert file name character representation between native/normal UTF-8 and MacOSX NFD (normalization form decomposed) representation.
- **file-tracker** [moved to  **linux-nas-scripts**]
- **encrypted-multi-disk-backup** [moved to  **linux-nas-scripts**]

In order to use these, you'll need to be able to execute these scripts from the command line, which you can do from an ssh session or from the task scheduler. Some commands only work properly when run with the appropriate permissions. It's up to the user if this implies root privileges or less.
