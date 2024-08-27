// Unit test framework.
//
// Copyright (c) 2021 Johannes Overmann
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifdef ENABLE_UNIT_TEST

# include <iostream>
# include "UnitTest.hpp"

UnitTestRegistry* unitTestRegistry{};

UnitTestRegistry* getUnitTestRegistry()
{
    if (unitTestRegistry == nullptr)
    {
        unitTestRegistry = new UnitTestRegistry();
    }
    return unitTestRegistry;
}


void UnitTestRegistry::registerTest(UnitTest* test)
{
    assert(test);
    const auto it = getUnitTestRegistry()->tests.find(test->testName);
    if (it != getUnitTestRegistry()->tests.end())
    {
        std::cout << "Error: registerTest(" << test->testName << "): Test already registered.\n";
        return;
    }

    getUnitTestRegistry()->tests[test->testName] = test;
}


int UnitTestRegistry::runTests()
{
    size_t maxNameLen = 0;
    for (const auto& kv: getUnitTestRegistry()->tests)
    {
        maxNameLen = std::max(maxNameLen, kv.second->getTestName().length());
    }

    size_t numTests = 0;
    for (const auto& kv: getUnitTestRegistry()->tests)
    {
        std::cout << "Test " << kv.second->getTestName() << std::string(maxNameLen - kv.second->getTestName().length() + 1, ' ');
        try
        {
            kv.second->run();
        }
        catch (const std::exception& e)
        {
            std::cout << std::string("Error: Exception: ") + e.what() + "\n";
            assert(!"exception caught");
        }
        std::cout << "OK\n";
        numTests++;
    }

    // Tests always abort() execution on errors so all tests passed when we get here.
    std::cout << "--\nAll " << std::dec << numTests << " tests passed\n";
    return 0;
}


UnitTest::~UnitTest()
{
}


// We do not want to declare this "noreturn" because we want to suppress the
// "unreachable-code" warning in main() without suppressing this rather useful
// warning globally.
# pragma GCC diagnostic ignored "-Wmissing-noreturn"
void UNIT_TEST_RUN()
{
    UnitTestRegistry::runTests();

    // If we ever get here all tests were ok.
    std::exit(0);
}


#endif
