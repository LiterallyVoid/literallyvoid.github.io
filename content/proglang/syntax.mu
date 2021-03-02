> header
  > breadcrumbs
    [Home](/) » [Acre](/proglang)

  > title
    Acre's Syntax

# Functions

Functions are defined with the following syntax:

> code:acre
  extern func <name>(<arg>: <type>) -> ();
  func <name>(<arg>: <type>, <arg>: <type>) -> <type> {
      ...body
  }

I'd like to use universal constant defining, but this either:
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

- `u8`, `u16`, `u32`, `u64`, `usize`: unsigned integers.
- `i8`, `i16`, `i32`, `i64`, `isize`: signed integers.

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

## Pairs

Pairs can exist of types and values.

The only way to create pairs is through the comma operator, which is right-associative; so the following are all equivalent:

> code:acre
  def MyPair = i32, i32, i64, i64;
  def MyPair = (i32, i32, i64, i64);
  def MyPair = (i32, (i32, (i64, (i64))));

However, this:
> code:acre
  def MyPair = (i32, (i32, i64), i64);

is /not/ equivalent.

Pairs with the same structure are implicitly casted between, so:

> code:acre
  def Ipv4Address = (u8, u8, u8, u8);
  def FourCC = (u8, u8, u8, u8);

  var address: Ipv4Address = (127, 0, 0, 1);
  var magic: FourCC = (0x7F, 0x45, 0x4c, 0x46);

  address = magic; // whoops!

## Pointers and slices

Slices cannot be used directly, they can only be used in pointer form.

Slices can be created from either arrays or pointer-to-unknown.

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
