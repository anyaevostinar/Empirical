/*
 *  This file is part of Empirical, https://github.com/devosoft/Empirical
 *  Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  date: 2021
*/
/**
 *  @file
 *  @brief Interfaces with emp::config objects to provide UI configuration.
 */

#ifndef EMP_PREFAB_CONFIGPANEL_HPP_INCLUDE
#define EMP_PREFAB_CONFIGPANEL_HPP_INCLUDE

#include <set>
#include <stddef.h>

#include "../datastructs/set_utils.hpp"
#include "../tools/string_utils.hpp"

#include "../config/config.hpp"
#include "../web/Div.hpp"
#include "../web/Element.hpp"
#include "../web/Input.hpp"

// Prefab elements
#include "Card.hpp"
#include "ValueBox.hpp"

namespace emp {
namespace prefab {

  namespace internal {
      /**
       * Shared pointer held by instances of ConfigPanel class representing
       * the same conceptual ConfigPanel DOM object.
       * Contains state that should persist while ConfigPanel DOM object
       * persists.
       */
      class ConfigPanelInfo : public web::internal::DivInfo {

      public:
        using on_change_fun_t = std::function<void(const std::string &, const std::string &)>;

      private:
        on_change_fun_t on_change_fun{ [](const std::string & name, const std::string & val) { ; } };

      public:
        /**
         * Construct a shared pointer to manage ConfigPanel state.
         *
         * @param in_id HTML ID of ConfigPanel div
         */
        ConfigPanelInfo( const std::string & in_id="" )
        : web::internal::DivInfo(in_id) { ; }

        /**
         * Get current on-update callback for a ConfigPanel.
         *
         * @return current callback function handle
         */
        on_change_fun_t & GetOnChangeFun() { return on_change_fun; }

        /**
         * Set on-update callback for a ConfigPanel.
         *
         * @param fun callback function handle
         */
        void SetOnChangeFun(const on_change_fun_t & fun) {
          on_change_fun = fun;
        }

      };

  }

  /**
   * Use the ConfigPanel class to easily add a dynamic configuration
   * panel to your web app. Users can interact with the config panel
   * by updating values.
   *
   * The ConfigPanel is constructed using sub-components. Groups of
   * settings are placed in Cards, and individual settings are represented
   * by ValueControls.
   */
  class ConfigPanel : public web::Div {
    public:
      using on_change_fun_t = internal::ConfigPanelInfo::on_change_fun_t;

    private:
      /**
       * Type of shared pointer shared among instances of ConfigPanel
       * representing the same conceptual DOM element.
       */
      using INFO_TYPE = internal::ConfigPanelInfo;

      /**
       * Get shared info pointer, cast to ConfigPanel-specific type.
       *
       * @return cast pointer
       */
      INFO_TYPE * Info() {
        return dynamic_cast<INFO_TYPE *>(info);
      }

      /**
       * Get shared info pointer, cast to const ConfigPanel-specific type.
       *
       * @return cast pointer
       */
      const INFO_TYPE * Info() const {
        return dynamic_cast<INFO_TYPE *>(info);
      }

      inline static std::set<std::string> numeric_types = {"int", "double", "float", "uint32_t", "uint64_t", "size_t"};

      #ifndef DOXYGEN_SHOULD_SKIP_THIS
      // Helper function to get pretty names from config values
      inline static std::function<std::string(const std::string &)> format_label = [](
        const std::string & name
      ) {
        return to_titlecase(join(slice(name, '_'), " "));
      };
      #endif // DOXYGEN_SHOULD_SKIP_THIS

      /**
       * Get current on-update callback.
       *
       * @return current callback function handle
       */
      on_change_fun_t& GetOnChangeFun() {
          return Info()->GetOnChangeFun();
      };

    public:
      /**
       * @param config config object used to construct this panel
       * @param open Should card for displaying this config default to being open?
       * @param div_name Name to use for html div id for this panel
       */
      ConfigPanel(
        Config & config,
        bool open = true,
        const std::string & div_name = ""
      ) {
        info = new internal::ConfigPanelInfo(div_name);
        this->AddAttr("class", "config_main");

        // Reset button redirects to a URL with the current config settings
        web::Element reload_button{"a", emp::to_string(GetID(), "_", "reload")};
        reload_button.SetAttr("class", "btn btn-danger");
        std::stringstream query;
        config.WriteUrlQueryString(query);
        reload_button.SetAttr("href", query.str());
        reload_button << "Reload with changes";

        on_change_fun_t & onChangeRef = GetOnChangeFun();

        for (auto & group : config.GetGroupSet()) {
          const std::string group_name(group->GetName());
          const std::string group_desc(group->GetDesc());

          // Setting groups have IDs generated by "{main id}_{group name}_outer"
          const std::string group_base(emp::to_string(GetID(), "_", group_name, "_outer"));

          Card group_card(open ? "INIT_OPEN" : "INIT_CLOSED", true, group_base);

          group_card.AddHeaderContent(group_desc);
          (*this) << group_card;
          // A div within card helps make grid without messing up collapse properties
          // and has ID "{main id}_{group name}" for ease of access
          Div settings(emp::to_string(GetID(), "_", group_name));
          settings.AddAttr("class", "settings_group");
          group_card << settings;

          for (size_t i = 0; i < group->GetSize(); ++i) {
            auto setting = group->GetEntry(i);
            // Get loads of information from the config setting
            const std::string name(setting->GetName());
            const std::string pretty_name(format_label(name));
            const std::string type(setting->GetType());
            const std::string desc(setting->GetDescription());
            const std::string value(setting->GetValue());

            // Settings have IDs generated by can be accessed via "{main id}_{setting name}"
            const std::string setting_base(emp::to_string(GetID(), "_", name));

            const auto handleChange = [
              name, reload=reload_button, &config, &handleChange = onChangeRef
            ] (const std::string & val) {
              config.Set(name, val);
              // Run the handler function in case user wants to trigger something when the config values
              // change (default does nothing)
              handleChange(name, val);
              // Update the reload button's href
              std::stringstream ss;
              config.WriteUrlQueryString(ss);
              static_cast<web::Div>(reload).SetAttr("href", ss.str());
              // ^ TODO: need the cast a bug with SetAttr doesn't work for Element here, why?
            };

            // Add a different control depending on the config types
            if (Has(numeric_types, type)) {
              settings << NumericValueControl(
                pretty_name, desc, value, type, handleChange, setting_base
              );
            } else if (type == "std::string") {
              settings << TextValueControl(
                pretty_name, desc, value, handleChange, setting_base
              );
            } else if (type == "bool") {
              settings << BoolValueControl(
                pretty_name, desc, emp::from_string<bool>(value), handleChange, setting_base
              );
            } else {
              // If a setting type is unrecognized (e.g. a new type becomes supported in Config.hpp)
              // just display its value (should we add a warning to the description?)
              settings << ValueDisplay(pretty_name, desc, value, setting_base);
            }
          }
        }

        // A div at the end for controls
        Div controls{emp::to_string(GetID(), "_", "controls")};
        controls.AddAttr("class", "config_controls");
        (*this) << controls;

        controls << reload_button;
      }

      /**
       * Sets on-update callback for a ConfigPanel.
       *
       * @param fun callback function handle
       */
      void SetOnChangeFun(const on_change_fun_t& fun) {
        Info()->SetOnChangeFun(fun);
      }

      /**
       * Sets the range of a slider for a numeric setting.
       *
       * @param setting the numeric config value which will have its range slider updated
       * @param min minimum value of the slider for this config value (use "DEFAULT"
       * to leave unchanged)
       * @param max maximum value of the slider for this config value (use "DEFAULT"
       * to leave unchanged)
       * @param step step size of the slider for this config value (use "DEFAULT"
       * to leave unchanged)
       */
      void SetRange(
        const std::string & setting,
        const std::string & min,
        const std::string & max = "DEFAULT",
        const std::string & step = "DEFAULT"
      ) {
        const std::string target_id{emp::to_string(GetID(), "_", setting, "_view")};
        if (this->HasChild(target_id)) {
          Div target(this->Find(target_id));
          if (target.Children()[0].IsInput()) {
            web::Input slider{target.Children()[0]};
            if (slider.GetType() == "range") {
              if (min != "DEFAULT") {
                slider.Min(min);
              }
              if (max != "DEFAULT") {
                slider.Max(max);
              }
              if (step != "DEFAULT") {
                slider.Step(step);
              }
            }
          }
        }
      }

      /**
       * Excludes a setting or group of settings, recommend using ExcludeSetting
       * or ExcludeGroup instead
       *
       * @param setting The name of a single setting that should not be
       * displayed in the config panel
       * @deprecated renamed to ExcludeSetting
       */
      [[deprecated("Use 'ExcludeSetting' to remove a single config parameter.")]]
      void ExcludeConfig(const std::string & setting) {
        ExcludeSetting(setting);
      }

      /**
       * Excludes a specific setting from the config panel
       *
       * @param setting name of the setting that should not be
       * displayed in the config panel
       */
      void ExcludeSetting(const std::string & setting) {
        const std::string target_id(emp::to_string(GetID(), "_", setting));
        if (this->HasChild(target_id)) {
          Div target(this->Find(target_id));
          target.AddAttr("class", "excluded");
        }
      }

      /**
       * Excludes an entire group of settings from the config panel
       *
       * @param setting_group name of the group that should not be
       * displayed in the config panel
       */
      void ExcludeGroup(const std::string & setting_group) {
        const std::string target_id(emp::to_string(GetID(), "_", setting_group, "_outer"));
        if (this->HasChild(target_id)) {
          Div target(this->Find(target_id));
          target.AddAttr("class", "excluded");
        }
      }

      /**
       * Arranges config panel based configuration pass to constructor
       * @param config the config object used to create this panel
       * @param open should the card for the panel start open?
       * @param id_prefix string appended to id for each setting (unused)
       * @deprecated No longer necessary for config panel to function.
       * This function was a work around to fix a bug.
       */
      [[deprecated("Prefer construction of ConfigPanel after config values have been set.")]]
      void Setup(Config & config, bool open = true, const std::string & id_prefix = "") {
        // Setup should no longer be used since it was a work-around to a bug,
        // but if called will reinitialize the component by just making a new one
        // to mimic earlier behavior
        *this = ConfigPanel(config, open, id_prefix);
      }

      /** @return Div containing the entire config panel
       *  @deprecated Can directly stream this component
       */
      [[deprecated("Can directly stream this component into another.")]]
      web::Div & GetConfigPanelDiv() { return (*this); }

  };
}
}

#endif // #ifndef EMP_PREFAB_CONFIGPANEL_HPP_INCLUDE
