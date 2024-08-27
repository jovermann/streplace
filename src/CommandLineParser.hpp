// Command line parser.
//
// Copyright (c) 2021-2024 Johannes Overmann
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <vector>
#include <map>
#include <exception>
#include <algorithm>
#include <functional>
#include <iostream>
#include "MiscUtils.hpp"


namespace ut1
{


/// Command line parser.
/// Most features of GNU getopt_long() are implemented.
class CommandLineParser
{
public:
    /// Command line option.
    struct Option
    {
        /// --- Modifiers for addOption() ---

        /// Make this option a list option.
        Option& listOption()
        {
            isList = true;
            return *this;
        }

        /// Set short option alias.
        Option& addAlias(char alias) { shortOptionAlias = alias; parent->shortOptionToLongOption[shortOptionAlias] = longOption; return *this; }

        /// --- Get/set interface ---

        /// Has formal argument.
        bool hasArg() const noexcept { return !argName.empty(); }

        /// Set value.
        void setValue(const std::string& v, char listSepChar_);

        /// Get "longName[=argName] string length.
        size_t getHelpNameLen() const;

        /// Meta info (initialized by addOption()).
        char        shortOption{};
        char        shortOptionAlias{};
        std::string longOption;
        std::string help;
        std::string argName;
        std::string defaultValue;
        bool        isList{};

        /// Actual value.
        std::string value;

        /// Number of times specified on the command line.
        unsigned count{};

        /// Pointer to enclosing CommandLineParser instance.
        CommandLineParser *parent{};
    };

    friend struct Option;

    /// Constructor.
    CommandLineParser(const std::string& programName_, const std::string& usage_, const std::string& footer_, const std::string& version_);

    /// Add option header.
    void addHeader(const std::string& header);

    /// Add option.
    /// - argName: This must be non-empty for options which take an argument. If this is empty the option is a switch option and switches are counted.
    Option& addOption(char shortOption, const std::string& longOption, const std::string& help, const std::string& argName = std::string(), const std::string& defaultValue = std::string());

    /// Parse command line.
    /// By default parse() does not return for --help/--version or on errors.
    /// Return 0 on success.
    void parse(int argc, const char* argv[]);
    void parse(int argc, char* argv[]) { parse(argc, const_cast<const char **>(argv)); }

    /// Get switch value.
    bool operator()(const std::string& longOption) const { return isSet(longOption); }

    /// Get switch value.
    bool getBool(const std::string& longOption) const { return isSet(longOption); }

    /// Get switch value.
    bool isSet(const std::string& longOption) const { return getCount(longOption) > 0; }

    /// Get number of times option was specified on the command line.
    unsigned getCount(const std::string& longOption) const;

    /// Get string value.
    const std::string& getStr(const std::string& longOption) const;

    /// Get string list;
    std::vector<std::string> getList(const std::string& longOption) const;

    /// Get signed int value.
    long long getInt(const std::string& longOption) const;

    /// Get unsigned int value.
    unsigned long long getUInt(const std::string& longOption) const;

    /// Get double value.
    double getDouble(const std::string& longOption) const;

    /// Get positional args.
    const std::vector<std::string>& getArgs() const noexcept { return args; }

    /// Set option value from within the program.
    /// This is useful to set logical non-static default values.
    void setValue(const std::string& longOption, const std::string& value, bool clearList = true);

    /// Set bool option (to true by default).
    void setOption(const std::string& longOption, bool value = true) { setValue(longOption, value ? "1" : "0"); }

    /// Print error and exit.
    [[noreturn]] void error(const std::string& message, int exitStatus = 1) const;

    /// Print error and exit.
    [[noreturn]] static void reportErrorAndExit(const std::string& message, int exitStatus = 1);

    /// Print message and potentially exit.
    void printMessage(const std::string& message) const;

    /// Get usage string.
    std::string getUsageStr() const;

private:
    /// Get option.
    Option* getOption(const std::string& longOption);

    /// Get option (const).
    const Option* getOption(const std::string& longOption) const;

    /// Get option by short option name.
    Option* getShortOption(char shortOption);

    /// Parse long option in argv[i], potentially consuming an argument in argv[++i].
    void parseLongOption(int argc, const char* argv[], int& i);

    /// Parse short options in argv[i], potentially consuming an argument in argv[++i].
    void parseShortOptions(int argc, const char* argv[], int& i);

    /// Options.
    std::map<std::string, Option> options;

    /// List of options in the order they were declared using addOption()
    /// and in the order they will appear in --help.
    /// This also contains header strings for --help which start with "header:".
    std::vector<std::string> optionList;

    /// Map short options to long options.
    std::map<char,std::string> shortOptionToLongOption;

    /// Positional arguments.
    std::vector<std::string> args;

    /// List separator char.
    const char listSepChar{'\1'};

    /// Program name.
    std::string programName;

    /// Usage text.
    std::string usage;

    /// Footer text.
    std::string footer;

    /// Version text.
    std::string version;

    /// Global instance pointer for reportErrorAndExit().
    static CommandLineParser *instance;
};


} // namespace ut1
