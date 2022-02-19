/*GPL*START*
 * streplace - replace strings/bytes/regexps in files and/or their filenames
 * 
 * Copyright (C) 1997-2008,2010 by Johannes Overmann <Johannes.Overmann@gmx.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * *GPL*END*/  

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "tappconfig.h"
#include "tfiletools.h"
#include "tregex.h"

// history:
// 1997:
// 22:45 10 Jun start of new substool (to replace old stool)
// 01:45 11 Jun application config done (47 lines)
// 11:00 18 Jun file handling (201 lines)
// 01:00 03 Jul minor
// 01:00 31 Jul canceled filename modification, regex (235)
// 06:00 31 Jul v0.3: first usable version (359 lines)
// 23:00 02 Aug v0.4: debugged memory leakage
// 02:30 13 Aug v0.5: check for write access and --ignore-errors (385)
// 22:00 13 Aug v0.6: --rename implementation (545)
// 12:30 14 Aug v0.7: backup implementation (627 lines)
// 13:00 02 Sep v0.7a: preserve-case added for simple (632)
// 21:00 04 Sep v0.7b: preserve-case added for regex (636)
// 21:00 28 Sep v0.7c: progress indicator (646)
// 16:00 29 Dec v0.8: trace mode

// 1998:
// 23:00 12 Jan v0.9: upper/lower/capitalize and some colorization
// 02:30 23 Jan v0.9a: --no-color option added
// 12:36 23 Aug v0.9b: show non-printable chars in verbose file list (feature :)
// 12:36 23 Aug v0.9c: bugfix: open read-only in dummy mode (722)
// 14:48 18 Sep v0.9.1: project renamed to streplace, prepare for uploading to sunsite
// 11:43 30 Sep v0.9.2: starting modify-symlinks support (747)
// 18:46 01 Oct v0.9.3: better directory semantics (rename), mod symlinks works (891)
// 01:31 17 Oct v0.9.4: avoid renaming of '.' with -r .

// 1999:
// 15:03 31 Jan v0.9.5: fixed trace/preserve case bug (tnx to Kai Schuetz)
// 15:36 31 Jan v0.9.6: modify case bug for -x fixed
// 21:27 03 Feb v0.9.7: --force allows control chars on left side for rename, man page almost finished
// 23:59 03 Feb         --more removed,
// 00:34 05 Feb v0.9.8: --dummy-linetrace/--context added (936)
// 23:08 06 Feb v0.9.9: --context +N option (sep) added, multi rule --context bug fixed, thin color (sep)
// 22:45 08 Feb v0.9.10: __STRICT_ANSI__, -ansi support, truncate removed
// 01:24 15 Feb v0.9.11: renamed some options, manpage improved
// 18:35 15 Feb v0.9.12: tappframe simplified, only first file -L bug fixed
// 20:01 04 Jun v0.9.13: ignore pattern feature added, dos-to-unix added, max-filesize added
// 20:59 14 Oct v0.9.14: bug in perhapsRename fixed: only last rule was applied
// 21:47 14 Oct v0.9.15: stdio mode '-' (no real streaming) (1113 lines)

// 2000:
// 08:40 02 Feb v0.9.16: color config added
// 20:40 09 Jul v0.9.17: tappframe simplified, some bugs may come up ;-)

// 2001:
// 00:00 14 Mar v0.9.18: --shorten filenames added, default rule feature added, rename dir / missing bug fixed (was not in .17)
// 21:20 02 Apr v0.9.19: empty RHS caused TZeroBasedIndexOutOfRange bug fixed (in tregex.cc)
// 
// 2002:
//       20 Feb v0.9.20: --sane-filename[2] added, --norm-extension added
//       08 Apr v0.9.21: removed umlauts from --sane-filename, quoting bug fixed (in tstring), really allow space for --sane-filename
//       28 May v0.9.22: '=' not quotable bug fixed (\== now working again)
//       13 Nov v0.9.23: use automake/autoconf to build, first cvs revision (1255)
//       19 Nov v0.9.24: across-dirs added
//
// 2003: 17 Mar v0.9.25: --const-length and --try-all-pos added for patching binaries
// 2004: 05 May v0.9.26: hpp added to --c-only option
//       23 May v0.9.27: --rename which only changes the case (upper/lower) of the filename now should also work on MACOSX
//       30 Jun v0.9.28: --resolve-clash added
//       06 Jul v0.9.29: -u (dos2unix) added
//       16 Jul v0.9.30: Added Colins changes: --norm-ext and --norm-ext-dos
//       16 Jul v0.9.31: improved color configuration
// 2005: 19 Nov v0.9.32: --rename now really works on apple (case insensitive filesystem), --assume-cifs/--assume-csfs added, warnings removed
//        6 Dec v0.9.33: symlink behavior fixed (do not ignore link on rename), behavior on ignore file error fixed (resume with other files), __STRICT_ANSI__ nonsense removed
// 2007:  5 Apr v0.9.34: --select-lines and --ignore-lines added
// 2008: 28 Aug v0.9.35: removed autotools overhead, simplified Makefile
// 2010: 23 Dec v0.9.36: --sane-filename[2]: be more restrictive
// 2013:  3 Feb v0.9.37: fixed '""""="""' bug (was treated like '""""=""""'), fixed '"""=aaa' bug (was not recognized as a rule at all)
// 2022: 19 Feb v0.9.38: Re-released under MTI license. Renamed *.cc files to *.cpp (using streplace -xN .cc=.cpp *.cc). Compiles without warnings (under -Wall -Wextra) on clang 13.

// please update VERSION
#define VERSION "0.9.38"



#define HTML_EXT "html,htm,shtml,dhtml"
#define C_EXT "c,cc,C,cxx,cpp,h,hh,H,hxx,hpp,i,inc,m,mm,M"
#define CONFIGFILE "streplacerc"

const char *options[] = {
   "#usage='Usage: %n [OPTIONS, FILES, DIRS and RULES] [--] [FILES and DIRS]\nor:    %n [OPTIONS and RULES] -       (read from stdin/write to stdout)\n"
     "this program substitutes strings in files according to rules:\n"
     "\t- a rule is of the from FOO=BAR which replaces FOO by BAR\n"
     "\t- use C escape sequences \\n \\t \\xff, use \\\\ to get a verbatim backslash\n"
     "\t- use \\= to get a verbatim =\n"
     "\t- use ([0-9]*).jpeg=pic\\\\1.jpg to reuse subexpressions'"
     ,
     "#trailer='\n%n version %v *** (C) 1997-2010 by Johannes Overmann\ncomments, bugs and suggestions welcome: %e\n%gpl'",
     "#stopat--", // stop option scanning after a -- parameter
     // options
     "name=recursive        , type=switch, char=r, help=recurse directories, headline=file options:",
     "name=follow-links     , type=switch, char=l, help=follow symbolic links",
     "name=only             , type=string, char=o, help=process only files with extension in comma separated LIST, string-mode-append, string-append-separator=',', param=LIST",
     "name=html-only        , type=switch, char=H, help='same as -o \"" HTML_EXT "\"'",
     "name=c-only           , type=switch, char=C, help='same as -o \"" C_EXT "\"'",
     "name=ignore-errors    , type=switch, char=E, help='skip files/directories that can\\'t be read/written/renamed'",

     "name=ignore-case      , type=switch, char=i, help=ignore case, headline=matching options:",
     "name=preserve-case    , type=switch, char=I, help=try to preserve case in replacements",
     "name=lower-case       , type=switch,         help=lower replacements",
     "name=upper-case       , type=switch,         help=upper replacements",
     "name=capitalize-case  , type=switch,         help=capitalize replacements",
     "name=match-newline    , type=switch, char=n, help=match any character operators also match newline char",
     "name=select-lines     , type=string,       , help=split file into lines and replace only in lines matching regex R, param=R",
     "name=ignore-lines     , type=string,       , help=split file into lines and replace only in lines not matching matching regex R, param=R",
     "name=no-regexp        , type=switch, char=x, help=match the left side of each rule as a simple string\\, not as a regexp (substring search\\, useful with binary files)",
     "name=whole-words      , type=switch, char=w, help=match only whole words (only for -x mode\\, not for regex)",
     "name=const-length     , type=switch,         help='make sure that the left and the right side of each rule have the same length (only if -x, useful with binary files)'",
     "name=try-all-pos      , type=switch,         help='write N files nnn_hexpos_FILE if there are N locations for a pattern (only if -x\\, useful for patching binaries)'",
     

     "name=backup           , type=switch, char=b, help='make backup before a file is modified if no backup exists', headline='backup options:'",
     "name=suffix           , type=string, char=S, help='set the backup suffix to SUF', default='.~sub~', param=SUF",
     "name=restore          , type=switch,       , help='restore all files from their backups (==> undo all changes), specify the files to be restored'",

     "name=rename           , type=switch, char=A, help='rules work also on names: rename files and directories', headline='renaming options: (restore can\\'t unrename)'",
     "name=rename-only      , type=switch, char=N, help='same as --rename, but do not modify files contents'",
     "name=across-dirs      , type=switch,       , help='for -A and -N: rules work on the whole path possibly moving the file across directories up and down the hierachy (i.e. slash is a normal character which can be replaced by anything, the resulting path is the new filename)'",
     "name=force            , type=switch, char=f, help='replace existing files during rename, use with caution'",
     "name=modify-symlinks  , type=switch, char=s, help='rules work only on the contents of symlinks (not their name), all other files and their contents are ignored'",
     "name=shorten          , type=int   , char=t, lower=0, param=N, help='for -N and -A: shorten filenames to a maximum of N characters while keeping the extension by using certain heuristics after replacing'",
     "name=sane-filename    , type=switch,       , help='strip all kinds of illegal chars from filenames but allow spaces (useful with -N)'",
     "name=sane-filename2   , type=switch,       , help='strip all kinds of illegal chars from filenames including spaces (useful with -N)'",
     "name=resolve-clash    , type=switch, char=R, help='resolve nameclashes by prepending N_ before the new filename where N makes the filename unique'",
     "name=norm-ext-dos     , type=switch,       , help='normalize certain extensions like jpeg -> jpg, mpeg -> mpg to 3 characters'",
     "name=norm-ext         , type=switch,       , help='normalize certain extensions like jpg -> jpeg, MPEG3 -> mp3 with canonical length'",
     "name=assume-cifs      , type=switch,       , help='assume case-insensitive filesystem (default on Apple systems)'",
     "name=assume-csfs      , type=switch,       , help='assume case-sensitive filesystem (default on non-Apple systems)'",

     "name=ignfilepat       , type=string,         string-mode-append, string-append-separator='\1', param=P, help='ignore files in which pattern P is found', headline='special options:'",
     "name=ignfilepat-nore  , type=switch,       , help=match ignore pattern as a simple string\\, not as a regexp",
     "name=max-filesize     , type=int   , char=M, lower=0, param=N, help=skip files which are larger than N megabytes (default 0 \\= unlimited)",
     "name=dos-to-unix      , type=switch, char=U, help='convert all files which *only* contain DOS line termination to unix line termination, works also on binary files and is very robust'",
     "name=dos-to-unix2     , type=switch, char=u, help='convert all DOS line termination to unix line termination (for files which partly contain DOS line termination)'",
   
     "name=color-bold       , type=string,         save, param=SEQ, default='\033[01m', help='escape sequence used to switch terminal to bold mode', headline='color configuration:'",
     "name=color-thin       , type=string,         save, param=SEQ, default='\033[07m', help='escape sequence used to switch terminal to thin mode'",
     "name=color-nor        , type=string,         save, param=SEQ, default='\033[00m', help='escape sequence used to switch terminal back to normal mode'",
     "name=no-color         , type=switch, char=Q, help='do *not* colorize output with ansi sequences'",
     "name=bold             , type=switch,         help='do not use colors, but use bold and reverse to hilite text'",
     "name=bcolor           , type=switch,         help='use colors which are useful on a bright terminal background'",
     "name=dcolor           , type=switch,         help='use colors which are useful on a dark terminal background'",
     "name=save-colors      , type=switch,         help='save colors to config file ~/." CONFIGFILE "'",
   
     "name=dummy-mode       , type=switch, char=0, alias=-O;-d, help='do *not* write/change anything', headline=common options:",
     "name=dummy-trace      , type=switch, char=T, help='do *not* write/change anything, but print matching files to stdout and highlight replacements'",
     "name=dummy-linetrace  , type=switch, char=L, help='do *not* write/change anything, but print matching lines of matching files to stdout and highlight replacements'",
     "name=context          , type=int   ,       , param='[+]N', default=1, lower=0, help='set number of context lines for --dummy-linetrace to N (use +N to hide line separator)'",
     "name=quiet            , type=switch, char=q, help=quiet execution",
     "name=progress         , type=int,    char=P, param=NUM, default=0, lower=0, help='progress indicator, useful with large files, print a \\'.\\' per NUM replacements",
     "EOL" // end of list
};

#define STDIO_BLOCKSIZE 4096

#define DEFAULT_RULE_LEFT "^.*$"
#define DEFAULT_RULE_RIGHT "\\0"
#define DEFAULT_RULE_HELP "id"




tstring color_bold;
tstring color_nor;
tstring color_thin;


bool preserve_case = false;
int modify_case = tstring::NOT;
bool modify_symlinks = false;
bool ignore_errors = false;
int shorten_filenames = 0;
bool acrossDirs = false;
bool resolveNameClash = false;
int numResolvedClashes = 0;
bool caseInsensitiveFilesystem = false;


// get temporary filename
tstring getTempFileName(const tstring& fname)
{
    for(int i = 0;; i++)
    {
	tstring r = fname + tstring(i);
	if(!fexists(r.c_str()))
	    return r;
    }
    return fname + "too_many_files"; // can virtually never happen
}


// this is a wrapper for the ansi 'rename' function which also works for MacOS X when only changing the case for a filename
// (which would not work with the unix rename)
int caseSafeRename(const char *oldpath, const char *newpath)
{
    if(caseInsensitiveFilesystem)
    {
	tstring tmp = getTempFileName(newpath);
	int err = rename(oldpath, tmp.c_str());
	if(err)
	    return err;
	return rename(tmp.c_str(), newpath);
    }
    else
    {
	return rename(oldpath, newpath);
    }
}


// make backup if fname does not exist
void makeBackup(FILE *fp, int len, const tstring& fname, const tstring& suf,
		bool ignore_errors_, int& errors, bool quiet) {
   tstring bname(fname+suf);
   if(fexists(bname.c_str())) {
      if(!quiet) printf("keeping old version of backup file '%s'\n", bname.c_str());
   } else {
      rewind(fp);
      FILE *f = fopen(bname.c_str(), "w");
      if(f==0) {
	 if(ignore_errors_) {
	    if(!quiet) printf("ignoring error: can't create backup file '%s'\n", bname.c_str());
	    errors++;
	 } else {
	    userError("error while opening backup file '%s' for writing!\n", bname.c_str());
	 }
      } else {
	 if(!quiet) printf("creating backup file '%s'\n", bname.c_str());
	 char *mem = new char[len];
	 if(fread(mem, 1, len, fp)!=(unsigned)len) 
	   userError("error while reading file '%s'!\n", fname.c_str());
	 if(fwrite(mem, 1, len, f)!=(unsigned)len)
	   userError("error while writing file '%s'!\n", bname.c_str());
	 fclose(f);
	 delete[] mem;
      }
   }   
   rewind(fp);
}


tstring shortenFilename(tstring filename, size_t max_len) {
   if(filename.length() <= max_len) return filename;
   tstring ext = filename;
   ext.extractFilenameExtension();
   if(!ext.empty()) ext = "." + ext;
   size_t extlen = ext.length();
   // truncate if extension is longer than max_len
   if(extlen >= max_len) return filename.substr(0, max_len);
   // remove extension   
   filename = filename.substr(0, filename.length() - extlen);
   max_len -= extlen;
   // remove common terms
   while(filename.searchReplace("the", "", true, true, false, 0, tstring(), tstring(), 0, 1) && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   // remove duplicates
   while(filename.searchReplace("  ", " ") && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   while(filename.searchReplace("..", ".") && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   while(filename.searchReplace("__", "-") && (filename.length() > max_len)) {}
   while(filename.searchReplace("_", "-") && (filename.length() > max_len)) {}
   while(filename.searchReplace("- ", "-") && (filename.length() > max_len)) {}
   while(filename.searchReplace(" -", "-") && (filename.length() > max_len)) {}
   while(filename.searchReplace("--", "-") && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   // remove separators
   while(filename.searchReplace(" ", "", true, false, false, 0, tstring(), tstring(), 0, 1) && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   while(filename.searchReplace("-", "", true, false, false, 0, tstring(), tstring(), 0, 1) && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
#if 0   
   // remove consonants
   while(filename.searchReplace("a", "", true, false, false, 0, tstring(), tstring(), 0, 1) && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   while(filename.searchReplace("e", "", true, false, false, 0, tstring(), tstring(), 0, 1) && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   while(filename.searchReplace("i", "", true, false, false, 0, tstring(), tstring(), 0, 1) && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   while(filename.searchReplace("o", "", true, false, false, 0, tstring(), tstring(), 0, 1) && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
   while(filename.searchReplace("u", "", true, false, false, 0, tstring(), tstring(), 0, 1) && (filename.length() > max_len)) {}
   if(filename.length() <= max_len) return filename + ext;
#endif
   // take leftmost part
   return filename.substr(0, max_len) + ext;
}


// return num renamings (0 or 1)
int perhapsRename(tstring& name, const tvector<tstring>& l_rules, 
		   const tvector<tstring>& r_rules, bool quiet, 
		   bool ignore_errors_, bool whole_words, bool ignore_case,
		   bool use_regexp, tvector<tstring>& clash_from,
		   tvector<tstring>& clash_to, const tvector<TRegEx*> regex,
		   bool force, bool dummy, int& errors) {
   // avoid processing of dir '.' and '..' (which should never be renamed)
   if((name==".")||(name=="..")) {
      if(!quiet) {
	 printf("omitting dir '%s%s%s' for possible rename operation\n",
		color_bold.c_str(), name.c_str(), color_nor.c_str());
      }
      return 0; // nothing renamed
   }
   
   if(modify_symlinks) { // modify symlinks

      // this is very different from renamimg a file!
      tstring file(name);
      file.removeDirSlash();
      
      // check if file is a symlink
      if(!fissymlink(file.c_str())) return 0;
      
      // maximal symlink len, 4k should be enough 
      // (linux ext2fs limits this to 1023 bytes anyway)
      const int BUFLEN = 1024*4; 
      char buf[BUFLEN];
      memset(buf, 0, BUFLEN);
      if(readlink(file.c_str(), buf, BUFLEN)==-1) {
	 if(ignore_errors_) {                  
	    if(!quiet) {
	       perror("readlink");
	       printf("can't read symlink '%s%s%s'\n",
		      color_bold.c_str(), file.c_str(), color_nor.c_str());
	    }
	    errors++;
	    return 0;
	 } else {
	    perror("fatal error during readlink");
	    userError("can't read symlink '%s%s%s'!\n", 
		      color_bold.c_str(), file.c_str(), color_nor.c_str());
	 }	 
      }
      buf[BUFLEN-1] = 0; // sentinel
      if(strlen(buf) == BUFLEN-1) 
	userError("content of symbolic link '%s%s%s' too long! (len>=%d chars)\n",
		  color_bold.c_str(), file.c_str(), color_nor.c_str(), int(strlen(buf)));
      
      // apply rules
      tstring oldc(buf);
      tstring newc;      
      for(size_t r=0; r<l_rules.size(); r++) {
	 if(use_regexp) {
	    tvector<tvector<int> > all;
	    regex[r]->allMatchesSubstring(oldc.c_str(), all);
	    parameterSubstitution(oldc, newc, r_rules[r], all,
				  preserve_case, modify_case);
	 } else {
	    newc = oldc;
	    newc.searchReplace(l_rules[r], r_rules[r],
				  ignore_case, whole_words, preserve_case);
	 }
      }

      if(oldc != newc) {
	 // change link	 
	 if(!quiet) printf("changing symlink '%s%s%s' from '%s%s%s' to '%s%s%s'\n", 
			   color_bold.c_str(), file.c_str(), color_nor.c_str(),
			   color_bold.c_str(), oldc.c_str(), color_nor.c_str(),
			   color_bold.c_str(), newc.c_str(), color_nor.c_str());	 
	 if(!dummy) {
	    // remove old link
	    if(unlink(file.c_str())) { 
	       if(ignore_errors_) {                  
		  if(!quiet) {
		     perror("unlink");
		     printf("can't unlink '%s%s%s'\n",
			    color_bold.c_str(), file.c_str(), color_nor.c_str());
		  }
		  errors++;
		  return 0;
	       } else {
		  perror("fatal error during unlink");
		  userError("can't unlink '%s%s%s'!\n", 
			    color_bold.c_str(), file.c_str(), color_nor.c_str());
	       }
	    }	   
	    // create new link
	    if(symlink(newc.c_str(), file.c_str())) {
	       if(ignore_errors_) {                  
		  if(!quiet) {
		     perror("symlink");
		     printf("can't create symlink '%s%s%s' to '%s%s%s')\n",
			    color_bold.c_str(), file.c_str(), color_nor.c_str(), 
			    color_bold.c_str(), newc.c_str(), color_nor.c_str());
		  }
		  errors++;
		  return 0;
	       } else {
		  perror("fatal error during symlink");
		  userError("can't symlink '%s%s%s' to '%s%s%s')!\n", 
			    color_bold.c_str(), file.c_str(), color_nor.c_str(), 
			    color_bold.c_str(), newc.c_str(), color_nor.c_str());
	       }	       
	    }	    
	 }
	 return 1;
      } 
      
      // nothing done
      return 0;

   } else { // perhaps rename a file
      
      // get oldname
      tstring oldname(name);
      if(!acrossDirs) {
	 oldname.removeDirSlash();
	 oldname.extractFilename();
      }
      
      // get newname (apply all rules)
      tstring newname = oldname;
      for(size_t r = 0; r < l_rules.size(); r++) {
	 if(use_regexp) {
	    tvector<tvector<int> > all;
	    regex[r]->allMatchesSubstring(newname.c_str(), all);
	    parameterSubstitution(newname, newname, r_rules[r], all,
				  preserve_case, modify_case);
	 } else {
	    newname.searchReplace(l_rules[r], r_rules[r],
				  ignore_case, whole_words, preserve_case);
	 }
      }

      // shorten new name
      if(shorten_filenames > 0) newname = shortenFilename(newname, shorten_filenames);
      
//      printf("oldname='%s' newname='%s'\n", oldname.c_str(), newname.c_str());
      if(oldname!=newname) {
	 if(!acrossDirs)
	 {
	    // get original path
	    tstring path(name);
	    path.removeDirSlash();
	    path.extractPath();
	    path.removeDirSlash();
	    if(!path.empty()) path.addDirSlash();
	    
	    // rename 
	    newname = path + newname;
	 }
	 else
	 {
	    // get new path
	    tstring path(newname);
	    path.removeDirSlash();
	    path.extractPath();
	    path.normalizePath();
	    // craete new directory
	    makeDirectoriesIncludingParentsIfNecessary(path, !quiet, dummy);
	 }

	 // printable names
	 tstring pnewname = newname;
	 tstring pname = name;
	 pnewname.expandUnprintable();
	 pname.expandUnprintable();
	 tstring orignewname = newname;
	 int uniqNum = 0;
retry:
	  // for apple a file would always clash with itself if it just would differ in its case
	  // ignore such clashes, caseSafeRename() will handle this correctly
	  if(((!caseInsensitiveFilesystem) || strcasecmp(newname.c_str(), name.c_str())) &&
	     fexists(newname.c_str()))
	  {	     
	    // name clash
	    
	    // resolve nameclash
	    if(resolveNameClash)
	    {
		tstring opath = orignewname;
		tstring ofile = orignewname;
		opath.extractPath();
		ofile.extractFilename();
		newname = opath + "/" +tstring(uniqNum++) + "_" + ofile;
		pnewname = newname;
		pnewname.expandUnprintable();		
		goto retry;
	    }
	    if(force) {
	       if(!quiet) printf("--force specified: replacing '%s%s%s'\n", 
				 color_bold.c_str(), pnewname.c_str(), color_nor.c_str());
	    } else {
	       clash_from += pname;
	       clash_to += pnewname;
	       if(!quiet) 
		 printf("nameclash while renaming '%s%s%s' as '%s%s%s', not renamed\n", 
			color_bold.c_str(), pname.c_str(), color_nor.c_str(), 
			color_bold.c_str(), pnewname.c_str(), color_nor.c_str());
	       return 0;
	    }
	 }
	 if(orignewname != newname)
	      numResolvedClashes++;
	 if(!quiet) printf("renaming '%s%s%s' as '%s%s%s'%s\n", 
			   color_bold.c_str(), pname.c_str(), color_nor.c_str(), 
			   color_bold.c_str(), pnewname.c_str(), color_nor.c_str(),
			   (color_bold + (orignewname == newname ? "" : " (resolved nameclash)") + color_nor).c_str());
	 if(!dummy) {
	    if(caseSafeRename(name.c_str(), newname.c_str())) {
	       // error
	       if(ignore_errors_) {
		  if(!quiet) {
		     perror("rename");
		     printf("can't rename '%s%s%s' as '%s%s%s'\n", 
			    color_bold.c_str(), pname.c_str(), color_nor.c_str(),
			    color_bold.c_str(), pnewname.c_str(), color_nor.c_str());
		  }
		  errors++;
	       } else {
		  perror("fatal error during rename");
		  userError("can't rename '%s%s%s' as '%s%s%s'!\n", 
			    color_bold.c_str(), pname.c_str(), color_nor.c_str(), 
			    color_bold.c_str(), pnewname.c_str(), color_nor.c_str());
	       }
	    }      
	    name = newname;
	 }
	 return 1;
      }
      return 0;
   } // else(modify_symlimks)
}


void splitFilesAndDirs(const tvector<tstring>& par, tvector<tstring>& files, 
		       tvector<tstring>& dirs, bool follow_links, bool quiet, bool rename_files) {
    struct stat filestat;
    int r;
    
    for(size_t i = 0; i < par.size(); i++) {
	// get file info
	if(follow_links) r =  stat(par[i].c_str(), &filestat);
	else             r = lstat(par[i].c_str(), &filestat);
	
	// error?
	if(r) {
	    // cannot stat file
	    if(ignore_errors) {
		if(!quiet) printf("'%s': no such file or directory\n", par[i].c_str());
		continue;
	    } else {
		userError("'%s': no such file or directory\n", par[i].c_str());
	    }
	}
	
	// check file type
	if(S_ISREG(filestat.st_mode)) {
	    // regular file
	    files += par[i];
	} else if(S_ISDIR(filestat.st_mode)) {
	    // dir
	    dirs += par[i];
	} else if(S_ISLNK(filestat.st_mode)) {
	    // symbolic link
	    if(modify_symlinks || rename_files) {
		// symlinks are like files in this case
		files += par[i];
	    } else {
		if(!quiet) {
		    printf("ignoring symbolic link '%s'\n", par[i].c_str());
		}	 
	    }
	} else {
	    if(!quiet) {
		printf("ignoring non-regular file '%s' (mode=%#o)\n",
		       par[i].c_str(), (int)filestat.st_mode);
	    }
	}
    }      
}


tvector<tstring> scanDirectory(const char *dirname) {
   tvector<tstring> ret;
   struct dirent *direntry;   
   DIR *dir = opendir(dirname);
   if(dir == 0) 
     userError("error while opening directory '%s' (%s)!\n", dirname, strerror(errno));
   while((direntry = readdir(dir)) != 0) {
      if(strcmp(".", direntry->d_name) && strcmp("..", direntry->d_name))
	ret += dirname + tstring("/") + tstring(direntry->d_name);
   }   
   closedir(dir);
   return ret;
}


namespace
{
   struct Norm
   {
      const char * const pattern;
      const char * const dos_ext;
      const char * const gen_ext;
   };


   Norm norm[] = {
      { ".jpe?g", ".jpg", ".jpeg" },
      { ".mpe?g", ".mpg", ".mpeg" },
      { ".mpe?g?3", ".mp3", ".mp3" },
      { ".cpp", ".cpp", ".cc" },
      { ".hpp", ".hpp", ".hh" },
      { ".cc", ".cpp", ".cc" },
      { ".hh", ".hpp", ".hh" },
      { ".html", ".htm", ".html" },
      { ".tar.gz", ".tgz", ".tar.gz" },
      { ".tgz", ".tgz", ".tar.gz" },
      { ".gif", ".gif", ".gif" },
      { 0, 0, 0 }
   };


   tstring norm_pattern( const char * const pattern )
   {
      tstring nrv;

      assert( pattern );

      for ( const char * r = pattern; * r; ++r )
      {
	 if ( r[ 0 ] == '.' )
	 {
	    nrv += "[.]";
	 }
	 else if ( isalpha( r[ 0 ] ) )
	 {
	    nrv += '[';
	    nrv += toupper( * r );
	    nrv += tolower( * r );
	    nrv += ']';
	 }
	 else
	 {
	    nrv += r[ 0 ];
	 }
      }
      nrv += '$';
      return nrv;
   }

} //


int main(int argc, char *argv[]) {
   try {
      // get parameters
      TAppConfig ac(options, "options", argc, argv, "STREPLACE", CONFIGFILE, VERSION);
      
      // setup
      int errors=0;
      int ren_dirs=0, ren_files=0;
      bool stdiomode = false;
      modify_symlinks = ac("modify-symlinks");
      acrossDirs = ac("across-dirs");
      shorten_filenames = ac.getInt("shorten");
      color_bold = ac.getString("color-bold");
      color_thin = ac.getString("color-thin");
      color_nor = ac.getString("color-nor");
      resolveNameClash = ac("resolve-clash");
      bool quiet = ac("quiet");   
      bool trace = ac("dummy-trace");   
      bool ltrace = ac("dummy-linetrace");   
      size_t ltrace_context = ac.getInt("context");
      bool ltrace_sep = (ac.getString("context"))[0] != '+';
      bool follow_links = ac("follow-links");
      bool use_regexp = !ac("no-regexp");
      bool ignorepattern_use_regexp = !ac("ignfilepat-nore");
      bool ignore_case = ac("ignore-case");
      preserve_case = ac("preserve-case");
      ignore_errors = ac("ignore-errors");
      bool whole_words = ac("whole-words");
      bool dummy = ac("dummy-mode");
      bool force = ac("force");
      bool constLength = ac("const-length");
      bool tryAllPos = ac("try-all-pos");
      int progressNum = ac.getInt("progress");
      bool progress = (progressNum>0);
      bool rename_files = false;
      bool restore_files = false;
      bool modify_files = true;
      tstring baksuf = ac.getString("suffix");
      tstring pre_padstring;
      tstring post_padstring;
      if(ac.wasSetByUser("ignore-lines") && ac.wasSetByUser("select-lines"))
	   userError("only ione out of --ignore-lines and --select-lines may be specified\n");
      if(ac("rename") + ac("rename-only") + ac("modify-symlinks") > 1)
	userError("--rename, --rename-only or --modify-symlinks make no sense together!\n");
      if(ac("rename") || ac("rename-only"))
	rename_files = true;
      if(ac("rename-only"))
	modify_files = false;
      if(use_regexp && whole_words) 
	userError("--whole-words (-w) not applicable in regexp mode (try -x)\n");
      if(use_regexp && constLength) 
	userError("--const-length not applicable in regexp mode (try -x)\n");
      int max_filesize = ac.getInt("max-filesize");
      if(max_filesize == 0) max_filesize = INT_MAX;
      else max_filesize <<= 20;
      if(acrossDirs && (!rename_files))
	userError("--across-dirs only useful for --rename/--rename-only\n");
#ifdef __APPLE__
       caseInsensitiveFilesystem = true;
#endif
       if(ac("assume-cifs"))
	   caseInsensitiveFilesystem = true;	
       if(ac("assume-csfs"))
	   caseInsensitiveFilesystem = false;	
      
      
      // modify symlinks setup
      if(modify_symlinks) {
	 modify_files = false;
	 rename_files = true;
      }
      
      // restore setup
      if(ac("restore")) {
	 if(rename_files) 
	   userError("--restore does not like the --rename, --rename-only, --modify-symlinks options!\n");
	 modify_files=false;
	 rename_files=false;
	 restore_files=true;
      }
      
      // dummy, trace, quiet and progress setup
      if(quiet) {
	 progress=false;
	 progressNum=0;
      }
      if(trace||ltrace) {
	 dummy = true;
	 quiet = true;
	 pre_padstring  = color_bold;
	 post_padstring = color_nor;
      }
      
      // compile modify_case
      modify_case = tstring::NOT;
      if(ac("upper-case")) modify_case = tstring::UPPER;
      if(ac("lower-case")) modify_case = tstring::LOWER;
      if(ac("capitalize-case")) modify_case = tstring::CAPITALIZE;
      {
	 int i=0;
	 if(ac("preserve-case")) ++i;
	 if(ac("lower-case")) ++i;
	 if(ac("upper-case")) ++i;
	 if(ac("capitalize-case")) ++i;
	 if(i>1)
	   userError("specify only one out of --preserve-case, --upper-case, --lower-case and --capitalize-case!\n");
      }
      if(ac("no-color"))
	   color_thin = color_bold = color_nor = "";
       if(ac("bold"))
       {
	   color_bold = "\33[01m";
	   color_thin = "\33[07m";
	   color_nor = "\33[00m";
       }
       if(ac("bcolor"))
       {
	   color_bold = "\33[34m";
	   color_thin = "\33[31m";
	   color_nor = "\33[00m";
       }
       if(ac("dcolor"))
       {
	   color_bold = "\33[33;01m";
	   color_thin = "\33[34m";
	   color_nor = "\33[00m";
       }
      // dummy message
      if(dummy && (!quiet)) printf("dummy mode: nothing will change ...\n");
      
      // get rules and files
      tvector<tstring> l_rules;
      tvector<tstring> r_rules;
      tvector<tstring> par;
      tvector<TRegEx *> regex;
      tvector<tstring> clash_from;
      tvector<tstring> clash_to;
      size_t ipar;
      for(ipar = 0; ipar < ac.numParam(); ipar++) {
	 if(ac.param(ipar) == "--") { // files follow
	    ipar++;
	    break;
	 }
	 tvector<tstring> a(splitAndAllowBackslashEscapedSeparator(ac.param(ipar), "="));
	 switch(a.size()) {
	  case 1: // file
	    par += a[0];
	    break;
	  case 2: // rule
	    a[0].compileCString();
	    a[1].compileCString();
	    if(a[0].len()==0) 
	      userError("can't handle empty left side of rule '%s'!\n", ac.param(ipar).c_str());
	    l_rules += a[0];
	    r_rules += a[1];
	    break;
	  default: // error
	    userError("invalid rule '%s'!\n(rule must be of format 'from=to', if you want a literal '=' char use '\\=')\n", ac.param(ipar).c_str());
	 }
      }
      
      // add dos-to-unix rule
      if(ac("dos-to-unix") || ac("dos-to-unix2")) {
	 l_rules += "\r\n";
	 r_rules += "\n";
	 use_regexp = false;
      }

      // add norm-ext-dos rules
      if(ac("norm-ext-dos"))
      {
	 for ( unsigned i = 0; norm[ i ].pattern; ++i )
	 {
	    l_rules += norm_pattern( norm[ i ].pattern );
	    r_rules += tstring( norm[ i ].dos_ext );
	 }
      }

      // add norm-ext rule rules
      if(ac("norm-ext"))
      {
	 for ( unsigned i = 0; norm[ i ].pattern; ++i )
	 {
	    l_rules += norm_pattern( norm[ i ].pattern );
	    r_rules += tstring( norm[ i ].gen_ext );
	 }
      }
      
      // add sane-filename rule
      if(ac("sane-filename") || ac("sane-filename2"))
      {
#if 1	 
	 l_rules += "\xdf";                  r_rules += "ss";
	 l_rules += "\xdc";                  r_rules += "Ue";
	 l_rules += "\xd6";                  r_rules += "Oe";
	 l_rules += "\xc4";                  r_rules += "Ae";
	 l_rules += "\xfc";                  r_rules += "ue";
	 l_rules += "\xf6";                  r_rules += "oe";
	 l_rules += "\xe4";                  r_rules += "ae";
#endif	 
	 l_rules += "&";                  r_rules += "and";
	 l_rules += "\'";                 r_rules += "";
	 l_rules += "[(]";                 r_rules += " - ";
	 l_rules += "[)]";                 r_rules += " - ";
	 l_rules += "[[]";                 r_rules += " - ";
	 l_rules += "[]]";                 r_rules += " - ";
	 l_rules += "----";                 r_rules += "--";
	 l_rules += "---";                 r_rules += "--";
	 l_rules += "--";                 r_rules += " - ";
	 l_rules += " - [.]";                 r_rules += ".";
	 l_rules += " [.]";                 r_rules += ".";
//	 l_rules += "[^a-zA-Z0-9\xA0-\xFF.,~+ -]"; r_rules += "_";
	 l_rules += "[^a-zA-Z0-9._-]"; r_rules += "_";
	 if(ac("sane-filename2")) { l_rules += " "; r_rules += "_"; }
	 // avoid repeating spaces
	 l_rules += "- -"; r_rules += " - ";
	 l_rules += "____"; r_rules += "_";
	 l_rules += "    "; r_rules += " ";
	 l_rules += "___"; r_rules += "_";
	 l_rules += "   "; r_rules += " ";
	 l_rules += "__"; r_rules += "_";
	 l_rules += "^ "; r_rules += "";
      }
      
      // add remaining parameters as files (only after a '--') 
      for(; ipar<ac.numParam(); ipar++)
	par += ac.param(ipar);
      
      // check for stdin/out
      if((par.size()==1) && (par[0] == "-")) { // stdin/out found
	 quiet = true;
	 trace = ltrace = progress = false;
	 stdiomode = true;
	 par.clear();
	 if(restore_files || rename_files) userError("renaming and restoring of files not allowed when using stdio ('-')!\n");
	 if(dummy) userError("dummy mode not allowed when using stdio ('-')!\n");
      }
      
      // check rules
      if(restore_files) {
	 if(l_rules.size()!=0)
	   userError("rules not applicable during --restore!\n");
      } else {
	 if(l_rules.size()==0) {
	     userError("no rules given (specify at least one rule of the form FOO=BAR to replace FOO by BAR)\n");
#if 0	     
	    if(!quiet)
	      userWarning("no rules given: using implicit regular expression rule '%s=%s' (%s)\n", DEFAULT_RULE_LEFT, DEFAULT_RULE_RIGHT, DEFAULT_RULE_HELP);
	    use_regexp = true;
	    l_rules += DEFAULT_RULE_LEFT;
	    r_rules += DEFAULT_RULE_RIGHT;
#endif	     
	 }
	 for(size_t i = 0; i < l_rules.size(); i++) {
	    if(use_regexp && l_rules[i].containsNulChar()) {
	       l_rules[i].expandUnprintable();
	       userError("illegal null char in regexp '%s'!\n", l_rules[i].c_str());
	    }
	    if(rename_files&&(!force)) {
	       size_t j;
	       for(j = 0; j < r_rules[i].len(); j++) 
		 if((r_rules[i][j]<32)&&(r_rules[i][j]>=0))
		   break;
	       if(j < r_rules[i].len()) {
		  r_rules[i].expandUnprintable();
		  userError("--rename/--rename-only/--modify-symlinks do not like control chars in right hand side '%s'!\n", r_rules[i].c_str());
	       }
	    }
	    // check whether left and rigth side are of equal length (useful for patching binaries)
	    if(constLength && (!use_regexp)) {
	       if(l_rules[i].length() != r_rules[i].length()) {
		  size_t l = l_rules[i].length();
		  size_t r = r_rules[i].length();
		  l_rules[i].expandUnprintable();
		  r_rules[i].expandUnprintable();
		  userError("different length of left (%d) and right side (%d) of rule '%s' -> '%s'\n", int(l), int(r), l_rules[i].c_str(), r_rules[i].c_str());
	       }
	    }
	 }
      }
      
      // print rules
      if((!quiet)&&(!restore_files)) {
	 size_t n = l_rules.size();
	 printf("%d rule%s given:\n", int(n), (n==1)?"":"s");
	 for(size_t i = 0; i < n; i++) {
	    tstring l(l_rules[i]);
	    tstring r(r_rules[i]);
	    l.expandUnprintable();
	    r.expandUnprintable();
	    printf("  '%s%s%s' ==> '%s%s%s'\n", 
		   color_bold.c_str(), l.c_str(), color_nor.c_str(),
		   color_bold.c_str(), r.c_str(), color_nor.c_str());
	 }
	 printf("\n");
      }
      
      // compile rules
      if(use_regexp) {
	 int flags=0;
	 if(ignore_case) flags |= REG_ICASE;
	 flags |= REG_EXTENDED;
	 if(!ac("match-newline")) flags |= REG_NEWLINE;
	 for(size_t i = 0; i < l_rules.size(); i++) 
	   regex.push_back(new TRegEx(l_rules[i].c_str(), flags));
      } 
      
      // modify case for non regexp
      if(!use_regexp) {
	 for(size_t i = 0; i < r_rules.size(); i++) {
	    r_rules[i] = modifyCase(r_rules[i], modify_case);
	 }      
      }
      
      // get / compile ignore pattern 
      tvector<tstring> ignorepattern;
      tvector<TRegEx*> ignorepatternregex;
      if(!ac.getString("ignfilepat").empty()) {
	 // commandline args
	 ignorepattern = split(ac.getString("ignfilepat"), "\1");
	 for(size_t i = 0; i < ignorepattern.size(); i++) {
	    ignorepattern[i].compileCString();
	 }
	 if(ignorepattern_use_regexp) {
	    int flags=0;
	    if(ignore_case) flags |= REG_ICASE;
	    flags |= REG_EXTENDED;
	    if(!ac("match-newline")) flags |= REG_NEWLINE;
	    for(size_t i = 0; i < ignorepattern.size(); i++)
	      ignorepatternregex.push_back(new TRegEx(ignorepattern[i].c_str(), flags));
	 }
      }
      
      // get / compile select-lines and ignore-lines pattern 
      TRegEx *selectLinesRe = 0;
      bool ignoreLines = false;
      if(ac.wasSetByUser("select-lines") || ac.wasSetByUser("ignore-lines")) 
      {
	  tstring s = ac.getString("select-lines");
	  if (ac.wasSetByUser("ignore-lines"))
	  {
	      s = ac.getString("ignore-lines");
	      ignoreLines = true;
	  }
	  s.compileCString();
	  int flags=0;
	  if(ignore_case) 
	      flags |= REG_ICASE;
	  flags |= REG_EXTENDED;
	  if(!ac("match-newline")) 
	      flags |= REG_NEWLINE;
	  selectLinesRe = new TRegEx(s.c_str(), flags);
      }
      
      // get files 
      tvector<tstring> files;
      tvector<tstring> dirs;
      splitFilesAndDirs(par, files, dirs, follow_links, quiet, rename_files);
      if(ac("recursive")) { // recursive
	 tvector<tstring> newfiles;
	 while(!dirs.empty()) {
	    for(size_t i = 0; i < dirs.size(); i++) {
	       if(rename_files) {
		  ren_dirs += perhapsRename(dirs[i], l_rules, r_rules, quiet, 
					    ignore_errors, whole_words, 
					    ignore_case, use_regexp, clash_from,
					    clash_to, regex, force, dummy, errors);
	       }
	       newfiles += scanDirectory(dirs[i].c_str());
	    }
	    dirs.clear();
	    splitFilesAndDirs(newfiles, files, dirs, follow_links, quiet, rename_files);
	    newfiles.clear();
	 }
      } else { // not recursive
	 if(rename_files) {
	    for(size_t i = 0; i < dirs.size(); i++) {
	       ren_dirs += perhapsRename(dirs[i], l_rules, r_rules, quiet, 
					 ignore_errors, whole_words, 
					 ignore_case, use_regexp, clash_from,
					 clash_to, regex, force, dummy, errors);	    
	    }	 
	 } else {
	    if(!quiet) {
	       for(size_t i = 0; i < dirs.size(); i++)
		 printf("ignoring directory '%s'\n", dirs[i].c_str());
	    }
	 }
      }
      
      // filter files
      tstring filter;   
      if(ac("html-only")) filter += "," HTML_EXT;
      if(ac("c-only")) filter += "," C_EXT;
      if(!ac.getString("only").empty()) filter += "," + ac.getString("only");
      if(!filter.empty()) {
	 if(!quiet) printf("filtering files with extension in '%s'\n", filter.substr(1).c_str());
	 tvector<tstring> filt = split(filter, ",");
	 for(size_t i = 0; i < files.size(); i++) {
	    tstring ext(files[i]);
	    ext.extractFilenameExtension();
	    if(ext.empty()) {
	       files[i].clear(); // no ext ==> no match
	    } else {
	       size_t j;
	       for(j = 0; j < filt.size(); j++)
		 if(ext == filt[j]) break;
	       if(j == filt.size()) files[i].clear(); // no match
	    }
	 }
      }
      
      // filter backup files
      for(size_t i = 0; i < files.size(); i++) {
	 if(files[i].hasSuffix(baksuf)) {
	    if((!quiet)&&(!restore_files)) 
	      printf("ignoring backup file '%s'\n", files[i].c_str());
	    files[i].clear(); // ignore backup file	 
	 }
      }
      
      // process files
      tstring f;
      tvector<int> matchrule(l_rules.size(), 0);
      int total_match=0;
      int total_skip=0;
      int total_mod=0;
      int total_ignore_pat=0;
      int total_large_skip=0;
      int total_dos_detect=0;
      if(stdiomode) files += "";
      for(size_t i = 0; (i < files.size()) || ((i == 0) && stdiomode); i++) {
	 if(stdiomode || (!files[i].empty())) {
	    // printable filename
	    tstring pfile = files[i];
	    pfile.expandUnprintable();
	    
	    // perhaps restore files
	    if(restore_files) {
	       tstring bname(files[i] + baksuf);
	       if(fexists(bname.c_str())) {
		  if(!quiet) printf("restoring file '%s'\n", pfile.c_str());
		  if(!dummy) {
		     if(caseSafeRename(bname.c_str(), files[i].c_str())) {
			// error
			if(ignore_errors) {
			   if(!quiet) 
			     printf("can't rename '%s' as '%s'\n", bname.c_str(), pfile.c_str());
			   errors++;
			} else {
			   perror("fatal error during rename");
			   userError("can't rename '%s' as '%s'!\n", bname.c_str(), pfile.c_str());
			}		  
		     }
		  }
	       }
	    }
	    
	    // perhaps rename files
	    if(rename_files) {
	       ren_files += perhapsRename(files[i], l_rules, r_rules, quiet, 
					  ignore_errors, whole_words, ignore_case,
					  use_regexp, clash_from,
					  clash_to, regex, force, dummy, errors);
	    }
	    
	    // perhaps modify files
	    if(modify_files) {
	       // setup
	       int matches=0;
	       int len = 0;
	       FILE *fp = 0;
	       
	       if(stdiomode) { // stdin/stdout?
		  f.clear();
		  tstring t;
		  int r;
		  do {
		     r = t.read(stdin, STDIO_BLOCKSIZE);
		     f += t;		  
		  } while(r == STDIO_BLOCKSIZE);
		  len = f.len();
	       } else { // normal file	       
		  // read file
		  if(!quiet) {
		     printf("processing '%s' ", pfile.c_str());
		     fflush(stdout);
		  }
		  len = flen(files[i].c_str());
		  
		  // check for filesize
		  if(len > max_filesize) {
		     if(!quiet) 
		       printf(": skipping large file\n");
		     total_large_skip++;
		     continue;	       
		  }
		  
		  tstring mode = "rb";
		  tstring pmode = "readonly";
		  fp = fopen(files[i].c_str(), mode.c_str());
		  if(fp==0) {
		     if(ignore_errors) {
			if(!quiet) 
			  printf(": skipping erroneous file (%s access denied)\n", pmode.c_str());
			errors++;
			continue;
		     }
		     userError("error while opening file '%s' for mode %s!\n",
			       pfile.c_str(), pmode.c_str());
		  }
		  f.read(fp, len);
	       }
	       
	       // process file
	       if(use_regexp && f.containsNulChar()) {
		  if(!quiet) {
		     printf(": skipping binary file\n");
		  }
		  total_skip++;
	       } else {
		  size_t pat;
		  
		  // check if file should be ignored
		  for(pat = 0; pat < ignorepattern.size(); pat++) {
		     if(ignorepattern_use_regexp) {
			if(ignorepatternregex[pat]->match(f.c_str())) break;
		     } else {
			if(f.search(ignorepattern[pat])) break;
		     }
		  }
		  if(pat < ignorepattern.size()) {
		     // ignore file
		     tstring ppat = ignorepattern[pat];
		     ppat.expandUnprintable();
		     if(!quiet) 
		       printf(": ignoring due to pattern '%s'\n", ppat.c_str());
		     total_ignore_pat++;
		  } else {
		     bool ignore = false;
		     
		     // check for dos to unix conversion
		     if(ac("dos-to-unix") && (!ac("dos-to-unix2"))) {
			int found = 0;
			if(f.len() < 2) ignore = true;
			else {
			   if(f[0] == '\n') ignore = true;
			}
			for(size_t pos = 1; pos < f.len(); pos++) {
			   if(f[pos] == '\n') {
			      if(f[pos-1] != '\r') {
				 ignore = true;
				 break;
			      } else found++;
			   }
			}
			if(found == 0) ignore = true;
			if(!ignore) {
			   if(!quiet) 
			     printf(": dos file detected ");
			   total_dos_detect++;
			} else {
			   if(!quiet) 
			     printf("(no dos file)\n");
			}
		     }
		     
		     if(!ignore) {
			// apply all rules	       
			tvector<int> match;
			tvector<int> lines;
			for(size_t r = 0; r < l_rules.size(); r++) {
			   int m = 0;
			   if(use_regexp) {
			       // apply only to certain lines?
			       if (selectLinesRe)
			       {
				   tvector<tstring> flines = split(f.c_str(), "\n");
				   int offset = 0; // offset of current line in file
				   for(size_t i = 0; i < flines.size(); ++i)
				   {
				       printf("l '%s' %d\n", flines[i].c_str(), selectLinesRe->match(flines[i].c_str()));
				       if (selectLinesRe->match(flines[i].c_str()) != ignoreLines)
				       {
					   tvector<int> linematch;
					   tvector<tvector<int> > all;
					   tstring out;
					   regex[r]->allMatchesSubstring(flines[i].c_str(), all);
					   parameterSubstitution(flines[i], out, r_rules[r], all, 
								 preserve_case, modify_case,
								 progressNum, pre_padstring, post_padstring, &linematch);
					   flines[i] = out;
					   m += all.size();				       
					   for (size_t j = 0; j < linematch.size(); ++j)
					       match.push_back(linematch[j] + offset);
				       }
				       offset += flines[i].length() + 1;
				   }
				   f = join(flines, "\n");
			       }
			       else
			       {
				   tvector<tvector<int> > all;
				   tstring out;
				   if(progress) {putchar('S');fflush(stdout);}
				   regex[r]->allMatchesSubstring(f.c_str(), all, 0, progressNum);
				   if(progress) {putchar('R');fflush(stdout);}
				   parameterSubstitution(f, out, r_rules[r], all, 
							 preserve_case, modify_case,
							 progressNum, pre_padstring, post_padstring, &match);
				   f=out;
				   m = all.size();
			       }
			   } else {
			      // try out all possible positions? (to patch/hack a binary)
			      if(tryAllPos) {
				 m = f.search(l_rules[r], ignore_case, whole_words, progressNum, &match);
				 for(size_t l = 0; l + 1 < match.size(); l += 2) {
				    tstring fname;
				    fname.sprintf("%03d_%08X_", l >> 1, match[l]);
				    fname += files[i];
				    if(!quiet)
				      printf("\rwriting patched file to '%s%s%s'\n", color_bold.c_str(), fname.c_str(), color_nor.c_str());
				    if(!dummy) {
				       tstring tmp = f;
				       tmp.replace(match[l], match[l + 1] - match[l], r_rules[r]);
				       if(tmp.writeFile(fname.c_str()))
					 userError("error while writing file '%s'\n", fname.c_str());
				    }
				 }
			      }
			      // just normal replacement
			      m = f.searchReplace(l_rules[r], r_rules[r],
						  ignore_case, whole_words, 
						  preserve_case, progressNum, pre_padstring, post_padstring, &match);
			   }
			   matches += m;
			   matchrule[r] += m;
			   if(ltrace && matches) { // mark matching lines
			      int num_lf=0;
			      size_t pos_lf=0;
			      size_t start, end, pos;
			      for(size_t l = 0; l < match.size(); l+=2) {
				 // get start 
				 pos = match[l];
				 if(pos < pos_lf) {
				    num_lf = 0;
				    pos_lf = 0;
				 }
				 for(; (pos_lf < f.len()) && (pos_lf < pos); pos_lf++)
				   if(f[pos_lf] == '\n') num_lf++;
				 start = num_lf;
				 // get end
				 pos = match[l+1];
				 if(pos < pos_lf) {
				    num_lf = 0;
				    pos_lf = 0;
				 }
				 for(; (pos_lf < f.len()) && (pos_lf < pos); pos_lf++) {
				    if(f[pos_lf] == '\n') num_lf++;
				 }
				 end = num_lf;
				 // mark lines
				 size_t j = 0;
				 if(start > ltrace_context)
				   j = start - ltrace_context;
				 for(size_t k = lines.size(); k < j; k++) lines[k] = 0;
				 for(; j <= end + ltrace_context; j++) lines[j] = 1;
			      }		     
			      match.clear();
			   }
			}
			
			// print file (trace)
			if(trace) {
			   if(matches) {
			      fprintf(stdout, "%s%s%s (%s%d%s match%s)\n", 
				      color_bold.c_str(), pfile.c_str(), color_nor.c_str(), color_bold.c_str(), matches,
				      color_nor.c_str(), (matches==1)?"":"es");		  
			      f.write(stdout);
			      fprintf(stdout, "\n");
			   } else {
			      fprintf(stdout, "%s%s%s (no match)\n", 
				      color_bold.c_str(), pfile.c_str(), color_nor.c_str());		  
			   }
			}
			
			// print matching lines (ltrace)
			if(ltrace) {
			   if(matches) {
			      fprintf(stdout, "%s%s%s (%s%d%s match%s)\n",
				      color_bold.c_str(), pfile.c_str(), color_nor.c_str(), color_bold.c_str(), matches,
				      color_nor.c_str(), (matches==1)?"":"es");
			      size_t scan = 0;
			      size_t start;
			      for(size_t l = 0; l < lines.size(); l++) {
				 start = scan;
				 f.scanUpTo(scan, '\n');
				 if(scan < f.len()) scan++;
				 if(lines[l]) f.substr(start, scan).write(stdout);
				 else {
				    if(ltrace_sep) if(l<lines.size()-1) if(lines[l+1]) printf("%s--%d--%s\n", color_thin.c_str(), int(l+2), color_nor.c_str());
				 }
				 if(f.scanEOS(scan)) break;
			      }		     
			      fprintf(stdout, "\n");
			   } 
			}
			
			// print/update local stats
			if(!quiet) {
			   if(progress) printf(" ");
			   if(matches) {
			      printf("(%s%d%s match%s)\n", color_bold.c_str(), matches,
				     color_nor.c_str(), (matches==1)?"":"es");
			   } else {
			      printf("(no match)\n");
			   }
			}
			total_match += matches;
		     } // if(!ignore)
		  } // if else ignore file
	       } // if else containsNulChar
	       
	       if(stdiomode) { // write to stdout?
		  f.write(stdout);
	       } else { // normal file	       
		  // perhaps write/trunc and close file
		  if((!dummy) && (matches>0)) {
		     if(ac("backup"))
		       makeBackup(fp, len, files[i], baksuf, ignore_errors, 
				  errors, quiet);
		     fclose(fp);
		     fp = fopen(files[i].c_str(), "wb");
		     if(fp == 0) {
			if(ignore_errors) {
			   if(!quiet) 
			     printf("warning: error while opening file '%s' for writing! (%s)\n", files[i].c_str(), strerror(errno));
			   errors++;
			} else {
			   userError("error while opening file '%s' for writing! (%s)\n", files[i].c_str(), strerror(errno));
			}
		     } else {		 		 
			if(f.write(fp) != f.len()) {
			   if(ignore_errors) {
			      if(!quiet) 
				printf("warning: error while writing to file '%s'! (%s)\n", files[i].c_str(), strerror(errno));
			      errors++;
			   } else {
			      userError("error while writing to file '%s'! (%s)\n", files[i].c_str(), strerror(errno));
			   }
			}
			fclose(fp);
			total_mod++;
		     }
		  } else {
		     fclose(fp);
		  }
	       }
	       
	    } // if modify_files
	    
	 } // if files
      } // for files
      
      // statistics
      if((!quiet) && modify_files) {
	 size_t n = l_rules.size();
	 printf("\nmodification statistics for %d rule%s:\n", int(n), (n==1)?"":"s");
	 for(size_t i = 0; i < n; i++) {
	    tstring l(l_rules[i]);
	    tstring r(r_rules[i]);
	    l.expandUnprintable();
	    r.expandUnprintable();
	    printf("%s%8d%s replacement%s with '%s%s%s' ==> '%s%s%s'\n",
		   color_bold.c_str(), matchrule[i], color_nor.c_str(), matchrule[i]==1?"":"s",
		   color_bold.c_str(), l.c_str(), color_nor.c_str(),
		   color_bold.c_str(), r.c_str(), color_nor.c_str());
	 }
	 printf("===========================\n");
	 printf("%s%8d%s replacement%s total\n", 
		color_bold.c_str(), total_match, color_nor.c_str(), total_match==1?"":"s");
	 if(total_skip) 
	   printf("(%s%d%s binary file(s) skipped since regexp is useless on files containing '\\0')\n", 
		  color_bold.c_str(), total_skip, color_nor.c_str());
	 if(total_ignore_pat) 
	   printf("(%s%d%s file(s) containing the ignore-file-pattern ignfilepat skipped)\n",
		  color_bold.c_str(), total_ignore_pat, color_nor.c_str());
	 if(total_large_skip) 
	   printf("(%s%d%s large file(s) (>%dMB) skipped)\n", 
		  color_bold.c_str(), total_large_skip, color_nor.c_str(), max_filesize>>20);
	 if(total_dos_detect) 
	   printf("(%s%d%s dos files detected)\n", 
		  color_bold.c_str(), total_dos_detect, color_nor.c_str());
	 if(total_mod) 
	   printf("(%s%d%s file%s modified)\n", 
		  color_bold.c_str(), total_mod, color_nor.c_str(), total_mod==1?"":"s");
	 else printf("(no files modified)\n");
      }
      
      if((!quiet) && rename_files) {      
	 if(ren_files + ren_dirs) {
	    if(modify_symlinks) {
	       printf("%s%d%s symlink%s changed\n",
		      color_bold.c_str(), ren_files + ren_dirs, color_nor.c_str(), ren_files==1?"":"s");
	    } else {
	       printf("%s%d%s file%s and %s%d%s director%s renamed (%s%d%s total)\n",
		      color_bold.c_str(), ren_files, color_nor.c_str(), ren_files==1?"":"s", color_bold.c_str(), ren_dirs, color_nor.c_str(), ren_dirs==1?"y":"ies",
		      color_bold.c_str(), ren_files + ren_dirs, color_nor.c_str());
	    }
	 } else {
	    if(modify_symlinks)
	      printf("(no symlinks changed)\n");
	    else
	      printf("(no files or directories renamed)\n");
	 }
      }
      
      if((!quiet) && errors) {
	 printf("%s%d%s error%s ignored\n", color_bold.c_str(), errors,
		color_nor.c_str(), errors==1?"":"s");
      }
      
      // print name clashes
      if(rename_files && (!modify_symlinks)) {
	 if(clash_from.size()) {
	    printf("\nwarning: %d name clash%s during rename encountered:\n", int(clash_from.size()), clash_from.size()==1?"":"es");
	    for(size_t i = 0; i < clash_from.size(); i++) {
	       printf("tried to rename '%s' as '%s', which exists\n", clash_from[i].c_str(), clash_to[i].c_str());
	    }
	 } else {
	     if(!quiet) 
	     {
		 if(numResolvedClashes)
		     printf("(%s%d%s name clashes resolved by prepending N_)\n", color_bold.c_str(), numResolvedClashes, color_nor.c_str());
		 else
		     printf("(no name clashes found)\n");
	     }
	 }
      }
      
      // cleanup
      if(use_regexp) {
	 for(size_t i = 0; i < l_rules.size(); i++) {
	    delete regex[i];
	 }
      }
      
       // save config
       if(ac("save-colors"))
       {
	   ac.setValue("color-bold", color_bold);
	   ac.setValue("color-thin", color_thin);
	   ac.setValue("color-nor", color_nor);
       }
       ac.save();
      
   }
   catch(const TException& e) {
      fprintf(stderr, "%s exception caught: %s!\n", e.name(), e.message());
      exit(1);
   }
   catch(...) {
      fprintf(stderr, "unhandled exception caught!\n");
      exit(1);
   }
   
   // normal exit
   return 0;
}

