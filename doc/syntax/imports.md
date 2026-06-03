# Imports
- There are two kinds of imports in porpoise
    - Library imports: These are imports denoted by identifiers and are declared in dedicated 'library' files
    - File imports: These are imports denoted by a string that is a file path, followed by an identifier alias
- The standard file extension for porpoise files is `.porp`
- Both file and library imports allow an identifier alias using the `as` keyword, with this being required for user imports
- An import can be marked public in order to expose it to the outside world when the enclosing file is imported
- Note that these statements are private by default and can be prefixed with `pub` to unrestrict visibility
```porpoise
import std; // Library import
import std as stud; // Library import with alias
import "node.porp"; // Illegal, file imports always require alias
pub import "node.porp" as node; // Public file import with alias
```
- The filepath given to the import should be relative to the asking file, _not_ to a local project 'root'
- Import statements can be used anywhere in your program, though they do not yield a value (hence why they are not called expressions) 

## Library Import Declaration
- Libraries are declared by indicating that a file is a root of an import tree
- A library's root file path must be provided to the compiler through the command line
- These imports can be aliased to avoid name collisions, though this is not required

## File Import Declaration
- A file in conch is importable through its relative path to the importing file
- These imports must be aliased to an identifier as the string path cannot be used implicitly as a namespace

### Example
- `std.porp` declares
```porpoise
pub import "io/io.porp" as io;
pub import "containers/array_list.porp" as array_list;
pub const ArrayList := array_list::ArrayList;
...
```

- `io/io.porp` declares a public function `println` which can be accessed via:
```porpoise
std::io::println(...);
```

- `containers/array_list.porp` declares a public function `ArrayList` that returns a generically-typed struct
```porpoise
pub const ArrayList := fn(T: type): type {...};
```
- This can be used just like any other function through the module access operator (`::`):
```porpoise
var a: std::array_list::ArrayList(i32);
var a: std::ArrayList(i32); // Allowed due to function declaration alias
```
