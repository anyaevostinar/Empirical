//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2016.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  Some examples code for using emp::Signal

#include <iostream>
#include "../../tools/signal.h"

// Build a set of potential actions.
void PrintInt(int i) { std::cout << "[" << i << "]" << std::endl; }
void PrintVoid() { std::cout << "***" << std::endl; }
void MultInt(int i, int j) { std::cout << "[" << i*j << "]" << std::endl; }
void Sum4(int w, int x, int y, int z, int & result) { result=w+x+y+z; PrintInt(result); }

int main()
{
  // A simple signal that sends an int.
  emp::Signal<int> test_sig("test");
  test_sig.AddAction(PrintInt);
  test_sig.AddAction(PrintVoid);
  test_sig.AddAction([](int x){ std::cout << "---:" << x << "\n\n"; });

  // Trigger actions!
  std::cout << "Phase 1: For each tigger, print [value], print ***, and print ---:value\n";
  test_sig.Trigger(12);
  test_sig.Trigger(-1);

  // Signals dont need to take arguments or have names.
  std::cout << "Phase 2: Create and trigger a signal with no args; added action to print ***\n";
  emp::Signal<> test_sig2;
  test_sig2.AddAction(PrintVoid);
  test_sig2.Trigger();


  // Actions can be turned into named objects as well.
  std::cout << "Phase 3: Create action objects and link them to signals by name.\n"
            << "...setup three actions (print val three times, and track sum of vals so far)\n";
  int total=0;
  emp::Action<int> act1([&total](int inc){total+=inc;}, "sum");
  emp::Action<int> act2(PrintInt, "iprint");
  emp::Action<int,int> act_mint(MultInt, "mint");

  emp::Signal<int> test_sig3("test3");
  test_sig3.AddAction(act1);

  emp::LinkSignal("test3", "iprint");
  emp::LinkSignal("test3", "iprint");
  test_sig3.AddAction(act2);

  test_sig3.Trigger(10);

  std::cout << "Phase 4: Trigger again from BASE SIGNAL CLASS with 20!\n";
  emp::internal::Signal_Base * base_sig = &test_sig3;
  //test_sig3.Trigger(20);
  base_sig->BaseTrigger(20);

  // Trigger by signal name!
  std::cout << "Phase 5: Trigger original signal with some of values calculated so far!\n";
  emp::TriggerSignal("test", total);

  // Build a signal setup to provide many arguments.
  std::cout << "Phase 6: Sum 1,2,3,4!\n";
  emp::Signal<int,int,int,int,int&> sum4_sig;
  sum4_sig.AddAction(Sum4);
  int result=0;
  sum4_sig.Trigger(1,2,3,4,result);
  std::cout << "result variable is now set to " << result << std::endl;

  std::cout << "Phase 7: Add mult 2 to prev signal... using only two args!  Call with 2,3,4,5\n";
  std::function<void(int,int)> mult_pair = [](int x, int y){std::cout << x << "*" << y << "=" << x*y << std::endl;};
  sum4_sig.AddAction(mult_pair);
  sum4_sig.Trigger(2,3,4,5,result);
  std::cout << "result variable is now set to " << result << std::endl;
}
