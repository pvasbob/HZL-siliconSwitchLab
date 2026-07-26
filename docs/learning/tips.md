namespace {

[[nodiscard]] constexpr std::optional<std::uint8_t>
hex_digit_value(const char character) noexcept {
    if (character >= '0' && character <= '9') {
        return static_cast<std::uint8_t>(character - '0');
    }

    if (character >= 'a' && character <= 'f') {
        return static_cast<std::uint8_t>(character - 'a' + 10);
    }

    if (character >= 'A' && character <= 'F') {
        return static_cast<std::uint8_t>(character - 'A' + 10);
    }

    return std::nullopt;
}
}

A namespace-scope static function would be valid here. We use an anonymous namespace because it is the modern C++ convention and can consistently group all private implementation declarations in the .cpp file.


=========================

How would you restrict a helper to one function?
If a helper should only be accessible within one function, make it a local lambda:
std::optional<MacAddress> MacAddress::parse(
    const std::string_view text) noexcept {

    const auto hex_digit_value =
        [](const char character) -> std::optional<std::uint8_t> {
            // ...
        };

    const auto value = hex_digit_value('A');
}
Now the lambda variable’s scope is limited to parse():
Inside parse()  → accessible
Outside parse() → name does not exist
C++ does not allow defining an ordinary named function inside another function, so a lambda is the usual local callable.

=========================

Corrected rule
Namespace-scope static restricts linkage between translation units. It does not restrict calls to code textually inside the namespace braces.

To restrict a callable to one function’s scope, use a local lambda. To restrict access to a class, use a private member.

=========================

NRVO means Named Return Value Optimization.

Without NRVO:
Yes, the local vec_int object is destroyed when the function ends, but its owned dynamic storage has first been moved into the return object.

With NRVO:
There is no separate local and returned vector. They are treated as one object, which is destroyed later in the caller’s scope.

==========================

Your concern would be correct for certain non-inline definitions placed carelessly in a header.
But test_support.hpp is safe because:
#pragma once prevents repeated processing within one translation unit.
Class definitions may appear identically across translation units.
Functions defined inside the class are implicitly inline.
Template definitions are expected to be visible in headers.
Therefore:
Including test_support.hpp inside mac_address_test.hpp would be legal and would not create multiple-definition errors. We use a forward declaration to reduce unnecessary header dependency, not to prevent an ODR violation.


A normal non-inline, non-template function with external linkage must have exactly one definition in the program.

An inline function may have identical definitions in multiple translation units.
Worker::do_work() is implicitly inline because it is defined inside the class definition.
A constexpr function is implicitly inline.
A function template is normally defined in a heade.
    Its definition must generally be visible where the compiler instantiates it.


Multiple identical definitions across translation units are permitted for categories including:
Class types
Enumeration types
Inline functions
Inline variables
Templated entities
constexpr functions, because they are implicitly inline
Functions defined inside class definitions, because they are implicitly inline

| Header definition | Allowed across translation units? |
|---|---|
| Ordinary non-inline function | No |
| `inline` function | Yes, if ODR requirements are satisfied |
| Function defined inside class | Yes; implicitly inline |
| `constexpr` function | Yes; implicitly inline |
| Function template | Yes, subject to template/ODR rules |
| Class definition | Yes, if ODR requirements are satisfied |
| Ordinary external global variable | No |
| Inline variable | Yes |
| Namespace-scope `static` function | Each translation unit gets a separate function |

Ordinary non-inline, non-template functions with external linkage require one definition in the program. Class definitions, inline entities, and templates can have matching definitions in multiple translation units under the One Definition Rule.

==========================

