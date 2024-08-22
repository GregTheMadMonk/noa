# Enzyme tests

Repo for Enzyme test until I find time to move all this to main NOA.

Use Ninja and Clang+`libc++` 18.1.0+ because the whole repo uses C++23 and
`import std`.

List of targets:

## `enzyme`, `torch`

Module names: `enzyme`, `torchext`.

Wrap Enzyme and Torch libraries in modules.

TODO: Extend exported interfaces if needed.

Torch library exports a minimal amount of names (right now, only
`torch::Tensor`), but provides useful utilities to provide a Torch interface
to algorithms implemented with pure STL. This is because it is built against
GNU `libstdc++` and using anything from it other than the most basic
get-tensor-size-and-pointer-and-wrap-it-in-an-STD-container may cause undefined
symbol errors when importing a Python module build with other standard libary
implementations (Clang's `libc++`).

`torchext` module also exports some `pybind11` names that are needed to
declare a module. You only need to include the header that contains PyBind11's
macros (ses `python_bindings/bindings.cc`).

All of this may sound like doing stuff for the sake of doing stuff, but on
my laptop it reduced the compile time of `python_bindings/bindings.cc` from
over 2 minutes to around 30 second. Which might not sound like a lot, but sure
feels like much and actually makes the development viabble. And who knows how
much more time is saved by not `#include`ing STL (hence the need for `libc++`:
`libstdc++` doesn't support `import std` yet!)...

## `example_module`

Example C++ module with two simple functions and their respective derivatives
calculated using Enzyme.

## `example_consumer`

Example of how an application might use `example_module`

## `python_bindings`

Example of how all above can be used to provide a PyTorch-friendly interface
to `example_module` (and more... later).

To use, `cd` into a `python_bindings` subfolder in your build directory.

## `local_vol`

**Under construction**

**local_vol** example from NOA, modernized. Enzyme has decided that it's
more than it can chew, but that's the main idea for now

## `local_vol_consumer`

**Under construction**

An example of how an application may use `local_vol`. Doesn't work since, well,
`local_vol` doesn't.
