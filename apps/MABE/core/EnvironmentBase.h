/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2018
 *
 *  @file  EnvironmentBase.h
 *  @brief Base class for all Environments in MABE
 *
 *  This file details all of the basic functionality that all environments MUST have, providing
 *  reasonable defaults when such are possible.  Environments can describe the surrounding
 *  world that organisms can interact with -or- be a fitness function for use in an evolutionary
 *  algorithm.
 */

#ifndef MABE_ENVIRONMENT_BASE_H
#define MABE_ENVIRONMENT_BASE_H

#include "tools/GenericFunction.h"

#include "ModuleBase.h"

namespace mabe {

  class EnvironmentBase : public ModuleBase {
  private:
    using fun_ptr_t = emp::Ptr<emp::GenericFunction>;

    /// These are functions built by the derived environment that will be called when specific
    /// events are triggered.  To setup these functions (since different organism types will
    /// have different ways of calling them) they are passed to an organism type which builds
    /// a new version of the function that always takes a mabe::Organsm reference and returns
    /// the correct result.  For simplicity, the only return type allowed is double.  Anything
    /// more complex should be handled with a callback using one of the action function in the
    /// next group.
    std::map< std::string, fun_ptr_t > event_fun_map;

    /// These are functions that will be provided to the Organisms in this environment.  The
    /// organisms can call these functions (with the appropriate arguments) in order to
    /// senese or act in their environment.  The only return type allowed is a double; anything
    /// more complex should be handed with a callback using one of the event functions in the
    /// previous group.
    std::map< std::string, fun_ptr_t > action_fun_map;

  public:
    EnvironmentBase(const std::string & in_name) : ModuleBase(in_name) { ; }

    static constexpr mabe::ModuleType GetModuleType() { return ModuleType::ENVIRONMENT; }
  };

}

#endif

