> header
  > breadcrumbs
    [Home](/) »
  > title
    Ludum Dare 55 post-mortem
    > published
      2024-05-04

I made a game (shocker!). It placed 169\<sup\>th\</sup\> overall from a field of 462 other games submitted to The Compo, a forty-eight hour mad dash to make a game solo. You can play it from [the game's itch.io page](https://literallyvoid.itch.io/ludum-dare-55), tell me where I went wrong on [the game's Ludum Dare page](https://ldjam.com/events/ludum-dare/55/triage-and-motion), or view [the source code on GitHub](https://github.com/LiterallyVoid/ludum-dare-55/).

Because I \<del\>love to subvert expectations\</del\>ran out of ideas, I ended up making a game that had, at best, passing reference to the theme. I turned off theme voting, but in hindsight I should've kept that on—see if I'd get a new world record for lastplace%.

# Pre-jam

Before the jam started, I wrote code to play background music, with rudimentary crossfade support. I didn't end up making (let alone publishing) more than one track, so this code has never been used, and frankly is more likely to catch fire than succeed. To test this out, I made some placeholder music—and I never actually got around to removing it, so it's still just /there/ in the bundle I uploaded to [itch.io](https://itch.io).

Most of the idea (the title: /Triage and Motion/, the theme: multiple side-by-side tower defense arenas) had already been running around in my head before the jam started, along with a single theme: /economy/. In the hours that followed the theme announcement, when all of my creativity was replaced with writer's block, I made the difficult (easy) decision to simply ignore the prompt and accept the rating hit.

# Programming

Making a game that runs in the browser is such a superpower.

I wrote the game in plain JavaScript. One cool pattern I used was /configuration objects/ (it's my blog so I get to name the thing, alright‽):

> code:c
  /* foo */ bar
  const buildables = {
  	repeater: {
  		cls: RepeaterTurret,

  		sound: sound.load("sounds/repeater.mp3"),

  		hurtbox: img("assets/damageprojline.svg"),
  		image: img("assets/turret-repeater.svg"),
  		image_bullet: img("assets/repeater-bullet.svg"),

  		rotatable: true,

  		// [...]

I only used this to define the different kinds of turrets and even then I ended up using it mostly as a way of storing references to long-lived sound and image objects. Here, `rotatable` is only used by the building placement overlay, which ignores rotation inputs if a turret isn't rotatable. `cls` (a JavaScript `class` constructor!) is instantiated, then spawned into the board when the turret's placed. When the turret fires a bullet the entire configuration object is also passed into the bullet, which can then read the `image_bullet` property to draw itself.

Even with how little I used it, this pattern really punched above its weight. This is probably helped a ton by the game being written in JavaScript, where heterogeneous dictionaries are table stakes.

For the core update loop I ended up writing an ad-hoc node-tree-alike hierarchy, with a downwards traversal for events and game logic followed by an upwards traversal for rendering. Anything that was drawable had a global, screen-space `pos` field, which was manually updated by parent nodes before calling `update` on their children in turn. Here, an /actual/ node tree (instead of an ad-hoc hierarchy) would've been great.

The randomly-placed mountains(?) used some form of the [flyweight pattern](https://gameprogrammingpatterns.com/flyweight.html). This was also a bad move, and actual instantiated entities would've allowed me to randomly flip and/or rotate these objects to add some much-needed variation.

# Periphery

I didn't sink all of my free time into the game. There were entire hours—/hours!/—when I wasn't working on the game. This was a good thing.

I've submitted a game to Ludum Dare four times, and each time I've flipped the order of the "HTML5" and "Source Code" links. I don't know, maybe I'll keep up the tradition 🙂

# Audio 🎵

When I listen to the music I don't get a visceral feeling of embarrasment, and that's certainly an encouraging sign. And when I'm playing the game the music just fades into the background, which is perhaps the greatest compliment any music could get. (While writing this, I was listening to the music on repeat and it's just... /pleasant./)

One thing that still catches me out about making music is how repetitive it can be before anyone notices. Like, okay, I realize that repetition is music's whole thing, I get that, but there's only like six dang patterns in the soundtrack and I can't even notice! Although maybe this is a me thing.

![The pattern overview for /Triage and Motion/'s soundtrack](./ld55/music.png)

I didn't want to use any samples for the music, so I had to make (read: bash together) all the instruments myself.

The beat was pretty simple: I used LMMS's built-in Kicker instrument for the kick drum, and for the hi-hat I used LMMS's Triple Oscillator with one oscillator set to noise and the others off, filtered through a high-resonance lowpass filter to bring through one high note. The same technique goes for the riser (downer?), which had an ADSR on its cutoff frequency, closing the lowpass over the note. I pretty much just placed kicks and hi-hats randomly in the Beat+Bassline editor until it no longer sounded like some sort of demonic diesel engine failing to start.

The main melody and bass were created using LMMS's "BitInvader" wavetable synthesizer by drawing some messed-up shapes, enabling interpolation, clicking the "S" (for Smooth) button a bunch of times, adding enough effects to drown out my tears, and then adding some ADSR to taste.

I also made all the sound effects in LMMS! Like the music, these were all combinations of various kick drums and wavetables with note-level frequency sweeps. Source files for the music and the sound effects are available on [the GitHub page for /Triage and Motion/](https://github.com/LiterallyVoid/ludum-dare-55/).

# Game design

It was a last-minute decision (as much as any decision durnig a 48-hour game jam can be last-minute) to add an arbitrary level limit, but I'm glad I did. Initially, I was "planning" (taking the path of least resistance) to make an infinite score-chasing game (which is reflected by the "levels cleared" counter being called `score` internally), like all the other games I've made for Ludum Dare.

On difficulty: I can just barely get to the win screen by the skin of my teeth, with maybe 30-40 levels cleared. I'm still not sure if this is too easy or too difficult—it's really easy to lose turrets early in the game, which is pretty much an instant loss.

It was a brilliant idea (and I'm giving myself several awards for this, mind you! They're free!) to start out with only three of five lives, so that on the first easy levels the player learns that clearing a level is worth a token.

And while we're on the topic of user experience, another quick win was to animate turrets flying back towards the turret bar (called the palette internally, and the bottom bar in-game; I was running on entirely not enough sleep, so I'm giving myself a little slack for that one.) when a level's cleared. And to clarify that turrets are lost when the arena is, I also gave the procedural levels some procedural broken turrets. I don't know if this was effective, but hey, at least it was easy.

Every level (or arena, depending on which part of the code) is procedurally generated. The high-level feel of each arena is controlled by two parameters: the track temperature (how much it moves horizontally as it goes down the level), and how much of the empty area around the track is blocked by mountains. This was an easy way to mitigate [the 10,000 Bowls of Oatmeal problem](https://galaxykate0.tumblr.com/post/139774965871/so-you-want-to-build-a-generator).

There were a couple of last-minute additions: the mortar turret which doesn't require line-of-sight to its target, and the grunt enemy which is very large and has a lot of health. Game balance was very much a guess-and-check, and I made several surprise nerfs to make the game harder right before release (/definitely/ not rushed in during submission hour—the commit timestamps are lying and why are you trying to prove me right–erm, wrong anyway 🙂)

The optimal way to play the game is to place as many turrets as possible, as fast as possible, to spread out damage across your turrets. It turns out that (I say, having no idea how other people played the game) this isn't signposted very well, and a pinch more Game Design here probably would've helped a lot.

# Somehow, typography

For the menus, I used [Inter](https://rsms.me/inter/)—/the one true sans font!/—with a fallback to the system `sans-serif` font. But I never actually got around to bundling Inter, so only systems that have Inter already installed will see that font! For any canvas text, I just used `sans` instead, which happened to not work in Safari so the first hotfix was to change every place that set `ctx.font` to use `sans-serif` instead.

The cover was made using [Rubik](https://fonts.google.com/specimen/Rubik), an awesome presentation font which admittedly just carries the whole thing. Speaking of:

# \<del\>Marketing\</del\> Cover art

The first obstacle in the marketing funnel™ is the cover art, so I took about a half hour to make this cover in Inkscape:

![/Triage and Motion/'s cover art](./ld55/capsule.svg)

The main consideration here was not grossly misrepresenting a tower defense game. The name "Triage and Motion" is quite frankly rubbish at suggesting the tower defense genre, so the heavy lifting had to be found elsewhere.

## One Possible Future

I really want to branch out into any genre other than "kill or be killed". Like, it's a nice and easy positive feedback loop, but if you've seen one you've seen them all. And I'd like to make more story-driven games: it's genuinely uncanny how overpowered storytelling is, when you sit down and think about it.

Every game I've made for Ludum Dare has been the content-light, game-design-heavy sort of game, another habit I'd love to break. Maybe next time I'll do an open-world walking simulator, who knows? Not me, certainly! Hah, do you really think I'm planning ahead? Ever?

