# Linked collections

Scripts to scan your file system for files/directories tagged with your Finder tags and collect them in a directory by creating links to them.
The linked items don't take up more disk space because it uses hard links (for files) and symlinks (for directories). There's a variant for
files and for directories, and variants that auto-create the collection directories or the scripts that require you to predefine the output directories.
The latter case allows you to filter which tags makes it into the collection.

- `mk_tag_links` creates hard links to files in a collection directory, but only if the directory with the tag name exists.
- `mk_tag_links_and_dirs` creates hard links to files in a collection directory, and auto-creates the output directories.
- `mk_tag_dirlinks` creates symlinks to directories in a collection directory, but only if the directory with the tag name exists.
- `mk_tag_dirlinks_and_dirs` creates symlinks to directories in a collection directory, and auto-creates the output directories.
- `cleanup_links` is used by the above scripts to cleanup remnants in the collection directory. Remnants are: files whose tag has
been removed or renamed, or files that are no longer in the source tree. The script has several options to move deleted files to a trash
directory and/or to cleanup any empty collection subdirectories (when a certain tag is no longer used).

## Convention

The collection directory name is assumed to end with a `-`; that's to terminate recursion if the collection directory is within the source
tree that is scanned. It also makes it easier to spot the linked files, and and indication that its contents are linked files and can be
deleted safely, not touching the originals.

Symlinks to directories are created with names ending in `~`; that's just for convenience and a visual indication that it is a symlink
created with this mechanism.

The tags are standard or custom Finder tags. That is, any of the 'label colours' Red, Orange, Yellow, Green, Blue, Purple or Gray, or any custom
tag that you make up yourself. Tag may contain slashes (`/`) to indicate subtags (see example below).

## Example

Suppose you have a collection of movies in, say, `/volume/share/movies`. There may be any number of subdirectories and stucture below this. Now
you want to create a cross-collection with genres and you assign Finder custom tags to a number of movies based on its genre, such as "Science
Fiction", "Drama", "Art movie" and you want these genres to go in `/volume/share/movies/genres-`. Note the `-` at the end, because `genres-` is
located inside the source tree `movies` and the `-` will prevent endless recursion.

Then, the command `mk_tag_links_and_dirs /volume/share/movies /volume/share/movies/genres-` will scan the movies, find the tags on those files and create
the directories `/volume/share/movies/genres-/Science Fiction`, `/volume/share/movies/genres-/Drama` and `/volume/share/movies/genres-/Art movie`
as it discovers the movies tagged as such, and creates hard links to those movies inside the genre directory. It even works with subgenres
(subtags); for instance, when you tag a movie with "Science Fiction/Robot", the hard link will appear in the directory created `/volume/share/movies/genres-/Science Fiction/Robot`.
The algorithm is self-managing: if you change tags (add, remove, rename), the links will be adjusted accordingly due to the cleanup mechanism in `cleanup_links`.

## Hard links

The scripts use hard links for files throughout. It would use hard links for directories, if that were possible. It isn't, hence the symlinks. Hard links are great because they remain in tact if any of the linked files are renamed or moved around (as opposed to symlinks, which immediatly stop working in those
cases). The scripts use the `ln_with_ea` scripts to keep consistent links between the actual file and its extended attributes. This effectively creates the
`@SynoEAStream` file as a hard link sidecar file, which in turn allows you to tag the 'originals' or the 'linked copies'. Again, in the above example, if
you tag the movie inside `/volume/share/movies/genres-/Science Fiction/Robot` with "Art movie", running the script a second time will create another hard link to
the movie (and its `@Syno` files) inside `/volume/share/movies/genres-/Art movie`. So, you don't have to locate the original Robot movie to do so.

## Other uses

The mechanism of creating file or directory collections by tag is powerful. For instance, when working from a directory tree with master photos, albums can be
created by tagging the appropriate photos with the album name. Also, the collection mechanism isn't necessary limited to tagging files (i.e. through the
`com.apple.metadata:_kMDItemUserTags` extended attribute from the Finder). This is just the mechanism used by the above scripts (and because it is easy to use and appeals tot the imagination).
Other applications might involve extracting or calculating a certain piece from the file's other attributes, such as part of the filename, modification date
or file size. Example: suppose your movies are named "Title (year).ext", then "year" could be extracted from the name and used as if it were a tag, creating
a collection of movies by year. As long as the linked files are tracked in a listing file, maintenance and cleanup can be done with `cleanup_links`.
