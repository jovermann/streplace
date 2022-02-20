// Misc utility functions.
//
// Copyright (c) 2021-2022 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#include <unistd.h>
#include "MiscUtils.hpp"
#include "UnitTest.hpp"

namespace ut1
{


bool hasPrefix(const std::string& s, const std::string& prefix)
{
    return s.compare(0, prefix.length(), prefix) == 0;
}


UNIT_TEST(hasPrefix,
{
    ASSERT_EQ(hasPrefix("foobar", "foo"), true);
    ASSERT_EQ(hasPrefix("foobar", "bar"), false);
    ASSERT_EQ(hasPrefix("foobar", ""), true);
    ASSERT_EQ(hasPrefix("", "bar"), false);
    ASSERT_EQ(hasPrefix("foo", "foobar"), false);
})


bool hasSuffix(const std::string& s, const std::string& suffix)
{
    if (suffix.length() > s.length())
    {
        return false;
    }
    return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
}


UNIT_TEST(hasSuffix,
{
    ASSERT_EQ(hasSuffix("foobar", "foo"), false);
    ASSERT_EQ(hasSuffix("foobar", "bar"), true);
    ASSERT_EQ(hasSuffix("foobar", ""), true);
    ASSERT_EQ(hasSuffix("", "bar"), false);
    ASSERT_EQ(hasSuffix("bar", "foobar"), false);
})


bool contains(const std::string& s, char c)
{
    return s.find(c) != std::string::npos;
}


UNIT_TEST(contains,
{
    ASSERT_EQ(contains("abc", 'd'), false);
    ASSERT_EQ(contains("abc", 'b'), true);
    ASSERT_EQ(contains("", 'b'), false);
    ASSERT_EQ(contains("abc", '\0'), false);
    ASSERT_EQ(contains(std::string("ab\0c", 4), '\0'), true);
})


bool contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}


UNIT_TEST(containsStr,
{
    ASSERT_EQ(contains("abc", "abcd"), false);
    ASSERT_EQ(contains("abcd", "abc"), true);
    ASSERT_EQ(contains("abc", "abc"), true);
    ASSERT_EQ(contains("abc", ""), true);
    ASSERT_EQ(contains("", ""), true);
    ASSERT_EQ(contains(std::string("ab\0c", 4), "\0"), true);
    ASSERT_EQ(contains(std::string("ab\0c", 4), "c"), true);
})


void replaceStringInPlace(std::string& s, const std::string& from, const std::string& to)
{
    if(!from.empty())
    {
        for(size_t pos = 0;; pos += to.size())
        {
            pos = s.find(from, pos);
            if (pos == std::string::npos)
            {
                break;
            }
            s.replace(pos, from.size(), to);
        }
    }
}


std::string replaceString(const std::string& s, const std::string& from, const std::string& to)
{
    std::string r = s;
    replaceStringInPlace(r, from, to);
    return r;
}


UNIT_TEST(replaceString,
{
    ASSERT_EQ(replaceString("foobar", "foo", "abc"), "abcbar");
    ASSERT_EQ(replaceString("foobar", "foo", ""), "bar");
    ASSERT_EQ(replaceString("foobar", "bar", ""), "foo");
    ASSERT_EQ(replaceString("foobar", "o", ""), "fbar");
    ASSERT_EQ(replaceString("foofoo", "foo", "foobar"), "foobarfoobar");
    ASSERT_EQ(replaceString("", "foo", "bar"), "");
    ASSERT_EQ(replaceString("xx", "x", "xx"), "xxxx");
})


std::string expandUnprintable(const std::string& s, char quotes, char addQuotes)
{
    std::string r;
    char buf[4];

    if (addQuotes)
    {
        r += addQuotes;
    }

    for(const char& c: s)
    {
        if(isprint(unsigned(c)))
        {
            if((c =='\\') || (quotes && (c == quotes)))
            {
                // Backslashify backslash and quotes.
                r += '\\';
            }
            r += c;
        }
        else
        {
            // Unprintable char.
            r += '\\'; // Leading backslash.
            switch (c)
            {
                // Named control chars.
            case '\a': r += 'a'; break;
            case '\b': r += 'b'; break;
            case '\f': r += 'f'; break;
            case '\n': r += 'n'; break;
            case '\r': r += 'r'; break;
            case '\t': r += 't'; break;
            case '\v': r += 'v'; break;
            default:
                // Hex/octal byte.
                char next = *(&c + 1);
                if (isxdigit(unsigned(next)))
                {
                    // Next digit is a valid hex digit:
                    // Use 3-digit octal variant to limit the length of the numeric escape sequence.
                    ::snprintf(buf, sizeof(buf), "%03o", uint8_t(c));
                }
                else
                {
                    // Hex byte.
                    ::snprintf(buf, sizeof(buf), "x%02x", uint8_t(c));
                }
                r += buf;
                break;
            }
        }
    }

    if (addQuotes)
    {
        r += addQuotes;
    }

    return r;
}


UNIT_TEST(expandUnprintable,
{
    ASSERT_EQ(expandUnprintable("abc"), "abc");
    ASSERT_EQ(expandUnprintable("ab\"c", '"', '"'), "\"ab\\\"c\"");
    ASSERT_EQ(expandUnprintable("abc\r\n\t   "), "abc\\r\\n\\t   ");
    ASSERT_EQ(expandUnprintable("\xaa\x61"), "\\252a");
    ASSERT_EQ(expandUnprintable(std::string("a\0b", 3)), "a\\000b");
})


std::string compileCString(const std::string& s, std::string *errorMessageOut)
{
    std::string r;
    const char *p = s.c_str();
    const char *end;
    char c;
    char buf[4];

    if (errorMessageOut)
    {
        errorMessageOut->clear();
    }
    while ((c = *(p++)))
    {
        if (c == '\\')
        {
            // Escape sequence.
            c = *(p++);
            switch (c)
            {
                // End of string in escape sequence: Emit verbatim backslash.
            case 0:
                c = '\\';
                p--;
                if (errorMessageOut)
                {
                    *errorMessageOut = "unexpected end of string in escape sequence";
                }
                break;

                // Named control chars.
            case 'a': c = '\a'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'v': c = '\v'; break;

                // Hex.
            case 'x':
                if (isxdigit(unsigned(p[0])))
                {
                    c = strtoul(p, const_cast<char**>(&end), 16);
                    p = end;
                }
                else
                {
                    r += '\\';
                    if (errorMessageOut)
                    {
                        *errorMessageOut = "non hex char following \\x";
                    }
                }
                break;

                // Octal.
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                // Copy octal number.
                buf[0] = c;
                buf[1] = p[0];
                buf[2] = p[0] ? p[1] : 0;
                buf[3] = 0;
                c = strtoul(buf, const_cast<char**>(&end), 8);
                p += end - buf - 1;
                break;

                // Known backslash sequence: Leave escape sequence intact.
            default:
                r += '\\';
                if (errorMessageOut)
                {
                    *errorMessageOut = "unknown backslash sequence '\\" + expandUnprintable(std::string(1, c)) + "'";
                }
                break;
            }
        }
        r += c;
    }
    return r;
}


UNIT_TEST(compileCString,
{
    ASSERT_EQ(compileCString(""), "");
    ASSERT_EQ(compileCString("abc"), "abc");
    ASSERT_EQ(compileCString("\\x61\\x62\\x63"), "abc");
    ASSERT_EQ(compileCString("a\\r\\n\\tb"), "a\r\n\tb");
    ASSERT_EQ(compileCString("\\x1\\x2\\x3"), "\1\2\3");
    ASSERT_EQ(compileCString("\\101"), "A");
    ASSERT_EQ(compileCString("\\x41"), "A");
    ASSERT_EQ(compileCString("\\x41\\x41\\x41"), "AAA");
    ASSERT_EQ(compileCString("\\101\\101\\101"), "AAA");
    ASSERT_EQ(compileCString("\\1112"), "\x49\x32");

    // Errors:
    std::string err;
    ASSERT_EQ(compileCString("abc\\", &err), "abc\\");
    ASSERT_EQ(err, "unexpected end of string in escape sequence");
    ASSERT_EQ(compileCString("abc\\x", &err), "abc\\x");
    ASSERT_EQ(err, "non hex char following \\x");
    ASSERT_EQ(compileCString("abc\\xg", &err), "abc\\xg");
    ASSERT_EQ(err, "non hex char following \\x");
    ASSERT_EQ(compileCString("abc\\y", &err), "abc\\y");
    ASSERT_EQ(err, "unknown backslash sequence '\\y'");
})


std::vector<std::string> splitString(const std::string& s, char sep, int maxSplit)
{
    std::vector<std::string> r;

    // Special case: Empty string results in empty list (rather than a list with a single empty string).
    if (s.empty())
    {
        return r;
    }

    std::istringstream iss(s + std::string(1, sep));
    std::string elem;
    for(; (maxSplit != 0) && std::getline(iss, elem, sep); maxSplit--)
    {
        r.push_back(elem);
    }
    if (size_t(iss.tellg()) < s.size())
    {
        r.push_back(s.substr(iss.tellg()));
    }
    return r;
}


UNIT_TEST(splitString,
{
    std::vector<std::string> ref;
    ASSERT_EQ(splitString("", ','), ref);
    ref.push_back("abc");
    ASSERT_EQ(splitString("abc", ','), ref);
    ref.push_back("foo");
    ref.push_back("bar");
    ASSERT_EQ(splitString("abc,foo,bar", ','), ref);
    ref.back() = "bar,x,y,z";
    ASSERT_EQ(splitString("abc,foo,bar,x,y,z", ',', 2), ref);
    (ref = {"", "", ""});
    ASSERT_EQ(splitString(",,", ','), ref);
    (ref = {"", ""});
    ASSERT_EQ(splitString(",", ','), ref);
    (ref = {"abc", "def", ""});
    ASSERT_EQ(splitString("abc,def,", ','), ref);
    (ref = {"", "abc", "def"});
    ASSERT_EQ(splitString(",abc,def", ','), ref);
})


std::vector<std::string> splitLines(const std::string& s, size_t wrapCol)
{
    std::vector<std::string> r;
    size_t start = 0;
    size_t pos = 0;
    size_t splitPos = 0;
    size_t col = 0;
    std::string indent;
    bool firstPart = true; // First subline of a wrapped line.
    while (pos < s.length())
    {
        if (s[pos] == '\n')
        {
            std::string line = s.substr(start, pos - start);
            r.push_back(indent + line);
            pos++;
            start = pos;
            splitPos = pos;
            col = 0;
            indent = "";
            firstPart = true;
            continue;
        }
        if (!isalnum(unsigned(s[pos])))
        {
            splitPos = pos + 1;
        }
        if ((wrapCol > 0) &&
            (col == wrapCol))
        {
            if (splitPos == start)
            {
                splitPos = pos;
            }
            std::string line = s.substr(start, splitPos - start);
            r.push_back(indent + line);
            pos = splitPos;
            start = pos;
            col = indent.length();
            if (firstPart)
            {
                firstPart = false;
                size_t i = 0;
                while ((i < line.size()) && ((line[i] == ' ') || (line[i] == '-')))
                {
                    i++;
                    indent += ' ';
                }
            }
            continue;
        }
        pos++;
        col++;
    }
    if (start < s.length())
    {
        r.push_back(s.substr(start, s.length() - start));
    }
    return r;
}


UNIT_TEST(splitLines,
{
    std::vector<std::string> ref;
    ASSERT_EQ(splitLines(""), ref);
    ref.push_back("a");
    ASSERT_EQ(splitLines("a"), ref);
    ASSERT_EQ(splitLines("a\n"), ref);
    ref.push_back("");
    ASSERT_EQ(splitLines("a\n\n"), ref);
})


std::string joinStrings(const std::vector<std::string>& stringList, const std::string& sep)
{
    std::stringstream r;
    if (!stringList.empty())
    {
        r << stringList[0];
        for (size_t i = 1; i < stringList.size(); i++)
        {
            r << sep << stringList[i];
        }
    }
    return r.str();
}


UNIT_TEST(joinStrings,
{
    ASSERT_EQ(joinStrings({"a", "b", "c"}, ","), "a,b,c");
    ASSERT_EQ(joinStrings({"a", "b"}, ","), "a,b");
    ASSERT_EQ(joinStrings({"a"}, ","), "a");
    ASSERT_EQ(joinStrings({}, ","), "");
    ASSERT_EQ(joinStrings({"", "", ""}, ","), ",,");
})


UNIT_TEST(regex_replace,
{
    ASSERT_EQ(regex_replace("aa XX bb YY cc", std::regex("[A-Z]+"), [&](const std::smatch& match) { return "(" + match[0].str() + ")"; }), "aa (XX) bb (YY) cc");
    ASSERT_EQ(regex_replace("XX bb YY cc", std::regex("[A-Z]+"), [&](const std::smatch& match) { return "(" + match[0].str() + ")"; }), "(XX) bb (YY) cc");
    ASSERT_EQ(regex_replace("aa XX bb YY", std::regex("[A-Z]+"), [&](const std::smatch& match) { return "(" + match[0].str() + ")"; }), "aa (XX) bb (YY)");
    ASSERT_EQ(regex_replace("aa", std::regex("bb"), [&](const std::smatch& match) { return match[0].str(); }), "aa");
    ASSERT_EQ(regex_replace("", std::regex("bb"), [&](const std::smatch& match) { return match[0].str(); }), "");
    ASSERT_EQ(regex_replace("aa", std::regex("aa"), [&](const std::smatch& match) { return match[0].str(); }), "aa");
    ASSERT_EQ(regex_replace("aa XX bb YY cc", std::regex("[A-Z]+"), [&](const std::smatch& match) { return tolower(match[0].str()); }), "aa xx bb yy cc");
    ASSERT_EQ(regex_replace("aa XX bb YY cc", std::regex("[A-Z]+"), [&](const std::smatch& match) { return match.format("f($&)"); }), "aa f(XX) bb f(YY) cc");
    ASSERT_EQ(regex_replace("aa XX.jpg bb YY.jpg cc", std::regex("([A-Z]+)[.]jpg"), [&](const std::smatch& match) { return match.format("pic_$1.png"); }), "aa pic_XX.png bb pic_YY.png cc");
})


void skipSpace(const char *& s)
{
    if (s)
    {
        while(isspace(uint8_t(*s)))
        {
            s++;
        }
    }
}


UNIT_TEST(skipSpace,
{
    const char *p = " \t\n\ra \t";
    skipSpace(p);
    ASSERT_EQ(*p, 'a');
})


std::string tolower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}


UNIT_TEST(tolower,
{
    ASSERT_EQ(tolower("ABC"), "abc");
    ASSERT_EQ(tolower("\xff\x80 C"), "\xff\x80 c");
    ASSERT_EQ(tolower(""), "");
})


std::string toupper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::toupper(c); });
    return s;
}


UNIT_TEST(toupper,
{
    ASSERT_EQ(toupper("abc"), "ABC");
    ASSERT_EQ(toupper("\xff\x80 c"), "\xff\x80 C");
    ASSERT_EQ(toupper(""), "");
})


std::string capitalize(std::string s)
{
    s = tolower(s);
    if (!s.empty())
    {
        s[0] = std::toupper(unsigned(s[0]));
    }
    return s;
}


UNIT_TEST(capitalize,
{
    ASSERT_EQ(capitalize("abc"), "Abc");
    ASSERT_EQ(capitalize("ABC"), "Abc");
    ASSERT_EQ(capitalize("a"), "A");
    ASSERT_EQ(capitalize("\xff\x80 c"), "\xff\x80 c");
    ASSERT_EQ(capitalize(""), "");
    ASSERT_EQ(capitalize(" abc"), " abc");
    ASSERT_EQ(capitalize("one two"), "One two");
})


void addTrailingLfIfMissing(std::string& s)
{
    if (s.empty() || (s.back() != '\n'))
    {
        s.append("\n");
    }
}


UNIT_TEST(addTrailingLfIfMissing,
{
    std::string s("abc");
    addTrailingLfIfMissing(s);
    ASSERT_EQ(s, "abc\n");
    s = "abc\n";
    addTrailingLfIfMissing(s);
    ASSERT_EQ(s, "abc\n");
    s = "";
    addTrailingLfIfMissing(s);
    ASSERT_EQ(s, "\n");
    s = "\n";
    addTrailingLfIfMissing(s);
    ASSERT_EQ(s, "\n");
})


std::ostream& operator<<(std::ostream& s, const std::vector<std::string>& v)
{
    s << "{";
    bool first = true;
    for(const auto& elem: v)
    {
        if (!first)
        {
            s << ", ";
        }
        else
        {
            first = false;
        }
        s << expandUnprintable(elem, '"', '"');
    }
    s << "}";
    return s;
}


std::ostream& flushTty(std::ostream& os)
{
    static int stdoutIsATty = -1;
    if (stdoutIsATty < 0)
    {
        stdoutIsATty = !!isatty(1);
    }

    // Do not flush stdout if os is not stdout.
    if (os.rdbuf() != std::cout.rdbuf())
    {
        return os;
    }

    // Flush stdout only if it is connected to a terminal.
    if (stdoutIsATty)
    {
        os << std::flush;
    }

    return os;
}


std::string readFile(const std::string& filename)
{
    std::ifstream is(filename, std::ios::in | std::ios::binary | std::ios::ate);
    size_t size = is.tellg();
    is.seekg(0, is.beg);
    std::string r(size, '\0');
    is.read(&r[0], size);
    return r;
}


void writeFile(const std::string& filename, const std::string& data)
{
    std::ofstream os(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    os.write(data.data(), data.size());
}


UNIT_TEST(readFile_WriteFile,
{
    std::string filename = "/tmp/UtilitiesTest";
    writeFile(filename, "abc");
    std::string s = readFile(filename);
    ASSERT_EQ(s, "abc");
})


} // namespace ut1

