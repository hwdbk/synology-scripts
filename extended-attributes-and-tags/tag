#!/bin/bash
# script to interpret binary @SynoEAStream files and extract the tags (labels) from the com.apple.metadata:_kMDItemUserTags bplist structure
# input:
#     usage: tag file
#     the parameter is either the @SynoEAStream file itself of the (mother) file to which the @SynoEAStream file belongs.
#     the script assumes that the com.apple.metadata:_kMDItemUserTags extended attribute is present in the file, so it is wise to grep first before calling this script
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
#     the bplist format is perfectly explained in https://medium.com/@karaiskc/understanding-apples-binary-property-list-format-281e6da00dbd

# generate hex strings. use -c <wide column> to prevent printing of spaces
BPLIST=$(echo -n "bplist" | xxd -ps -c 100)

f=$1
if ! get_attr -x -q com.apple.metadata:_kMDItemUserTags "$f" ; then exit 1 ; fi | while true ; do
	read -n ${#BPLIST} x # (try to) read "bplist" - this SHOULD be at this position in the file or the offset referencing didn't work
	if [[ -z $x ]] ; then break ; fi
	if [[ $x == "$BPLIST" ]] ; then
		read -n 4 x # read 4 hex digits (2 bytes) - this is the version number, usually "00" (but "14" and "18" has also been reported)
	else
		echo "$f: bplist not found (found 0x$x)" >&2 ; exit 1
	fi
	# at this point, we're at the array
	read -n 1 x # this should be the '0xAk' (array marker) - com.apple.metadata:_kMDItemUserTags is encoded as a bplist array of strings
	if [[ $x != "a" ]] ; then echo "$f: array marker not found (found '$x')" >&2 ; exit 1 ; fi
	read -n 1 k # this should be the number of elements
	if [[ $k == "f" ]] ; then
		# multi-byte array length: 0x1t kk [kk ...]
		read -n 1 t ; if [[ $t != "1" ]] ; then echo "$f: unexpected data in multi-byte array length parameter ($t)" >&2 ; exit 1 ; fi
		read -n 1 t
		read -n $((2*2**t)) k # the 4 bits after '1' defines how may bytes we need to describe the length: 2^t bytes
	fi
	k=$((16#$k)) # hex to decimal
#	echo "- '$x' $k" >&2
	read -n $((k*2)) x # skip the object refs
	# at this point, we're at the actual tag strings. these are preceded by 0x5l or 0x6l length byte(s)
	while ((k>0)) ; do
		read -n 1 x
		read -n 1 l
#		echo "-- $x$l" >&2
		if [[ ! ( $x == "5" || $x == "6" ) ]] ; then echo "$f: string marker not found (found 0x$x$l)" >&2 ; exit 1 ; fi
		if [[ $l == "f" ]] ; then
			# multi-byte string length: 0x1t kk [kk ...]
			read -n 1 t ; if [[ $t != "1" ]] ; then echo "$f: unexpected data in multi-byte string length parameter ($t)" >&2 ; exit 1 ; fi
			read -n 1 t
#			echo "--- 1$t" >&2
			read -n $((2*2**t)) l # the 4 bits after '1' defines how may bytes we need to describe the length: 2^t bytes
		fi
#		echo "---- $l" >&2
		l=$((16#$l)) # hex to decimal
		if [[ $x == "5" ]] ; then # regular ASCII string. note that as soon as you use a UTF-8 character, the string becomes UTF-16 ($x is 6)
			while ((l>0)) ; do
				# read the string but ignore the "\n<digit>" at the end, if found
				read -n 2 x ; if [[ $x == "0a" && $l == "2" ]] ; then read -n 2 x ; break; fi
				echo -n "$x" | xxd -ps -r
				((l--))
			done
			echo
		elif [[ $x == "6" ]] ; then # UTF-16 string (output as UTF-8)
			while ((l>0)) ; do
				# read the string but ignore the "\n<digit>" at the end, if found
				read -n 4 x ; if [[ $x == "000a" && $l == "2" ]] ; then read -n 4 x ; break; fi
				echo -n "$x" | xxd -ps -r
				((l--))
			done | iconv -f utf-16 -t utf-8
			echo
		fi
		((k--))
	done
	break # there is only one com.apple.metadata:_kMDItemUserTags per file - bail out after the first
done # while true do read stuff
#EOF
