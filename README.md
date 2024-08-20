# Enzyme tests

Repo for Enzyme test until I find time to move all this to main NOA.

Use Ninja and Clang 18.1.0+ because the whole repo uses C++23 + `import std`.

Available targets:

## `local_vol`/`local_vol_consumer`

**Under construction**

**local_vol** example from NOA, modernized. Enzyme has decided that it's
more than it can chew, but that's the main idea for now

## `example_module`

An exampe Python module that uses Enzyme to perform AD on a simple function and
`torch::Tensor` in its interface. To import, run Python in `build/example_module`
dir (that's where the `.so` is stored)
