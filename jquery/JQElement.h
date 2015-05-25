#ifndef EMP_JQ_ELEMENT_H
#define EMP_JQ_ELEMENT_H

////////////////////////////////////////////////////////////////////////////////////
//
//  A single element on a web page (a paragraph, a table, etc.)
//

#include <emscripten.h>
#include <string>

#include "../tools/assert.h"

namespace emp {

  class JQElement {
  private:
    std::string name;

  public:
    JQElement(const std::string & in_name) : name(in_name) {
      emp_assert(name.size() > 0);  // Make sure the name exists!
      // @CAO ensure the name consists of just alphanumeric chars (plus '_' & '-'?)
    }
    JQElement(const JQElement &) = delete;
    virtual ~JQElement() { ; }
    JQElement & operator=(const JQElement &) = delete;

    const std::string GetName() { return name; }

    virtual void SetText(const std::string & text) {
      EM_ASM_ARGS({
          var element_name = Pointer_stringify($0);
          var new_text = Pointer_stringify($1);
          $( '#' + element_name ).html( new_text );
      }, name.c_str(), text.c_str());
    }
  };

};

#endif
