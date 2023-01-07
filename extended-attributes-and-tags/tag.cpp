// tag.cpp
//
// program to interpret binary @SynoEAStream files and extract the tags (labels) from the com.apple.metadata:_kMDItemUserTags bplist structure
// input:
//	   usage: tag file
//	   the parameter is either a @SynoEAStream file or the (mother) file to which the @SynoEAStream file belongs.
//	   the script assumes that the com.apple.metadata:_kMDItemUserTags extended attribute is present in the file, so it is wise to grep first before calling this script
//	   a very efficient way of doing this is with:
//		   grep -rlF "com.apple.metadata:_kMDItemUserTags" <path> --include='*@SynoEAStream' | while read f ; do tag "$f" ; done
//	   or look at the listtags script.
//	   if you know which tag you're looking for (e.g. "Red"), a very efficient way is to further prefilter the list with:
//		   grep -rlF "com.apple.metadata:_kMDItemUserTags" <path> --include='*@SynoEAStream' | xargs -d'\n' grep -alF <tag> | while read f ; do tag "$f" ; done
//	   or look at the mk_tag_links script.
// output:
//	   prints the Finder tags (user tags and Finder labels) associated with file, each on a separate line,
//	   effectively implementing the 'tag -l -N -g' -equivalent of the jdberry Python script version (--list --no-name --garrulous).
//	   if the file does not contain tags (empty com.apple.metadata:_kMDItemUserTags bplist), the script prints nothing.
//	   prints a msg on stderr when the input is not according to expectation (parse error).
// note:
//	   the formatting of the com.apple.metadata:_kMDItemUserTags varies considerably, depending which application wrote the extended attributes (tag, Finder) or
//	   whether the list is empty or not (no com.apple.metadata:_kMDItemUserTags at all or empty bplist).
//	   the bplist format itself is perfectly explained in https://medium.com/@karaiskc/understanding-apples-binary-property-list-format-281e6da00dbd

#include "get_attr.h"

#include <locale>
#include <codecvt>

// string (utf8) -> u16string -> wstring
static std::wstring utf8_to_utf16(const std::string& utf8)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert; 
	std::u16string utf16 = convert.from_bytes(utf8);
	std::wstring wstr(utf16.begin(), utf16.end());
	return wstr;
}

// wstring -> u16string -> string (utf8)
static std::string utf16_to_utf8(const std::wstring& utf16)
{
	std::u16string u16str(utf16.begin(), utf16.end());
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert; 
	std::string utf8 = convert.to_bytes(u16str);
	return utf8;
}

static std::string read(std::string& str, int n)
{
	std::string result = str.substr(0,n);
	str = str.substr(n);
	return result;
}

int main(int argc, char** argv)
{
	std::string file = argv[1];
	std::string hex;
	if (! get_attr("com.apple.metadata:_kMDItemUserTags", file, true, true, hex)) return 1;
	
	// (try to) read "bplist" - this SHOULD be at this position in the file or the offset referencing didn't work
	std::string bplist = read(hex, 12);
	if (bplist != tohex("bplist")) { std::cerr << "tag: error in " << file << ":bplist not found (found 0x" << bplist << ")" << LF ; return 1; }
	
	std::string x,k,l,t;
	x = read(hex, 4); // read 4 hex digits (2 bytes) - this is the version number, usually "00" (but "14" and "18" has also been reported)
	// at this point, we're at the array
	x = read(hex, 1); // this should be the '0xAk' (array marker) - com.apple.metadata:_kMDItemUserTags is encoded as a bplist array of strings
	if ( x != "a" ) { std::cerr << "tag: error in " << file << ": array marker not found (found '" << x << "')" << LF; return 1; }
	k = read(hex, 1); // this should be the number of elements
	if ( k == "f" ) {
		// multi-byte array length: 0x1t kk [kk ...]
		t = read(hex, 1); if ( t != "1" ) { std::cerr << "tag: error in " << file << ": unexpected data in multi-byte array length parameter (" << t << ")" << LF; return 1; }
		t = read(hex, 1);
		k = read(hex, 2*(1<<fromhex(t))); // the 4 bits after '1' defines how may bytes we need to describe the length: 2^t bytes
	}
	int kk = fromhex(k); // hex to decimal
//	std::cerr << "- '" << x << "' " << kk << LF;
	x = read(hex, 2*kk); // skip the object refs
	// at this point, we're at the actual tag strings. these are preceded by 0x5l or 0x6l length byte(s)
	while ( kk>0 ) {
		x = read(hex, 1);
		l = read(hex, 1);
//		std::cerr << "-- " << x << l << LF;
		if (! ( x == "5" || x == "6" ) ) { std::cerr << "tag: error in " << file << ": string marker not found (found 0x" << x << l << ")" << LF; return 1; }
		if ( l == "f" ) {
			// multi-byte string length: 0x1t kk [kk ...]
			t = read(hex, 1) ; if ( t != "1" ) { std::cerr << "tag: error in " << file << ": unexpected data in multi-byte string length parameter (" << t << ")"; return 1; }
			t = read(hex, 1);
//			std::cerr << "--- 1" << t << LF;
			l = read(hex, 2*(1<<fromhex(t))); // the 4 bits after '1' defines how may bytes we need to describe the length: 2^t bytes
		}
//		std::cerr << "---- " << l << LF;
		int ll=fromhex(l); // hex to decimal
		if ( x == "5" ) { // regular ASCII string. note that as soon as you use a UTF-8 character, the string becomes UTF-16 ($x is 6)
			while ( ll>0 ) {
				// read the string but ignore the "\n<digit>" at the end, if found
				x = read(hex, 2); if ( x == "0a" && ll == 2 ) { x = read(hex, 2) ; break; }
				std::cout << static_cast<char>(fromhex(x));
				--ll;
			}
			std::cout << LF;
		}
		else if ( x == "6" ) { // UTF-16 string (output as UTF-8)
			std::wstring out;
			while ( ll>0 ) {
				// read the string but ignore the "\n<digit>" at the end, if found
				x = read(hex, 4); if ( x == "000a" && ll == 2 ) { x = read(hex, 4) ; break; }
				out += static_cast<wchar_t>(fromhex(x));
				--ll;
			}
			std::cout << utf16_to_utf8(out) << LF;
		}
		--kk;
	}
}

//EOF
