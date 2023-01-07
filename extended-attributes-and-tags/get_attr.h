// get_attr.h
//	code to interpret binary @SynoEAStream files and extract the xattr from the structure
//      bool get_attr(const std::string& attr_name, const std::string& file, bool useHex, bool quiet, std::string& result)
//	input:
//	    saves the value of the xattr with key attr_name in result. use useHex = true to print output in hex (useful for binary values)
//	    the file parameter is either the @SynoEAStream file itself of the (mother) file to which the @SynoEAStream file belongs. if the @SynoEAStream file isn't there, you'll get an error.
//	    the script assumes that the attr_name extended attribute is present in the file. if not, you'll get an error, unless called with quiet = true.
//	    a very efficient way of checking whether the attribute is present in the file is with:
//	        grep -rlF <attr_name> <path> --include='*@SynoEAStream' | while read f ; do get_attr <attr_name> "$f" ; done
//	    (look at the listtags script, which uses this mechanism)
//	output:
//	    the code returns the value of the attribute and a result code
//	    if the file does not contain the attribute, the script prints nothing (with quiet) or an error (without quiet)
//	    prints a msg on stderr when the input is not according to expectation (parse error)
//	note:
//	    the location of the attribute's value is not related to the location of the attribute key string itself, or so investigation revealed
//	    (they are not always following each other). reverse-engineering learnt that the attribute key string is preceded by a string length byte and 10 index bytes
//	    comprising of 4 bytes file offset to the value data, 4 bytes length of the value data and two bytes 0x00.

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

const char TAB = '\t';
const char LF  = '\n';

static void shift(int& argc, char**& argv, int pos = 1)
{
    if(pos < argc) {
        for(int i=pos; i<argc; ++i)
            argv[i] = argv[i+1];
        --argc;
    }
}

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && str.compare(str.size()-suffix.size(), suffix.size(), suffix) == 0;
}

static bool startsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

static std::string dirname(const std::string& path)
{
	std::string p = path;
	while (endsWith(p,"/")) p = p.substr(0, p.length()-1);
	size_t found = p.find_last_of("/");
	if (found == std::string::npos) return ".";
	return p.substr(0,found);
}

static std::string basename(const std::string& path)
{
	std::string p = path;
	while (endsWith(p,"/")) p = p.substr(0, p.length()-1);
	size_t found = p.find_last_of("/");
	if (found == std::string::npos) return p;
	return p.substr(found+1);
}

static std::string tohex(const char* str, size_t len)
{
	static const char* hexchr = "0123456789abcdef"; // we use lower case a-f
	std::string hex;
	for (int n=0; n<len; ++n) { unsigned char ch = str[n]; hex += hexchr[ch/16]; hex += hexchr[ch%16]; }
	return hex;
}
static std::string tohex(const std::string& str)
{
	return tohex(str.c_str(), str.length());
}

static uint64_t fromhex(const char* str, size_t len)
{
	static bool once = true;
	static unsigned char chrhex[256];
	if (once) {
		for(int n=0; n<256; ++n) chrhex[n] = 0;
		for(int n='0'; n<='9'; ++n) chrhex[n] = (n-'0');
		for(int n='a'; n<='f'; ++n) chrhex[n] = (n-'a'+10); // we use lower case a-f
		once=false;
	}
	uint64_t result = 0;
	for (int n=0; n<len; ++n) result = result*16 + chrhex[str[n]];
	return result;
}
static uint64_t fromhex(const std::string& str)
{
	return fromhex(str.c_str(), str.length());
}

static bool get_attr(const std::string& attr, const std::string& fname, bool useHex, bool quiet, std::string& result)
{
	std::string file = fname;
	if (! endsWith(file, "@SynoEAStream"))
		file = dirname(file) + "/@eaDir/" + basename(file) + "@SynoEAStream";
	//std::cerr << file << LF;
	
	std::ifstream fin(file, std::ios::in | std::ios::binary | std::ios::ate);
    if(!fin.is_open()) {
    	if (! quiet) std::cerr << "get_attr: error opening " << file << LF;
    	return false;
    }
	std::streamsize size = fin.tellg();
	fin.seekg(0, std::ios::beg);

	// parse the @SynoEAStream as a hex string since it is a binary file
	std::vector<char> buffer(size);
	if (fin.read(buffer.data(), size))
	{
		// convert @SynoEAStream file to hex and skip to the attribute key string
		//std::cerr << "read " << size << " bytes from " << file << LF;
		std::string hex = tohex(buffer.data(), size); // read the entire file in a hex string
		char keylen = attr.length()+1;
		std::string keyhex = tohex(&keylen, 1) + tohex(attr.data(), attr.length()) + "00"; // read the key in a hex string and append with 00, then prepend with length byte
		//std::cerr << hex << LF << keyhex << LF;
		size_t keyoff = hex.find(keyhex);
		if (keyoff == std::string::npos) {
			if (! quiet) { std::cerr << "get_attr: " << file << ": No such xattr: " << attr << LF; }
			return false;
		}
		//std::cerr << "key offset is " << keyoff << LF;
		size_t keyhdr = keyoff - 20; // the key header is 11 bytes before the key itself, i.e. 10 bytes before the key length byte
		size_t valoff = fromhex(hex.data()+keyhdr,   8); // and consist of a 4-byte offset to the xattr value
		size_t vallen = fromhex(hex.data()+keyhdr+8, 8); // and a 4-byte length of the xattr value
		// (then two bytes padding + one byte strlen of the key (incl. the trailing 00) and the key string itself)
		//std::cerr << "value at " << valoff << " with length " << vallen << LF;
		if (useHex) {
			result = hex.substr(valoff*2,vallen*2);
		}
		else {
			result = "";
			for (int n=0 ; n<vallen ; ++n)
				result += static_cast<char>(fromhex(hex.substr((valoff+n)*2,2))); // convert hex to string byte (char) by byte
		}
	}
	return true;
}

// EOF
