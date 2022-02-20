// streplace - Replace strings in files, filenames and symbolic links, in place, recursively.
//
// Copyright (c) 2021-2022 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#include <regex>
#include <iostream>
#include <filesystem>
#include "CommandLineParser.hpp"
#include "MiscUtils.hpp"
#include "UnitTest.hpp"

/// Escape sequences used for verbose output/tracing.
class EscapeSequences
{
public:
    std::string bold = "\33[01m";
    std::string thin = "\33[07m";
    std::string normal = "\33[00m";
};


/// Rule.
class Rule
{
public:
    explicit Rule(const std::string& rule)
    {
        std::vector<std::string> sides = ut1::splitString(rule, '=');
        if (sides.size() != 2)
        {
            throw std::runtime_error("Rule \"" + rule + "\" must contain exactly one '=' char. Please escape verbatime '=' chars with a backslash.");
        }
        lhs = sides[0];
        rhs = ut1::compileCString(sides[1]);
    }

    std::string lhs;
    std::string rhs;
    uint64_t numMatches{};
};


/// Print Rule.
std::ostream& operator<<(std::ostream& os, const Rule& rule)
{
    return os << ut1::expandUnprintable(rule.lhs) << "=" << ut1::expandUnprintable(rule.rhs);
}


/// Steplace application logic.
/// Todo: Lots.
/// - Split out replacing logic from file handling logic.
/// - Implement all the options which are already here.
/// - Implement all the missing options.
class Streplace
{
public:
    /// Constructor.
    Streplace(const ut1::CommandLineParser& cl)
    {
        // Get command line options.
        recursive = cl("recursive");
        followLinks = cl("follow-links");
        all = cl("all");

        ignoreCase = cl("ignore-case");
        noRegex = cl("no-regex");
        wholeWords = cl("whole-words");

        modifySymlinks = cl("modify-symlinks");

        verbose = cl.getCount("verbose");
        dummyMode = cl("dummy-mode");
        dummyTrace = cl("dummy-trace");
        dummyLineTrace = cl("dummy-linetrace");
        dummyLineTraceHideSep = ut1::hasPrefix(cl.getStr("context"), "+");
        context = cl.getUInt("context");

        // Implicit options.
        modifyFiles = !modifySymlinks;
        dummyMode |= dummyTrace || dummyLineTrace;
        trace = dummyTrace || dummyLineTrace;

        // Regex flags.
        regexFlags = std::regex::ECMAScript; // | std::regex::multiline;
#if 0
        regexFlags = std::regex::egrep;
        regexFlags = std::regex::extended;
        regexFlags = std::regex::basic;
        regexFlags = std::regex::grep;
        regexFlags = std::regex::awk;
#endif
        if (ignoreCase)
        {
            regexFlags |= std::regex::icase;
        }
    }

    /// Add rule.
    void addRule(const std::string& rule)
    {
        rules.emplace_back(rule);
    }

    /// Print rules.
    void printRules()
    {
        std::cout << "Rules:\n";
        for (const Rule& rule: rules)
        {
            std::cout << rule << "\n";
        }
    }

    /// Process directory entry (rename and modify content).
    void processDirectoryEntry(std::filesystem::directory_entry& directoryEntry)
    {
        // Rename.

        // Process content.
        if ((!followLinks) && directoryEntry.is_symlink())
        {
            processSymlink(directoryEntry);
        }
        else if (directoryEntry.is_regular_file())
        {
            processRegularFile(directoryEntry);
        }
        else if (directoryEntry.is_directory())
        {
            processDirectory(directoryEntry);
        }
        else
        {
            processOther(directoryEntry);
        }
    }

    /// Print statistics.
    void printStats()
    {
        std::cout << "(" << numFilesRead << " files, " << numDirs << " dirs and " << numSymlinks << " symlinks processed and " << numIgnored << " directory entries ignored.)\n";
    }

private:
    /// Get file type string, e.g. "file" or "directory".
    std::string getFileTypeStr(const std::filesystem::directory_entry& directoryEntry)
    {
        if ((!followLinks) && directoryEntry.is_symlink())
        {
            return "symlink";
        }
        else if (directoryEntry.is_regular_file())
        {
            return "file";
        }
        else if (directoryEntry.is_directory())
        {
            return "directory";
        }
        else if (directoryEntry.is_block_file())
        {
            return "block device";
        }
        else if (directoryEntry.is_character_file())
        {
            return "characters device";
        }
        else if (directoryEntry.is_fifo())
        {
            return "fifo";
        }
        else if (directoryEntry.is_socket())
        {
            return "socket";
        }
        else
        {
            // Will never get here.
            return "unknown file type";
        }
    }

    /// Process regular file.
    void processRegularFile(const std::filesystem::directory_entry& directoryEntry)
    {
        if (modifyFiles)
        {
            if (verbose)
            {
                std::cout << "Processing " << directoryEntry.path() << ut1::flushTty;
            }

            // Read file.
            std::string data = ut1::readFile(directoryEntry.path());
            numFilesRead++;

            // Apply all rules.
            size_t numMatches = 0;
            for (Rule rule: rules)
            {
                numMatches += applyRule(data, rule);
            }

            if (verbose)
            {
                if (numMatches)
                {
                    std::cout << " (" << numMatches << ")";
                }
                std::cout << "\n";
            }

            if (numMatches == 0)
            {
                return;
            }

            // Write file.
            numFilesWritten++;
            if (!dummyMode)
            {
                ut1::writeFile(directoryEntry.path(), data);
            }

            // Trace.
            if (trace)
            {
                ut1::addTrailingLfIfMissing(data);
                printTrace(data, directoryEntry.path(), numMatches);
            }
        }
        else
        {
            if (verbose)
            {
                std::cout << "Ignoring file " << directoryEntry.path() << ".\n";
            }
            numIgnored++;
        }
    }

    /// Process symlink.
    void processSymlink(const std::filesystem::directory_entry& directoryEntry)
    {
        if (modifySymlinks)
        {
            if (verbose)
            {
                std::cout << "Processing symlink " << directoryEntry.path() << ".\n";
            }
            numSymlinks++;
        }
        else
        {
            if (verbose)
            {
                std::cout << "Ignoring symlink " << directoryEntry.path() << ".\n";
            }
            numIgnored++;
        }
    }

    /// Process directory.
    void processDirectory(const std::filesystem::directory_entry& directoryEntry)
    {
        if (recursive && (!skipDir(directoryEntry.path())))
        {
            if (verbose)
            {
                std::cout << "Processing dir  " << directoryEntry.path() << ".\n";
            }

            // Note: Take a copy of each directory_entry intentionally,
            // because we will potentially modify it (rename) in processDirectoryEntry.
            for (std::filesystem::directory_entry entry: std::filesystem::directory_iterator(directoryEntry))
            {
                processDirectoryEntry(entry);
            }

            numDirs++;
        }
        else
        {
            if (verbose)
            {
                std::cout << "Ignoring   dir  " << directoryEntry.path() << ".\n";
            }
            numIgnored++;
        }
    }

    /// Process non-file, non-dir and non-symlinks.
    void processOther(const std::filesystem::directory_entry& directoryEntry)
    {
        if (verbose)
        {
            std::cout << "Ignoring   " << getFileTypeStr(directoryEntry) << " '" << directoryEntry.path() << "'.\n";
        }
        numIgnored++;
    }

    /// Return true iff we should skip this dir.
    bool skipDir(const std::filesystem::path& dir)
    {
        if (all)
        {
            return false;
        }

        if (dir.filename() == ".git")
        {
            return true;
        }

        return false;
    }

    /// Replace single match.
    std::string replaceMatch(const std::smatch& match, Rule& rule, size_t& numMatches)
    {
        std::string r = match.format(rule.rhs);
        if (trace)
        {
            r = escapeSequences.bold + r + escapeSequences.normal;
        }
        numMatches++;
        return r;
    }

    /// Apply rule to string.
    /// Return number of matches.
    /// Increase rule.numMatches.
    uint64_t applyRule(std::string& s, Rule& rule)
    {
        size_t numMatches = 0;

        if (noRegex)
        {
        }
        else
        {
            s = ut1::regex_replace(s, std::regex(rule.lhs), [&](const std::smatch& match) { return replaceMatch(match, rule, numMatches); });
        }

        rule.numMatches += numMatches;
        return numMatches;
    }

    /// Print trace or line trace.
    void printTrace(const std::string& s, const std::string& filename, size_t numMatches)
    {
        std::cout << escapeSequences.bold << filename << escapeSequences.normal << " (" << escapeSequences.bold << numMatches << escapeSequences.normal << " matches):\n";

        if (dummyTrace)
        {
            std::cout << s;
        }
        else
        {
            // Line trace.
            std::vector<std::string> lines = ut1::splitLines(s);

            // Mark all lines which contain the escape sequences for matches including context lines.
            std::vector<bool> marked(lines.size());
            for (ssize_t line = 0; line < ssize_t(lines.size()); line++)
            {
                if (ut1::contains(lines[line], escapeSequences.bold) || ut1::contains(lines[line], escapeSequences.normal))
                {
                    for (ssize_t i = line - context; i <= ssize_t(line + context); i++)
                    {
                        if ((i >= 0) && (i < ssize_t(marked.size())))
                        {
                            marked[i] = true;
                        }
                    }
                }
            }

            // Print all marked lines.
            for (size_t line = 0; line < lines.size(); line++)
            {
                if ((!dummyLineTraceHideSep) && ((line == 0) || (marked[line] && (!marked[line - 1]))))
                {
                    std::cout << escapeSequences.thin << "--" << line + 1 << "--" << escapeSequences.normal << "\n";
                }
                if (marked[line])
                {
                    std::cout << lines[line] << "\n";
                }
            }
        }
    }


private:
    // --- Private data. ---

    /// Command line options.
    bool recursive{};
    bool followLinks{};
    bool all{};

    bool ignoreCase{};
    bool noRegex{};
    bool wholeWords{};

    bool modifySymlinks{};

    unsigned verbose{};
    bool dummyMode{};
    bool dummyTrace{};
    bool dummyLineTrace{};
    bool dummyLineTraceHideSep{};
    bool trace{};
    size_t context{};

    bool modifyFiles{};
    std::vector<Rule> rules;
    std::regex::flag_type regexFlags{};

    /// Statistics.
    uint64_t numIgnored{};
    uint64_t numDirs{};
    uint64_t numFilesRead{};
    uint64_t numFilesWritten{};
    uint64_t numSymlinks{};

    EscapeSequences escapeSequences;
};


int main(int argc, char *argv[])
{
    // Run unit tests and exit if enabled at compile time.
    UNIT_TEST_RUN();

    // Command line options.
    ut1::CommandLineParser cl("streplace", "Replace strings in files, filenames and symbolic links, in place, recursively.\n"
                         "\n"
                         "Usage: $programName [OPTIONS, FILES, DIRS and RULES] [--] [FILES and DIRS]\n"
                         "\n"
                         "This program substitutes strings in files, filenames and symbolic links according to rules:\n"
                         "- A rule is of the from FOO=BAR which replaces FOO by BAR. FOO is a regular expression by default (unless -x is specified).\n"
                         "- Use C escape sequences like \\n \\t \\xff. Use \\\\ to get a verbatim backslash. Note that you will need to protect backslashes from the shell by using single quotes or by duplicating backslashes.\n"
                         "- Use \\= to get a verbatim =.\n"
                     "- Use 'IMG([0-9]*).jpeg=pic$1.jpg' to reuse subexpressions of regular expressions ($& for the whole match, $n for subexpressions)."
                         "\n",
                         "\n"
                         "$programName version $version *** Copyright (c) 2021-2022 Johannes Overmann *** https://github.com/jovermann/streplace", "0.10.1");

    cl.addHeader("\nFile options:\n");
    cl.addOption('r', "recursive", "Recursively process directories.");
    cl.addOption('l', "follow-links", "Follow symbolic links.");
    cl.addOption(0,   "all", "Process all files and directories. By default '.git' directories are skipped.");

    cl.addHeader("\nMatching options:\n");
    cl.addOption('i', "ignore-case", "Ignore case.");
    cl.addOption('x', "no-regex", "Match the left side of each rule as a simple string, not as a regex (substring search, useful with binary files).");
    cl.addOption('w', "whole-words", "Match only whole words (only for -x mode, not for regex).");

    cl.addHeader("\nRenaming options:\n");
    cl.addOption('A', "rename", "Rename files and dirs in addition to modifying files contents. Use -N if you only want to rename and not modify the file content.");
    cl.addOption('s', "modify-symlinks", "Modify the path symlinks point to instead of file content and/or filename.");

    cl.addHeader("\nVerbose / common options:\n");
    cl.addOption('v', "verbose", "Increase verbosity. Specify multiple times to be more verbose.");
    cl.addOption('d', "dummy-mode", "Do not write/change anything.");
    cl.addOption('T', "dummy-trace", "Do not write/change anything, but print matching files to stdout and highlight replacements.");
    cl.addOption('L', "dummy-linetrace", "Do not write/change anything, but print matching lines of matching files to stdout and highlight replacements.");
    cl.addOption(0,   "context", "set number of context lines for --dummy-linetrace to N (use +N to hide line separator) (range=[0..], default=1).", "N", "1");


    // Parse command line options.
    cl.parse(argc, argv);

    // Steplace instance.
    Streplace streplace(cl);

    try
    {
        // Parse non-option arguments (paths and rules).
        std::vector<std::filesystem::directory_entry> paths;
        bool allowRules = true;
        for (const auto& arg: cl.getArgs())
        {
            if (allowRules && ut1::contains(arg, '='))
            {
                streplace.addRule(arg);
            }
            else if (allowRules && (arg == "--"))
            {
                allowRules = false;
            }
            else if (std::filesystem::exists(arg))
            {
                paths.emplace_back(arg);
            }
            else
            {
                cl.error("'" + arg + "': No such file or directory.");
            }
        }

        // Print rules.
        if (cl.getCount("verbose") >= 2)
        {
            streplace.printRules();
        }

        // Process files and directories.
        for (std::filesystem::directory_entry path: paths)
        {
            streplace.processDirectoryEntry(path);
        }

        // Print stats.
        if (cl("verbose"))
        {
            streplace.printStats();
        }
    }
    catch (const std::exception& e)
    {
        cl.error(e.what());
    }

    return 0;
}


