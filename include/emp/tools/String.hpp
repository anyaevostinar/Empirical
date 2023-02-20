/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file String.hpp
 *  @brief Simple class to facilitate string manipulations
 *  @note Status: ALPHA
 *
 *
 *  @todo Make constexpr
 *  @todo Make handle non-char strings (i.e., use CharT template parameter)
 *  @todo Make handle allocators
 *  @todo Make work with stringviews
 *  @todo Add construct types like RESERVE, REPEAT, and TO_STRING for special builds.
 *
 */

#ifndef EMP_TOOLS_STRING_HPP_INCLUDE
#define EMP_TOOLS_STRING_HPP_INCLUDE

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

#include "../base/array.hpp"
#include "../base/assert.hpp"
#include "../base/notify.hpp"
#include "../base/Ptr.hpp"
#include "../base/vector.hpp"
#include "../meta/reflection.hpp"
#include "../meta/type_traits.hpp"

#include "char_utils.hpp"

namespace emp {

  class String;

  // Some stand-alone functions.
  template <typename... Ts>
  [[nodiscard]] emp::String MakeString(Ts... args);
  [[nodiscard]] emp::String MakeEscaped(char c);
  [[nodiscard]] emp::String MakeEscaped(const emp::String & in);
  [[nodiscard]] emp::String MakeWebSafe(const emp::String & in);
  [[nodiscard]] emp::String MakeLiteral(char value);
  [[nodiscard]] emp::String MakeLiteral(char value);
  template <typename T>
  [[nodiscard]] emp::String MakeLiteral(const T & value);
  [[nodiscard]] emp::String MakeUpper(const emp::String & in);
  [[nodiscard]] emp::String MakeLower(const emp::String & in);
  [[nodiscard]] emp::String MakeTitleCase(const emp::String & in);
  [[nodiscard]] emp::String MakeRoman(int val);
  template <typename CONTAINER_T>
  [[nodiscard]] emp::String MakeEnglishList(const CONTAINER_T & container);
  template <typename... Args>
  [[nodiscard]] emp::String MakeFormatted(const emp::String& format, Args... args);

  template <typename CONTAINER_T>
  [[nodiscard]] emp::String Join(const CONTAINER_T & container, emp::String join_str="",
                                 emp::String open="", emp::String close="") {


  class String {
  private:
    std::string str;   // The main string that we are manipulating.

    enum Mask {
      USE_QUOTE_SINGLE =  1,
      USE_QUOTE_DOUBLE =  2,
      USE_QUOTE_BACK   =  4,
      USE_PAREN_ROUND  =  8,    // Parentheses
      USE_PAREN_SQUARE = 0x10,  // Brackets
      USE_PAREN_CURLY  = 0x20,  // Braces
      USE_PAREN_ANGLE  = 0x40,  // Chevrons
      USE_PAREN_QUOTES = 0x80   // Forward/back single quote
    };

    struct Mode {
      uint8_t val = USE_QUOTE_SINGLE + USE_QUOTE_DOUBLE +
                    USE_PAREN_ROUND + USE_PAREN_SQUARE + USE_PAREN_CURLY;
    } mode;

    // ------ HELPER FUNCTIONS ------

    String & _ChangeMode(Mask mask, bool use) {
      if (use) mode.val |= mask;
      else mode.val &= ~mask;
      return *this;      
    }

    // Tools for converting non-strings to a string.
    template <typename T>
    decltype(std::declval<T>().ToString()) _Convert(const T & in, bool) { return in.ToString(); }

    template <typename T>
    auto _Convert(const T & in, int) -> decltype(emp::ToString(in)) { return emp::ToString(in); }

    template <typename T> const T & _Convert(const T & in, ...) { return in; }


    bool IsQuote(char c) const {
      switch (c) {
        case '\'': return mode.val & USE_QUOTE_SINGLE;
        case '"': return mode.val & USE_QUOTE_DOUBLE;
        case '`': return mode.val & USE_QUOTE_BACK;
      }
      return false;
    }

    bool IsParen(char c) const {
      switch (c) {
        case '(': return mode.val & USE_PAREN_ROUND;
        case '[': return mode.val & USE_PAREN_SQUARE;
        case '{': return mode.val & USE_PAREN_CURLY;
        case '<': return mode.val & USE_PAREN_ANGLE;
        case '`': return mode.val & USE_PAREN_QUOTES;
      }
      return false;
    }

    static char GetMatch(char c) {
      switch (c) {
        case '`': return '\'';
        case '(': return ')';
        case '[': return ']';
        case '{': return '}';
        case '<': return '>';
      }
      return '\0';
    }

    void _AssertPos(size_t pos) const { emp_assert(pos < str.size(), pos, str.size()); }

  public:
    using value_type = std::string::value_type;
    using allocator_type = std::string::allocator_type;
    using size_type = std::string::size_type;
    using difference_type = std::string::difference_type;
    using reference = std::string::reference;
    using const_reference = std::string::const_reference;
    using pointer = std::string::pointer;
    using const_pointer = std::string::const_pointer;
    using iterator = std::string::iterator;
    using const_iterator = std::string::const_iterator;
    using reverse_iterator = std::string::reverse_iterator;
    using const_reverse_iterator = std::string::const_reverse_iterator;

    static constexpr size_t npos = std::string::npos;

    // Constructors duplicating from std::string
    String() = default;
    String(const std::string & _str) : str(_str) { }
    String(std::string && _str) : str(std::move(_str)) { }
    String(const std::string & _str, Mode _mode) : str(_str), mode(_mode) { }
    String(std::string && _str, Mode _mode) : str(std::move(_str)), mode(_mode) { }
    String(const char * _str) : str(_str) { }
    String(size_t count, char _str) : str(count, _str) { }
    String(std::initializer_list<char> _str) : str(_str) { }
    String(const String & _str, size_t start, size_t count=npos)
      : str(_str.str, start, count), mode(_str.mode) { }
    String(const std::string & _str, size_t start, size_t count=npos)
      : str(_str, start, count) { }
    String(const char * _str, size_t count) : str(_str, count) { }
    template< class InputIt >
    String(InputIt first, InputIt last) : str(first, last) { }
    String(std::nullptr_t) = delete;

    // ------ New constructors ------

    String(const String &) = default;
    String(String &&) = default;

    // Allow a string to be transformed during construction, 1-to-1
    String(const std::string & _str, std::function<char(char)> transform_fun) {
      str.reserve(_str.size());  // Setup expected size.
      for (auto & c : _str) { str.push_back(transform_fun(c)); }
    }

    // Allow a string to be transformed during construction, 1-to-any
    String(const std::string & _str, std::function<std::String(char)> transform_fun) {
      str.reserve(_str.size());  // Setup expected size; assume size will be 1-to-1 by default.
      for (auto & c : _str) { str += transform_fun(c); }
    }


    // ------ Assignment operators ------

    String & operator=(const String &) = default;
    String & operator=(String &&) = default;
    String & operator=(const std::string & _in) { str = _in; }
    String & operator=(std::string && _in) { str = std::move(_in); }
    String & operator=(const char * _in) { str = _in; }
    String & operator=(char _in) { str = _in; }
    String & operator=(std::initializer_list<char> _in) { str = _in; }
    String & operator=( std::nullptr_t ) = delete;


    // ------ Element Access ------

    char & operator[](size_t pos) { _AssertPos(pos); return str[pos]; }
    char operator[](size_t pos) const { _AssertPos(pos); return str[pos]; }
    char & front() { _AssertPos(0); return str.front(); }
    char front() const { _AssertPos(0); return str.front(); }
    char & back() { _AssertPos(0); return str.back(); }
    char back() const { _AssertPos(0); return str.back(); }
    char * data() { return str.data(); }
    const char * data() const { return str.data(); }
    const char * c_str() const { return str.c_str(); }
    const std::string & cpp_str() const { return str; }

    [[nodiscard]] String substr(size_t pos=0, size_t count=npos ) const
      { return String(str.substr(pos, count), mode); }
    [[nodiscard]] String GetRange(std::size_t start_pos, std::size_t end_pos) const
      { return substr(start_pos, end_pos - start_pos); }

    [[nodiscard]] std::string_view View(size_t start=0, size_t out_size=npos) const {
      emp_assert(start + npos <= str.size());
      return std::string_view(str.data()+start, out_size);
    }
    [[nodiscard]] std::string_view ViewFront(size_t out_size) const { return View(0, out_size); }
    [[nodiscard]] std::string_view ViewBack(size_t out_size) const
      { return View(str.data()+(str.size()-out_size), out_size); }
    [[nodiscard]] std::string_view ViewRange(size_t start, size_t end) const {
      emp_assert(start <= end && end <= str.size());
      return View(str.data()+start, end - start);
    }


    // ------ Iterators ------

    iterator begin() { return str.begin(); }
    const_iterator begin() const { return str.begin(); }
    const_iterator cbegin() const { return str.cbegin(); }
    reverse_iterator rbegin() { return str.rbegin(); }
    const_reverse_iterator rbegin() const { return str.rbegin(); }
    const_reverse_iterator crbegin() const { return str.crbegin(); }

    iterator end() { return str.end(); }
    const_iterator end() const { return str.end(); }
    const_iterator cend() const { return str.cend(); }
    reverse_iterator rend() { return str.rend(); }
    const_reverse_iterator rend() const { return str.rend(); }
    const_reverse_iterator crend() const { return str.crend(); }


    // ------ Capacity ------

    bool empty() const { return str.empty(); }
    size_t size() const { return str.size(); }
    size_t length() const { return str.length(); }
    size_t max_size() const { return str.max_size(); }
    void reserve(size_t new_cap) { str.reserve(new_cap); }
    void reserve() { str.reserve(); }
    size_t capacity() const { return str.capacity(); }
    void shrink_to_fit() { str.shrink_to_fit(); }
    

    // ------ Classification and Comparisons ------
    
    int compare(const String & in) { return str.compare(in.str); }
    template <typename... ARG_Ts> int compare(ARG_Ts &&... args)
      { return str.compare(std::forward<ARG_Ts>(args)...); }

    bool starts_with(const String & in) const noexcept { return str.starts_with(in.str); }
    template <typename ARG_T> bool starts_with( ARG_T && in ) const noexcept
      { return str.starts_with(std::forward<ARG_T>(arg)); }
    bool HasPrefix(const std::String & prefix) const { return str.rfind(prefix.str, 0) == 0; }

    bool ends_with(const String & in) const noexcept { return str.ends_with(in.str); }
    template <typename ARG_T> bool ends_with( ARG_T && in ) const noexcept
      { return str.ends_with(std::forward<ARG_T>(arg)); }

    bool contains(const String & in) const noexcept { return str.find(in.str) != npos; }
    template <typename ARG_T> bool contains( ARG_T && in ) const noexcept
      { return str.find(std::forward<ARG_T>(arg)) != npos; }


    // ------ Simple Analysis ------

    // Count the number of occurrences of a specific character.
    size_t Count(char c, size_t start=0) const
      { return (size_t) std::count(str.begin()+start, str.end(), c); }

    // Count the number of occurrences of a specific character within a range.
    size_t Count(char c, size_t start, size_t end) const
      { return (size_t) std::count(str.begin()+start, str.begin()+end, c); }

    /// Test if an string is formatted as a literal character.
    bool IsLiteralChar() const;

    /// Test if an string is formatted as a literal string.
    bool IsLiteralString(const std::string & quote_marks="\"") const;

    /// Explain what string is NOT formatted as a literal string.
    std::string DiagnoseLiteralString(const std::string & quote_marks="\"") const;

    /// Determine a string is composed only of a set of characters (represented as a string)
    bool IsComposedOf(const std::string & char_set) const {
      for (char x : str) if (!is_one_of(x, char_set)) return false;
      return true;
    }

    /// Determine if string is a valid number.
    bool IsNumber() const;

    /// Determine if string is a valid identifier (in most languages).
    bool IsIdentifier() const {
      // At least one character; cannot begin with digit, only letters, digits and `_`
      return str.size() && !is_digit(str[0]) && IDCharSet().Has(str);
    }

    bool OnlyLower() const { return (str.size()) ? LowerCharSet().Has(str) : true; }
    bool OnlyUpper() const { return (str.size()) ? UpperCharSet().Has(str) : true; }
    bool OnlyDigits() const { return (str.size()) ? DigitCharSet().Has(str) : true; }
    bool OnlyAlphanumeric() const { return (str.size()) ? AlphanumericCharSet().Has(str) : true; }
    bool OnlyWhitespace() const { return (str.size()) ? WhitespaceCharSet().Has(str) : true; }

    bool HasOneOf(const std::string & char_set) const {
      for (char c : str) if (is_one_of(c, char_set)) return true;
      return false;
    }
    bool HasWhitespace() const { return WhitespaceCharSet().HasAny(str); }
    bool HasNonwhitespace() const { return !WhitespaceCharSet().HasOnly(str); }
    bool HasUpperLetter() const { return UpperCharSet().HasAny(str); }
    bool HasLowerLetter() const { return LowerCharSet().HasAny(str); }
    bool HasLetter() const { return LetterCharSet().HasAny(str); }
    bool HasDigit() const { return DigitCharSet().HasAny(str); }
    bool HasAlphanumeric() const { return AlphanumericCharSet().HasAny(str); }

    bool HasCharAt(char c, size_t pos) const { return (pos < str.size()) && (str[pos] == c); }
    bool HasOneOfAt(const std::string & opts, size_t pos) const {
      return (pos < str.size()) && is_one_of(str[pos], opts);
    }
    bool HasDigitAt(size_t pos) const { return DigitCharSet().HasAt(str, pos); }
    bool HasLetterAt(size_t pos) const { return LetterCharSet().HasAt(str, pos); }


    // ------ Removals and Extractions ------

    void clear() noexcept { str.clear(); }

    String & erase(size_t index=0, size_t count=npos) { str.erase(index,count); return *this; }
    iterator erase(const_iterator pos) { return str.erase(pos); }
    iterator erase(const_iterator first, const_iterator last) { return str.erase(first, last); }

    void pop_back() { str.pop_back(); }

    emp::String & RemoveChars(const CharSet & chars);
    emp::String & RemoveWhitespace()  { return RemoveChars(WhitespaceCharSet()); }
    emp::String & RemoveUpper()       { return RemoveChars(UpperCharSet()); }
    emp::String & RemoveLower()       { return RemoveChars(LowerCharSet()); }
    emp::String & RemoveLetters()     { return RemoveChars(LettersCharSet()); }
    emp::String & RemoveDigits()      { return RemoveChars(DigitCharSet()); }
    emp::String & RemovePunctuation() { return RemoveChars(PunctuationCharSet()); }


    // ------ Insertions and Additions ------

    String & insert(size_t index, const String & in) { str.insert(index, in.str); return *this; }
    String & insert(size_t index, const String & in, size_t pos, size_t count=npos)
      { str.insert(index, in.str, pos, count); return *this; }
    template <typename... ARG_Ts> String & insert(size_t index, ARG_Ts &&... args) {
      str.insert(index, std::forward<ARG_Ts>(args)...);
      return *this;
    }
    template <typename... ARG_Ts> String & insert(const_iterator pos, ARG_Ts &&... args) {
      return str.insert(pos, std::forward<ARG_Ts>(args)...);
    }

    void push_back(char c) { str.push_back(c); }

    String & append(const String & in) { str.append(in.str); return *this; }
    String & append(const String & in, size_t pos, size_t count)
      { str.append(in.str, pos, count); return *this; }
    template <typename... ARG_Ts> String & append(ARG_Ts &&... args) {
      str.append(std::forward<ARG_Ts>(args)...);
      return *this;
    }

    String & operator+=(const String & in) { str += in.str; return *this; }
    template <typename ARG_T> String & operator+=(ARG_T && arg) {
      str += std::forward<ARG_T>(arg);
      return *this;
    }

    String & PadFront(char padding, size_t target_size) {
      if (str.size() < target_size) str = emp::String(target_size - size(), padding) + str;
      return *this;
    }

    String & PadBack(char padding, size_t target_size) {
      if (str.size() < target_size) str = str + emp::String(target_size - size(), padding);
      return *this;
    }

    // ------ Direct Modifications ------

    template <typename... ARG_Ts> String & replace(ARG_Ts &&... args)
      { str.replace(std::forward<ARG_Ts>(args)...); return *this; }

    size_t copy(char * dest, size_t count, size_t pos=0) const { return str.copy(dest, count, pos); }

    void resize( size_t count, char c='\0') { str.resize(count, c); }

    void swap(String & other) { str.swap(other.str); std::swap(mode, other.mode); }

    emp::String & ReplaceChar(char from, char to, size_t start=0)
      { for (size_t i=start; i < str.size(); ++i) if (str[i] == from) str[i] = to; return *this; }
    emp::String & ReplaceRange(size_t start, size_t end, std::string value)
      { return replace(start, end-start, value); }
    emp::String & TrimWhitespace();
    emp::String & CompressWhitespace();
    emp::String & Slugify();

    // Find any instances of ${X} and replace with dictionary lookup of X.
    template <typename MAP_T>
    [[nodiscard]] emp::String & ReplaceVars(const MAP_T & var_map);

    // Find any instance of MACRO_NAME(ARGS) and call replace it with return from fun(ARGS).
    template <typename FUN_T>
    [[nodiscard]] String & ReplaceMacro(std::string start_str, std::string end_str, FUN_T && fun, bool skip_quotes=true);


    // ------ Searching ------

    [[nodiscard]] size_t find(const std::string & str, size_t pos=0) const noexcept { return find(str,pos); }
    [[nodiscard]] size_t find(const char* s, size_t pos=0) const { return str.find(s,pos); }
    [[nodiscard]] size_t find(const char* s, size_t pos, size_t count) const { return str.find(s,pos,count); }
    [[nodiscard]] size_t find(char c, size_t pos=0) const noexcept { return str.find(c,pos); }
    template < class SVIEW_T > [[nodiscard]] size_t find(const SVIEW_T & sv, size_t pos=0) const
      { return str.find(sv,pos); }

    [[nodiscard]] size_t rfind(const std::string & str, size_t pos=0) const noexcept { return rfind(str,pos); }
    [[nodiscard]] size_t rfind(const char* s, size_t pos=0) const { return str.rfind(s,pos); }
    [[nodiscard]] size_t rfind(const char* s, size_t pos, size_t count) const { return str.rfind(s,pos,count); }
    [[nodiscard]] size_t rfind(char c, size_t pos=0) const noexcept { return str.rfind(c,pos); }
    template < class SVIEW_T > [[nodiscard]] size_t rfind(const SVIEW_T & sv, size_t pos=0) const
      { return str.rfind(sv,pos); }

    [[nodiscard]] size_t find_first_of(const std::string & str, size_t pos=0) const noexcept
      { return find_first_of(str,pos); }
    [[nodiscard]] size_t find_first_of(const char* s, size_t pos=0) const { return str.find_first_of(s,pos); }
    [[nodiscard]] size_t find_first_of(const char* s, size_t pos, size_t count) const
      { return str.find_first_of(s,pos,count); }
    [[nodiscard]] size_t find_first_of(char c, size_t pos=0) const noexcept { return str.find_first_of(c,pos); }
    template < class SVIEW_T > [[nodiscard]] size_t find_first_of(const SVIEW_T & sv, size_t pos=0) const
      { return str.find_first_of(sv,pos); }

    [[nodiscard]] size_t find_first_not_of(const std::string & str, size_t pos=0) const noexcept
      { return find_first_not_of(str,pos); }
    [[nodiscard]] size_t find_first_not_of(const char* s, size_t pos=0) const
      { return str.find_first_not_of(s,pos); }
    [[nodiscard]] size_t find_first_not_of(const char* s, size_t pos, size_t count) const
      { return str.find_first_not_of(s,pos,count); }
    [[nodiscard]] size_t find_first_not_of(char c, size_t pos=0) const noexcept
      { return str.find_first_not_of(c,pos); }
    template < class SVIEW_T > [[nodiscard]] size_t find_first_not_of(const SVIEW_T & sv, size_t pos=0) const
      { return str.find_first_not_of(sv,pos); }

    [[nodiscard]] size_t find_last_of(const std::string & str, size_t pos=0) const noexcept
      { return find_last_of(str,pos); }
    [[nodiscard]] size_t find_last_of(const char* s, size_t pos=0) const { return str.find_last_of(s,pos); }
    [[nodiscard]] size_t find_last_of(const char* s, size_t pos, size_t count) const
      { return str.find_last_of(s,pos,count); }
    [[nodiscard]] size_t find_last_of(char c, size_t pos=0) const noexcept { return str.find_last_of(c,pos); }
    template < class SVIEW_T > [[nodiscard]] size_t find_last_of(const SVIEW_T & sv, size_t pos=0) const
      { return str.find_last_of(sv,pos); }

    [[nodiscard]] size_t find_last_not_of(const std::string & str, size_t pos=0) const noexcept
      { return find_last_not_of(str,pos); }
    [[nodiscard]] size_t find_last_not_of(const char* s, size_t pos=0) const
      { return str.find_last_not_of(s,pos); }
    [[nodiscard]] size_t find_last_not_of(const char* s, size_t pos, size_t count) const
      { return str.find_last_not_of(s,pos,count); }
    [[nodiscard]] size_t find_last_not_of(char c, size_t pos=0) const noexcept
      { return str.find_last_not_of(c,pos); }
    template < class SVIEW_T > [[nodiscard]] size_t find_last_not_of(const SVIEW_T & sv, size_t pos=0) const
      { return str.find_last_not_of(sv,pos); }

    [[nodiscard]] size_t FindQuoteMatch(size_t pos) const;
    [[nodiscard]] size_t FindParenMatch(size_t pos, bool skip_quotes=true) const;
    [[nodiscard]] size_t FindMatch(size_t pos) const;
    [[nodiscard]] size_t Find(std::string target, size_t start, bool skip_quotes=false, bool skip_parens=false) const;
    [[nodiscard]] size_t Find(const CharSet & char_set, size_t start,
                bool skip_quotes=false, bool skip_parens=false) const;
    void FindAll(char target, emp::vector<size_t> & results,
                 const bool skip_quotes=false, bool skip_parens=false) const;
    [[nodiscard]] emp::vector<size_t> FindAll(char target, bool skip_quotes=false, bool skip_parens=false) const;
    template <typename... Ts>
    [[nodiscard]] size_t FindAnyOfFrom(size_t start, std::string test1, Ts... tests) const;
    template <typename T, typename... Ts>
    [[nodiscard]] size_t FindAnyOf(T test1, Ts... tests) const;
    [[nodiscard]] size_t FindID(std::string target, size_t start, bool skip_quotes=true, bool skip_parens=false) const;

    [[nodiscard]] size_t FindWhitespace(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(WhitespaceCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindNonWhitespace(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(!WhitespaceCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindUpperChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(UpperCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindNonUpperChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(!UpperCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindLowerChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(LowerCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindNonLowerChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(!LowerCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindLetterChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(LetterCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindNonLetterChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(!LetterCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindDigitChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(DigitCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindNonDigitChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(!DigitCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindAlphanumericChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(AlphanumericCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindNonAlphanumericChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(!AlphanumericCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindIDChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(IDCharSet(), start, skip_q, skip_p); }
    [[nodiscard]] size_t FindNonIDChar(size_t start=0, bool skip_q=false, bool skip_p=false) const 
      { return Find(!IDCharSet(), start, skip_q, skip_p); }


    // ------ Other Views ------
    std::string_view ViewNestedBlock(size_t start=0, bool skip_quotes=true)
      { return ViewRange(start+1, FindParenMatch(start, skip_quotes) - 1); }
    std::string_view ViewQuote(size_t start=0)
      { return ViewRange(start, FindQuoteMatch(start, skip_quotes)); }


    // ------ Transformations into non-Strings ------
    // Note: For efficiency there are two versions of most of these: one where the output
    // data structure is provided and one where it must be generated.

    void Slice(emp::vector<emp::String> & out_set, std::string delim=",",
               bool keep_quotes=true, bool keep_parens=true, bool trim_whitespace=true) const;

    [[nodiscard]]
    emp::vector<std::String> Slice(std::string delim=",", bool keep_quotes=true,
                                   bool keep_parens=true, bool trim_whitespace=true) const;

    void String::ViewSlices(
      emp::vector<std::string_view> & out_set,
      std::string delim=",",
      bool keep_quotes=true, bool keep_parens=true
    ) const;

    [[nodiscard]] emp::vector<std::string_view> String::ViewSlices(
      std::string delim=",",
      bool keep_quotes=true, bool keep_parens=true
    ) const;
  
    void SliceAssign(
      std::map<emp::String,emp::String> & out_map,
      std::string delim=",", std::string assign_op="=",
      bool keep_quotes=true, bool keep_parens=false, bool trim_whitespace=true
    ) const;

    [[nodiscard]] std::map<emp::String,emp::String> SliceAssign(
      std::string delim=",", std::string assign_op="=",
      bool keep_quotes=true, bool keep_parens=false, bool trim_whitespace=true
    ) const;


    // ------ Other Operators ------

    template <typename T> String operator+(T && in)
      { return String(str + std::forward<T>(in), mode); }
    template <typename T> bool operator==(T && in) { return in.str == std::forward<T>(in); }
    template <typename T> bool operator<=>(T && in) { return in.str <=> std::forward<T>(in); }
    

    // ------ SPECIAL CONFIGURATION ------

    String & UseQuoteSingle(bool use=true) { return _ChangeMode(USE_QUOTE_SINGLE, use); }
    String & UseQuoteDouble(bool use=true) { return _ChangeMode(USE_QUOTE_DOUBLE, use); }
    String & UseQuoteBack  (bool use=true) { return _ChangeMode(USE_QUOTE_BACK,   use); }
    String & UseParenRound (bool use=true) { return _ChangeMode(USE_PAREN_ROUND,  use); }
    String & UseParenSquare(bool use=true) { return _ChangeMode(USE_PAREN_SQUARE, use); }
    String & UseParenCurly (bool use=true) { return _ChangeMode(USE_PAREN_CURLY,  use); }
    String & UseParenAngle (bool use=true) { return _ChangeMode(USE_PAREN_ANGLE,  use); }
    String & UseParenQuotes(bool use=true) { return _ChangeMode(USE_PAREN_QUOTES, use); }

    bool Get_UseQuoteSingle() const { return mode.val & USE_QUOTE_SINGLE; }
    bool Get_UseQuoteDouble() const { return mode.val & USE_QUOTE_DOUBLE; }
    bool Get_UseQuoteBack  () const { return mode.val & USE_QUOTE_BACK; }
    bool Get_UseParenRound () const { return mode.val & USE_PAREN_ROUND; }
    bool Get_UseParenSquare() const { return mode.val & USE_PAREN_SQUARE; }
    bool Get_UseParenCurly () const { return mode.val & USE_PAREN_CURLY; }
    bool Get_UseParenAngle () const { return mode.val & USE_PAREN_ANGLE; }
    bool Get_UseParenQuotes() const { return mode.val & USE_PAREN_QUOTES; }


    //  ------ FORMATTING ------
    // Append* adds to the end of the current string.
    // Set* replaces the current string.
    // To* Converts the current string.
    // As* Returns a modified version of the current string, leaving original intact.
    // Most also have stand-along Make* versions where the core implementation is found.

    template <typename... Ts>
    String & Append(Ts... args) { str += MakeString(std::forward<Ts>(args)...); return *this; }
    template <typename... Ts>
    String & Set(Ts... args) { str = MakeString(std::forward<Ts>(args)...); return *this; }

    String & AppendEscaped(char c) { str += MakeEscaped(c); }
    String & SetEscaped(char c) { str = MakeEscaped(c); }

    String & AppendEscaped(const std::string & in) { str+=MakeEscaped(in); return *this; }
    String & SetEscaped(const std::string & in) { str = MakeEscaped(in); return *this; }
    String & ToEscaped() { str = MakeEscaped(str); }
    [[nodiscard]] std::string AsEscaped() { return MakeEscaped(str); }

    String & AppendWebSafe(std::string in) { str+=MakeWebSafe(in); return *this; }
    String & SetWebSafe(const std::string & in) { str = MakeWebSafe(in); return *this;; }
    String & ToWebSafe() { str = MakeWebSafe(str); }
    [[nodiscard]] std::string AsWebSafe() { return MakeWebSafe(str); }

    // <= Creating Literals =>
    template <typename T>
    String & AppendLiteral(const T & in) { str+=MakeLiteral(in); return *this; }
    template <typename T>
    String & SetLiteral(const T & in) { str = MakeLiteral(in); return *this;; }
    String & ToLiteral() { str = MakeLiteral(str); }
    [[nodiscard]] std::string AsLiteral() { return MakeLiteral(str); }

    String & AppendUpper(const std::string & in) { str+=MakeUpper(in); return *this; }
    String & SetUpper(const std::string & in) { str = MakeUpper(in); return *this; }
    String & ToUpper() { str = MakeUpper(str); }
    [[nodiscard]] std::string AsUpper() { return MakeUpper(str); }

    String & AppendLower(const std::string & in) { str+=MakeLower(in); return *this; }
    String & SetLower(const std::string & in) { str = MakeLower(in); return *this; }
    String & ToLower() { str = MakeLower(str); }
    [[nodiscard]] std::string AsLower() { return MakeLower(str); }

    String & AppendTitleCase(const std::string & in) { str+=MakeTitleCase(in); return *this; }
    String & SetTitleCase(const std::string & in) { str = MakeTitleCase(in); return *this; }
    String & ToTitleCase() { str = MakeTitleCase(str); }
    [[nodiscard]] std::string AsTitleCase() { return MakeTitleCase(str); }

    String & AppendRoman(int val) { str+=MakeRoman(val); return *this; }
    String & SetRoman(int val) { str = MakeRoman(val); return *this; }

    template <typename CONTAINER_T> String & AppendEnglishList(const CONTAINER_T & container)
      { str += MakeEnglishList(container); return *this;}
    template <typename CONTAINER_T> String & SetEnglishList(const CONTAINER_T & container)
      { str = MakeEnglishList(container); return *this;}
      
    template<typename... ARG_Ts> String & AppendFormatted(const std::string& format, ARG_Ts... args)
      { str += MakeFormatted(format, std::forward<ARG_Ts>(args)...); }
    template<typename... ARG_Ts> String & SetFormatted(const std::string& format, ARG_Ts... args)
      { str = MakeFormatted(format, std::forward<ARG_Ts>(args)...); }

    template <typename CONTAINER_T>
    String & AppendJoin(const CONTAINER_T & container, std::string delim,
                        std::string open, std::string close)
      { str += Join(container, delim, open, close); return *this;}
    template <typename CONTAINER_T>
    String & SetJoin(const CONTAINER_T & container, std::string delim,
                     std::string open, std::string close)
      { str = Join(container); return *this;}
      
  };


  /// Determine if this string represents a proper number.
  bool String::IsNumber() const {
    if (!str.size()) return false;           // If string is empty, not a number!
    size_t pos = 0;
    if (HasOneOfAt("+-", pos)) ++pos;        // Allow leading +/-
    while (HasDigitAt(pos)) ++pos;           // Any number of digits (none is okay)
    if (HasCharAt('.', pos)) {               // If there's a DECIMAL PLACE, look for more digits.
      ++pos;                                 // Skip over the dot.
      if (!HasDigitAt(pos++)) return false;  // Must have at least one digit after '.'
      while (HasDigitAt(pos)) ++pos;         // Any number of digits is okay.
    }
    if (HasOneOfAt("eE", pos)) {             // If there's an e... SCIENTIFIC NOTATION!
      ++pos;                                 // Skip over the e.
      if (HasOneOfAt("+-", pos)) ++pos;      // skip leading +/-
      if (!HasDigitAt(pos++)) return false;  // Must have at least one digit after 'e'
      while (HasDigitAt(pos)) ++pos;         // Allow for MORE digits.
    }
    // If we've made it to the end of the string AND there was at least one digit, success!
    return (pos == str.size()) && HasDigit();
  }

  // Given the start position of a quote, find where it ends; marks must be identical
  size_t String::FindQuoteMatch(size_t pos) const {
    while (++pos < str.size()) {
      const char mark = str[pos];
      if (str[pos] == '\\') { ++pos; continue; } // Skip escaped characters
      if (str[pos] == mark) { return pos; }      // Found match!
    }
    return npos; // Not found.
  }

  // Given an open parenthesis, find where it closes (including nesting).  Marks must be different.
  size_t String::FindParenMatch(size_t pos, bool skip_quotes) const {
    const char open = str[pos];
    const char close = GetMatch(open);
    size_t open_count = 1;
    while (++pos < str.size()) {
      if (str[pos] == open) ++open_count;
      else if (str[pos] == close) { if (--open_count == 0) return pos; }
      else if (skip_quotes && IsQuote(str[pos]) ) pos = FindQuoteMatch(pos);
    }

    return npos;
  }

  size_t String::FindMatch(size_t pos) const {
    if (IsQuote(str[pos])) return FindQuoteMatch(pos);
    if (IsParen(str[pos])) return FindParenMatch(pos);
    return npos;
  }

  // A version of string::find() that can skip over quotes.
  size_t String::Find(std::string target, size_t start, bool skip_quotes, bool skip_parens) const {
    size_t found_pos = str.find(target, start);
    if (!skip_quotes && !skip_parens) return found_pos;

    // Make sure found_pos is not in a quote and/or parens; adjust as needed!
    for (size_t scan_pos=0;
         scan_pos < found_pos && found_pos != npos;
         scan_pos++)
    {
      // Skip quotes, if needed...
      if (skip_quotes && IsQuote(str[scan_pos])) {
        scan_pos = FindQuoteMatch(scan_pos);
        if (found_pos < scan_pos) found_pos = str.find(target, scan_pos);
      }
      else if (skip_parens && IsParen(str[scan_pos])) {
        scan_pos = FindParenMatch(scan_pos);
        if (found_pos < scan_pos) found_pos = str.find(target, scan_pos);
      }
    }

    return found_pos;
  }

  // Find any of a set of characters.
  size_t String::Find(const CharSet & char_set, size_t start,
                           bool skip_quotes, bool skip_parens) const
  {
    // Make sure found_pos is not in a quote and/or parens; adjust as needed!
    for (size_t pos=start; pos < str.size(); ++pos) {
      if (char_set.Has(str[pos])) return pos;
      else if (skip_quotes && IsQuote(str[pos])) pos = FindQuoteMatch(pos);
      else if (skip_parens && IsParen(str[pos])) pos = FindParenMatch(pos);
    }

    return npos;
  }

  void String::FindAll(char target, emp::vector<size_t> & results,
                            const bool skip_quotes, bool skip_parens) const {
    results.resize(0);
    for (size_t pos=0; pos < str.size(); pos++) {
      if (str[pos] == target) results.push_back(pos);

      // Skip quotes, if needed...
      if (skip_quotes && IsQuote(str[pos])) pos = FindQuoteMatch(pos);
      else if (skip_parens && IsParen(str[pos])) pos = FindParenMatch(pos);
    }
  }

  emp::vector<size_t> String::FindAll(char target, bool skip_quotes, bool skip_parens) const {
    emp::vector<size_t> out;
    FindAll(target, out, skip_quotes, skip_parens);
    return out;
  }

  template <typename... Ts>
  size_t String::FindAnyOfFrom(size_t start, std::string test1, Ts... tests) const {
    if constexpr (sizeof...(Ts) == 0) return test_str.find(test1, start);
    else {
      size_t pos1 = test_str.find(test1, start);
      size_t pos2 = FindAnyOfFrom(start, tests...);
      return std::min(pos1, pos2);
    }
  }

  template <typename T, typename... Ts>
  size_t String::FindAnyOf(T test1, Ts... tests) const {
    // If an offset is provided, use it.
    if constexpr (std::is_arithmetic_v<T>) {
      return FindAnyOfFrom(test1, std::forward<Ts>(tests)...);
    } else {
      return FindAnyOfFrom(0, test1, std::forward<Ts>(tests)...);
    }
  }

  // Find an whole identifier (same as find, but cannot have letter, digit or '_' before or after.) 
  size_t String::FindID(std::string target, size_t start,
                             bool skip_quotes, bool skip_parens) const
  {
    size_t pos = Find(target, start, skip_quotes, skip_parens);
    while (pos != npos) {
      bool before_ok = (pos == 0) || !is_idchar(in_string[pos-1]);
      size_t after_pos = pos+target.size();
      bool after_ok = (after_pos == in_string.size()) || !is_idchar(in_string[after_pos]);
      if (before_ok && after_ok) return pos;

      pos = Find(target, pos+target.size(), skip_quotes, skip_parens);
    }

    return npos;
  }


  /// Remove whitespace from the beginning or end of a string.
  emp::String & String::TrimWhitespace() {
    size_t start_count=0;
    while (start_count < str.size() && is_whitespace(str[start_count])) ++start_count;
    if (start_count) str.erase(0, start_count);

    size_t new_size = str.size();
    while (new_size > 0 && is_whitespace(str[new_size-1])) --new_size;
    str.resize(new_size);

    return *this;
  }

  /// Every time one or more whitespace characters appear replace them with a single space.
  emp::String & String::CompressWhitespace() {
    bool skip_whitespace = true;          // Remove whitespace from beginning of line.
    size_t pos = 0;

    for (const auto c : str) {
      if (is_whitespace(c)) {          // This char is whitespace
        if (skip_whitespace) continue; // Already skipping...
        str[pos++] = ' ';
        skip_whitespace = true;
      } else {  // Not whitespace
        str[pos++] = c;
        skip_whitespace = false;
      }
    }

    if (pos && skip_whitespace) pos--;   // If the end of the line is whitespace, remove it.
    str.resize(pos);

    return *this;
  }

  /// Remove all instances of specified characters from file.
  emp::String & String::RemoveChars(const CharSet & chars) {
    size_t cur_pos = 0;
    for (const auto c : str) {
      if (!chars.Has(c)) str[cur_pos++] = c;
    }
    str.resize(cur_pos);
    return *this;
  }

  /// Make a string safe(r)
  emp::String & String::Slugify() {
    SetLower();
    RemovePunctuation();
    CompressWhitespace(res);
    ReplaceChar(' ', '-');
    return *this;
  }


  // Pop functions...

  bool String::PopIf(char c) {
    if (str.size() && str[0] == c) { str.erase(0,1); return true; }
    return false;
  }

  bool String::PopIf(String in) {
    if (HasPrefix(in))) { PopFixed(in.size()); return true; }
    return false;
  }

  /// Pop a segment from the beginning of a string as another string, shortening original.
  emp::String String::PopFixed(std::size_t end_pos, size_t delim_size=0)
  {
    if (!end_pos) return ""; // Not popping anything!

    if (end_pos >= str.size()) {  // Pop whole string.
      std::string out = str;
      str.resize(0);
      return out;
    }

    emp::String out = str.substr(0, end_pos); // Copy up to the deliminator for ouput
    str.erase(0, end_pos + delim_size);       // Delete output string AND delimiter
    return out;
  }

  /// Remove a prefix of the string (up to a specified delimeter) and return it.  If the
  /// delimeter is not found, return the entire string and clear it.
  emp::String String::Pop(CharSet chars=" \n\t\r", bool skip_quotes=false, bool skip_parens=false) {
    size_t pop_end = chars.FindIn(str);
    size_t delim_end = pop_end+1;
    while(delim_end < str.size() && chars.Has(str[delim_end])) ++delim_end;
    return PopFixed(pop_end, delim_end - pop_end);
  }

  /// Remove a prefix of the string (up to a specified delimeter) and return it.  If the
  /// delimeter is not found, return the entire string and clear it.
  emp::String String::PopTo(std::string delim, bool skip_quotes=false, bool skip_parens=false) {
    return PopFixed(Find(delim, skip_quotes, skip_parents), delim.size());
  }

  emp::String String::PopWord() { return Pop(); }
  emp::String String::PopLine() { return Pop("\n"); }

  emp::String String::PopQuote() {
    const size_t end_pos = FindQuoteMatch(0);
    return end_pos==std::string::npos ? "" : PopFixed(in_string, end_pos+1);
  }

  emp::String String::PopParen(bool skip_quotes=false) {
    const size_t end_pos = FindParenMatch(0, skip_quotes);
    return end_pos==std::string::npos ? "" : PopFixed(in_string, end_pos+1);
  }

  size_t String::PopUInt() {
    size_t uint_size = 0;
    while (uint_size < str.size() && isdigit(str[uint_size])) ++uint_size;
    std::string out_uint = PopFixed(uint_size);
    return std::stoull(out_uint);
  }

  /// @brief Cut up a string based on the provided delimiter; fill them in to the provided vector.
  /// @param out_set destination vector
  /// @param delim delimiter to split on (default: ",")
  /// @param keep_quotes Should quoted text be kept together? (default: true)
  /// @param keep_parens Should contents of parens be kept together? (default: true)
  /// @param trim_whitespace Should whitespace around delim or assign be ignored? (default: true)
  void String::Slice(
    emp::vector<emp::String> & out_set,
    std::string delim=",", bool keep_quotes, bool keep_parens, bool trim_whitespace
  ) const {
    if (str.empty()) return; // Nothing to set!

    size_t start_pos = 0;
    for (size_t found_pos = Find(delim, 0, keep_quotes, keep_parens);
         found_pos < str.size();
         found_pos = Find(delim, found_pos+1, keep_quotes, keep_parens))
    {
      out_set.push_back( GetRange(start_pos, found_pos) );
      if (trim_whitespace) out_set.back().TrimWhitespace();
      start_pos = found_pos+delim.size();
    }
    out_set.push_back( GetRange(start_pos, found_pos) );
  }


  /// @brief Slice a String on a delimiter; return a vector of results.
  /// @note May be less efficient, but easier than other version of Slice()
  /// @param delim delimiter to split on (default: ",")
  /// @param keep_quotes Should quoted text be kept together? (default: true)
  /// @param keep_parens Should contents of parens be kept together? (default: true)
  /// @param trim_whitespace Should whitespace around delim or assign be ignored? (default: true)
  emp::vector<std::String> String::Slice(
    std::string delim, bool keep_quotes, bool keep_parens, bool trim_whitespace
  ) const {
    emp::vector<emp::String> result;
    slice(result, delim, keep_quotes, keep_parens, trim_whitespace);
    return result;
  }

  /// @brief Fill vector out_set of string_views based on the provided delimiter.
  /// @param out_set destination vector
  /// @param delim delimiter to split on (default: ",")
  /// @param keep_quotes Should quoted text be kept together? (default: true)
  /// @param keep_parens Should contents of parens be kept together? (default: true)
  void String::ViewSlices(
    emp::vector<std::string_view> & out_set,
    std::string delim, bool keep_quotes, bool keep_parens
  ) const {
    out_set.resize(0);
    if (str.empty()) return; // Nothing to set!

    size_t start_pos = 0;
    for (size_t found_pos = Find(delim, 0, keep_quotes, keep_parens);
         found_pos < str.size();
         found_pos = Find(delim, found_pos+1, keep_quotes, keep_parens))
    {
      out_set.push_back( ViewRange(start_pos, found_pos) );
      start_pos = found_pos+delim.size();
    }
    out_set.push_back( ViewRange(start_pos, found_pos) );
  }

  /// @brief Generate vector of string_views based on the provided delimiter.
  /// @param delim delimiter to split on (default: ",")
  /// @param keep_quotes Should quoted text be kept together? (default: true)
  /// @param keep_parens Should contents of parens be kept together? (default: true)
  [[nodiscard]] emp::vector<std::string_view> String::ViewSlices(
    std::string delim, bool keep_quotes, bool keep_parens
  ) {
    emp::vector<std::string_view> result;
    ViewSlices(result, delim, keep_quotes, keep_parens);
    return result;
  }
  

  /// @brief Slice a string and treat each section as an assignment; place results in provided map.
  /// @param delim delimiter to split on (default ',')
  /// @param assign_op separator for left and right side of assignment (default: "=")
  /// @param keep_quotes Should quoted text be kept together? (default: yes)
  /// @param keep_parens Should contents of parentheses be kept together? (default: no)
  /// @param trim_whitespace Should whitespace around delim or assign be ignored? (default: true)
  void SliceAssign(
    std::map<emp::String,emp::String> & result_map,
    std::string delim, std::string assign_op,
    bool keep_quotes, bool keep_parens, bool trim_whitespace
  ) const {
    auto assign_set = Slice(delim, max_split, keep_quotes, keep_parens, false);
    for (auto setting : assign_set) {
      if (setting.IsWhitespace()) continue; // Skip blank settings (especially at the end).

      // Remove any extra spaces around parsed values.
      emp::String var_name = setting.PopTo(assign_op);
      if (trim_whitespace) {
        var_name.TrimWhitespace();
        setting.TrimWhitespace();
      }
      if (setting.empty()) {
        std::stringstream msg;
        msg << "No assignment found in slice_assign() for: " << var_name;
        emp::notify::Exception("emp::string_utils::slice_assign::missing_assign",
                               msg.str(), var_name);                               
      }
      result_map[var_name] = setting;
    }
    return result_map;
  }


  /// @brief Slice a string and treat each section as an assignment; fill out a map and return it.
  /// @param delim delimiter to split on (default ',')
  /// @param assign_op separator for left and right side of assignment (default: "=")
  /// @param keep_quotes Should quoted text be kept together? (default: yes)
  /// @param keep_parens Should contents of parentheses be kept together? (default: no)
  /// @param trim_whitespace Should whitespace around delim or assign be ignored? (default: true)
  std::map<emp::String,emp::String> SliceAssign(
    std::string delim, std::string assign_op,
    bool keep_quotes, bool keep_parens, bool trim_whitespace
  ) const {
    std::map<emp::String,emp::String> result_map;
    SliceAssign(result_map, delim, assign_op, keep_quotes, keep_parens, trim_whitespace);
    return result_map;
  }


  /// Find any instances of ${X} and replace with dictionary lookup of X.
  template <typename MAP_T>
  emp::String & String::ReplaceVars(const MAP_T & var_map) {
    for (size_t pos = Find('$');
         pos < size()-3;             // Need room for a replacement tag.
         pos = Find('$', pos+1)) {
      if (str[pos+1] == '$') {       // Compress two $$ into one $
        str.erase(pos,1);
        continue;
      }
      if (result[pos+1] != '{') continue; // Eval must be surrounded by braces.

      // If we made it this far, we have a starting match!
      size_t end_pos = FindParenMatch(pos+1);
      if (end_pos == npos) {
        emp::notify::Exception("emp::string_utils::replace_vars::missing_close",
                               "No close brace found in string_utils::replace_vars()",
                               str);
        break;
      }

      std::string key = GetRange(pos+2, end_pos);
      auto replacement_it = var_map.find(key);
      if (replacement_it == var_map.end()) {
        emp::notify::Exception("emp::string_utils::replace_vars::missing_var",
                               emp::to_string("Lookup variable not found in var_map (key=", key, ")"),
                               key);
        break;
      }
      ReplaceRange(pos, end_pos+1, replacement_it->second);   // Put into place.
      pos += replacement_it->second.size();                   // Continue at the next position...
    }

    return *this;
  }

  /// @brief Find any instance of MACRO_NAME(ARGS) and replace it with fun(ARGS).
  /// @param in_string String to perform macro replacement.
  /// @param start_str Initial sequence of macro to look for; for example "REPLACE("
  /// @param end_str   Sequence that ends the macro; for example ")"
  /// @param macro_fun Function to call with contents of macro.  Params are macro_args (string), line_num (size_t), and hit_num (size_t)
  /// @param skip_quotes Should we skip quotes when looking for macro?
  /// @return Processed version of in_string with macros replaced.
  template <typename FUN_T>
  emp::String & String::ReplaceMacro(
    std::string start_str,
    std::string end_str,
    FUN_T && macro_fun,
    bool skip_quotes
  ) {
    // We need to identify the comparator and divide up arguments in macro.
    size_t macro_count = 0;     // Count of the number of hits for this macro.
    size_t line_num = 0;        // Line number where current macro hit was found.
    size_t prev_pos = 0;
    for (size_t macro_pos = Find(start_str, 0, skip_quotes);
         macro_pos != npos;
         macro_pos = Find(start_str, macro_pos+1, skip_quotes))
    {
      // Make sure we're not just extending a previous identifier.
      if (macro_pos && is_idchar(str[macro_pos-1])) continue;

      line_num += Count('\n', prev_pos, macro_pos);  // Count lines leading up to this macro.

      // Isolate this macro instance and call the conversion function.
      size_t end_pos = Find(end_str, macro_pos+start_str.size(), skip_quotes);
      const std::string macro_body = GetRange(macro_pos+start_str.size(), end_pos);

      std::string new_str = macro_fun(macro_body, line_num, macro_count);
      ReplaceRange(macro_pos, end_pos+end_str.size(), new_str);
      prev_pos = macro_pos;
      macro_count++;
    }

    return *this;
  }





/////// @CAO CONTINUE HERE!!!!!!!!







  #ifndef DOXYGEN_SHOULD_SKIP_THIS

  // The next functions are not efficient, but they will take any number of inputs and
  // dynamically convert them all into a single, concatenated string.

  /// Setup emp::ToString declarations for built-in types.
  template <typename T, size_t N> inline std::string ToString(const emp::array<T,N> & container);
  template <typename T, typename... Ts>
  inline std::string ToString(const emp::vector<T, Ts...> & container);


  #endif // DOXYGEN_SHOULD_SKIP_THIS


  /// This function does its best to convert any type to a string. Accepts any number of
  /// arguments and returns a single concatenated string. Conversions attempted for an
  /// object 'x' include (in order):
  /// - Call a x.ToString()
  /// - Call appropriate emp::ToString(x) overload
  /// - Pass x through stringstream
  template <typename... Ts>
  inline std::string to_string(const Ts &... values) {
    std::stringstream ss;
    (ss << ... << internal::to_stream_item(values, true));
    return ss.str();
  }

  /// Overload of to_string() string arguments to be directly returned.
  inline const std::string & to_string(const std::string & value) {
    return value;
  }

  /// Setup emp::ToString to work on arrays.
  template <typename T, size_t N>
  inline std::string ToString(const emp::array<T,N> & container) {
    std::stringstream ss;
    ss << "[ ";
    for (const auto & el : container) {
      ss << to_string(el);
      ss << " ";
    }
    ss << "]";
    return ss.str();
  }

  /// Setup emp::ToString to work on vectors.
  template <typename T, typename... Ts>
  inline std::string ToString(const emp::vector<T, Ts...> & container) {
    std::stringstream ss;
    ss << "[ ";
    for (const auto & el : container) {
      ss << to_string(el);
      ss << " ";
    }
    ss << "]";
    return ss.str();
  }

  /// This function tries to convert a string into any type you're looking for...  You just
  /// need to specify the out type as the template argument.
  template <typename T>
  inline T from_string(const std::string & str) {
    std::stringstream ss;
    ss << str;
    T out_val;
    ss >> out_val;
    return out_val;
  }

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
  namespace internal {
    static inline void _from_string(std::stringstream &) { ; }

    template <typename T, typename... Ts>
    void _from_string(std::stringstream & ss, T & arg1, Ts... extra_args) {
      ss >> arg1;
      _from_string(ss, extra_args...);
    }
  }
  #endif // DOXYGEN_SHOULD_SKIP_THIS

  /// The from_string() function can also take multiple args instead of a return.
  template <typename... Ts>
  inline void from_string(const std::string & str, Ts &... args) {
    std::stringstream ss;
    ss << str;
    internal::_from_string(ss, args...);
  }

  /// The from_strings() function takes a vector of strings and converts them into a vector
  /// of the appropriate type.
  template <typename T>
  inline emp::vector<T> from_strings(const emp::vector<std::string> & string_v) {
    emp::vector<T> vals(string_v.size());
    for (size_t i = 0; i < string_v.size(); i++) {
      vals[i] = from_string<T>(string_v[i]);
    }
    return vals;
  }

  /// This function tries to convert a string_view into any other type...  You must
  /// need to specify the out type as the template argument.
  template <typename T>
  inline T from_string(std::string_view str) {
    std::stringstream ss;
    ss << str;
    T out_val;
    ss >> out_val;
    return out_val;
  }



  // Some ANSI helper functions.
  inline constexpr char ANSI_ESC() { return (char) 27; }
  inline std::string ANSI_Reset() { return "\033[0m"; }
  inline std::string ANSI_Bold() { return "\033[1m"; }
  inline std::string ANSI_Faint() { return "\033[2m"; }
  inline std::string ANSI_Italic() { return "\033[3m"; }
  inline std::string ANSI_Underline() { return "\033[4m"; }
  inline std::string ANSI_SlowBlink() { return "\033[5m"; }
  inline std::string ANSI_Blink() { return "\033[6m"; }
  inline std::string ANSI_Reverse() { return "\033[7m"; }
  inline std::string ANSI_Strike() { return "\033[9m"; }

  inline std::string ANSI_NoBold() { return "\033[22m"; }
  inline std::string ANSI_NoItalic() { return "\033[23m"; }
  inline std::string ANSI_NoUnderline() { return "\033[24m"; }
  inline std::string ANSI_NoBlink() { return "\033[25m"; }
  inline std::string ANSI_NoReverse() { return "\033[27m"; }

  inline std::string ANSI_Black() { return "\033[30m"; }
  inline std::string ANSI_Red() { return "\033[31m"; }
  inline std::string ANSI_Green() { return "\033[32m"; }
  inline std::string ANSI_Yellow() { return "\033[33m"; }
  inline std::string ANSI_Blue() { return "\033[34m"; }
  inline std::string ANSI_Magenta() { return "\033[35m"; }
  inline std::string ANSI_Cyan() { return "\033[36m"; }
  inline std::string ANSI_White() { return "\033[37m"; }
  inline std::string ANSI_DefaultColor() { return "\033[39m"; }

  inline std::string ANSI_BlackBG() { return "\033[40m"; }
  inline std::string ANSI_RedBG() { return "\033[41m"; }
  inline std::string ANSI_GreenBG() { return "\033[42m"; }
  inline std::string ANSI_YellowBG() { return "\033[43m"; }
  inline std::string ANSI_BlueBG() { return "\033[44m"; }
  inline std::string ANSI_MagentaBG() { return "\033[45m"; }
  inline std::string ANSI_CyanBG() { return "\033[46m"; }
  inline std::string ANSI_WhiteBG() { return "\033[47m"; }
  inline std::string ANSI_DefaultBGColor() { return "\033[49m"; }

  inline std::string ANSI_BrightBlack() { return "\033[30m"; }
  inline std::string ANSI_BrightRed() { return "\033[31m"; }
  inline std::string ANSI_BrightGreen() { return "\033[32m"; }
  inline std::string ANSI_BrightYellow() { return "\033[33m"; }
  inline std::string ANSI_BrightBlue() { return "\033[34m"; }
  inline std::string ANSI_BrightMagenta() { return "\033[35m"; }
  inline std::string ANSI_BrightCyan() { return "\033[36m"; }
  inline std::string ANSI_BrightWhite() { return "\033[37m"; }

  inline std::string ANSI_BrightBlackBG() { return "\033[40m"; }
  inline std::string ANSI_BrightRedBG() { return "\033[41m"; }
  inline std::string ANSI_BrightGreenBG() { return "\033[42m"; }
  inline std::string ANSI_BrightYellowBG() { return "\033[43m"; }
  inline std::string ANSI_BrightBlueBG() { return "\033[44m"; }
  inline std::string ANSI_BrightMagentaBG() { return "\033[45m"; }
  inline std::string ANSI_BrightCyanBG() { return "\033[46m"; }
  inline std::string ANSI_BrightWhiteBG() { return "\033[47m"; }

  /// Make a string appear bold when printed to the command line.
  inline std::string to_ansi_bold(const std::string & _in) {
    return ANSI_Bold() + _in + ANSI_NoBold();
  }

  /// Make a string appear italics when printed to the command line.
  inline std::string to_ansi_italic(const std::string & _in) {
    return ANSI_Italic() + _in + ANSI_NoItalic();
  }

  /// Make a string appear underline when printed to the command line.
  inline std::string to_ansi_underline(const std::string & _in) {
    return ANSI_Underline() + _in + ANSI_NoUnderline();
  }

  /// Make a string appear blink when printed to the command line.
  inline std::string to_ansi_blink(const std::string & _in) {
    return ANSI_Blink() + _in + ANSI_NoBlink();
  }

  /// Make a string appear reverse when printed to the command line.
  inline std::string to_ansi_reverse(const std::string & _in) {
    return ANSI_Reverse() + _in + ANSI_NoReverse();
  }


  //////////////////////////////////////////////////////
  //  Implementations of larger functions (>25 lines)
  //////////////////////////////////////////////////////

  /// Test if an input string is properly formatted as a literal character.
  static inline bool is_literal_char(const std::string & value) {
    // A literal char must begin with a single quote, contain a representation of a single
    // character, and end with a single quote.
    if (value.size() < 3) return false;
    if (value[0] != '\'' || value.back() != '\'') return false;

    // If there's only a single character in the quotes, it's USUALLY legal.
    if (value.size() == 3) {
      switch (value[1]) {
        case '\'':         // Can't be a single quote (must be escaped!)
        case '\\':         // Can't be a backslash (must be followed by something!)
          return false;
        default:
          return true;
      }
    }

    // If there are more characters, must be an escape sequence.
    if (value.size() == 4) {
      if (value[1] != '\\') return false;

      // Identify legal escape sequences.
      // @CAO Need more here!
      switch (value[2]) {
        case 'n':   // Newline
        case 'r':   // Return
        case 't':   // Tab
        case '0':   // Empty (character 0)
        case '\\':  // Backslash
        case '\'':  // Single quote
          return true;
        default:
          return false;
      }
    }

    // @CAO: Need to add special types of numerical escapes here (e.g., ascii codes!)

    // If we made it here without a problem, it must be correct!
    return true;
  }


  /// Test if an input string is properly formatted as a literal string.
  static inline bool is_literal_string(const std::string & value,
                                       const std::string & quote_marks) {
    if (value.size() < 2) return false;               // Two short to contain even quote marks!
    char quote = value[0];
    if (!is_one_of(quote, quote_marks)) return false; // Must be working with allowed quote mark.
    if (value.back() != quote) return false;          // Must use same quote at front and back.

    // Are all of the characters valid?
    for (size_t pos = 1; pos < value.size() - 1; pos++) {
      if (value[pos] == quote) return false;          // Cannot have a raw quote in the middle.
      if (value[pos] == '\\') {                       // Allow escaped characters...
        if (pos == value.size()-2) return false;      // Backslash must have char to escape.

        // Move to the next char and make sure it is legal to be escaped.
        // @CAO Expand on options!
        pos++;
        switch (value[pos]) {
          case 'b':   // Backspace
          case 'f':   // Form feed
          case 'n':   // Newline
          case 'r':   // Return
          case 't':   // Tab
          case 'v':   // Vertical tab.
          case '0':   // Empty (character 0)
          case '\\':  // Backslash
          case '"':   // Double quote
          case '\'':  // Single quote
          case '`':   // Back quote
            continue;
          default:
            return false;
        }
      }
    }

    // @CAO: Need to check special types of numerical escapes (e.g., ascii codes!)

    // If we made it here without a problem, it must be correct!
    return true;
  }


  /// Test if an input string is properly formatted as a literal string.
  static inline std::string diagnose_literal_string(const std::string & value,
                                                    const std::string & quote_marks) {
    // A literal string must begin and end with a double quote and contain only valid characters.
    if (value.size() < 2) return "Too short!";
    char quote = value[0];
    if (!is_one_of(quote, quote_marks)) return "Must begin an end in quotes.";
    if (value.back() != quote) return "Begin and end quotes must match.";

    // Are all of the characters valid?
    for (size_t pos = 1; pos < value.size() - 1; pos++) {
      if (value[pos] == quote) return "Has a floating quote.";
      if (value[pos] == '\\') {
        if (pos == value.size()-2) return "Cannot escape the final quote.";  // Backslash must have char to escape.

        // Move to the next char and make sure it's legal to be escaped.
        // @CAO Expand on options!
        pos++;
        switch (value[pos]) {
          case 'b':   // Backspace
          case 'f':   // Form feed
          case 'n':   // Newline
          case 'r':   // Return
          case 't':   // Tab
          case 'v':   // Vertical tab.
          case '0':   // Empty (character 0)
          case '\\':  // Backslash
          case '"':   // Double quote
          case '\'':  // Single quote
          case '`':   // Back quote
            continue;
          default:
            return "Unknown escape charater.";
        }
      }
    }

    // @CAO: Need to check special types of numerical escapes (e.g., ascii codes!)

    // If we made it here without a problem, it must be correct!
    return "Good!";
  }












  //------------- Stand-alone function definitions --------------

  template <typename... Ts>
  emp::String MakeString(Ts... args) {
    std::stringstream ss;
    (ss << ... << std::forward<Ts>(args));
    return ss.str();
  }

  emp::String MakeEscaped(char c) {
    // If we just append as a normal character, do so!
    if ( (c >= 40 && c < 91) || (c > 96 && c < 127)) return emp::String(c);
    switch (c) {
    case '\0': return "\\0";
    case 1: return "\\001";
    case 2: return "\\002";
    case 3: return "\\003";
    case 4: return "\\004";
    case 5: return "\\005";
    case 6: return "\\006";
    case '\a': return "\\a";  // case  7 (audible bell)
    case '\b': return "\\b";  // case  8 (backspace)
    case '\t': return "\\t";  // case  9 (tab)
    case '\n': return "\\n";  // case 10 (newline)
    case '\v': return "\\v";  // case 11 (vertical tab)
    case '\f': return "\\f";  // case 12 (form feed - new page)
    case '\r': return "\\r";  // case 13 (carriage return)
    case 14: return "\\016";
    case 15: return "\\017";
    case 16: return "\\020";
    case 17: return "\\021";
    case 18: return "\\022";
    case 19: return "\\023";
    case 20: return "\\024";
    case 21: return "\\025";
    case 22: return "\\026";
    case 23: return "\\027";
    case 24: return "\\030";
    case 25: return "\\031";
    case 26: return "\\032";
    case 27: return "\\033";  // case 27 (ESC), sometimes \e
    case 28: return "\\034";
    case 29: return "\\035";
    case 30: return "\\036";
    case 31: return "\\037";

    case '\"': return "\\\"";  // case 34
    case '\'': return "\\\'";  // case 39
    case '\\': return "\\\\";  // case 92
    case 127: return "\\177";  // (delete)
    };

    return emp::String(c);
  }
  
  emp::String MakeEscaped(const std::string & in) { return emp::String(in, MakeEscaped); }

  /// Take a string and replace reserved HTML characters with character entities
  emp::String MakeWebSafe(const std::string & in) {
    emp::String out;
    out.reserve(in.size());
    for (char c : in) {
      switch (c) {
      case '&': out += "&amp"; break;
      case '<': out += "&lt"; break;
      case '>': out += "&gt"; break;
      case '\'': out += "&apos"; break;
      case '"': out += "&quot"; break;
      default: out += c;
      }
    }
    return out;
  }

  /// Take a char and convert it to a C++-style literal.
  [[nodiscard]] emp::String MakeLiteral(char value) {
    std::stringstream ss;
    ss << "'" << MakeEscaped(value) << "'";
    return ss.str();
  }

  /// Take a string or iterable and convert it to a C++-style literal.
  // This is the version for string. The version for an iterable is below.
  [[nodiscard]] emp::String MakeLiteral(const std::string & value) {
    // Add quotes to the ends and convert each character.
    std::stringstream ss;
    ss << "\"";
    for (char c : value) {
      ss << MakeEscaped(c);
    }
    ss << "\"";
    return ss.str();
  }

  #ifndef DOXYGEN_SHOULD_SKIP_THIS

  /// Take a value and convert it to a C++-style literal.
  template <typename T> [[nodiscard]] emp::String MakeLiteral(const T & value) {
    std::stringstream ss;
    if constexpr (emp::IsIterable<T>::value) {
      ss << "{ ";
      for (auto it = std::begin( value ); it != std::end( value ); ++it) {
        if (it != std::begin( value )) ss << ",";
        ss << MakeLiteral< std::decay_t<decltype(*it)> >( *it );
      }
      ss << " }";
    }
    else ss << value;
    return ss.str();
  }

  #endif

  [[nodiscard]] emp::String MakeFromLiteral(const std::string & value) {
    if (value.size() == 0) return "";
    if (value[0] == '\'') return emp::String(MakeFromLiteral_Char(value)); 
    if (value[0] == '"') return MakeFromLiteral_String(value);
    // @CAO Add conversion from numerical literals, and especially octal (0-), binary (0b-), and hex (0x-)
  }

  /// Convert a literal character representation to an actual string.
  /// (i.e., 'A', ';', or '\n')
  [[nodiscard]] char MakeFromLiteral_Char(const std::string & value) {
    emp_assert(is_literal_char(value));
    // Given the assert, we can assume the string DOES contain a literal representation,
    // and we just need to convert it.

    if (value.size() == 3) return value[1];
    if (value.size() == 4) return ToEscapeChar(value[2]);

    return '\0'; // Error!
  }


  /// Convert a literal string representation to an actual string.
  [[nodiscard]] emp::String MakeFromLiteral_String(const std::string & value) {
  /// Convert a literal string representation to an actual string.
    emp_assert(is_literal_string(value),
               value, diagnose_literal_string(value));
    // Given the assert, we can assume string DOES contain a literal string representation.

    std::string out_string;
    out_string.reserve(value.size()-2);  // Make a guess on final size.

    for (size_t pos = 1; pos < value.size() - 1; pos++) {
      // If we don't have an escaped character, just move it over.
      if (value[pos] != '\\') out_string.push_back(value[pos]);
      else out_string.push_back(ToEscapeChar(value[++pos]));
    }

    return out_string;
  }

  /// Convert a string to all uppercase.
  [[nodiscard]] emp::String MakeUpper(std::string value) {
    return emp::String(value, std::toupper);
  }

  /// Convert a string to all lowercase.
  [[nodiscard]] emp::String MakeLower(std::string value) {
    return emp::String(value, std::tolower);
  }

  /// Make first letter of each word upper case
  [[nodiscard]] emp::String MakeTitleCase(std::string value) {
    constexpr int char_shift = 'a' - 'A';
    bool next_upper = true;
    for (size_t i = 0; i < value.size(); i++) {
      if (next_upper && value[i] >= 'a' && value[i] <= 'z') {
        value[i] = (char) (value[i] - char_shift);
      } else if (!next_upper && value[i] >= 'A' && value[i] <= 'Z') {
        value[i] = (char) (value[i] + char_shift);
      }

      next_upper = (value[i] == ' ');
    }
    return value;
  }

  /// Convert an integer to a roman numeral string.
  [[nodiscard]] emp::String MakeRoman(int val) {
    emp::String out;
    if (val < 0) { out += "-"; val *= -1; }

    // If out of bounds, divide up into sections of 1000 each.
    if (val > 3999) { out += MakeRoman(val/1000); val %= 1000; out += '|'; }

    // Loop through dealing with the rest of the number.
    while (val > 0) {
      else if (val >= 1000) { out += "M";  val -= 1000; }
      else if (val >= 900)  { out += "CM"; val -= 900; }
      else if (val >= 500)  { out += "D";  val -= 500; }
      else if (val >= 400)  { out += "CD"; val -= 400; }
      else if (val >= 100)  { out += "C";  val -= 100; }
      else if (val >= 90)   { out += "XC"; val -= 90; }
      else if (val >= 50)   { out += "L";  val -= 50; }
      else if (val >= 40)   { out += "XL"; val -= 40; }
      else if (val >= 10)   { out += "X";  val -= 10; }
      else if (val == 9)    { out += "IX"; val -= 9; }
      else if (val >= 5)    { out += "V";  val -= 5; }
      else if (val == 4)    { out += "IV"; val -= 4; }
      else                  { out += "I";  val -= 1; }
    }

    return out;
  }

  template <typename CONTAINER_T>
  emp::String MakeEnglishList(const CONTAINER_T & container) {
    if (container.size() == 0) return "";
    if (container.size() == 1) return to_string(container.front());

    auto it = container.begin();
    if (container.size() == 2) return to_string(*it, " and ", *(it+1));

    auto last_it = container.end() - 1;
    String out(to_string(*it))
    ++it;
    while (it != last_it) {
      out += ',';
      out += to_string(*it);
      ++it;
    }
    out += to_string(", and ", *it);

    return out;
  }

  /// Apply sprintf-like formatting to a string.
  /// See https://en.cppreference.com/w/cpp/io/c/fprintf.
  /// Adapted from https://stackoverflow.com/a/26221725.
  template<typename... Args>
  emp::String MakeFormatted(const std::string& format, Args... args) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-security"

    // Extra space for '\0'
    const size_t size = static_cast<size_t>(std::snprintf(nullptr, 0, format.c_str(), args...) + 1);

    emp::vector<char> buf(size);
    std::snprintf(buf.data(), size, format.c_str(), args...);

     // We don't want the '\0' inside
    return emp::String( buf.data(), buf.data() + size - 1 );

    #pragma GCC diagnostic pop
  }

  /// Concatenate n copies of a string.
  emp::String MakeRepeat(emp::String base, size_t n ) {
    emp::String out;
    out.reserve(n * base.size());
    for (size_t i=0; i < n; ++i) out += base;
    return out;
  }

  /// This function returns values from a container as a single string separated
  /// by a given delimeter and with optional surrounding strings.
  /// @param container is any standard-interface container holding objects to be joined.
  /// @param join_str optional delimeter
  /// @param open string to place before each string (e.g., "[" or "'")
  /// @param close string to place after each string (e.g., "]" or "'")
  /// @return merged string of all values
  template <typename CONTAINER_T>
  emp::String Join(const CONTAINER_T & container, std::string join_str,
                      std::string open, std::string close) {
    if (container.size() == 0) return "";
    if (container.size() == 1) return to_string(open, container.front(), close);

    std::stringstream out;
    for (auto it = container.begin(); it != container.end(); ++it) {
      if (it != container.begin()) out << join_str;
      out << open << to_string(*it) << close;
    }

    return out.str();
  }



  // ------ External function overrides ------

  std::ostream & operator<<(std::ostream & os, const emp::String & str) {
    return os << str;
  }

  std::istream & operator>>(std::istream & is, emp::String & str) {
    return is >> str;
  }

  template<typename STREAM_T>
  STREAM_T & getline(STREAM_T && input, emp::String str, char delim) {
    return getline(std::forward<STREAM_T>(input), str.str, delim);
  }

  template<typename STREAM_T>
  STREAM_T & getline(STREAM_T && input, emp::String str) {
    return getline(std::forward<STREAM_T>(input), str.str);
  }
}

#endif // #ifndef EMP_TOOLS_STRING_UTILS_HPP_INCLUDE
