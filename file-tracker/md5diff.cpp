// md5diff.cpp
// compile with: g++ -o md5diff md5diff.cpp -lstdc++

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

inline std::string UFBaseDir  (const std::string& path) { return path.substr(0, path.find_last_of("/") - 1); }
inline std::string UFFileName (const std::string& path) { return path.substr(path.find_last_of("/") + 1); } // filename including extension
inline std::string UFExtension(const std::string& path) { return path.substr(path.find_last_of(".") + 1); } // extension only
inline bool        UFExists   (const std::string& path) { struct stat buffer; return (::stat(path.c_str(), &buffer) == 0); }

// flags that define how to treat the line items - set to true here but can be set to false while reading the files, depending on the availability of the data in those files
static bool sUseModDate = true; // use modification date in comparisons?
static bool sUseNano    = true; // use nanosecond precision in moddate comparison? (only works if *all* moddates are specified in nanosecond precision) (requires sUseModDate true). start with true, can be reset while reading input files
static bool sUseMD5     = true; // use the md5sum. if false, use the moddate and file size instead (requires sUseModDate true). start with true, can be reset while reading input files (of .fst files, for instance)
static bool sUseTable   = false; // if true, print output in a fixed-format table output style

class FileInfo
{
	public:
		std::string moddate;
		std::string fsize;
		std::string md5sum;
		std::string path;    // full path (directory/filename.ext)
};

class BupInfo : public FileInfo
{
	public:
//		std::string bupdate;
//		std::string inode; std::string diskid;
		std::string fname;   // filename.ext only, calculated field; used in comparisons and would dramatically slow down program if calculated inline
		FileInfo    rhsinfo; // field with rhs info to facilitate printout of moved, renamed and modified items (the part after the ->)
	private:
		std::string mdd_fsz; // combined moddate & fsize, calculated field; used in comparisons and would dramatically slow down program if calculated inline
	public:
		void calc_fields()   // to calculate/modify fields after reading from file
		{
			if(!sUseNano) moddate.resize(15); // strip subsecond part
			mdd_fsz = moddate + "|" + fsize; // calculate field: combined moddate & fsize
			fname = UFFileName(path); // calculate field: filename (basename) of the full path
		}
		inline const std::string& md5equiv() const { return sUseMD5 ? md5sum : mdd_fsz ; } // use the md5sum for .md5 input files; mdd_fsz for .fst files (or mixed .fst and .md5 files). overridde with -nomd5.
};

//original comparisons used fsize if there was no md5sum. this sort of worked, except in cases when a file was modified and the size didn't change (and that can happen quite easily).
//struct lt_path                 { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return LT1(lhs.path, rhs.path); } };
//struct lt_md5sum_path          { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return LT2((sUseMD5?lhs.md5sum:lhs.fsize),lhs.path, (sUseMD5?rhs.md5sum:rhs.fsize),rhs.path); } };
//struct lt_md5sum_moddate       { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return sUseModDate ? LT2((sUseMD5?lhs.md5sum:lhs.fsize),lhs.moddate,           (sUseMD5?rhs.md5sum:rhs.fsize),rhs.moddate)           : LT1(lhs.md5sum, rhs.md5sum); } };
//struct lt_md5sum_fname_moddate { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return sUseModDate ? LT3((sUseMD5?lhs.md5sum:lhs.fsize),lhs.fname,lhs.moddate, (sUseMD5?rhs.md5sum:rhs.fsize),rhs.fname,rhs.moddate) : LT2(lhs.md5sum,lhs.fname, rhs.md5sum,rhs.fname); } };

//20210815: changed to moddate comparison (introduced md5equiv() helper function)
struct lt_path                 { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return LT1(lhs.path, rhs.path); } };
struct lt_md5sum_path          { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return LT2(lhs.md5equiv(),lhs.path, rhs.md5equiv(),rhs.path); } };
struct lt_md5sum_moddate       { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return sUseModDate ? LT2(lhs.md5equiv(),lhs.moddate,           rhs.md5equiv(),rhs.moddate)           : LT1(lhs.md5sum,           rhs.md5sum);           } };
struct lt_md5sum_fname_moddate { bool operator()(const BupInfo& lhs, const BupInfo &rhs) { return sUseModDate ? LT3(lhs.md5equiv(),lhs.fname,lhs.moddate, rhs.md5equiv(),rhs.fname,rhs.moddate) : LT2(lhs.md5sum,lhs.fname, rhs.md5sum,rhs.fname); } };

std::ostream& operator<< (std::ostream& strm, const FileInfo& info)
{
	strm << info.moddate << TAB << info.fsize << TAB << info.md5sum << TAB << info.path;
	return strm;
}

#define copy_list(A, src, dst) { for(A::iterator n = src.begin(); n != src.end(); ++n) dst.insert(*n); }

typedef std::set     <BupInfo, lt_md5sum_path>          BupInfoList;
	// file list sorted by md5sum+path (unique). if only md5sum was used, the list would not be
	// unique (think about empty files, duplicate files, or the next.gif and back.gif files in common web pages, for instance)
typedef std::multiset<BupInfo, lt_md5sum_fname_moddate> BupInfoMoved;
	// same list but sorted by md5sum+fname+moddate (not unique) -> used to find moved items
typedef std::multiset<BupInfo, lt_md5sum_moddate>       BupInfoRenamed;
	// same list but sorted by md5sum+moddate (not unique) -> used to find renamed items
typedef std::multiset<BupInfo, lt_path>                 BupInfoPrint;
	// same list but sorted by file path -> used for printout and to find modified items

static bool read_info(std::istream& fin, BupInfo& info)
{
	std::string line; int cnt = 0;
	while(std::getline(fin, line)) {
		++cnt;
		if(line[0] == '#') continue; // skip comment lines
		std::stringstream tokens(line); std::string s1,s2,s3,s4;
		// read 4 tokens (columns) from the line
		std::getline(tokens, s1, TAB);
		std::getline(tokens, s2, TAB);
		std::getline(tokens, s3, TAB);
		std::getline(tokens, s4, TAB);
		// the meaning of s1,s2,s3,s4 depends on the file format
		if(s4 != "") { // found a path: it must be an .md5 or .fst file (modification_date<tab>file_length<tab>[md5sum]<tab>file_path); in outputs created from mkfilelist_fast, the md5sum field is empty or "-"
			info.moddate = s1;
			info.fsize   = s2;
			info.md5sum  = s3; if(info.md5sum == "") info.md5sum = "-"; // convert to new form .fst (20210723) which outputs "\t-\t" to prevent "\t\t" in the output; this program assumes "-" (new form)
			info.path    = s4;
		}
		else if(s2 != "") { // it must be a plain .md5sum file: moddate and fsize columns are missing (md5sum<tab>file_path)
			info.moddate = "";
			info.fsize   = "";
			info.md5sum  = s1;
			info.path    = s2;
		}
		else {
			std::cerr << "WARNING: skipping misformed line " << cnt << ": '" << line << "'" << LF;
			continue; // skip misformed line (instead of returning false)
		}
		// post processing
		if(info.moddate.length() > 25) info.moddate.resize(25); // limit to nanosecond (gnu find prints .10 accuracy YYYYMMDD_HHMMSS.1234567890 -> YYYYMMDD_HHMMSS.123456789)
		if(info.md5sum.length() > 32) info.md5sum.resize(32); // strip any trailing dashes
		if(info.fname == ".DS_Store" || info.fname.substr(0,2) == "._") continue; // skip stupid files and attribute sidecar files from (ex)FAT(32) volumes (._*)
		
		// set flags
		if(info.moddate == "") sUseModDate = false; // found one empty moddate (e.g. .md5sum file) - can't use moddate
		else if(info.moddate.length() == 15) sUseNano = false; // found one second-accurate moddate - can't use nanosecond precision
		if(info.md5sum == "-") sUseMD5 = false ; // found one empty md5sum - can't use md5sum
		
		return true; // found a good info line
	}
	return false;
}

static void shift(int& argc, char**& argv, int pos = 1)
{
	if(pos < argc) {
		for(int i=pos; i<argc; ++i)
			argv[i] = argv[i+1];
		--argc;
	}
}

int main(int argc, char** argv)
{
	//std::cout << argc << LF ; for(int i=0; i<argc; ++i) std::cout << argv[i] << LF;
	
	bool error = false;
	
	// process options
	bool ignoreModDate = false, ignoreMD5 = false, useNano = false, reverse = false;
	for(int i=1 ; i<argc ; ) {
		if(std::string(argv[i]) == "-nodate") {
			ignoreModDate = true;
			shift(argc,argv,i);
		}
		else if(std::string(argv[i]) == "-nomd5") {
			ignoreMD5 = true;
			shift(argc,argv,i);
		}
		else if(std::string(argv[i]) == "-nano") {
			useNano = true;
			shift(argc,argv,i);
		}
		else if(std::string(argv[i]) == "-table") {
			sUseTable = true;
			shift(argc,argv,i);
		}
		else if(std::string(argv[i]) == "-r") {
			reverse = true;
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
	
	// get the input files - these should be the only args left
	std::string fnameL, fnameR;
	if(argc == 3 && UFExists(argv[1]) && UFExists(argv[2])) {
		fnameL = reverse ? argv[2] : argv[1];
		fnameR = reverse ? argv[1] : argv[2];
	}
	else
		error = true;
	
	if(error) {
		std::cerr << "usage: md5diff [-nodate|-nomd5] [-nano] [-table] [-r] file1[.md5|.fst] file2[.md5|.fst]" << LF;
		std::cerr << "options:" << LF;
		std::cerr << "  -nodate: ignore modification date fields (use md5sum only)" << LF;
		std::cerr << "  -nomd5: ignore md5sum checksum fields (use modification date and file size only)" << LF;
		std::cerr << "  -nano: use nanosecond precision on modification dates (if present)" << LF;
		std::cerr << "  -table: print output in a fixed-format table" << LF;
		std::cerr << "  -r: reverse (swap) file1 and file2" << LF;
		std::cerr << "input:" << LF;
		std::cerr << "  reads tab-separated files (file1 and file2):" << LF;
		std::cerr << "    .md5: files created with mkfilelist_md5" << LF;
		std::cerr << "          with lines formatted like moddate<tab>fsize<tab>md5sum<tab>path" << LF;
		std::cerr << "    .fst: files created with mkfilelist_fast" << LF;
		std::cerr << "          with lines formatted like moddate<tab>fsize<tab><tab>path (old form)" << LF;
		std::cerr << "                                 or moddate<tab>fsize<tab>-<tab>path (new form)" << LF;
		std::cerr << "    .md5sum: simple md5 files created directly with md5sum" << LF;
		std::cerr << "          with lines formatted like md5sum<tab>path" << LF;
		std::cerr << "  skips lines starting with a #" << LF;
		std::cerr << "output (stdout):" << LF;
		std::cerr << "  tab-separated list of changes found when going from the state in file1 to file2:" << LF;
		std::cerr << "  lines starting with -: deleted files" << LF;
		std::cerr << "  format:             -<tab>moddate<tab>size<tab>[md5sum|-]<tab>path<lf>" << LF;
		std::cerr << "  lines starting with =: files that moved to a different directory, but keeping its file name" << LF;
		std::cerr << "  format:             =<tab>moddate<tab>size<tab>[md5sum|-]<tab>path<tab>-><tab>newpath<lf>" << LF;
		std::cerr << "  lines starting with >: files that were renamed (possibly to a different directory)" << LF;
		std::cerr << "  format:             ><tab>moddate<tab>size<tab>[md5sum|-]<tab>path<tab>-><tab>newpath<lf>" << LF;
		std::cerr << "  lines starting with *: files that were modified" << LF;
		std::cerr << "  format:             *<tab>moddate<tab>size<tab>-><tab>newmoddate<tab>newsize<tab>[newmd5sum|-]<tab>path<lf>" << LF;
		std::cerr << "  lines starting with +: files that were added (new files)" << LF;
		std::cerr << "  format:             +<tab>moddate<tab>size<tab>[md5sum|-]<tab>path<lf>" << LF;
		std::cerr << "table output (stdout):" << LF;
		std::cerr << "  tab-separated list of changes found when going from the state in file1 to file2:" << LF;
		std::cerr << "  format:             [-=>*+]<tab>moddate1<tab>size1<tab>md5sum1<tab>path1<tab>moddate2<tab>size2<tab>md5sum2<tab>path2" << LF;
		return 1;
	}
	
	// build initial file lists
	// just read the file data in a BupInfoPrint beacause its sort order is defined by path and hence
	// independent of the flags - this is important because the flags' value(s) may change during
	// reading of the files, based on fields present, and would cause inconsistent sort order (in short: a mess).
	BupInfoPrint inL; int countL = 0;
	{	std::ifstream finL; finL.open(fnameL);
		if(finL.is_open()) {
			BupInfo info;
			while(read_info(finL, info)) {
				++countL;
				inL.insert(info);
			}
			finL.close();
		}
	}
	std::cout << "# Read " << countL << " items from " << fnameL << LF;
	
	BupInfoPrint inR; int countR = 0;
	{	std::ifstream finR; finR.open(fnameR);
		if(finR.is_open()) {
			BupInfo info;
			while(read_info(finR, info)) {
				++countR;
				inR.insert(info);
			}
			finR.close();
		}
	}
	std::cout << "# Read " << countR << " items from " << fnameR << LF;
	// handle flags override options
	if(ignoreModDate) {
		if(sUseModDate) {
			std::cout << "# " << "Ignoring file modification dates (using md5sum instead)" << LF; sUseModDate = false;
		}
		else {
			std::cerr << "WARNING: Can't ignore file modification dates because these aren't available for all entries in the first place" << LF;
		}
	}
	if(ignoreMD5) {
		if(sUseMD5) {
			std::cout << "# " << "Ignoring md5sums (using file modification dates and file size instead)" << LF; sUseMD5 = false;
		}
		else {
			std::cerr << "WARNING: Can't ignore md5sums because these aren't available for all entries in the first place" << LF;
		}
	}
	if(sUseModDate == false && sUseMD5 == false) {
		std::cerr << "ERROR: input files not compatible for comparison (missing modification date(s) and/or md5sums)" << LF;
		return 1;
	}
	if(useNano) {
		if(!sUseNano) {
			std::cerr << "WARNING: Can't use nanosecond modification dates because this precision isn't available for all entries in the input files" << LF;
		} // else sUseNano = true; // well, that's obvious
	}
	else {
		if(sUseNano) {
			// there ara serveral mechanisms that will truncate/reset the subsecond modification time on your files: for example:
			// setting a Finder tag over afpd, creating an .fst or .md5 using the mac os smb client, using an older version of rsync.
			// to prevent these truncations to appear as false-positive 'file modified' events, don't use the subsecond modification date by default.
			// this is also the default behaviour of rsync. you can still override with -nano.
			std::cerr << "INFO: Reducing modification date precision to 1 second for increased compatibility of timestamps (reduces false-positives on modified files)" << LF;
		}
		sUseNano = false; // default false because there are several cases where the subsecond accuracy is lost and the fractional seconds are reset
	}
	// print flags status
	if(sUseModDate)
		std::cout << "# " << "Using file modification dates and " << (sUseMD5 ?  "md5sum" : "file size") << LF;
	else
		std::cout << "# " << "Using md5sums only, ignoring file modification dates and file size" << LF;
	std::cout << "# " << "Using " << (sUseNano ? "nanosecond" : "1-second") << " precision file modification dates" << LF;
	
	// now transfer the items to proper sorted BupInfoList lists for lhs:
	BupInfoList listL;
	{	for(BupInfoPrint::iterator n = inL.begin(); n != inL.end(); ++n) {
			BupInfo info = *n;
			info.calc_fields(); // _now_ calculate the (other) fields
			std::pair<BupInfoList::iterator, bool> ins = listL.insert(info);
			if(!ins.second) { // insertion failed
				// panic (this shouldn't happen because the path part should definitely be unique)
				std::cerr << "ERROR: failed to insert " << info.path << " in list read from " << fnameL << "; entry with this signature already found in " << ins.first->path << LF;
			}
		}
	}
	// now transfer the items to proper sorted BupInfoList lists for rhs:
	// get rid of all identical items (identical here is based on md5sum+path)
	// while reading the rhs file (for performance reasons: two birds with one stone)
	BupInfoList listR;
	{	for(BupInfoPrint::iterator n = inR.begin(); n != inR.end(); ++n) {
			BupInfo info = *n;
			info.calc_fields(); // _now_ calculate the (other) fields
			BupInfoList::iterator l = listL.find(info);
			if(l != listL.end()) // found item with matching md5sum and path
				listL.erase(l); // don't print anything here because this is the majority
			else
				listR.insert(info);
		}
	}
	
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
				BupInfo info = *f; info.rhsinfo = *n; // store rhs path in rhsinfo
				moved.insert(info);
				movedL.erase(f);
			}
			else
				movedR.insert(*n); // movedR is a multiset - no failed inserts here
		}
		std::cout << LF << "# Moved " << moved.size() << " items between " << fnameL << " and " << fnameR << LF;
		for(BupInfoPrint::iterator n = moved.begin(); n != moved.end(); ++n)
			if(sUseTable)
				std::cout << "=\t" << *n << "\t" << n->rhsinfo << LF;
			else
				std::cout << "=\t" << *n << "\t->\t" << n->rhsinfo.path << LF;
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
				BupInfo info = *f; info.rhsinfo = *n; // store rhs path in rhsinfo
				renamed.insert(info);
				renamedL.erase(f);
			}
			else
				renamedR.insert(*n); // renamedR is a multiset - no failed inserts here
		}
		std::cout << LF << "# Renamed " << renamed.size() << " items between " << fnameL << " and " << fnameR << LF;
		for(BupInfoPrint::iterator n = renamed.begin(); n != renamed.end(); ++n)
			if(sUseTable)
				std::cout << ">\t" << *n << "\t" << n->rhsinfo << LF;
			else
				std::cout << ">\t" << *n << "\t->\t" << n->rhsinfo.path << LF;
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
				//BupInfo info = *f; info.rhsinfo = n->moddate + TAB + n->fsize + TAB + n->md5sum + TAB + n->path; // store rhs info for printout
				BupInfo info = *f; info.rhsinfo = *n; // store rhs info for printout
				modified.insert(info);
				modifiedL.erase(f);
			}
			else
				modifiedR.insert(*n);
		}
		std::cout << LF << "# Modified " << modified.size() << " items between " << fnameL << " and " << fnameR << LF;
		for(BupInfoPrint::iterator n = modified.begin(); n != modified.end(); ++n)
			if(sUseTable)
				std::cout << "*\t" << *n << "\t" << n->rhsinfo << LF;
			else
				std::cout << "*\t" << n->moddate << TAB << n->fsize << "\t->\t" << n->rhsinfo << LF;
	}
	
	// now sort the remaining trees by file path
	BupInfoPrint& sortL = modifiedL; // copy_list(BupInfoRenamed, modifiedL, sortL);
	BupInfoPrint& sortR = modifiedR; // copy_list(BupInfoRenamed, modifiedR, sortR);
	
	// print result
	std::cout << LF << "# Deleted " << sortL.size() << " items since " << fnameL << LF;
	for(BupInfoPrint::iterator n = sortL.begin(); n != sortL.end(); ++n)
		if(sUseTable)
			std::cout << "-\t" << *n << "\t-\t-\t-\t-" << LF;
		else
			std::cout << "-\t" << *n << LF;
	std::cout << LF << "# Added " << sortR.size() << " items in " << fnameR << LF;
	for(BupInfoPrint::iterator n = sortR.begin(); n != sortR.end(); ++n)
		if(sUseTable)
			std::cout << "+\t-\t-\t-\t-\t" << *n << LF;
		else
			std::cout << "+\t" << *n << LF;
	
	return 0;
}

//EOF
