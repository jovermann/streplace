// Unit test framework.
//
// Copyright (c) 2021 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#ifdef ENABLE_UNIT_TEST

#include "UnitTest.hpp"
#include <iostream>

UnitTestRegistry* unitTestRegistry{};

UnitTestRegistry* getUnitTestRegistry()
{
   if( unitTestRegistry == nullptr ) {
      unitTestRegistry = new UnitTestRegistry();
   }
   return unitTestRegistry;
}

void UnitTestRegistry::registerTest( UnitTest* test )
{
   assert( test );
   const auto it = getUnitTestRegistry()->tests.find( test->testName );
   if( it != getUnitTestRegistry()->tests.end() ) {
      std::cout << "Error: registerTest(" << test->testName << "): Test already registered.\n";
      return;
   }

   getUnitTestRegistry()->tests[ test->testName ] = test;
}

int UnitTestRegistry::runTests()
{
   size_t maxNameLen = 0;
   for( const auto& kv : getUnitTestRegistry()->tests ) {
      maxNameLen = std::max( maxNameLen, kv.second->getTestName().length() );
   }

   for( const auto& kv : getUnitTestRegistry()->tests ) {
      std::cout << "Test " << kv.second->getTestName() << std::string( maxNameLen - kv.second->getTestName().length() + 1, ' ' );
      kv.second->run();
      std::cout << "OK\n";
   }
   return 0;
}

UnitTest::~UnitTest()
{
}

// We do not want to declare this "noreturn" because we want to suppress the
// "unreachable-code" warning in main() without suppressing this rather useful
// warning globally.
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
void UNIT_TEST_RUN()
{
   UnitTestRegistry::runTests();

   // If we ever get here all tests were ok.
   std::exit( 0 );
}

#endif
