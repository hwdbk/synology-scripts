# Linked collections

Scripts to scan your file system for files/directories tagged with your Finder tags and collect them in a directory by creating links to them.
The linked items don't take up more disk space because it uses hard links (for files) and symlinks (for directories). There's a variant for
files and for directories, and variants that auto-create the collection directories or the scripts that require you to predefine the output directories.
The latter case allows you to filter which tags makes it into the collection.

- `mk_tag_links` creates hard links to files in a collection directory, but only if the directory with the tag name exists.
- `mk_tag_links_and_dirs` creates hard links to files in a collection directory, and auto-creates the output directories.
- `mk_tag_dirlinks` creates symlinks to directories in a collection directory, but only if the directory with the tag name exists.
- `mk_tag_dirlinks_and_dirs` creates symlinks to directories in a collection directory, and auto-creates the output directories.

## Example
