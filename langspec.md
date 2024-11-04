<style>:root{--key:#d5d;--key2:#8ee;--type:#7bf;--iden:#66ddff;--back:#ccc;--nonfocus:#888;--lit:#ada;--func:#efde74;--comment:#3a3;--str:#e92;}.snippet{color:var(--back);}.key{color:var(--key);}.key2{color:var(--key2);}.type{color:var(--type);}.iden{color:var(--iden);}.func{color:var(--func);}.nfoc{color:var(--nonfocus);}.norm{color:var(--back);}.lit{color:var(--lit);}.comm{color:var(--comment);}.str{color:var(--str);}</style>

> # WARNING
> THIS DOCUMENTATION IS INCOMPLETE AND SUBJECT TO CHANGE WITHOUT NOTICE

# Contents
1. [Principles](#principles)
2. [Syntax](#syntax)
    - [typing](#syntax-typing)
    - [variables](#syntax-variables)
    - [functions](#syntax-functions)
    - [blocks](#syntax-blocks)
3. [Memory Management](#memory-management)
    - [tagging](#the-tagging-system)
4. [Data](#data)
    - [primitives](#primitives)
    - [structures](#data-structures)
        - [sum](#sum-types)
        - [product](#product-types)
        - [pattern](#patterns)

# Principles
- a mix of functional and imperative programming that allows for better optimization
- easy-to-use style and systems that make programming faster and less aggravating
- memory safety and automatic management without the need for either a garbage collector or a borrow checker

# Syntax
<a id="syntax-typing"></a>

## Typing
Variable types within function bodies are not required as they will be inferred. However, due to complexity involving overloading, function parameters and returns must always be explicitly typed. Constants also require explicit types.

Type aliases and definitions both make use of the `typedef` keyword.

<a id="syntax-variables"></a>

## Variables
All variables are mutable within their scopes. All variables with the exception of primitives are references.

Variable creation and assignment have identical syntax, the name may be optionally be followed by a type, separated by a colon, and set as the right hand side, demarcated with the assignment operator `=`.

<a id="syntax-functions"></a>

## Functions
Functions have three variants: pure, impure, and foreign.

Pure functions are declared with the `func` keyword. Pure functions may not mutate input parameters, and can never return void. Pure functions may also not call impure functions or otherwise have side-effects

Impure functions are declared with the keyword `func` preceded by the modifier `impure`. Impure functions may call both pure and impure functions, mutate input parameters, have side-effects, and return void.

Foreign functions are declared with the keyword `func` preceded by both the modifier `foreign` and the ABI name in single quotes. Foreign functions act as wrappers around the actual foreign function, and may only call the foreign function that they wrap. Additionally, foreign functions are the only functions allowed to use raw pointers. The function that the wrapper is made for is the same as the wrapper's name.<br>
Example: `foreign 'C' function malloc(u32): ptr` is a wrapper around the C malloc function.

<a id="syntax-blocks"></a>

## Blocks
Block statements include the following: for loops, do _ loops, while loops, if else if else statements, match statements.

For loops take one of two forms, iteration and counting. The iteration form allows iterating through the elements of an array, or any data structure following the Iterable pattern. The counting form allows going from the start to end numerical values with an optional step size. The step size of the counting form defaults to 1, unless the start is greater than the end, in which case it defaults to -1.<br>
Counting Example:
<pre class="snippet">
<span class="comm">// 0, 1, 2, 3, 4, 5, 6, 7, 8, 9</span>
<span class="key">for</span> <span class="iden">i</span> <span class="key">in</span> <span class="lit">0</span> <span class="key">to</span> <span class="lit">10</span>
    <span class="func">print</span>(<span class="iden">i</span>)
<span class="key">end</span>
</pre>
With Step Parameter:
<pre class="snippet">
<span class="comm">// 0, 2, 4, 6, 8</span>
<span class="key">for</span> <span class="iden">i</span> <span class="key">in</span> <span class="lit">0</span> <span class="key">to</span> <span class="lit">10</span> <span class="key">by</span> <span class="lit">2</span>
    <span class="func">print</span>(<span class="iden">i</span>)
<span class="key">end</span>
</pre>
Iteration Example:
<pre class="snippet">
<span class="iden">list</span>: [<span class="type">u8</span>] = [<span class="lit">10</span>, <span class="lit">20</span>]
<span class="comm">// 10, 20</span>
<span class="key">for</span> <span class="iden">n</span> <span class="key">in</span> <span class="iden">list</span>
    <span class="func">print</span>(<span class="iden">n</span>)
<span class="key">end</span>
</pre>
While and do _ loops. While loops take the form of `while condition ... end`. Do _ loops take the form of `do ... _`, where `_` is either the keyword `forever`, or the keyword `while` followed by a condition.<br>
While Loop:
<pre class="snippet">
<span class="comm">// x will equal y</span>
<span class="key">while</span> <span class="iden">x</span> < <span class="iden">y</span>
    <span class="iden">x</span> ++
<span class="key">end</span>
</pre>
Do While:
<pre class="snippet">
<span class="comm">// if x == y, x = y + 1, else x = y</span>
<span class="key">do</span>
    <span class="iden">x</span> ++
<span class="key">while</span> <span class="iden">x</span> < <span class="iden">y</span>
</pre>
Do Forever:
<pre class="snippet">
<span class="comm">// prints 'y' until program is interrupted</span>
<span class="key">do</span>
    <span class="func">print</span>(<span class="str">'y'</span>)
<span class="key">forever</span>
</pre>

# Memory Management
Programs may not directly allocate or dispose of memory, with the exception of data from foreign functions.

To memory is managed automatically without need for a garbage collector or borrowing system through the use of compile-time tagging, which maps out when and how resources are freed.

## The Tagging System
The compiler will perform static analysis to determine resource lifetimes, note that this is not applicable to primitive types, as they are always allocated on the stack.

All variables are tagged with their enclosing scope. When this scope ends, they are freed. When a value is returned, it gets tagged as the scope that it is being returned into. If a member of a structure with broader scope is set to a value with narrower scope, the value will inherit the tag of the parent.

# Data
## Primitives
Primitives consist of the base types that all others are built on<br><br>
List of primitives:
- `[u|s][8|16|32|64]` -- (un)signed integers declared with their width
- `f[32|64]` -- IEEE 754 floating point number
- `bool`
- `char` -- 32bit unicode
- `[T]` -- array of `T`
- `ptr` -- raw pointer (foreign functions only)

## Data Structures
Watercress supports algebraic sum and product data types<br>

### Sum Types
A sum type is composed of one or more variants which contain zero or more anonymous members.

Example:
<pre class="snippet">
<span class="key">typedef</span> <span class="type">MySum</span> <span class="key">is</span> <span class="key2">sum</span>
    <span class="iden">Foo</span>
    <span class="iden">Bar</span>(<span class="type">u8</span>)
<span class="key">end</span>
</pre>
In this example, `MySum` is a sum containing the variants `Foo` (which has no members) and `Bar` (which has one `u8` member).<br>
The variant of a sum type can be checked with the `is` keyword, and members can be accessed with either match syntax or tuple syntax.

Example with tuple syntax:
<pre class="snippet">
<span class="iden">baz</span>: <span class="type">MySum</span> = <span class="iden">Bar</span>(<span class="lit">0</span>)
<span class="key">if</span> <span class="iden">baz</span> <span class="key">is</span> <span class="iden">Bar</span>
    <span class="func">print</span>(<span class="iden">baz</span>.<span class="iden">0</span>)
<span class="key">end</span>
</pre>
Example with match syntax:
<pre class="snippet">
<span class="iden">baz</span>: <span class="type">MySum</span> = <span class="iden">Bar</span>(<span class="lit">0</span>)
<span class="key">match</span> <span class="iden">baz</span>
    <span class="iden">Bar</span>(<span class="iden">n</span>) => <span class="func">print</span>(<span class="iden">n</span>)
<span class="key">end</span>
</pre>

### Product Types
A product type is composed of one or more named members.

Example:
<pre class="snippet">
<span class="key">typedef</span> <span class="type">MyProd</span> <span class="key">is</span> <span class="key2">prod</span>
    <span class="iden">foo</span>: <span class="type">u8</span>
<span class="key">end</span>
</pre>
In this example, `MyProd` is a product type with member `foo` which is a `u8`.<br>
The value of `foo` can be accessed with the dot syntax, like so: `MyProdInstance.foo`.

### Patterns
Patterns are interfaces that allow generics to be more useful.<br>
A pattern consists of a name, any super-patterns, and a list of functions that must be defined for a type to conform to said pattern.

Here is the definition for the standard library's `Readable` pattern:
<pre class="snippet">
    <span class="key">pattern</span> <span class="type">Readable</span> <span class="key">is</span>
        <span class="key">impure</span> <span class="key">func</span> <span class="func">read</span>(<span class="iden">self</span>: <span class="type">Readable</span>, <span class="iden">amount</span>: <span class="type">u32</span>): [<span class="type">u8</span>]
        <span class="key">impure</span> <span class="key">func</span> <span class="func">peek</span>(<span class="iden">self</span>: <span class="type">Readable</span>): <span class="type">Option</span> <span class="key2">of</span> <span class="type">u8</span>
        <span class="key">impure</span> <span class="key">func</span> <span class="func">poll</span>(<span class="iden">self</span>: <span class="type">Readable</span>): <span class="type">bool</span>
    <span class="key">end</span>
</pre>
