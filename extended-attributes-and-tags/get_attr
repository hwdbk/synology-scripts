#!/bin/bash
# script to interpret binary @SynoEAStream files and extract the xattr from the structure
# input:
#     get_attr [-x] [-q] attr_name file
#     prints the value of the xattr with key attr_name belonging to the file. use -x to print output in hex (useful for binary values)
#     the file parameter is either the @SynoEAStream file itself of the (mother) file to which the @SynoEAStream file belongs. if the @SynoEAStream file isn't there, you'll get an error.
#     the script assumes that the attr_name extended attribute is present in the file. if not, you'll get an error, unless called with -q (quiet).
#     a very efficient way of checking whether the attribute is present in the file is with:
#         grep -arlF <attr_name> <path> --include='*@SynoEAStream' | while read f ; do tag "$f" ; done
#     (look at the listtags script, which uses this mechanism)
# output:
#     the script prints the value of the attribute
#     if the file does not contain the attribute, the script prints nothing (with -q) or an error (without -q)
#     prints a msg on stderr when the input is not according to expectation (parse error)
# note:
#     the location of the attribute's value is not related to the location of the attribute key string itself, or so investigation revealed
#     (they are not always following each other). reverse-engineering learnt that the attribute key string is preceded by a string length byte and 10 index bytes
#     comprising of 4 bytes file offset to the value data, 4 bytes length of the value data and two bytes 0x00.

while [[ $1 == -* ]] ; do
	if   [[ $1 == "-x" ]] ; then usehex=y
	elif [[ $1 == "-q" ]] ; then quiet=y
	else echo "error: unknown option $1" >&2 ; exit 1 ; fi
	shift
done
if [[ $# != 2 ]] ; then echo "usage: $0 [-x] [-q] attr_name file" >&2 ; exit 1 ; fi
if [[ $2 == *@SynoEAStream ]] ; then
	f=$2
else
	f="$(dirname "$2")/@eaDir/$(basename "$2")@SynoEAStream"
fi
if [[ ! -f $f ]] ; then if [[ ! -n $quiet ]] ; then echo "$0: $f not found" >&2 ; fi ; exit 1 ; fi
# convert @SynoEAStream file to hex and skip to the attribute key string
#if ! grep -qF "$1" "$f" ; then echo "$f: no $1 attribute found" >&2; exit ; fi
# parse the @SynoEAStream as a hex string since it is a binary file
hex=$(xxd -ps "$f" | tr -d '\n') # read the entire file in a hex string (strip the newlines)
keyhex=$(xxd -ps - <<< "$1" | tr -d '\n') ; keyhex=${keyhex:0:-2}00 # read the key in a hex string and strip the \n (0a) at the end
keyhdr=${hex%%$keyhex*}
if [[ -z $hex || ${#hex} == ${#keyhdr} ]] ; then if [[ ! -n $quiet ]] ; then echo "$0: $f: No such xattr: $1" >&2 ; fi ; exit 1 ; fi
keyhdr=${keyhdr: -22} # the key header is 11 bytes before the key itself
valoff=$((16#${keyhdr:0:8})) # and consist of a 4-byte offset to the xattr value
vallen=$((16#${keyhdr:8:8})) # and a 4-byte length of the xattr value
keylen=$((16#${keyhdr:20:2})) # and two bytes padding + one byte strlen of the key (incl. the trailing 00)
if (( keylen*2 != ${#keyhex} )) ; then if [[ ! -n $quiet ]] ; then echo "$0: $f: xattr keylen mismatch: $1" >&2 ; fi ; exit 1 ; fi
#printf "value at 0x%08x with length 0x%08x\n" $valoff $vallen
if [[ -n $usehex ]] ; then
	echo "${hex:$((valoff*2)):$((vallen*2))}"
else
	echo "${hex:$((valoff*2)):$((vallen*2))}" | xxd -ps -r -
fi
#EOF
