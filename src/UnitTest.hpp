// Unit test framework.
//
// Copyright (c) 2021 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#ifndef include_UnitTest_hpp
#define include_UnitTest_hpp

#include <cassert>
#include <iostream>
#include <map>
#include <string>

#ifdef ENABLE_UNIT_TEST

class UnitTest;

class UnitTestRegistry
{
public:
   /// Register test in global test registry.
   /// This is implicitly called by UNITS_TEST().
   static void registerTest( UnitTest* test );

   /// Run all tests.
   static int runTests();

private:
   std::map< std::string, UnitTest* > tests;
};

class UnitTest
{
public:
   UnitTest( const std::string& testName_, const std::string& testFile_, int testLine_ )
      : testName( testName_ ), testFile( testFile_ ), testLine( testLine_ )
   {
      UnitTestRegistry::registerTest( this );
   }

   virtual ~UnitTest();

   virtual void run() = 0;
   std::string getTestName() const
   {
      return testFile + ":" + testName;
   }

   std::string testName;
   std::string testFile;
   int testLine{};
};

void UNIT_TEST_RUN();

#define UNIT_TEST( name, code ) class UnitTest_##name : public UnitTest{ public : UnitTest_##name() : UnitTest( #name, __FILE__, __LINE__ ){} virtual void run() override code } UnitTest_instance_##name;
#define ASSERT_EQ( a, b )                                                                         \
   {                                                                                              \
      if( ( a ) != ( b ) ) {                                                                      \
         std::cout << "\nError: ASSERT_EQ(" << toStr( a ) << ", " << toStr( b ) << ") failed!\n"; \
         assert( ( a ) == ( b ) );                                                                \
      }                                                                                           \
   }
#define ASSERT_NE( a, b )                                                                         \
   {                                                                                              \
      if( ( a ) == ( b ) ) {                                                                      \
         std::cout << "\nError: ASSERT_NE(" << toStr( a ) << ", " << toStr( b ) << ") failed!\n"; \
         assert( ( a ) != ( b ) );                                                                \
      }                                                                                           \
   }

#else

#define UNIT_TEST( name, code )
#define UNIT_TEST_RUN() \
   do {                 \
   } while( false )

#endif

#endif /* include_UnitTest_hpp */
