#!/bin/bash
# script to interpret binary @SynoEAStream files and extract the tags (labels) from the com.apple.metadata:_kMDItemUserTags bplist structure
# input:
#     usage: tag file
#     the parameter is either the @SynoEAStream file itself of the (mother) file to which the @SynoEAStream file belongs.
#     the script assumes that the com.apple.metadata:_kMDItemUserTags extended attribute is present in the file (so you must grep first before calling this script or you'll get an error).
#     a very efficient way of doing this is with:
#         grep -arlF "com.apple.metadata:_kMDItemUserTags" <path> --include='*@SynoEAStream' | while read f ; do tag "$f" ; done
#     or look at the listtags script.
#     if you know which tag you're looking for (e.g. "Red"), a very efficient way is to further prefilter the list with:
#         grep -arlF "com.apple.metadata:_kMDItemUserTags" <path> --include='*@SynoEAStream' | xargs -d'\n' grep -alF <tag> | while read f ; do tag "$f" ; done
#     or look at the mk_tag_links script.
# output:
#     prints the Finder tags (user tags and Finder labels) associated with file (implements the 'tag -l -N' -equivalent of the mac version).
#     the script prints each found tag on a separate line (implements the '-g | --garrulous : Display tags each on own line' -equivalent of the mac version).
#     if the file does not contain tags (empty com.apple.metadata:_kMDItemUserTags bplist), the script prints nothing.
#     prints a msg on stderr when the input is not according to expectation (parse error).
# note:
#     the formatting of the com.apple.metadata:_kMDItemUserTags varies considerably, depending which application wrote the extended attributes (tag, Finder) or
#     whether the list is empty or not (no com.apple.metadata:_kMDItemUserTags at all or empty bplist).

# generate hex strings. use -c <wide column> to prevent printing of spaces
BPLIST=$(echo -n "bplist" | xxd -ps -c 100)

f=$1
if ! /root/bin/get_attr -x com.apple.metadata:_kMDItemUserTags "$f" ; then exit 1 ; fi | while true ; do
	read -n ${#BPLIST} x # (try to) read "bplist" - this SHOULD be at this position in the file or the offset referencing didn't work
	if [[ -z $x ]] ; then break ; fi
	if [[ $x == $BPLIST ]] ; then
		read -n 4 x # read 4 hex digits (2 bytes) - this is the version number, usually "00" (but "14" and "18" has also been reported)
	else
		echo "$f: bplist not found" >&2 ; exit 1
	fi
	# at this point, we're at the array
	read -n 1 x # this should be the 'A' (array marker)
	if [[ "$x" != "a" ]] ; then echo "$f: array marker not found" >&2 ; exit 1 ; fi
	read -n 1 k # this should be the number of elements
#	echo "- $k"
	read -n $((k*2)) x # skip the object refs
	# at this point, we're at the actual tag strings. these are preceded by 0x5l length byte(s)
	while ((k>0)) ; do
		read -n 1 x ;
#		echo "-- x$x"
		if [[ "$x" != "5" ]] ; then echo "$f: string marker not found" >&2 ; exit 1 ; fi
		read -n 1 l
		if [[ "$l" == "f" ]] ; then
			# multi-byte length - assume it can be coded in 1 byte (255 max)
			read -n 2 x ; if [[ "$x" != "10" ]] ; then echo "$f: multi-byte string length parameter not supported ($x)" >&2 ; exit 1 ; fi
			read -n 2 l 
		fi
		l=$((16#$l)) # hex to decimal
#		echo "--- $l"
		while ((l>0)) ; do
			# read the string but ignore the "\n<digit>" at the end
			read -n 2 x ; if [[ "$x" == "0a" && "$l" == "2" ]] ; then read -n 2 x ; break; fi
			echo -n "$x" | xxd -ps -r
			l=$((l-1))
		done
		echo
		k=$((k-1))
	done
	break # there is only one com.apple.metadata:_kMDItemUserTags per file - bail out after the first
done # while true do read stuff
#EOF
