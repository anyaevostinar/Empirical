//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2016.
//  Released under the MIT Software license; see doc/LICENSE
//
//  A bunch of C++ Template Meta-programming tricks.

#ifndef EMP_META_H
#define EMP_META_H

#include <tuple>
#include <utility>

namespace emp {

  // Effectively create a function (via constructor) where all args are computed, then ignored.
  struct run_and_ignore { template <typename... T> run_and_ignore(T&&...) {} };

  // Trim off the first type from a pack.
  template <typename T, typename... Ts> using first_type = T;


  namespace internal {
    template <int ID, typename T, typename... Ts>
    struct pack_id_impl { using type = typename pack_id_impl<ID-1,Ts...>::type; };

    template <typename T, typename... Ts>
    struct pack_id_impl<0,T,Ts...> { using type = T; };
  }

  template <int ID, typename ...Ts>
  using pack_id = typename internal::pack_id_impl<ID,Ts...>::type;

  // Trim off the last type from a pack.
  template <typename... Ts> using last_type = pack_id<sizeof...(Ts)-1,Ts...>;

  // Trick to call a function using each entry in a parameter pack.
#define EMP_EXPAND_PPACK(PPACK) ::emp::run_and_ignore{ 0, ((PPACK), void(), 0)... }

  // Check to see if a specified type is part of a set of types.
  template <typename TEST> constexpr bool has_type() { return false; }
  template <typename TEST, typename FIRST, typename... OTHERS>
  constexpr bool has_type() { return std::is_same<TEST, FIRST>() || has_type<TEST,OTHERS...>(); }

  // The following functions take a test type and a list of types and return the index that
  // matches the test type in question.
  template <typename TEST_TYPE>
  constexpr int get_type_index() {
    // @CAO We don't have a type that matches, so ideally trigger a compile time error.
    // Given we need this to be constexpr, we can't easily put even a static assert here
    // until we require C++14.
    // static_assert(false && "trying to find index of non-existant type");
    return -1000000;
  }
  template <typename TEST_TYPE, typename FIRST_TYPE, typename... TYPE_LIST>
  constexpr int get_type_index() {
    return (std::is_same<TEST_TYPE, FIRST_TYPE>()) ? 0 : (get_type_index<TEST_TYPE,TYPE_LIST...>() + 1);
  }


  // These functions can be used to test if a type-set has all unique types or not.

  // Base cases...
  template <typename TYPE1> constexpr bool has_unique_first_type() { return true; }
  template <typename TYPE1> constexpr bool has_unique_types() { return true; }

  template <typename TYPE1, typename TYPE2, typename... TYPE_LIST>
  constexpr bool has_unique_first_type() {
    return (!std::is_same<TYPE1, TYPE2>()) && emp::has_unique_first_type<TYPE1, TYPE_LIST...>();
  }

  template <typename TYPE1, typename TYPE2, typename... TYPE_LIST>
  constexpr bool has_unique_types() {
    return has_unique_first_type<TYPE1, TYPE2, TYPE_LIST...>()  // Check first against all others...
      && has_unique_types<TYPE2, TYPE_LIST...>();               // Recurse through other types.
  }


  // Apply a tuple as arguments to a function!

  // implementation details, users never invoke these directly
  // Based on Kerrek SB in http://stackoverflow.com/questions/10766112/c11-i-can-go-from-multiple-args-to-tuple-but-can-i-go-from-tuple-to-multiple

  namespace internal {
    template <typename FUN_T, typename TUPLE_T, bool is_done, int TOTAL, int... N>
    struct apply_impl {
      static auto apply(FUN_T fun, TUPLE_T && t) {
        return apply_impl<FUN_T, TUPLE_T, TOTAL == 1+sizeof...(N), TOTAL, N..., sizeof...(N)>
                 ::apply(fun, std::forward<TUPLE_T>(t));
      }
    };

    template <typename FUN_T, typename TUPLE_T, int TOTAL, int... N>
    struct apply_impl<FUN_T, TUPLE_T, true, TOTAL, N...> {
      static auto apply(FUN_T fun, TUPLE_T && t) {
        return fun(std::get<N>(std::forward<TUPLE_T>(t))...);
      }
    };
  }

  // user invokes this
  template <typename FUN_T, typename TUPLE_T>
  auto ApplyTuple(FUN_T fun, TUPLE_T && tuple) {
    using tuple_decay_t = typename std::decay<TUPLE_T>::type;
    constexpr auto tup_size = std::tuple_size<tuple_decay_t>::value;
    constexpr bool no_args = (tup_size == 0);
    return internal::apply_impl<FUN_T, TUPLE_T, no_args, tup_size>::apply(fun, std::forward<TUPLE_T>(tuple));
  }


  // Combine multiple keys into a single hash value.
  template <typename T>
  std::size_t CombineHash(const T & x) { return std::hash<T>()(x); }

  template<typename T1, typename T2, typename... EXTRA>
  std::size_t CombineHash(const T1 & x1, const T2 & x2, const EXTRA &... x_extra) {
    const std::size_t hash2 = CombineHash(x2, x_extra...);
    return std::hash<T1>()(x1) + 0x9e3779b9 + (hash2 << 19) + (hash2 >> 13);
  }



  // The following template takes two parameters; the real type you want it to be and a decoy
  // type that should just be evaluated for use in SFINAE.
  // To use: typename sfinae_decoy<X,Y>::type
  // This will always evaluate to X no matter what Y is.
  template <typename REAL_TYPE, typename EVAL_TYPE>
  struct sfinae_decoy { using type = REAL_TYPE; };

  // Most commonly we will use a decoy to determine if a member exists, but be treated as a
  // bool value.

#define emp_bool_decoy(TEST) typename emp::sfinae_decoy<bool, decltype(TEST)>::type
#define emp_int_decoy(TEST) typename emp::sfinae_decoy<int, decltype(TEST)>::type

  // Change the internal type arguments on a template...
  // From: Sam Varshavchik
  // http://stackoverflow.com/questions/36511990/is-it-possible-to-disentangle-a-template-from-its-arguments-in-c
  namespace internal {
    template<typename T, typename ...U> class AdaptTemplateHelper {
    public:
      using type = T;
    };

    template<template <typename...> class T, typename... V, typename... U>
    class AdaptTemplateHelper<T<V...>, U...> {
    public:
      using type = T<U...>;
    };
  }

  template<typename T, typename... U>
  using AdaptTemplate=typename internal::AdaptTemplateHelper<T, U...>::type;


  // Variation of AdaptTemplate that only adapts first template argument.
  namespace internal {
    template<typename T, typename U> class AdaptTemplateHelper_Arg1 {
    public:
      using type = T;
    };

    template<template <typename...> class T, typename X, typename ...V, typename U>
    class AdaptTemplateHelper_Arg1<T<X, V...>, U> {
    public:
      using type = T<U, V...>;
    };
  }

  template<typename T, typename U>
  using AdaptTemplate_Arg1=typename internal::AdaptTemplateHelper_Arg1<T, U>::type;

}


#endif
