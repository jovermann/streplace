// Misc utility functions.
//
// Copyright (c) 2021-2022 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#ifndef include_MiscUtils_hpp
#define include_MiscUtils_hpp

#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <regex>

#ifdef _WIN32
using ssize_t = ptrdiff_t;
#define isatty(fd) ((fd) == 1)
#endif

namespace ut1
{
// --- String utilities: Operations on one string. ---

/// Has prefix.
bool hasPrefix(const std::string& s, const std::string& prefix) noexcept;

/// Has suffix.
bool hasSuffix(const std::string& s, const std::string& suffix) noexcept;

/// Contains character.
bool contains(const std::string& s, char c) noexcept;

/// Contains string.
bool contains(const std::string& haystack, const std::string& needle) noexcept;

/// Replace substring in place.
/// If from is empty, s is left unmodified.
void replaceStringInPlace(std::string& s, const std::string& from, const std::string& to);

/// Replace substring.
/// If from is empty, s is left unmodified.
std::string replaceString(const std::string& s, const std::string& from, const std::string& to);

/// Expand unprintable chars to C-style backslash sequences.
std::string expandUnprintable(const std::string& s, char quotes = 0, char addQuotes = 0);

/// Compile C-style backslash sequences back to unprintable chars.
std::string compileCString(const std::string& s, std::string* errorMessageOut = nullptr);

/// Skip whitespace (as in isspace()).
void skipSpace(const char*& s) noexcept;

/// Convert char to lowercase.
inline char tolower(char c) { return char(std::tolower(static_cast<unsigned char>(c))); }

/// Convert string to lowercase.
std::string tolower(std::string s);

/// Convert char to uppercase.
inline char toupper(char c) { return char(std::toupper(static_cast<unsigned char>(c))); }

/// Convert string to uppercase.
std::string toupper(std::string s);

/// Capitalize (first char uppercase, rest lowercase).
std::string capitalize(std::string s);

/// Sane isalnum().
inline bool isalnum(char c) noexcept { return std::isalnum(static_cast<unsigned char>(c)); }

/// Sane isalnum() with _.
inline bool isalnum_(char c) noexcept { return isalnum(c) || (c == '_'); }

/// Sane isprint().
inline bool isprint(char c) noexcept { return std::isprint(static_cast<unsigned char>(c)); }

/// Add trailing LF if missing.
void addTrailingLfIfMissing(std::string& s);

/// Convert a string to a regex matching itself by backslash escaping all
/// regex special chars.
std::string quoteRegexChars(const std::string& s);

// --- String utilities: Misc. ---

/// Split string at separator char.
/// An empty string returns an empty list.
std::vector<std::string> splitString(const std::string& s, char sep, int maxSplit = -1);

/// Split string at separator string.
/// An empty string returns an empty list.
std::vector<std::string> splitString(const std::string& s, const std::string& sep, int maxSplit = -1);

/// Split text into lines at LF and optionally wrap text at wrapCol.
/// A trailing LF is ignored and does not result in an extra empty line at the end.
/// The input "a\n" and "a" both result in ["a"].
std::vector<std::string> splitLines(const std::string& s, size_t wrapCol = 0);

/// Join vector of strings.
std::string joinStrings(const std::vector<std::string>& stringList, const std::string& sep);

/// std::regex_replace() with a callback function instead of a format string.
template<typename FormatMatch>
std::string regex_replace(const std::string& s, const std::regex& re, FormatMatch f)
{
    std::string r;

    size_t               endOfMatch = 0;
    std::sregex_iterator end;
    for (std::sregex_iterator it(s.begin(), s.end(), re); it != end; it++)
    {
        r.append(it->prefix());
        r.append(f(*it));
        endOfMatch = it->position(0) + it->length(0);
    }

    r.append(s.substr(endOfMatch));

    return r;
}


// --- Printing ---

/// Print vector<T>.
template<typename T>
std::ostream& operator<<(std::ostream& s, const std::vector<T>& v)
{
    s << "{";
    bool first = true;
    for (const auto& elem: v)
    {
        if (!first)
        {
            s << ", ";
        }
        else
        {
            first = false;
        }
        s << elem;
    }
    s << "}";
    return s;
}

/// Print vector<string>.
std::ostream& operator<<(std::ostream& s, const std::vector<std::string>& v);

/// Safely convert to printable string.
template<typename T>
std::string toStr(const T& t)
{
    std::stringstream r;
    r << t;
    return r.str();
}

/// Convert std::string to a printable std::string.
inline std::string toStr(const std::string& s)
{
    return expandUnprintable(s, '"', '"');
}

/// Convert const char * to a printable std::string.
inline std::string toStr(const char* s)
{
    return "(const char*)" + expandUnprintable(s, '"', '"');
}

/// Flush output stream, but only if:
/// 1. it is std::cout.
/// 2. it is connected to a TTY.
/// (Flushing stdout which is redirected to a file has a disasterous
/// performance impact. Use this to suppress flushes whiuch are just
/// inserted to progress output to the same line.)
std::ostream& flushTty(std::ostream& os);

/// Get plural s or not.
/// (Note that not all nouns get their plural by appending an s.)
inline std::string pluralS(size_t n, const std::string& pluralSuffix = "s", const std::string& singularSuffix = std::string())
{
    return (n == 1) ? singularSuffix : pluralSuffix;
}


// --- File utilities. ---

/// Read string from file.
std::string readFile(const std::string& filename);

/// Write string to file.
void writeFile(const std::string& filename, const std::string& data);


} // namespace ut1

#endif /* include_MiscUtils_hpp */
