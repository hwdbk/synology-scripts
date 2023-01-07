// get_attr.cpp
//	program to interpret binary @SynoEAStream files and extract the xattr from the structure
//	input:
//		get_attr [-x] [-q] attr_name file
//		prints the value of the xattr with key attr_name of file. use -x to print output in hex (useful for binary values)
//		the file parameter is either the @SynoEAStream file itself of the (mother) file to which the @SynoEAStream file belongs. if the @SynoEAStream file isn't there, you'll get an error.
//		the script assumes that the attr_name extended attribute is present in the file. if not, you'll get an error, unless called with -q (quiet).
//		a very efficient way of checking whether the attribute is present in the file is with:
//			grep -rlF <attr_name> <path> --include='*@SynoEAStream' | while read f ; do get_attr <attr_name> "$f" ; done
//		(look at the listtags script, which uses this mechanism)
//	output:
//		the script prints the value of the attribute
//		if the file does not contain the attribute, the script prints nothing (with -q) or an error (without -q)
//		prints a msg on stderr when the input is not according to expectation (parse error)
//
//	compile with: g++ -o get_attr get_attr.cpp -lstdc++

#include "get_attr.h"

int main(int argc, char** argv)
{
	//std::cout << argc << LF ; for(int i=0; i<argc; ++i) std::cout << argv[i] << LF;
	//std::cout << "'" << dirname(argv[1]) << "'   '" << basename(argv[1]) << "'" << LF; return 0;
	//int n=0x12345678 ; std::cout << tohex((const char*)&n,4) << LF ; return 0;
	//std::cout << fromhex("400") << LF; return 0;
	
	bool error = false;

	// process options
	bool useHex = false, quiet = false;
	for(int i=1 ; i<argc ; ) {
		if(std::string(argv[i]) == "-x") {
			useHex = true;
			shift(argc,argv,i);
		}
		else if(std::string(argv[i]) == "-q") {
			quiet = true;
			shift(argc,argv,i);
		}
		else if(argv[i][0] == '-') {
			std::cerr << "unknown option: " << argv[i] << LF;
			error = true;
			shift(argc,argv,i);
		}
		else
			++i;
	}

	// get the input params - these should be the only args left
	if (error || argc != 3) {
		std::cerr << "usage: " << argv[0] << " [-x] [-q] attr_name file" << LF;
		return 1;
	}
	
	std::string result;
	if (! get_attr(argv[1], argv[2], useHex, quiet, result)) return 1;
	std::cout << result;
	return 0;	
}

// EOF
