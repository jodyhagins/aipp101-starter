# Atlas Strong Types -- Reference for AI Assistants

This document teaches AI coding assistants how to **define** and **use** Atlas strong types correctly.
Atlas has no public training data, so you must rely on this reference.

## Core Concept

Atlas generates C++ structs that wrap a single underlying value.
The wrapper is **opaque by default** -- no implicit conversions, no inherited operators.
Everything must be explicitly opted in.

```
auto_hash=true

[struct app::UserId]
description=int; ==, <=>
```

This generates a struct that wraps `int`, supports `==`, all relational comparisons (via `<=>`), and `std::hash` (via file-level `auto_hash`).
Nothing else.
You cannot add two `UserId` values, print them, or convert them to `int` unless you opt in.

---

## How to Define Types

### In a `.atlas` file (most common)

```
# comments start with #

[struct namespace::TypeName]
description=underlying_type; feature1, feature2, ...
```

Or the long form:
```
[type]
kind=struct
namespace=my_ns
name=MyType
description=underlying_type; feature1, feature2, ...
```

### In CMakeLists.txt

```cmake
# Single type
atlas_add_type(UserId int "==, <=>" TARGET my_lib)

# Multiple types from file
add_atlas_strong_types_from_file(
    INPUT types.atlas
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/types_gen.hpp
    TARGET my_lib)

# Multiple types inline
add_atlas_strong_types_inline(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/types_gen.hpp
    TARGET my_lib
    CONTENT "
auto_hash=true

[struct app::UserId]
description=int; ==, <=>

[struct app::SessionId]
description=int; ==, <=>
")
```

---

## Feature Reference (What to Put After the Semicolon)

### Operators

| Feature | What it enables |
|---------|----------------|
| `+`, `-`, `*`, `/`, `%` | Arithmetic binary + compound assignment |
| `+*`, `-*` | Shorthand: `+*` = `+` + `u+`; `-*` = `-` + `u-` (binary + unary) |
| `&`, `\|`, `^`, `<<`, `>>` | Bitwise binary + compound assignment |
| `u+`, `u-`, `u~`, `~` | Unary operators |
| `++`, `--` | Pre and post increment/decrement |
| `==`, `!=` | Equality |
| `<`, `<=`, `>`, `>=` | Relational |
| `<=>` | Spaceship (generates all 6 comparisons; works on C++11-23) |
| `!`, `not`, `&&`, `and`, `\|\|`, `or` | Logical |

### Access

| Feature | What it enables |
|---------|----------------|
| `->` | `operator->` pointer-like member access |
| `@` | `operator*` dereference |
| `()` | Nullary `operator()` returning the value |
| `(&)` | `operator()` forwarding to callable underlying |
| `[]` | `operator[]` subscript |
| `&of` | `operator&` returning address of underlying value |
| `bool` | Explicit `operator bool` |

### I/O, Hashing, Formatting

| Feature | What it enables |
|---------|----------------|
| `out` | **Deprecated.** Use `auto_ostream=true` at file level instead. |
| `in` | **Deprecated.** Use `auto_istream=true` at file level instead. |
| `hash` | **Deprecated.** Use `auto_hash=true` at file level instead. |
| `fmt` | **Deprecated.** Use `auto_format=true` at file level instead. |

> **Note:** Per-type `out`, `in`, `hash`, and `fmt` tokens still parse and still enable the corresponding `auto_*` preamble for the entire file, but they are deprecated because the effect is file-wide (not per-type) and the `auto_*` directives make this explicit.

### Construction and Conversion

| Feature | What it enables |
|---------|----------------|
| `from_atlas_cast` | Constructor accepting other atlas types with compatible underlying type |
| `cast<Type>` | Explicit `operator Type()` |
| `implicit_cast<Type>` | Implicit `operator Type()` (use sparingly) |
| `assign` | Template assignment from compatible types |

### Control

| Feature | What it enables |
|---------|----------------|
| `no-constexpr` | Remove constexpr from all operations |
| `no-constexpr-hash` | Remove constexpr from hash only |
| `#<header>` | Add explicit `#include <header>` |
| `#"header"` or `#'header'` | Add explicit `#include "header"` (single quotes are converted to double) |
| `iterable` | Member `begin()`/`end()` for range-for |

### Member Function Forwarding

```
forward=size,empty,clear               # inline syntax
forward=size:length,empty:is_empty     # with aliasing
forward=const,size,empty               # const-only
forward=substr->TypeName               # return type transformation
forward=substr:sub->TypeName           # alias + return type transformation
```

Can be in the description line or on a separate `forward=` line.

The `->ReturnType` syntax wraps the forwarded function's return value in the specified type.
This is useful when the underlying member function returns the underlying type but you want the strong type back:

```
[struct test::Username]
description=std::string; ==
forward=substr->Username
```

```cpp
Username u{"hello_world"};
Username sub = u.substr(0, 5);   // returns Username{"hello"}, not std::string
```

### Arithmetic Modes

| Mode | Behavior |
|------|----------|
| `checked` | Throws `atlas::OverflowError` on overflow |
| `saturating` | Clamps to type limits |
| `wrapping` | Two's complement wrap-around |

### Constraints

| Constraint | Meaning |
|------------|---------|
| `positive` | value > 0 |
| `non_negative` | value >= 0 |
| `non_zero` | value != 0 |
| `bounded<Min,Max>` | Min <= value <= Max |
| `bounded_range<Min,Max>` | Min <= value < Max |
| `non_empty` | !value.empty() |
| `non_null` | value != nullptr |

### File-Level Auto-Enable Directives

These are set at the **top** of a `.atlas` file (or via command-line flags) and apply to **all** types in that file.
They eliminate the need to add `hash`, `out`, `in`, or `fmt` to every individual type description.

| Directive | Effect |
|-----------|--------|
| `auto_hash=true` | Generates a C++20 concept-constrained `std::hash<T>` partial specialization for all atlas types |
| `auto_ostream=true` | Generates a SFINAE-constrained `operator<<` for all atlas types |
| `auto_istream=true` | Generates a SFINAE-constrained `operator>>` for all atlas types |
| `auto_format=true` | Generates a C++20 concept-constrained `std::formatter<T>` for all atlas types (requires `__cpp_lib_format`) |
| `auto_from_atlas_cast=true` | Adds `from_atlas_cast` constructor to all types (cross-type construction) |

Example:
```
auto_hash=true
auto_ostream=true

[struct app::UserId]
description=int; ==, <=>

[struct app::SessionId]
description=int; ==, <=>
```

Both types automatically get `std::hash` and `operator<<` without listing `hash` or `out` in their descriptions.

### Named Constants

```
[struct math::Distance]
description=double; +, -, <=>
constants=zero:0.0; infinity:std::numeric_limits<double>::infinity()
```

Generates `static constexpr Distance zero = Distance(0.0);` etc.

### Default Values

```
[struct app::UserId]
description=int; ==, <=>
default_value=-1
```

---

## How to Use Strong Types in Code

### Construction

```cpp
// Explicit construction only (no implicit conversions)
app::UserId id{42};
app::UserId id2{};           // default constructed (value = 0, or default_value)
app::UserId id3 = app::UserId{42};   // OK
// app::UserId id4 = 42;     // ERROR: no implicit conversion
```

### Comparison and Arithmetic (when declared)

```cpp
// Given: description=int; +, -, <=>
math::Distance a{10}, b{3};
auto c = a + b;              // Distance{13}
a -= b;                      // Distance{7}
bool ok = a > b;             // true
```

### Stream I/O (when enabled)

```cpp
// Given: auto_ostream=true at file level
std::cout << id;             // prints the underlying int
```

### Hashing (when enabled)

```cpp
// Given: auto_hash=true at file level
std::unordered_set<app::UserId> ids;
std::unordered_map<app::UserId, std::string> names;
```

### Member Function Forwarding (when declared)

```cpp
// Given: description=std::string; forward=size,empty,const; ==
net::Hostname h{"example.com"};
h.size();                    // calls underlying std::string::size()
h.empty();                   // calls underlying std::string::empty()
```

### Cross-Type Interactions (when declared)

```cpp
// Given interaction: Price * Quantity -> Total
commerce::Price price{9.99};
commerce::Quantity qty{3};
commerce::Total total = price * qty;   // Total{29.97}
```

### Direct Access Operators (when declared)

```cpp
// Given: description=std::shared_ptr<Resource>; ->, @, bool
ResourceHandle h{std::make_shared<Resource>()};
h->method();                 // -> operator
Resource & r = *h;           // @ (deref) operator
if (h) { ... }              // bool conversion
```

---

## How to Access the Underlying Value (Escape Hatches)

Use these **only** at system boundaries (serialization, FFI, legacy interop).
If you find yourself using these in domain logic, **add the missing feature to the type description instead**.

### `atlas::unwrap(x)` -- Remove One Layer

```cpp
// SimpleInt wraps int
// NestedInt wraps SimpleInt
NestedInt n{SimpleInt{42}};

atlas::unwrap(n);            // SimpleInt& (one layer removed)
atlas::unwrap(SimpleInt{42}); // int (by value for rvalues)
```

### `atlas::undress(x)` -- Remove All Layers

```cpp
NestedInt n{SimpleInt{42}};

atlas::undress(n);           // int& (drilled to the bottom)
```

### `atlas::cast<T>(x)` -- Drill to a Specific Type

```cpp
NestedInt n{SimpleInt{42}};

atlas::cast<int>(n);         // int (by value)
atlas::cast<SimpleInt>(n);   // SimpleInt (stopped at first match)
atlas::cast<int &>(n);       // int& (mutable reference)
atlas::cast<int &&>(std::move(n)); // int&& (reference to live storage)
```

### Return Type Rules

| Function | Lvalue source | Rvalue source |
|----------|--------------|---------------|
| `atlas::unwrap` | reference | by value |
| `atlas::undress` | reference | by value |
| `atlas::cast<T>` (value T) | by value | by value |
| `atlas::cast<T&>` | reference | N/A (lvalue only) |
| `atlas::cast<T&&>` | N/A | rvalue reference (safe) |

**Rvalue returns are by value** to prevent dangling.
**Reference-target `cast` is the exception**: the caller explicitly requested a reference.

---

## Common Mistakes AI Assistants Make

### Mistake 1: Using raw types instead of strong types

**Wrong:**
```cpp
void process_order(int user_id, int order_id, double price, int quantity);
```

**Right:**
```cpp
void process_order(UserId user_id, OrderId order_id, Price price, Quantity quantity);
```

Every parameter with semantic meaning needs its own type.

### Mistake 2: Forgetting to declare needed features

**Wrong:** Defining a type then immediately unwrapping it everywhere:
```cpp
// Type has no operators declared
std::cout << atlas::undress(id);                    // unwrapping to print
if (atlas::undress(a) < atlas::undress(b)) { ... }  // unwrapping to compare
```

**Right:** Declare the features on the type:
```
description=int; ==, <=>, out
```
```cpp
std::cout << id;           // out operator handles it
if (a < b) { ... }        // <=> operator handles it
```

### Mistake 3: Using int/double parameters where strong types exist

**Wrong:**
```cpp
Price calculate_total(double price, int qty) {
    return Price{price * qty};
}
```

**Right:**
```cpp
// With interaction: Price * Quantity -> Total
Total calculate_total(Price price, Quantity qty) {
    return price * qty;
}
```

### Mistake 4: Using atlas::undress inside domain logic

**Wrong:**
```cpp
bool is_expensive(Price p) {
    return atlas::undress(p) > 100.0;  // unwrapping for comparison
}
```

**Right:**
```cpp
// Given: description=double; <=>
// And: constants=expensive_threshold:100.0
bool is_expensive(Price p) {
    return p > Price::expensive_threshold;
}
```

### Mistake 5: Not using interactions for cross-type operations

**Wrong:**
```cpp
Total compute(Price p, Quantity q) {
    return Total{atlas::undress(p) * atlas::undress(q)};
}
```

**Right:**
```cpp
// Given interaction: Price * Quantity -> Total
Total compute(Price p, Quantity q) {
    return p * q;
}
```

### Mistake 6: Defining a type with no features at all

A type with zero features is useless -- you can construct it but do nothing with it.
At minimum, most types need `==` and often `<=>` and `hash`:

```
description=int; ==, <=>, hash
```

### Mistake 7: Using std::hash without enabling hashing

```cpp
// COMPILE ERROR if hashing not enabled for UserId
std::unordered_set<UserId> ids;
```

Add `auto_hash=true` at file level.

### Mistake 8: Printing without enabling ostream

```cpp
// COMPILE ERROR if ostream not enabled
std::cout << user_id;
```

Add `auto_ostream=true` at file level.

---

## Decision Flowchart

When you need to do something with a strong type value:

1. **Can the type's declared operators handle it?**
   Yes -> Use them directly. Done.

2. **Could it be handled by adding a feature?**
   Yes -> Add the feature (`auto_hash=true`, `auto_ostream=true`, `<=>`, `forward=...`, etc.).

3. **Is this a cross-type operation?**
   Yes -> Define an interaction (`Price * Quantity -> Total`).

4. **Is this at a system boundary (serialization, FFI, logging)?**
   Yes -> Use `atlas::undress(x)` or `atlas::cast<T>(x)`.

5. **Do you need exactly one layer removed?**
   Yes -> Use `atlas::unwrap(x)`.

6. **Do you need a specific intermediate type from a nested wrapper?**
   Yes -> Use `atlas::cast<SpecificType>(x)`.

---

## Complete Example: Defining a Domain Model

### Type definitions (`types.atlas`)

```
guard_prefix=COMMERCE_TYPES
auto_from_atlas_cast=true
auto_hash=true
auto_ostream=true
auto_format=true

[struct commerce::Price]
description=double; +, -, <=>
constants=zero:0.0

[struct commerce::Quantity]
description=int; +, -, <=>
constants=zero:0

[struct commerce::Total]
description=double; +, -, <=>
constants=zero:0.0

[struct commerce::ProductId]
description=int; ==, <=>

[struct commerce::OrderId]
description=int; ==, <=>

[struct commerce::ProductName]
description=std::string; ==, no-constexpr
forward=size,empty,const
```

### Interactions (`interactions.atlas`)

```
include "types_gen.hpp"

namespace=commerce

Price * Quantity -> Total
Total / Price -> Quantity
Total / Quantity -> Price
Quantity * int <-> Quantity
Price * double <-> Price
```

---

## Interactions Reference

Interactions define cross-type operators. They are defined in separate `.atlas` files.

### Supported operators

All of these operators can be used in interactions:

| Category | Operators |
|----------|-----------|
| Arithmetic | `+`, `-`, `*`, `/`, `%` |
| Bitwise | `&`, `\|`, `^`, `<<`, `>>` |
| Comparison | `==`, `!=`, `<`, `<=`, `>`, `>=`, `<=>` |
| Logical | `&&`, `\|\|` |

Compound assignment operators (`+=`, `-=`, `*=`, etc.) are generated automatically for arithmetic and bitwise interactions where the result type matches the LHS type.

### Syntax

```
LhsType op RhsType -> ResultType        # one-directional
LhsType op RhsType <-> ResultType       # commutative (both orderings)
```

### Interaction directives

```
include "types_gen.hpp"             # include header for type definitions
namespace=mylib                     # namespace for all interactions below

value_access=atlas::undress         # default value access for all types (default: atlas::undress)
lhs_value_access=atlas::undress     # override value access for LHS only
rhs_value_access=.getValue()        # override value access for RHS only

constexpr                           # interactions below are constexpr (default)
no-constexpr                        # interactions below are NOT constexpr
```

The `value_access` directives control how the underlying value is extracted from types.
Primitives and std library types are always used directly, regardless of the setting.
Supported forms:
- `atlas::undress` (default) -- function call: `atlas::undress(x)`
- `.value` -- member access: `x.value`
- `()` -- function call operator: `x()`

### Template interactions with type constraints

```
# C++20 concept constraint
concept=std::integral T
T + T -> T

# C++17 SFINAE constraint
enable_if=std::is_floating_point<U>::value
U * U -> U

# Both (auto-selects based on compiler support)
concept=std::integral V
enable_if=std::is_integral_v<V>
V - V -> V
```

### CMakeLists.txt

```cmake
add_atlas_strong_types_from_file(
    INPUT types.atlas
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/types_gen.hpp
    TARGET commerce_lib)

add_atlas_interactions_from_file(
    INPUT interactions.atlas
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/interactions_gen.hpp
    TARGET commerce_lib)
```

### Using the types

```cpp
#include "types_gen.hpp"
#include "interactions_gen.hpp"

commerce::Total calculate_order_total(
    commerce::Price unit_price,
    commerce::Quantity qty)
{
    return unit_price * qty;   // interaction handles this
}

void print_receipt(
    commerce::OrderId order_id,
    commerce::ProductName name,
    commerce::Price price,
    commerce::Quantity qty,
    commerce::Total total)
{
    // out operator on each type -- no unwrapping
    std::cout << "Order " << order_id << ": "
              << qty << "x " << name
              << " @ " << price
              << " = " << total << '\n';
}

// Serialization boundary -- unwrapping is appropriate here
void serialize_to_json(commerce::Price price, JsonWriter & w) {
    w.write("price", atlas::undress(price));
}
```
