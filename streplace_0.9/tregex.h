// encapsulated POSIX regular expression (regex) interface header
//
// Copyright (c) 1998 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#ifndef _ngw_tregex_h_
#define _ngw_tregex_h_

#include <sys/types.h>
#include <regex.h>
#include "tstring.h"


class TRegEx {
 public:
   // ctor & dtor
   // cflags: REG_EXTENDED, REG_ICASE, REG_NOSUB, REG_NEWLINE
   TRegEx(const char *regex, int cflags);
   ~TRegEx();

   // matching:
   // eflags: REG_NOTBOL, REG_NOTEOL
   
   // return true if str matches the regex
   bool match(const char *str, int eflags=0) const;
   // return true if str matches the regex and set position
   bool firstMatch(const char *str, int& start, int& len, int eflags=0) const;
   // return true if str matches the regex and return all positions
   // in all: all[0]=start1, all[1]=len1, all[2]=start2, all[3]=len2, ...
   bool allMatches(const char *str, tvector<int>& all, int eflags=0) const;
   // same as above but also recognize substrings in all
   bool allMatchesSubstring(const char *str, tvector<tvector<int> >& all, 
			    int eflags=0, int progress=0, int progmode=0) const;

   // constants for allMatchesSubstring(..., progmode)
   // default: print '.' to stdout
   // P_STDERR: print to stderr
   // P_NUMBER: print number instead of a '.'
   enum {P_STDERR=1, P_NUMBER=2};

   // error exceptions
   class TErroneousRegexException: public TException {
      TExceptionN(TErroneousRegexException);
      TErroneousRegexException(int error_, const regex_t& preg_): error(error_), preg(preg_) {}
      const char *str() const {
	 static char buf[256];
	 regerror(error, &preg, buf, sizeof(buf));
	 return buf;
      }
      int error;
      regex_t preg;
      TExceptionM2("%s (error #%d)", str(), error);
   };
   class TNoSubRegexException: public TException {TExceptionN(TErroneousRegexException);};

 private: 
   // private data
   regex_t preg;
   bool nosub;
   
   // forbid default copy and assignment
   TRegEx(const TRegEx&);                  
   const TRegEx& operator=(const TRegEx&);    
};


// substitute occurences occ in string with sub and write to out
// and use back references in sub from occ (\0 .. \9 \a .. \z)
void parameterSubstitution(const tstring& in, tstring& out, const tstring& sub,
			   const tvector<tvector<int> >& occ, 
			   bool preserve_case=false, int modify_case=0,
			   int progress=0, const tstring& pre_padstring=tstring(), 
			   const tstring& post_padstring=tstring(), tvector<int> *match_pos=0);


#endif /* tregex.h */
