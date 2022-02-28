 streplace
Replace strings in files, filenames and symbolic links, in place, recursively.

## Status: Not yet ready

This is not yet usable in a safe way (only the basics are just coming together). Use streplace v0.9.x in the meantime.

### Known issues

- Rename not yet implemented.



## Differences compared to streplace 0.9.x

- --whole-words now also works for regex (it used to work only for -x/--no-regex)
- Octal notation is no longer supported for Specifying arbitrary byte values in LHS strings (i.e. \1 
  for ASCII is no longer supported). Hex byte values now must have exactly two hex digits, e.g. 
  \xaZZZ is no longer supported. But \0 for 0x00 is supported. This is all supported by the std::regex library.

### New features:

- --equals: An arbitrary rule separator string can be optionally specified instead of '=', avoiding the need to quote '=' chars in rules.
- TODO: --dollar: An arbitrary substring-reference string can be optionally specified for RHS of rules, to refer to matches substrings, avoiding the need to quote dollar chars.



