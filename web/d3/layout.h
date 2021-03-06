#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include "d3_init.h"
#include "dataset.h"
#include "selection.h"
#include "svg_shapes.h"

#include "../../tools/tuple_struct.h"
#include "../../web/JSWrap.h"

#include <functional>
#include <array>

namespace D3{

  class Layout : public D3_Base {
  protected:
    Layout(int id) : D3_Base(id){;};
    Layout() {;};
  };

  struct JSONTreeNode {
    EMP_BUILD_INTROSPECTIVE_TUPLE( double, x,
				   int, name,
				   int, parent,
                   double, y,
                   int, depth
				   )
  };

  /// A TreeLayout can be used to visualize hierarchical data as a tree (a series of edges
  /// connecting parent and child nodes).
  ///
  /// Since hierarchical data is much more pleasant to store in
  /// JSON format than anything C++ can offer, the TreeLayout expects your data to be stored in a
  /// D3::JSONDataset. Each node is expected to have, at a minimum, the following values:
  /// @param name - a name that uniquely identifies a single node
  /// @param parent - the name of this node's parent (each node is expected to have exactly one
  /// parent, unless it is the root, in which case the parent should be "null")
  /// @param children - an array containing all of the node's children (yes, the nesting gets
  /// intense).
  ///
  /// Calculating the tree layout will automatically create three additional values for each node:
  /// @param x - the x coordinate of the node
  /// @param y - the y coordinate
  /// @param depth - the depth of the node in the tree
  ///
  /// You can include any additional parameters that you want to use to store data.
  /// The dataset is expected to be an array containing one element: the root node object, which in
  /// turn has the other nodes nested inside it.
  /// You must provide a dataset to the TreeLayout constructor.
  ///
  /// A TreeLayout must be templated off of a type that describes all of the values that a node
  /// contains (or at least the ones you care about using from C++, as well as x and y). This
  /// allows nodes to be passed back up to C++ without C++ throwing a fit about types. If you
  /// don't need access to any data other than name, parent, x, y, and depth from C++, you can
  /// use the default, JSONTreeNode.
  ///
  /// If you need access to additional data, you can build structs to template TreeLayouts off
  /// with Empirical Introspective Tuple Structs (see JSONTreeNode as an example). Don't be scared
  /// by the complicated sounding name - you just need to list the names and types of values you
  /// care about.
  template <typename NODE_TYPE = JSONTreeNode>
  class TreeLayout : public Layout {
  protected:

  public:
    /// Pointer to the data - must be in hierarchical JSON format
    JSONDataset * data;

    /// Function used to make the lines for the edges in the tree
    DiagonalGenerator * make_line;

    /// Constructor - handles creating a default DiagonalGenerator and links the specified dataset
    /// up to this object's data pointer.
    TreeLayout(JSONDataset * dataset){
        //Create layout object
        EM_ASM_ARGS({js.objects[$0] = d3.layout.tree();}, this->id);

        make_line = new D3::DiagonalGenerator();

        std::function<std::array<double, 2>(NODE_TYPE, int, int)> projection = [](NODE_TYPE n, int i, int k){
          return std::array<double, 2>({n.y(), n.x()});
        };

        emp::JSWrap(projection, "projection");
        make_line->SetProjection("projection");
        SetDataset(dataset);
    };

    /// Default constructor - if you use this you need connect a dataset with SetDataset
    TreeLayout(){
        //Create layout object
        EM_ASM_ARGS({js.objects[$0] = d3.layout.tree();}, this->id);

        make_line = new D3::DiagonalGenerator();

        std::function<std::array<double, 2>(NODE_TYPE, int, int)> projection = [](NODE_TYPE n, int i, int k){
          return std::array<double, 2>({{n.y(), n.x()}});
        };

        emp::JSWrap(projection, "projection");
        make_line->SetProjection("projection");
    };


    /// Change this TreeLayout's data to [dataset]
    void SetDataset(JSONDataset * dataset) {
      this->data = dataset;
    }

    /// This function does the heavy lifting of visualizing your data. It generates nodes and links
    /// between them based on this object's dataset. [svg] must be a selection containing a single
    /// svg element on which to draw the the visualization.
    ///
    /// In case you want to further customize the tree, this method returns an array of selections,
    /// containing: the enter selection for nodes (i.e. a selection containing all nodes that were
    /// just added to the tree), the exit selection for nodes (i.e. a selection containing any
    /// nodes that are currently drawn but are no longer in the dataset), the enter selection for
    /// links, and the exit selection for links.
    //Returns the enter selection for the nodes in case the user wants
    //to do more with it. It would be nice to return the enter selection for
    //links too, but C++ makes that super cumbersome, and it's definitely the less
    //common use case
    std::array<Selection, 4> GenerateNodesAndLinks(Selection svg) {
      int node_enter = EM_ASM_INT_V({return js.objects.length});
      int node_exit = node_enter + 1;
      int link_enter = node_exit + 1;
      int link_exit = link_enter + 1;

      EM_ASM_ARGS({

        // Based on code from http://www.d3noob.org/2014/01/tree-diagrams-in-d3js_11.html
        var nodes = js.objects[$0].nodes(js.objects[$1][0]).reverse();
        links = js.objects[$0].links(nodes);

        nodes.forEach(function(d) { d.y = d.depth * 20; });

        // Declare the nodesâ€¦
        var node = js.objects[$3].selectAll("g.node")
            .data(nodes, function(d) { return d.name; });

        var nodeExit = node.exit();
        var nodeEnter = node.enter().append("g")
                .attr("class", "node")
                .attr("transform", function(d) {
                    return "translate(" + d.y + "," + d.x + ")"; });

        node.attr("transform", function(d) {
        		  return "translate(" + d.y + "," + d.x + ")"; });

        var link = js.objects[$3].selectAll("path.link")
      	  .data(links, function(d) { return d.target.name; });

        var linkExit = link.exit();
        // Enter the links.
        var linkEnter = link.enter().insert("path", "g")
      	    .attr("class", "link")
      	    .attr("d", js.objects[$2])
            .attr("fill", "none")
            .attr("stroke", "black")
            .attr("stroke-width", 1);

        link.attr("class", "link")
            .attr("d", js.objects[$2]);

        js.objects.push(nodeEnter);
        js.objects.push(nodeExit);
        js.objects.push(linkEnter);
        js.objects.push(linkExit);
      }, this->id, data->GetID(), make_line->GetID(), svg.GetID());
      return std::array<Selection, 4>({{Selection(node_enter), Selection(node_exit),
                                        Selection(link_enter), Selection(link_exit)}});
    }


    /// Set the width of the tree area to [w] and the height to [h]
    void SetSize(int w, int h) {
      EM_ASM_ARGS({js.objects[$0].size([$1,$2]);}, this->id, w, h);
    }

  };
}


#endif
