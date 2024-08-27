// Unit test framework.
//
// Copyright (c) 2021 Johannes Overmann
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef include_UnitTest_hpp
#define include_UnitTest_hpp

#include <string>
#include <map>
#include <cassert>
#include <iostream>

#ifdef ENABLE_UNIT_TEST

// Always enable asserts() when running unit tests.
#undef NDEBUG

struct UnitTest;

class UnitTestRegistry
{
public:
    /// Register test in global test registry.
    /// This is implicitly called by UNITS_TEST().
    static void registerTest(UnitTest* test);

    /// Run all tests.
    static int runTests();

private:
    std::map<std::string, UnitTest*> tests;
};

struct UnitTest
{
    UnitTest(const std::string& testName_, const std::string& testFile_, int testLine_)
    : testName(testName_)
    , testFile(testFile_)
    , testLine(testLine_)
    {
        UnitTestRegistry::registerTest(this);
    }

    virtual ~UnitTest();

    virtual void run() = 0;
    std::string  getTestName() const { return testFile + ":" + testName; }

    std::string testName;
    std::string testFile;
    int         testLine{};
};

void UNIT_TEST_RUN();

# define UNIT_TEST(name)                  \
  struct UnitTest_##name: public UnitTest \
  {                                       \
   UnitTest_##name()                      \
   : UnitTest(#name, __FILE__, __LINE__)  \
   {                                      \
   }                                      \
   virtual void run() override;           \
  } UnitTest_instance_##name;             \
  inline void UnitTest_##name::run()

#else

# define UNIT_TEST(name) \
  class UnitTest_##name  \
  {                      \
   void run();           \
  };                     \
  inline void UnitTest_##name::run()
# define UNIT_TEST_RUN() \
  do                     \
  {                      \
  } while (false)

#endif

#define ASSERT_EQ(a, b)                                                                 \
 {                                                                                      \
  if ((a) != (b))                                                                       \
  {                                                                                     \
   std::cout << "\nError: ASSERT_EQ(" << toStr(a) << ", " << toStr(b) << ") failed!\n"; \
   assert((a) == (b));                                                                  \
  }                                                                                     \
 }

#define ASSERT_NE(a, b)                                                                 \
 {                                                                                      \
  if ((a) == (b))                                                                       \
  {                                                                                     \
   std::cout << "\nError: ASSERT_NE(" << toStr(a) << ", " << toStr(b) << ") failed!\n"; \
   assert((a) != (b));                                                                  \
  }                                                                                     \
 }

#endif /* include_UnitTest_hpp */
