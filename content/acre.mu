> header
  > breadcrumbs
    [Home](/) »

  > title
    Acre

Acre is a language that doesn't exist yet.

# Functions

Functions are defined with the following syntax:

> code:acre
  extern func <name>(<arg>: <type>) -> ();
  func <name>(<arg>: <type>, <arg>: <type>) -> <type> {
      ...body
  }

Making functions and constants use the same syntax is attractive, but that either:
- requires semicolons after function definitions, or
- requires the parser to allow missing semicolons.

# Variables

> code:acre
  def <name> = <value>;
  def <name>: <type>;
  def <name>: <type> = <value>;

  var <name> = <value>;
  var <name>: <type>;
  var <name>: <type> = <value>;

Where `def` is for constants, and `var` is for variables.

# Types

## Built-in types

- `u8`, `u16`, `u32`, `u64`, `usize`: Unsigned integers.
- `i8`, `i16`, `i32`, `i64`, `isize`: Signed integers.
- `f32`, `f64`: Floating-point numbers.
- `opaque`: Unsized type.

## Opaque

The `opaque` type has no size.

Without the `#unique` qualifier, any instance of `opaque` can be casted into any other.

Pointers to `opaque` may exist, but slices cannot.

> code:acre
  def c_void = #unique opaque;

## Unique

The `#unique` qualifier can be used to make a type with the same operations and storage as the underlying type, but without implicit casting, either to the underlying type or to any type that the underlying type could be implicitly cast to.

> code:acre
  def utf8_byte = #unique u8;
  def utf8_codepoint = #unique u32;

## Tuples

Tuples can be types or values.

They're created by putting several values in parentheses, with commas separating them.

When creating a tuple of only one element, a comma is required.

> code:acre
  def Vec0 = ();
  def Vec1 = (f32,);
  def Vec2 = (f32, f32);
  def Vec3 = (f32, f32, f32);

  var my_vec3: Vec3 = (1.0, 0.0, 1.0);

Tuples with the same structure are implicitly casted between, so:

> code:acre
  def Ipv4Address = (u8, u8, u8, u8);
  def FourCC = (u8, u8, u8, u8);

  var address: Ipv4Address = (127, 0, 0, 1);
  var magic: FourCC = (0x7F, 0x45, 0x4c, 0x46);

  address = magic; // whoops!

## Unit

The unit type (spelled `()`, and pronounced `()`) has one possible value: `()`. It takes the place of Rust's `()` type, or C's `void` type.

## References

References behave nearly identically to pointers.
The only difference is that pointers can be implicitly casted to references, but not the other way around.

## Pointers and slices

Slices cannot be used directly; they can only be used in pointer form.

Slices can be created from either arrays or pointers-to-unknown.

> code:acre
  var bytes: [4]u8 = { 0xDE, 0xAD, 0xBE, 0xEF };

  var dead = bytes[0..2];
  // the type of `dead` is [2]u8

  dead[0] = 0xBA;

  // without a reference, the slice operator copies the created slice.
  assert(bytes[0] == 0xDE);

  var reference = &bytes[0..2];
  // the type of `reference` is &[2]u8
  reference[0] = 0xBA;
  assert(bytes[0] == 0xBA); // with a reference, the backing array can be modified.

  // casts from &[<number>]u8 to &[..]u8 are allowed..
  var slice: &[..]u8 = reference;

  // ..and so are casts from &[..]u8 to &[?]u8
  var ptr_unknown_length: &[?]u8 = reference;
  assert(ptr_unknown_length[3] == 0xEF);

  var beef: &[..]u8 = &ptr_unknown_length[2..4];
  assert(beef[0] == 0xBE);
  assert(beef[1] == 0xEF);

  // casting from a slice to an array is allowed, but safety-checked.
  // this is fine:
  var two: &[2]u8 = beef;

  // but this is a panic:
  var three: &[3]u8 = beef;
