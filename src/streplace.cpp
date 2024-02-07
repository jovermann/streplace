// streplace - Replace strings in files, filenames and symbolic links, in place, recursively.
//
// Copyright (c) 2021-2022 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#include <regex>
#include <iostream>
#include <filesystem>
#include <utility>
#include "CommandLineParser.hpp"
#include "MiscUtils.hpp"
#include "UnitTest.hpp"

/// Escape sequences used for verbose output/tracing.
class EscapeSequences
{
public:
    std::string bold   = "\33[01m";
    std::string thin   = "\33[07m";
    std::string normal = "\33[00m";
};


/// Error exception.
class Error: public std::runtime_error
{
public:
    explicit Error(const std::string& message)
    : std::runtime_error(message)
    {
    }

    virtual ~Error() override;
};

Error::~Error() { }


/// Rule.
class Rule
{
public:
    Rule(const std::string& rule, const std::string& separator, bool noRegex)
    {
        std::vector<std::string> sides = ut1::splitString(rule, separator);
        if (sides.size() != 2)
        {
            throw Error("Rule \"" + rule + "\" must contain exactly one separator '" + separator + "' (got " + std::to_string(sides.size() - 1) + ")." + ((sides.size() >= 2) ? " You can choose a different/unique separator string using --equals to avoid conflicts with the left and right side of the rule." : ""));
        }
        lhs = std::move(sides[0]);
        rhs = ut1::compileCString(sides[1]);
        if (noRegex)
        {
            lhs = ut1::quoteRegexChars(lhs);
        }
    }

    std::string lhs;
    std::string rhs;
    uint64_t    numMatches{};
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
    explicit Streplace(const ut1::CommandLineParser& cl)
    {
        // Get command line options.
        recursive   = cl("recursive");
        followLinks = cl("follow-links");
        all         = cl("all");

        ignoreCase = cl("ignore-case");
        noRegex    = cl("no-regex");
        wholeWords = cl("whole-words");
        equals     = cl.getStr("equals");
        dollar     = cl.getStr("dollar");

        verbose        = cl.getCount("verbose");
        dummyMode      = cl("dummy-mode");
        preview        = cl("preview");
        context        = int(cl.getInt("context"));
        previewHideSep = ut1::hasPrefix(cl.getStr("context"), "+");

        // Derive rename/symlink/file content mode.
        modifySymlinks = cl("modify-symlinks");
        rename = cl("rename") || cl("rename-only");
        modifyFiles = (!modifySymlinks) && (!cl("rename-only"));
        if (modifySymlinks && rename)
        {
            throw Error("--modify-symlinks cannot be combined with --rename or --rename-only");
        }
        if (cl("rename") && cl("rename-only"))
        {
            throw Error("--rename cannot be combined with --rename-only");
        }

        // Implicit options.
        dummyMode |= preview;

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

        if (equals.empty())
        {
            throw Error("--equals must not be empty");
        }
        if (dollar.empty())
        {
            throw Error("--dollar must not be empty");
        }
    }

    /// Add rule.
    void addRule(const std::string& rule)
    {
        rules.emplace_back(rule, equals, noRegex);
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
        // Rename files and dirs.
        if (rename)
        {
            std::filesystem::path oldPath = directoryEntry.path();
            std::filesystem::path basePath = oldPath.parent_path();
            std::string oldName = oldPath.filename().string();
            std::string newName = applyAllRules(oldName);
            if (oldName != newName)
            {
                std::filesystem::path newPath = basePath / newName;
                if (verbose)
                {
                    std::cout << "Renaming " << oldPath.string() << " -> " << newPath.string() << ".\n";
                }
                if (!dummyMode)
                {
                    std::filesystem::rename(oldPath, newPath);
                    directoryEntry.replace_filename(newName);
                }
                numDirsRenamed++;
            }
        }

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
        std::vector<std::string> l;
        if (numFilesProcessed)
        {
            l.push_back(std::to_string(numFilesModified) + "/" + std::to_string(numFilesProcessed) + " file" + ut1::pluralS(numFilesModified) + " modified");
        }
        if (numSymlinksProcessed)
        {
            l.push_back(std::to_string(numSymlinksModified) + "/" + std::to_string(numSymlinksProcessed) + " symlink" + ut1::pluralS(numSymlinksModified) + " modified");
        }
        if (numFilesRenamed)
        {
            l.push_back(std::to_string(numFilesRenamed) + "/" + std::to_string(numFilesProcessed) + " file" + ut1::pluralS(numFilesRenamed) + " renamed");
        }
        if (numDirsRenamed)
        {
            l.push_back(std::to_string(numDirsRenamed) + "/" + std::to_string(numDirsProcessed) + " dir" + ut1::pluralS(numDirsRenamed) + " renamed");
        }
        else if (numDirsProcessed)
        {
            l.push_back(std::to_string(numDirsProcessed) + " dir" + ut1::pluralS(numDirsProcessed) + " processed");
        }
        if (!l.empty())
        {
            std::cout << "(" << ut1::joinStrings(l, ", ") << ")\n";
        }
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

    /// Apply all rules.
    std::string applyAllRules(const std::string& input, size_t* numMatchesOut = nullptr)
    {
        std::string r = input;
        size_t numMatches = 0;
        for (Rule& rule: rules)
        {
            numMatches += applyRule(r, rule);
        }
        if (numMatchesOut)
        {
            *numMatchesOut = numMatches;
        }
        return r;
    }

    /// Process regular file.
    void processRegularFile(const std::filesystem::directory_entry& directoryEntry)
    {
        if (!modifyFiles)
        {
            return;
        }

        if (verbose >= 2)
        {
            std::cout << "Processing " << directoryEntry.path().string() << ut1::flushTty;
        }

        // Read file.
        std::string data = ut1::readFile(directoryEntry.path().string());
        numFilesProcessed++;

        // Apply all rules.
        size_t numMatches = 0;
        data = applyAllRules(data, &numMatches);

        if (verbose)
        {
            if (numMatches)
            {
                std::cout << "\rModifying " << directoryEntry.path().string() << " (" << numMatches << " matches)\n";
            }
            else
            {
                if (verbose >= 2)
                {
                    std::cout << "\n";
                }
            }
        }

        if (numMatches)
        {
            // Write file.
            numFilesModified++;
            if (!dummyMode)
            {
                ut1::writeFile(directoryEntry.path().string(), data);
            }

            // Preview.
            if (preview)
            {
                ut1::addTrailingLfIfMissing(data);
                printPreview(data, directoryEntry.path().string(), numMatches);
            }
        }
    }

    /// Process symlink.
    void processSymlink(const std::filesystem::directory_entry& directoryEntry)
    {
        if (modifySymlinks)
        {
            if (verbose >= 2)
            {
                std::cout << "Processing symlink " << directoryEntry.path().string() << ".\n";
            }
            std::string oldp = std::filesystem::read_symlink(directoryEntry);
            std::string newp = applyAllRules(oldp);
            if (newp != oldp)
            {
                if (verbose)
                {
                    std::cout << "Modifying symlink target of " << directoryEntry.path().string() << ": " << oldp << " -> " << newp << ".\n";
                }
                if (!dummyMode)
                {
                    std::filesystem::remove(directoryEntry);
                    std::filesystem::create_symlink(newp, directoryEntry);
                }
                numSymlinksModified++;
            }
            numSymlinksProcessed++;
        }
        else
        {
            if (verbose >= 2)
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
            if (verbose >= 2)
            {
                std::cout << "Processing dir " << directoryEntry.path().string() << ".\n";
            }

            // Note: Take a copy of each directory_entry intentionally,
            // because we will potentially modify it (rename) in processDirectoryEntry.
            for (std::filesystem::directory_entry entry: std::filesystem::directory_iterator(directoryEntry))
            {
                processDirectoryEntry(entry);
            }

            numDirsProcessed++;
        }
        else
        {
            std::cout << "Ignoring dir " << directoryEntry.path().string() << ".\n";
            numIgnored++;
        }
    }

    /// Process non-file, non-dir and non-symlinks.
    void processOther(const std::filesystem::directory_entry& directoryEntry)
    {
        if (verbose)
        {
            std::cout << "Ignoring " << getFileTypeStr(directoryEntry) << " " << directoryEntry.path().string() << ".\n";
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
        // --whole-words
        if (wholeWords && match.length())
        {
            if ((ut1::isalnum_(match.str()[0]) && match.prefix().length() && ut1::isalnum_(match.prefix().str().back())) || (ut1::isalnum_(match.str().back()) && match.suffix().length() && ut1::isalnum_(match.suffix().str()[0])))
            {
                // Ignore match, return original string.
                return match.str();
            }
        }

        // Format according to rhs and return replacement string.
        std::string r = match.format(rule.rhs);
        if (preview)
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

        s = ut1::regex_replace(s, std::regex(rule.lhs), [&](const std::smatch& match)
            { return replaceMatch(match, rule, numMatches); });

        rule.numMatches += numMatches;
        return numMatches;
    }

    /// Print preview.
    void printPreview(const std::string& s, const std::string& filename, size_t numMatches)
    {
        std::cout << escapeSequences.bold << filename << escapeSequences.normal << " (" << escapeSequences.bold << numMatches << escapeSequences.normal << " match" + ut1::pluralS(numMatches, "es") + "):\n";

        if (context == -1)
        {
            // Print whole file.
            std::cout << s;
        }
        else
        {
            // Print only changed lines plus context.
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
                if ((!previewHideSep) && (marked[line] && ((line == 0) || (!marked[line - 1]))))
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

    unsigned verbose{};
    bool     dummyMode{};
    bool     preview{};
    bool     previewHideSep{};
    int      context{};

    /// Main operations.
    bool modifyFiles{};
    bool rename{};
    bool modifySymlinks{};

    /// Matching options.
    std::vector<Rule>     rules;
    std::regex::flag_type regexFlags{};
    bool ignoreCase{};
    bool noRegex{};
    bool wholeWords{};
    std::string equals;
    std::string dollar;

    /// Statistics.
    uint64_t numIgnored{};
    uint64_t numFilesProcessed{};
    uint64_t numFilesModified{};
    uint64_t numFilesRenamed{};
    uint64_t numSymlinksProcessed{};
    uint64_t numSymlinksModified{};
    uint64_t numDirsProcessed{};
    uint64_t numDirsRenamed{};

    EscapeSequences escapeSequences;
};


int main(int argc, char* argv[])
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
        "$programName version $version *** Copyright (c) 2021-2022 Johannes Overmann *** https://github.com/jovermann/streplace",
        "0.10.2");

    cl.addHeader("\nFile options:\n");
    cl.addOption('r', "recursive", "Recursively process directories.");
    cl.addOption('l', "follow-links", "Follow symbolic links.");
    cl.addOption(' ', "all", "Process all files and directories. By default '.git' directories are skipped.");

    cl.addHeader("\nMatching options:\n");
    cl.addOption('i', "ignore-case", "Ignore case.");
    cl.addOption('x', "no-regex", "Match the left side of each rule as a simple string, not as a regex (substring search, useful with binary files).");
    cl.addOption('w', "whole-words", "Match only whole words. A word is an alphanumeric seuqnece with underscores. If the match begins/ends with a non-word char then this is always considered to be a word boundary, e.g. 'foo;' matches '::foo;' but not 'barfoo;'.");
    cl.addOption(' ', "equals", "Use STR instead of \"=\" as the rule lhs/rhs-separator, e.g. fooSTRbar. This may be one or more chars long. Example: --equals==== allows rules to have the form \"int a = 0;===unsigned a = 0;\"", "STR", "=");
    cl.addOption(' ', "dollar", "Use STR instead of \"$\" in substring references in the replacement string, e.g. STR&, STR1, STR12. This may be one or more chars long. Example: --dollar=SUB for \"0x([0-9A-Za-z]+)=$SUB1\"", "STR", "$");

    cl.addHeader("\nRenaming options:\n");
    cl.addOption('N', "rename-only", "Rename files and dirs. Do not modify files contents.");
    cl.addOption('A', "rename", "Rename files and dirs and modify files contents.");
    cl.addOption('s', "modify-symlinks", "Modify the path symlinks point to. Do not modify files contents. Do not rename. -N, -A and -s are mutually exclusive.");

    cl.addHeader("\nVerbose / common options:\n");
    cl.addOption('v', "verbose", "Increase verbosity. Specify multiple times to be more verbose.");
    cl.addOption('d', "dummy-mode", "Do not write/change anything.").addAlias('0');
    cl.addOption('P', "preview", "Do not write/change anything, but print matching lines of matching files with context to stdout and highlight replacements.");
    cl.addOption(' ', "context", "set number of context lines for --preview to N (use +N to hide line separator, use -1 to display the whole file) (range=[-1..], default=1).", "N", "1");

    // Parse command line options.
    cl.parse(argc, argv);

    try
    {
        // Steplace instance.
        Streplace streplace(cl);

        // Parse non-option arguments (paths and rules).
        std::vector<std::filesystem::directory_entry> paths;
        bool                                          allowRules = true;
        for (const std::string& arg: cl.getArgs())
        {
            if (arg.empty())
            {
                cl.printMessage("Ignoring empty argument.");
                continue;
            }
            if (allowRules && ut1::contains(arg, cl.getStr("equals")))
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
