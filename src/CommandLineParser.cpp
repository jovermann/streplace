// Command line parser.
//
// Copyright (c) 2021-2022 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#include <string>
#include <vector>
#include <map>
#include <exception>
#include <algorithm>
#include <functional>
#include <iostream>
#include "CommandLineParser.hpp"

namespace ut1
{


void CommandLineParser::Option::setValue(const std::string& v, char listSepChar_)
{
    if (isList && (count > 0))
    {
        value += listSepChar_;
        value += v;
    }
    else
    {
        value = v;
    }
    count++;
    if (argName.empty())
    {
        value = std::to_string(count);
    }
}


size_t CommandLineParser::Option::getHelpNameLen() const
{
    return longOption.length() + argName.length() + (!argName.empty());
}


CommandLineParser::CommandLineParser(const std::string& programName_, const std::string& usage_, const std::string& footer_, const std::string& version_)
: programName(programName_)
, usage(usage_)
, footer(footer_)
, version(version_)
{
    // Appear in --help in reverse order:
    addOption(0, "version", "Print version and exit.");
    addOption('h', "help", "Print this help message and exit.");
}


void CommandLineParser::addHeader(const std::string& header)
{
    // Keep --help and --version at the end of the list.
    optionList.insert(optionList.begin() + optionList.size() - std::min(optionList.size(), size_t(2)), "header:" + header);
}


CommandLineParser::Option& CommandLineParser::addOption(char shortOption, const std::string& longOption, const std::string& help, const std::string& argName, const std::string& defaultValue)
{
    // Check for duplicate long option.
    if (options.count(longOption))
        throw std::runtime_error("addOption(longOption=" + longOption + "): option already exists\n");

    // Check for duplicate short option.
    if (shortOption)
    {
        Option* existingOption = getShortOption(shortOption);
        if (existingOption)
            throw std::runtime_error("addOption(shortOption=" + std::string(1, shortOption) + "): option already exists\n");
    }

    Option opt;
    opt.shortOption  = shortOption;
    opt.longOption   = longOption;
    opt.help         = help;
    opt.argName      = argName;
    opt.defaultValue = defaultValue;
    opt.value        = defaultValue;

    options[longOption] = opt;
    // Keep --help and --version at the end of the list.
    optionList.insert(optionList.begin() + optionList.size() - std::min(optionList.size(), size_t(2)), longOption);
    return options[longOption];
}


void CommandLineParser::parse(int argc, char* argv[])
{
    // Parse arguments.
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == '-')
            {
                if (argv[i][2] == 0)
                {
                    // "--": End of options.
                    // Keep the "--" as an argument so the application can add further semantics to it.
                    for (; i < argc; i++)
                    {
                        args.push_back(argv[i]);
                    }
                }
                else
                {
                    // Long option.
                    parseLongOption(argc, argv, i);
                }
            }
            else if (argv[i][1] == 0)
            {
                // Positional arg '-'.
                args.push_back(argv[i]);
            }
            else
            {
                // Short option(s).
                parseShortOptions(argc, argv, i);
            }
        }
        else
        {
            // Positional arg.
            args.push_back(argv[i]);
        }
    }

    if (isSet("help"))
    {
        printMessage(getUsageStr(), 0, true);
    }

    if (isSet("version"))
    {
        printMessage("version " + version, 0, true);
    }
}


unsigned CommandLineParser::getCount(const std::string& longOption) const
{
    const Option* option = getOption(longOption);
    if (option)
    {
        return option->count;
    }
    else
    {
        throw std::runtime_error("Unknown option '" + longOption + "'!");
    }
}


const std::string& CommandLineParser::getStr(const std::string& longOption) const
{
    const Option* option = getOption(longOption);
    if (option)
    {
        return option->value;
    }
    else
    {
        throw std::runtime_error("Unknown option '" + longOption + "'!");
    }
}


std::vector<std::string> CommandLineParser::getList(const std::string& longOption)
{
    return ut1::splitString(getStr(longOption), listSepChar);
}


long long CommandLineParser::getInt(const std::string& longOption) const
{
    std::string value = getStr(longOption);
    const char* end   = nullptr;
    long long   r     = strtoll(value.c_str(), const_cast<char**>(&end), 0);
    ut1::skipSpace(end);
    if (end && (end != value.c_str()) && (*end == 0))
    {
        return r;
    }
    return 0;
}


unsigned long long CommandLineParser::getUInt(const std::string& longOption) const
{
    std::string        value = getStr(longOption);
    const char*        end   = nullptr;
    unsigned long long r     = strtoull(value.c_str(), const_cast<char**>(&end), 0);
    ut1::skipSpace(end);
    if (end && (end != value.c_str()) && (*end == 0))
    {
        return r;
    }
    return 0;
}


double CommandLineParser::getDouble(const std::string& longOption) const
{
    std::string value = getStr(longOption);
    const char* end   = nullptr;
    double      r     = strtod(value.c_str(), const_cast<char**>(&end));
    ut1::skipSpace(end);
    if (end && (end != value.c_str()) && (*end == 0))
    {
        return r;
    }
    return 0.0;
}


void CommandLineParser::setValue(const std::string& longOption, const std::string& value, bool clearList)
{
    Option* option = getOption(longOption);
    if (option)
    {
        if (clearList)
        {
            option->value = "";
        }
        option->setValue(value, listSepChar);
    }
    else
    {
        throw std::runtime_error("Unknown option '" + longOption + "'!");
    }
}


int CommandLineParser::error(const std::string& message)
{
    return printMessage("Error: " + message, 1, true);
}


int CommandLineParser::printMessage(const std::string& message, int exitStatus, bool exit)
{
    std::cout << programName << ": " << message << "\n";
    if (exit)
    {
        std::exit(exitStatus);
    }
    return exitStatus;
}


std::string CommandLineParser::getUsageStr()
{
    std::stringstream ret;

    // Get usage header.
    std::vector<std::string> lines = ut1::splitLines(usage, 80);
    ret << ut1::joinStrings(lines, "\n") << "\n"; // ut1::splitLines() swallows the last LF.

    // Get maximum longOption[=argName] string len.
    size_t maxHelpNameLen = 0;
    for (const auto& opt: options)
    {
        maxHelpNameLen = std::max(maxHelpNameLen, opt.second.getHelpNameLen());
    }

    // Get width/wrap column of help text.
    size_t helpStartCol = maxHelpNameLen + 8;
    size_t helpWidth    = std::max(helpStartCol + 80, size_t(79));
    size_t helpWrapCol  = helpWidth - helpStartCol;

    // Print option list.
    for (const auto& name: optionList)
    {
        // Print option header.
        if (ut1::hasPrefix(name, "header:"))
        {
            ret << name.substr(7);
            continue;
        }

        const Option* option = getOption(name);
        ret << "  ";
        if (option->shortOption)
        {
            ret << "-" << option->shortOption;
        }
        else
        {
            ret << "  ";
        }
        std::string nameEqArg = option->longOption;
        if (!option->argName.empty())
        {
            nameEqArg += "=" + option->argName;
        }
        ret << " --" << nameEqArg;
        int pad = std::max(size_t(0), maxHelpNameLen - nameEqArg.length()) + 1;
        ret << std::string(pad, ' ');
        lines = ut1::splitLines(option->help, helpWrapCol);
        ret << ut1::joinStrings(lines, "\n" + std::string(helpStartCol, ' '));
        std::vector<std::string> braceItems;
        if (option->isList)
        {
            braceItems.push_back("list");
        }
        if (!option->defaultValue.empty())
        {
            braceItems.push_back("default=" + option->defaultValue);
        }
        if (option->value != option->defaultValue)
        {
            if (option->argName.empty())
            {
                braceItems.push_back("set");
            }
            else
            {
                braceItems.push_back("value=" + option->value);
            }
        }
        if (option->count > 1)
        {
            braceItems.push_back("count=" + std::to_string(option->count));
        }
        if (!braceItems.empty())
        {
            ret << " (" + ut1::joinStrings(braceItems, ", ") + ")";
        }
        ret << "\n";
    }

    // Get usage footer.
    ret << footer;

    std::string u = ret.str();
    ut1::replaceStringInPlace(u, "$programName", programName);
    ut1::replaceStringInPlace(u, "$version", version);

    return u;
}


CommandLineParser::Option* CommandLineParser::getOption(const std::string& longOption)
{
    auto it = options.find(longOption);
    if (it == options.end())
    {
        return nullptr;
    }
    return &(it->second);
}


const CommandLineParser::Option* CommandLineParser::getOption(const std::string& longOption) const
{
    return const_cast<CommandLineParser*>(this)->getOption(longOption);
}


CommandLineParser::Option* CommandLineParser::getShortOption(char shortOption)
{
    auto it = std::find_if(options.begin(), options.end(), [=](const auto& elem)
        { return elem.second.shortOption == shortOption; });
    if (it == options.end())
    {
        return nullptr;
    }
    return &(it->second);
}


void CommandLineParser::parseLongOption(int argc, char* argv[], int& i)
{
    std::vector<std::string> fields     = ut1::splitString(argv[i] + 2, '=', 1);
    std::string              longOption = fields[0];

    // First try exact match.
    Option* option = getOption(longOption);
    if (option == nullptr)
    {
        // Then try prefix and take it when it is unique.
        std::vector<std::string> matchingOptions;
        for (const auto& optName: optionList)
        {
            if (ut1::hasPrefix(optName, longOption))
            {
                matchingOptions.push_back(optName);
            }
        }
        if (matchingOptions.size() == 0)
        {
            error("Unknown option --" + longOption + ".");
        }
        else if (matchingOptions.size() == 1)
        {
            longOption = matchingOptions[0];
            option     = getOption(longOption);
        }
        else
        {
            error("Ambiguous option --" + longOption + " (matches --" + ut1::joinStrings(matchingOptions, ", --") + ").");
        }
    }
    if (option->hasArg())
    {
        if (fields.size() == 2)
        {
            // --foo=bar
            option->setValue(fields[1], listSepChar);
        }
        else if (++i < argc)
        {
            // --foo bar
            option->setValue(argv[i], listSepChar);
        }
        else
        {
            error("Option --" + longOption + " requires an argument.");
        }
    }
    else
    {
        if (fields.size() == 1)
        {
            // --foo
            option->setValue("", 0);
        }
        else
        {
            error("Option --" + longOption + " does not accept arguments.");
        }
    }
}


void CommandLineParser::parseShortOptions(int argc, char* argv[], int& i)
{
    for (int j = 1; argv[i][j]; j++)
    {
        char    opt    = argv[i][j];
        Option* option = getShortOption(opt);
        if (option)
        {
            if (option->hasArg())
            {
                if (argv[i][j + 1])
                {
                    // -fvalue
                    option->setValue(argv[i] + j + 1, listSepChar);
                    break;
                }
                else
                {
                    // -f value
                    if (++i < argc)
                    {
                        option->setValue(argv[i], listSepChar);
                    }
                    else
                    {
                        error("Option --" + option->longOption + " requires an argument.");
                    }
                }
            }
            else
            {
                // -f
                option->setValue("", 0);
            }
        }
        else
        {
            error("Unknown option -" + std::string(1, opt) + "!");
        }
    }
}


} // namespace ut1
