// md5diff.cpp
// compile with: gcc -o md5diff -lstdc++ md5diff.cpp

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <set>

#include <unistd.h>
#include <sys/stat.h>

#define TAB '\t'
#define LF  '\n'

// compound comparisons
#define EQ1(L1,       R1      ) ((L1) == (R1))
#define LT1(L1,       R1      ) ((L1) <  (R1))
#define LT2(L1,L2,    R1,R2   ) (LT1(L1, R1) || (EQ1(L1, R1) && LT1(L2, R2)))
#define LT3(L1,L2,L3, R1,R2,R3) (LT1(L1, R1) || (EQ1(L1, R1) && LT2(L2,L3, R2,R3)))

inline std::string UFBaseDir  (const std::string& path) { return path.substr(0,path.find_last_of("/") - 1); }
inline std::string UFFileName (const std::string& path) { return path.substr(path.find_last_of("/") + 1); } // filename including extension
inline std::string UFExtension(const std::string& path) { return path.substr(path.find_last_of(".") + 1); } // extension only
inline bool        UFExists   (const std::string& path) { struct stat buffer; return (stat(path.c_str(), &buffer) == 0); }

static bool sUseModDate = true;  // use modification date in comparisons?
static bool sNoMD5      = false; // don't use the md5sum but use the moddate and file size instead (for .lst files) (requires sUseModDate true)

struct BupInfo
{
//	std::string bupdate;
	std::string moddate;
	std::string fsize;
//	std::string inode; std::string disk;
	std::string md5sum;
	std::string path;    // full path
	std::string fname;   // file name only, calculated field
};

struct lt_path                 { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return LT1(lhs.path, rhs.path); } };
struct lt_md5sum               { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return LT1((sNoMD5?lhs.fsize:lhs.md5sum),          (sNoMD5?rhs.fsize:rhs.md5sum)); } };
struct lt_md5sum_path          { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return LT2((sNoMD5?lhs.fsize:lhs.md5sum),lhs.path, (sNoMD5?rhs.fsize:rhs.md5sum),rhs.path); } };
struct lt_md5sum_moddate       { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return sUseModDate ? LT2((sNoMD5?lhs.fsize:lhs.md5sum),lhs.moddate, (sNoMD5?rhs.fsize:rhs.md5sum),rhs.moddate) : LT1(lhs.md5sum, rhs.md5sum); } };
struct lt_md5sum_fname_moddate { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return sUseModDate ? LT3((sNoMD5?lhs.fsize:lhs.md5sum),lhs.fname,lhs.moddate, (sNoMD5?rhs.fsize:rhs.md5sum),rhs.fname,rhs.moddate) : LT2(lhs.md5sum,lhs.fname, rhs.md5sum,rhs.fname); } };

std::ostream& operator<< (std::ostream& strm, const BupInfo& info)
{
	strm << info.moddate << TAB << info.fsize << TAB << info.md5sum << TAB << info.path;
	return strm;
}

#define copy_list(A, src, dst) { for(A::iterator n = src.begin(); n != src.end(); ++n) dst.insert(*n); }
#define print_list(A, lst, pfx) { for(A::iterator n = lst.begin(); n != lst.end(); ++n) std::cout << pfx << *n << LF; }

typedef std::set     <BupInfo, lt_md5sum_path>          BupInfoList;
	// file list sorted by md5sum+path (unique). if only md5sum was used, the list would not be
	// unique (think about the next.gif and back.gif files in common web pages, for instance)
typedef std::multiset<BupInfo, lt_md5sum_fname_moddate> BupInfoMoved;
	// same list but sorted by md5sum+fname+moddate (not unique) -> used to find moved items
typedef std::multiset<BupInfo, lt_md5sum_moddate>       BupInfoRenamed;
	// same list but sorted by md5sum+moddate (not unique) -> used to find renamed items
typedef std::set     <BupInfo, lt_path>                 BupInfoPrint;
	// same list but sorted by file path -> used for printout and to find modified items

static bool read_info(std::istream& fin, BupInfo& info)
{
	std::string line; int cnt = 0;
	while(std::getline(fin, line)) {
		++cnt;
		if(line[0] == '#') continue; // skip comment lines
		std::stringstream tokens(line); std::string s1,s2,s3,s4;
		std::getline(tokens, s1, TAB);
		std::getline(tokens, s2, TAB);
		std::getline(tokens, s3, TAB);
		std::getline(tokens, s4, TAB);
		if(s4 != "") { // a .md5 or .fst file (modification_date<tab>file_length<tab>[md5sum]<tab>file_path)
			info.moddate = s1;
			info.fsize   = s2;
			info.md5sum  = s3; // note: in output created from mkfilelist_fast, this field is empty
			info.path    = s4;
		}
		else if(s2 != "") { // a .md5sum file: moddate and fsize columns missing (md5sum<tab>file_path)
			info.moddate = "";
			info.fsize   = "";
			info.md5sum  = s1;
			info.path    = s2;
		}
		else {
			std::cerr << "WARNING: skipping misformed line " << cnt << ": '" << line << "'" << LF;
			continue; // skip misformed line (instead of returning false)
		}
		info.fname = UFFileName(info.path); // calculated field: file name only of the full path
		
//		if(info.fname == ".DS_Store" || info.fname.substr(0,2) == "._") continue; // skip stupid files and hidden files
		if(info.moddate == "") sUseModDate = false; // file contains empty moddate - can't use in comparisons
		
		return true; // found a good info line
	}
	return false;
}

int main(int argc, char **argv)
{
	if(argc >=4 && std::string(argv[argc-1]) == "-r") {
		// swap argv[argc-2] and argv[argc-3]
		char* t = argv[argc-2] ; argv[argc-2] = argv[argc-3] ; argv[argc-3] = t;
		--argc;
	}
	std::string fnameL, fnameR;
	if(argc >= 3) {
		fnameL = argv[argc-2];
		fnameR = argv[argc-1];
	}
	
	if(UFExtension(fnameL) == "md5sum" || UFExtension(fnameR) == "md5sum")
		sUseModDate = false;
	if(UFExtension(fnameL) == "fst" || UFExtension(fnameR) == "fst")
		sNoMD5 = true;

	if(argc >= 3 && UFExists(argv[argc-2]) && UFExists(argv[argc-1])) {
		if(argc == 4 && std::string(argv[1]) == "-m")
			sUseModDate = false;
		if(argc == 4 && std::string(argv[1]) == "-n")
			sNoMD5 = true;
	}
	else {
		std::cout << "usage: md5diff [OPTIONS] file1 file2 [-r]" << LF;
		std::cout << "options:" << LF;
		std::cout << "  -m: ignore modification date fields (use md5sum only)" << LF;
		std::cout << "  -n: ignore md5sum checksum fields (use modification date and file size only)" << LF;
		std::cout << "  -r: reverse (swap) file1 and file2" << LF;
		std::cout << "reads tab-separated files:" << LF;
		std::cout << "  .md5: files created with mkfilelist_md5 or mkfilelist_tag" << LF;
		std::cout << "        with lines formatted like moddate<tab>fsize<tab>md5sum<tab>path" << LF;
		std::cout << "  .fst: files created with mkfilelist_fast" << LF;
		std::cout << "        with lines formatted like moddate<tab>fsize<tab><tab>path" << LF;
		std::cout << "  .md5sum: simple md5 files created directly with md5sum" << LF;
		std::cout << "        with lines formatted like md5sum<tab>path" << LF;
		std::cout << "skips lines starting with a #" << LF;
		exit(1);
	}
	
	if(sUseModDate == false && sNoMD5 == true) {
		std::cout << "input files not compatible for comparison" << LF;
		exit(1);
	}

	// build initial file lists
	// note: the BupInfoList sort order is independent of sUseModDate
	//       (this is important because its value may change during reading the files)
	BupInfoList listL; int countL = 0;
	{	std::ifstream finL; finL.open(fnameL);
		if(finL.is_open()) {
			BupInfo info;
			while(read_info(finL, info)) {
				++countL;
				std::pair<BupInfoList::iterator, bool> ins = listL.insert(info);
				if(!ins.second) { // insertion failed
					// panic (this shouldn't happen because the path part is definitely unique)
					std::cerr << "ERROR: failed to insert " << info.path << " in lhs list; md5sum already found in " << ins.first->path << LF;
				}
			}
			finL.close();
		}
	}
	std::cout << "# Read " << countL << " items from " << fnameL << LF;
	
	// first get rid of all identical items (identical here is based on md5sum+path).
	// do this while reading the rhs file (for performance reasons).
	BupInfoList listR; int countR = 0;
	{	std::ifstream finR; finR.open(fnameR);
		if(finR.is_open()) {
			BupInfo info;
			while(read_info(finR, info)) {
				++countR;
				BupInfoList::iterator f = listL.find(info);
				if(f != listL.end()) // found item with matching md5sum and path
					listL.erase(f); // don't print anything here because this is the majority
				else
					listR.insert(info);
			}
			finR.close();
		}
	}
	std::cout << "# Read " << countR << " items from " << fnameR << LF;
	if(sUseModDate)
		std::cout << "# " << "Using file modification dates and " << (sNoMD5 ? "file size" : "md5sum") << LF;
	else
		std::cout << "# " << "Using md5sums only, ignoring file modification dates and file size" << LF;
	
	// now find the items that have been moved from one place to another
	// this is tricky because the md5sum is not unique. if md5sum is the same,
	// so is the file length. most likely, the file name is also the same since it is
	// likely that it is a copy of a certain file (next.gif, back.gif) or a simple file
	// with common default settings (empty resource file, .DS_Store). same is true
	// for the moddate (if it is a copy of next.gif, for instance). so, the moved
	// item is best characterized by an identical md5sum+fname+moddate
	BupInfoPrint moved;
	BupInfoMoved movedL, movedR;
	{
		BupInfoList& srcL = listL, srcR = listR;
		copy_list(BupInfoList, srcL, movedL);
		for(BupInfoList::iterator n = srcR.begin(); n != srcR.end(); ++n) {
			BupInfoMoved::iterator f = movedL.find(*n);
			if(f != movedL.end()) { // found item with matching md5sum and path
				BupInfo info = *f; info.fname = n->path; // misuse fname field
				moved.insert(info);
				movedL.erase(f);
			}
			else
				movedR.insert(*n); // movedR is a multiset - no failed inserts here
		}
		std::cout << LF << "# Moved " << moved.size() << " items between " << fnameL << " and " << fnameR << LF;
		for(BupInfoPrint::iterator n = moved.begin(); n != moved.end(); ++n)
			std::cout << "=\t" << *n << "\t->\t" << n->fname << LF;
	}
	
	// now find the items that have been renamed (same dir or from one place to another)
	// item is best characterized by an identical md5sum+moddate
	BupInfoPrint renamed;
	BupInfoRenamed renamedL, renamedR;
	{
		BupInfoMoved& srcL = movedL, srcR = movedR;
		copy_list(BupInfoMoved, srcL, renamedL);
		for(BupInfoMoved::iterator n = srcR.begin(); n != srcR.end(); ++n) {
			BupInfoRenamed::iterator f = renamedL.find(*n);
			if(f != renamedL.end()) { // found item with matching md5sum and path
				BupInfo info = *f; info.fname = n->path; // misuse fname field to store rhs path for nice printout
				renamed.insert(info);
				renamedL.erase(f);
			}
			else
				renamedR.insert(*n); // renamedR is a multiset - no failed inserts here
		}
		std::cout << LF << "# Renamed " << renamed.size() << " items between " << fnameL << " and " << fnameR << LF;
		for(BupInfoPrint::iterator n = renamed.begin(); n != renamed.end(); ++n)
			std::cout << ">\t" << *n << "\t->\t" << (UFBaseDir(n->path) == UFBaseDir(n->fname) ? UFFileName(n->fname) : n->fname) << LF;
	}
	
	// now find the items that have been modified
	// this is tricky because the md5sum has changed, the file length may have
	// changed, the moddate has changed and possibly the file name has changed (for
	// instance after correcting a song title in iTunes). to keep things simple,
	// select the items whose path is still the same but md5sum has changed.
	// if everything (moddate, fsize, md5sum, path) is different, the item will show up
	// as a -deleted and +added pair in the final listings.
	BupInfoPrint modified;
	BupInfoPrint modifiedL, modifiedR;
	{
		BupInfoRenamed& srcL = renamedL, srcR = renamedR;
		copy_list(BupInfoPrint, srcL, modifiedL);
		for(BupInfoMoved::iterator n = srcR.begin(); n != srcR.end(); ++n) {
			BupInfoRenamed::iterator f = modifiedL.find(*n);
			if(f != modifiedL.end()) { // found item with matching path
				BupInfo info = *f; info.fname = n->moddate + "\t" + n->fsize; // misuse fname field to store rhs moddate\tfsize for nice printout
				modified.insert(info);
				modifiedL.erase(f);
			}
			else
				modifiedR.insert(*n);
		}
		std::cout << LF << "# Modified " << modified.size() << " items between " << fnameL << " and " << fnameR << LF;
		for(BupInfoPrint::iterator n = modified.begin(); n != modified.end(); ++n)
			std::cout << "*\t" << n->moddate << TAB << n->fsize << "\t->\t" << n->fname << TAB << n->md5sum << TAB << n->path << LF;
	}
	
	// now sort the remaining trees by file path
	BupInfoPrint& sortL = modifiedL; // copy_list(BupInfoRenamed, modifiedL, sortL);
	BupInfoPrint& sortR = modifiedR; // copy_list(BupInfoRenamed, modifiedR, sortR);
	
	// print result
	std::cout << LF << "# Deleted " << sortL.size() << " items since " << fnameL << LF;
	print_list(BupInfoPrint, sortL, "-\t");
	std::cout << LF << "# Added " << sortR.size() << " items in " << fnameR << LF;
	print_list(BupInfoPrint, sortR, "+\t");
	
	return 0;
}
