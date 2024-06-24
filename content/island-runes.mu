> header
  > breadcrumbs
    [Home](/) »
  > title
    Island Runes
    > published
      2024-06-23

I made a game again! This time, the catalyst was seeing someone mention [Godot Wild Jam #70](https://itch.io/jam/godot-wild-jam-70) was going on, a mere two days before the deadline. 

Forty-eight hours later, I had a game. Godot Wild Jam runs for nine days — two weekends — and I overscoped just barely past the single weekend I had.

[Go play it on itch.io!](https://literallyvoid.itch.io/island-runes) It'll probably take about five minutes to complete.

As per [my last blog post](/ld55#one-possible-future), I wanted to make a more narrative-focused game. By some measure, I succeeded! There's a box that people (okay, person) talk from!

# Godot Papercuts

By default, all text is just a little blurry. MSDF does make this better, but it seemed to round off corners when I tried it.

`MainLoop.NOTIFICATION_APPLICATION_FOCUS_OUT` doesn't seem to work with web exports.

Godot doesn't make it easy to reduce redundancy. In fact, Godot makes it really easy to make a big ball of redundant state.

A `Control` can change the mouse cursor for itself only. But if you have a `Node2D` that's hoverable, you don't have access to Godot's infrastructure anymore, and you have to reach into global state (`set_default_cursor_shape`).

If you modify a model's hierarchy in Blender, all Godot nodes that were parented into that hierarchy lose their saved transforms, and go back to the origin.

# Takeaways

This time, to abide by the jam's rules, I used [Godot](https://godotengine.org/). (I will not be elaborating on its pronounciation.)

# Tricks

Smashing a pixelated noise texture over everything was frighteningly effective.

# Case Study: The Interact Key

\<kbd\>E\</kbd\> is the interact key. Pressing E can either advance the currently active dialogue, or open an in-world "puzzleboard".

I didn't have a global system to redirect input. The /dialogue script/ (running on the dialogue box itself) intercepted the interact key whenever a dialogue was running, but there was no way to see if an interact keypress /would/ intercepted there. As it is, the crosshair changes every time you look at an interactable object, even if the interact key would be captured by the dialogue script.

# Dialogue

Here's a sample of the game's introduction:

> code:c
  @radio_up
  [radio] COORDINATES: 999999° 59′59″N, 999999° 59′59″E
  [radio] INCOMING CALL FROM:  [Maya Exeter]
  [other] Is this thing on? [...]

> code:acre
  func speaker_reset() -> void:
  	speaker.text = "Unspecified"
  	speaker.modulate = Color(1, 1, 1, 1)


The dialogue system ran 

The world was modelled in Blender.
