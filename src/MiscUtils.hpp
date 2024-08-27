// Misc utility functions.
//
// Copyright (c) 2021-2024 Johannes Overmann
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype>
#include <regex>
#include <filesystem>
#include <sys/stat.h>
#include <string_view>

// --- Windows support ---
#ifdef _WIN32
using ssize_t = ptrdiff_t;
#define isatty(fd) ((fd) == 1)
#define __PRETTY_FUNCTION__ __FUNCSIG__
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

/// Convert UTF-8 to NFD (canonical decomposed form).
std::string toNfd(const std::string& s);

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

/// Hexlify binary string.
inline std::string hexlify(const std::vector<uint8_t> &bytes)
{
    std::stringstream os;
    os << std::hex << std::setfill('0') << std::nouppercase;
    for (uint8_t byte : bytes)
    {
        os << std::setw(2) << unsigned(byte);
    }
    return os.str();
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

/// Get type name.
template<typename T> constexpr std::string_view typeNameHelper() { return __PRETTY_FUNCTION__; }
template<typename T>
constexpr std::string_view typeName()
{
    constexpr std::string_view name = typeNameHelper<T>();
    return name.substr(typeNameHelper<void>().find("void"), name.length() - typeNameHelper<void>().length() + 4);
}

// --- File utilities. ---

/// Read string from file.
std::string readFile(const std::string& filename);

/// Write string to file.
void writeFile(const std::string& filename, const std::string& data);

/// File type.
enum FileType { FT_REGULAR, FT_DIR, FT_SYMLINK, FT_FIFO, FT_BLOCK, FT_CHAR, FT_SOCKET, FT_NON_EXISTING };

/// Get file type.
FileType getFileType(const std::filesystem::directory_entry& entry, bool followSymlinks = true);
FileType getFileType(const std::filesystem::path& entry, bool followSymlinks = true);

/// Get file type string.
std::string getFileTypeStr(const std::filesystem::directory_entry& entry, bool followSymlinks = true);
std::string getFileTypeStr(const std::filesystem::path& entry, bool followSymlinks = true);

/// Get file type string.
std::string getFileTypeStr(FileType fileType);

/// Return true iff entry exists.
/// This returns true for broken symlinks.
bool fsExists(const std::filesystem::path& entry);

/// Return true iff entry is a directory.
bool fsIsDirectory(const std::filesystem::path& entry, bool followSymlinks = true);

/// Return true iff entry is a regular file.
bool fsIsRegular(const std::filesystem::path& entry, bool followSymlinks = true);

/// File stat() info.
/// This is only used to access stuff which is not accessible through std::filesystem::file_status (major/minor for block devices and st_dev/st_ino for inode identity (hardlink groups)).
class StatInfo
{
public:
    StatInfo();
    StatInfo(const std::filesystem::directory_entry& entry, bool followSymlinks);

    dev_t getRDev() const { return statData.st_rdev; }
    dev_t getDev() const { return statData.st_dev; }
    ino_t getIno() const { return statData.st_ino; }
    std::filesystem::file_time_type getMTime() const;

    struct timespec getMTimeSpec() const
    {
#ifdef __linux__
        return statData.st_mtim;
#endif
#ifdef __APPLE__
        return statData.st_mtimespec;
#endif
    }

    struct stat statData;
};

/// Get stat() information.
StatInfo getStat(const std::filesystem::directory_entry& entry, bool followSymlinks = true);

/// Get last write time (as in std::filesystem::last_write_time()).
std::filesystem::file_time_type getLastWriteTime(const std::filesystem::directory_entry& entry, bool followSymlinks = true);

/// Set last write time (as in std::filesystem::last_write_time()).
void setLastWriteTime(const std::filesystem::directory_entry& entry, std::filesystem::file_time_type new_time, bool followSymlinks = true);

// --- Misc ---

/// Get current absolute wallclock time in seconds.
inline double getTimeSec()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace ut1
