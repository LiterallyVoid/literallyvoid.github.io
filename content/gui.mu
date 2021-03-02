> header
  > breadcrumbs
    [Home](/) »

  > title
    How the GUI system works
    > published
      2021-02-11

# Elements

Any inline element found in the tree, without a current inline context, creates an "inline context" which all inline descendants (and itself) insert boxes into.
Each text element can then query the inline context to see if text can fit or must be wrapped.

Box element layouts are influenced by their parents in one way: the size hint.
If the size hint is -1 in either axis, that means that the element should have the natural (minimum) size in that axis; otherwise, the element should be exactly sized to the size hint in that axis.

This system allows users to do 90% of what HTML/CSS can do, with 1% of the code and a single pass. No O(n\<sup\>2\<\/sup\>) here!

After all children have been laid out (and sized), the parent element should move them to the correct places.

Pseudocode for the `flex` element:

> code:c
  float total_along = 0;
  float total_flex_grow = 0;

  box[children.len] boxes;

  // layout all minimum-size flex elements
  for i, child in children {
      if child.flex_grow != -1 {
          total_flex_grow += child.flex_grow;
          continue;
      }

      vec2 child_size_hint = {
          [primary_axis] = -1,
          [secondary_axis] = size_hint[secondary_axis],
      };
      boxes[i] = child.layout(size_hint = child_size_hint);
  }

  // layout all growing flex elements
  float extra_space = size_hint[primary_axis] - total_along;
  for i, child in children {
      if child.flex_grow == -1 {
          continue;
      }

      vec2 child_size_hint = {
          [primary_axis] = extra_space * (child.flex_grow / total_flex_grow),
          [secondary_axis] = size_hint[secondary_axis],
      };
      boxes[i] = child.layout(size_hint = child_size_hint);
  }

  // move elements to the correct place
  float current = 0;
  for i, _ in children {
      boxes[i].offset = {
          0, 0,
          [primary_axis] = current,
      };
      current += boxes[i].size[primary_axis];
  }
