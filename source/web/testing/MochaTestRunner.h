
/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2020
 *
 *  @file  MochaTestRunner.h
 *  @brief Utility class for managing software testing for Emscripten web code using the Karma + Mocha
 *         javascript testing framework.
 *
 */

#ifndef WEB_TESTING_MOCHA_TEST_RUNNER_H
#define WEB_TESTING_MOCHA_TEST_RUNNER_H

#include <functional>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <deque>
#include "control/Signal.h"
#include "base/vector.h"
#include "web/JSWrap.h"
#include "../../../tests2/unit_tests.h"

namespace emp {
namespace web {

  /// Base test class that all web tests managed by MochaTestRunner should inherit from.
  /// Order of operations: Construction, Setup, Describe, Destruction
  struct BaseTest {

    BaseTest() { ; }

    // Remember to clean up after your test!
    virtual ~BaseTest() { }

    /// Setup is run immediately after construction and before Describe.
    /// Setup should run any configuration/setup (e.g., dom manipulation, object creation/configuration)
    /// necessary for test.
    virtual void Setup() { ; }

    /// Describe is run after Setup.
    /// Describe should contain the Mocha testing statements (e.g., 'describes', 'its', etc)
    /// [https://mochajs.org/#getting-started](https://mochajs.org/#getting-started)
    virtual void Describe() { ; }

    /// This is a utility function that can be used to trigger test failure from C++. (it is not automatically
    /// run for you).
    /// @param result this test should fail if result is false.
    /// @param msg print this message on test failure.
    void Require(bool result, const std::string & msg="") {
      if (result) return;
      if (msg == "") {
        EM_ASM({ chai.assert.fail(); });
      } else {
        EM_ASM({ chai.assert.fail(UTF8ToString($0)); }, msg.c_str());
      }
    }

  };

  /// Utility class for managing software tests written for Emscripten web code.
  /// IMPORTANT: This utility assumes the Karma + Mocha javascript testing framework.
  // MochaTestRunner is useful because emscripten does not play very nice with the browser event queue (i.e.,
  // it does not relinquish control back to the browser until it finishes executing the compiled 'C++'
  // code). This interacts poorly with Mocha because Mocha's 'describe' statements do not execute when
  // they are called; instead, they are added to the browser's event queue.
  // The MochaTestRunner exploits Mocha's describe statements + the browser's event queue to chain together
  // the tests added to the MochaTestRunner.
  class MochaTestRunner {
  protected:

    /// TestRunner encapsulates everything needed to create, run, and cleanup a test.
    struct TestRunner {
      emp::Ptr<BaseTest> test{nullptr};
      std::function<void()> create;
      std::function<void()> describe;
      std::function<void()> cleanup;
      std::string test_name{};
      bool done=false;
      size_t before_test_error_count=0;
    };

    emp::Signal<void()> before_each_test_sig;   ///< Is triggered before each test.
    emp::Signal<void()> after_each_test_sig;    ///< Is triggered after each test (after test marked 'done', but before test is deleted).
    std::deque<TestRunner> test_runners;        ///< Store test runners in a first-in-first-out (out=run) queue

    const size_t next_test_js_func_id;
    const size_t pop_test_js_func_id;
    const size_t cleanup_all_js_func_id;

    /// Run the next test!
    void NextTest() {
      emp_assert(test_runners.size(), "No tests to run!");
      before_each_test_sig.Trigger();
      test_runners.front().create();    // Create test object
      test_runners.front().describe();  // This will queue up describe clause for this test and either (1) queue up the next test or (2) queue up manager cleanup
    }

    /// Cleanup and pop the front of the test queue.
    void PopTest() {
      emp_assert(test_runners.size());
      test_runners.front().cleanup();
      test_runners.pop_front();
    }

    /// Cleanup all test runners.
    void CleanupTestRunners() {
      // finished running all tests. Make sure all dynamically allocated memory is cleaned up.
      std::for_each(
        test_runners.begin(),
        test_runners.end(),
        [](TestRunner & runner) {
          emp_assert(runner.done);
          if (runner.test != nullptr) runner.test.Delete();
        }
      );
      // Clear test runners.
      test_runners.clear();
    }

  public:

    MochaTestRunner()
      : next_test_js_func_id(emp::JSWrap([this]() { this->NextTest(); }, "NextTest")),
        pop_test_js_func_id(emp::JSWrap([this]() { this->PopTest(); }, "PopTest")),
        cleanup_all_js_func_id(emp::JSWrap([this]() { this->CleanupTestRunners(); }, "CleanupTestRunners"))
    { }

    ~MochaTestRunner() {
      CleanupTestRunners();
      emp::JSDelete(next_test_js_func_id);
      emp::JSDelete(pop_test_js_func_id);
      emp::JSDelete(cleanup_all_js_func_id);
    }

    /// Add a test type to be run. The MochaTestRunner creates, runs, and cleans up each test.
    /// This function should be called with the test type (which should inherit from BaseTest) as a
    /// template argument (e.g., AddTest<TEST_TYPE>(...) ).
    /// Tests are eventually run in the order they were added (first-in-first-out).
    /// @param test_name specifies the name of the test (this is only used when printing which test is running
    ///   and does not need to be unique across tests).
    /// @param constructor_args All subsequent arguments (after test_name) are forwarded to the TEST_TYPE constructor.
    // For variatic capture: https://stackoverflow.com/questions/47496358/c-lambdas-how-to-capture-variadic-parameter-pack-from-the-upper-scope
    //  - NOTE: this can get cleaned up quite a bit w/C++ 20!
    template<typename TEST_TYPE, typename... Args>
    void AddTest(const std::string & test_name, Args&&... constructor_args) {

      // create a new test runner for this test
      test_runners.emplace_back();
      auto & runner = test_runners.back();

      // name it!
      runner.test_name = test_name;

      // configure the function that, when called, will create a new instance of TEST_TYPE (using forwarded
      // arguments as constructor arguments) and then call TEST_TYPE::Setup.
      runner.create = [constructor_args = std::make_tuple(std::forward<Args>(constructor_args)...), this]() mutable {
        std::apply([this](auto&&... constructor_args) {
          auto & cur_runner = this->test_runners.front(); // Create the test in the front of the queue.
          // Record the before test error count
          cur_runner.before_test_error_count = emp::GetUnitTestOutput().errors;
          // Allocate memory for test
          cur_runner.test = emp::NewPtr<TEST_TYPE>(std::forward<Args>(constructor_args)...);
          cur_runner.done = false;
          // Run test setup
          cur_runner.test->Setup();
        }, std::move(constructor_args));
      };

      // configure the function that, when called, will call TEST_TYPE::Describe, which should queue
      // up a Mocha describe function (containing js tests). Next, this function will either (1) queue
      // up the next test to be run or (2) queue up general MochaTestRunner cleanup if there are no more
      // tests to run.
      // This function takes advantage of Mocha describe functions to use 'browser' events to chain
      // together tests (which is necessary because js 'describe' calls add themselves to the browser's
      // event queue instead of executing as soon as you call them).
      runner.describe = [this]() {
        auto & cur_runner = this->test_runners.front();
        cur_runner.test->Describe(); // this will queue up the next test's describe

        auto & test_name = cur_runner.test_name;

        EM_ASM({
          const test_name = UTF8ToString($0);

          // Queue up cleanup for this test
          describe("Cleanup " + test_name, function() {
            it('should clean up test ', function() {
              emp.PopTest();
            });
          });
        }, test_name.c_str());

        // If there are still more tests to do (i.e., this is not the last test), queue them
        // otherwise, queue up a cleanup
        if (test_runners.size() > 1) {
          auto & next_test_name = this->test_runners[1].test_name;
          EM_ASM({
            const next_test_name = UTF8ToString($0);
            // Queue up next test
            describe("Queue " + next_test_name , function() {
              it("should queue the next test", function() {
                emp.NextTest();
              });
            });
          }, next_test_name.c_str());
        } else {
          EM_ASM({
            describe("Finished running tests.", function() {
              it("should cleanup test manager", function() {
                emp.CleanupTestRunners();
              });
            });
          });
        }
      };

      // configure the function that, when called, triggers the 'after each test' signal, and cleans
      // up the dynamically allocated TEST_TYPE object.
      runner.cleanup = [this]() {
        auto & cur_runner = this->test_runners.front();
        // Mark test as done.
        cur_runner.done = true;

        // Did this test trigger any C++ test failures?
        const size_t post_test_error_cnt = emp::GetUnitTestOutput().errors;
        auto & test_name = cur_runner.test_name;

        // Did the error count increase after running this test? If so, force failure.
        if (post_test_error_cnt != cur_runner.before_test_error_count) {
          EM_ASM({
            const test_name = UTF8ToString($0);
            describe(test_name + " - Failed C++ unit test", function() {
              it("failed at least one C++ unit test", function() {
                chai.assert(false);
              });
            });
          }, test_name.c_str());
        }

        this->after_each_test_sig.Trigger();
        cur_runner.test.Delete();
        cur_runner.test = nullptr;
      };
    }

    /// Run all tests that have been added to the MochaTestRunner thus far.
    /// Running a test consumes it (i.e., executing Run a second time will not re-run previously run
    /// tests).
    /// Tests are run in the order they were added.
    void Run() { if (test_runners.size()) NextTest(); }

    /// Provide a function for MochaTestRunner to call before each test is created and run.
    /// @param fun a function to be run before each test is created and run
    void OnBeforeEachTest(const std::function<void()> & fun) { before_each_test_sig.AddAction(fun); };

    /// Provide a function for MochaTestRunner to call before after each test runs (but before it is deleted).
    /// @param fun a function to be run after each test is run (but before it is deleted)
    void OnAfterEachTest(const std::function<void()> & fun) { after_each_test_sig.AddAction(fun); };

  };


}
}

#endif