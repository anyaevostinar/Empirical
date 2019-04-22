  /**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2019
 *
 *  @file  AST.h
 *  @brief AST nodes generated by Emphatic.
 **/

#include "../../source/base/Ptr.h"
#include "../../source/base/vector.h"
#include "../../source/tools/string_utils.h"

/// All AST Nodes have a common base class.
  struct AST_Node {
    /// Echo the original code passed into each class.
    virtual void PrintEcho(std::ostream &, const std::string & prefix) const = 0;
    virtual void PrintOutput(std::ostream &, const std::string & prefix) const = 0;
  };

  /// AST Node for a new scope level.
  struct AST_Scope : AST_Node {
    emp::vector<emp::Ptr<AST_Node>> children;
    ~AST_Scope() { for (auto x : children) x.Delete(); }
    void AddChild(emp::Ptr<AST_Node> node_ptr) { children.push_back(node_ptr); }

    /// Scope should run echo on each of its children.
    void PrintEcho(std::ostream & os, const std::string & prefix) const override {
      for (auto x : children) { x->PrintEcho(os, prefix); }
    }

    /// Scope should run output on each of its children.
    void PrintOutput(std::ostream & os, const std::string & prefix) const override {
      for (auto x : children) { x->PrintOutput(os, prefix); }
    }
  };

  struct AST_Namespace : public AST_Scope {
    std::string name;
  };

  /// AST Node for outer level using statement...
  struct AST_Using : AST_Node {
    std::string type_name;
    std::string type_value;

    void PrintEcho(std::ostream & os, const std::string & prefix) const override {
      os << prefix << "using " << type_name << " = " << type_value << "\n";
    }

    /// Output for a using should be identical to the input.
    void PrintOutput(std::ostream & os, const std::string & prefix) const override {
      os << prefix << "using " << type_name << " = " << type_value << "\n";
    }
  };

  /// AST Node for variable defined inside of a concept.
  struct ConceptVariable {
    std::string var_type;
    std::string var_name;
    std::string default_code;
  };

  /// AST Node for function defined inside of a concept.
  struct ConceptFunction {
    std::string return_type;
    std::string fun_name;
    struct Param { std::string type; std::string name; };
    emp::vector<Param> params;
    std::set<std::string> attributes;     // const, noexcept, etc.
    std::string default_code;
    bool is_required = false;
    bool is_default = false;

    std::string AttributeString() const {
      std::string out_str;
      for (const auto & x : attributes) {
        out_str += " ";
        out_str += x;
      }
      return out_str;
    }

    std::string ParamString() const {
      std::string out_str;
      for (size_t i = 0; i < params.size(); i++) {
        if (i) out_str += ", ";
        out_str += emp::to_string(params[i].type, " ", params[i].name);
      }
      return out_str;
    }

    std::string ArgString() const {
      std::string out_str;
      for (size_t i = 0; i < params.size(); i++) {
        if (i) out_str += ", ";
        out_str += params[i].name;
      }
      return out_str;
    }
  };

  /// AST Node for type definition inside of a concept.
  struct ConceptTypedef {
    std::string type_name;
    std::string type_value;
  };


  /// AST Node for concept information.
  struct AST_Concept : AST_Node {

    std::string name;
    std::string base_name;

    emp::vector<ConceptVariable> variables;
    emp::vector<ConceptFunction> functions;
    emp::vector<ConceptTypedef> typedefs;


    void PrintEcho(std::ostream & os, const std::string & prefix) const override {
      // Open the concept
      os << prefix << "concept " << name << " : " << base_name << " {\n";

      // Print info for all typedefs
      for (auto & t : typedefs) {
        os << prefix << "  using " << t.type_name << " = " << t.type_value << "\n";
      }

      // Print info for all variables...
      for (auto & v : variables) {
        os << prefix << "  " << v.var_type << " " << v.var_name << " = " << v.default_code << "\n";
      }

      // Print info for all functions...
      for (auto & f : functions) {
        os << prefix << "  " << f.return_type << " " << f.fun_name << "(" << f.ParamString() << ") "
           << f.AttributeString();
        if (f.is_required) os << " = required;\n";
        else if (f.is_default) os << " = default;\n";
        else os << " {\n" << prefix << "    " << f.default_code << "\n" << prefix << "  }\n";
      }

      // Close the concept
      os << prefix << "};\n";
    }

    void PrintOutput(std::ostream & os, const std::string & prefix) const override {
      os << prefix << "/// Base class for concept wrapper " << name << "<>.\n"
         << prefix << "class " << base_name << " {\n"
         << prefix << "public:\n";

      // Print all of the BASE CLASS details.
      for (auto & f : functions) {
        os << prefix << "  " << f.return_type << " " << f.fun_name << "(" << f.ParamString() << ") "
           << f.AttributeString() << " = 0;\n";
      }

      os << prefix << "};\n\n";

      // Print all of the TEMPLATE WRAPPER details.
      os << prefix << "/// === Concept wrapper (base class is " << base_name << ") ===\n"
         << prefix << "template <typename WRAPPED_T>\n"
         << prefix << "class " << name << " : WRAPPED_T, " << base_name << " {\n"
         << prefix << "  using this_t = " << name << "<WRAPPED_T>;\n\n";

      os << prefix << "  ----- VARIABLES -----\n";
      for (auto & v : variables) {
        os << prefix << "  " << v.var_type << " " << v.var_name;
        if (v.default_code.size() > 0) os << " = " << v.default_code << "\n";
        else os << "\n";
      }

      os << prefix << "\n  ----- FUNCTIONS -----\n";
      os << prefix << "protected:\n";
      os << prefix << "  // FIRST: Determine the return type for each function.\n";
      for (auto & f : functions) {
        os << prefix << "  template <typename T>"
                     << "  using return_t_" << f.fun_name
                     << " = decltype( std::declval<T>()." << f.fun_name
                     << "( " << f.ParamString() << " );\n";
      }

      os << prefix << "\n  // SECOND: Determine if each function exists in wrapped class.\n";
      os << prefix << "public:\n";
      for (auto & f : functions) {
        os << prefix << "  static constexpr bool HasFun_" << f.fun_name << "() {\n"
           << prefix << "    return emp::test_type<return_t_" << f.fun_name << ", WRAPPED_T>();\n"
           << prefix << "  }\n";
      }

      os << prefix << "\n  // THIRD: Call the functions, redirecting as needed\n";
      for (auto & f : functions) {
        os << prefix << "  " << f.return_type << " " << f.fun_name << "(" << f.ParamString() << ") "
                     << f.AttributeString() << " {\n"
           << prefix << "    " << "static_assert( HasFun_" << f.fun_name
                     << "(), \"\\n\\n  ** Error: concept instance missing required function "
                     << f.fun_name << " **\\n\";"
           << prefix << "  if constexpr (HasFun_" << f.fun_name << "()) {\n"
           << prefix << "    ";
        if (f.return_type != "void") os << "return ";
        os << "WRAPPED_T::" << f.fun_name << "( " << f.ArgString() << " );\n"
          << prefix << "  }\n";
      }


      os << prefix << "};\n\n";
    }
  };
