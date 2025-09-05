# Incremental (encrypted) multi-disk backup

`backup_update_generate` and `backup_update_copy` have been moved to **linux-nas-scripts**

# Extras

- `mount_ecryptfolder` is a small helper script that can be used to manually mount encrypted folders. It wraps `mount.ecryptfs` with the mount parameters prefilled with defaults, which also happen to be the parameters used by Synology to mount an encrypted share from the DSM user interface and the above `backup_diskset_copy` and `backup_update_copy` scripts.
