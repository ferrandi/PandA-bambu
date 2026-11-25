# Mixed C and C++ inputs

Bambu currently chooses the Clang driver language mode globally for a
multi-source invocation, based on the input list's final source classification.
As a result, mixing C and C++ sources can be order-dependent. For example,
`input.c` below is valid C, but the command that lists a C++ source last can
process the C file in C++ mode, where `class` is reserved:

```c
/* input.c */
void kernel(int *a) {
   int class = 7;
   a[0] = class;
}
```

```cpp
// input.cpp
void helper_cpp(int *a) { a[0] = 2; }
```

With implicit language selection, `bambu input.c input.cpp --top-fname=kernel`
can fail while compiling `input.c`; reversing the two input paths succeeds.
The current multi-source tests include code valid in both languages and do not
remove this limitation.

The `-x` option is a global language override for the invocation. It can force
all inputs to C or C++, but does not select a different language per source in
a mixed-language input list.
