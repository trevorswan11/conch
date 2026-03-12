# Imports
- There are two kinds of imports in conch
    - Module imports: These are imports denoted by identifiers and are reserved for the standard library
    - User imports: These are imports denoted by a string that is a file path, followed by an identifier alias
- Both user and module imports allow an identifier alias using the `as` keyword, with this being required for user imports
- An imports publicly available symbols are namespaced behind the alias, and can be accessed using the `::` operator
```conch
import std; // Module import
import std as stud; // Module import with alias
import "node.conch"; // Illegal, user imports always require alias
import "node.conch" as node; // User import with alias
```
- The filepath given to the import should be relative to the asking file, _not_ to a local project 'root'
- Imports are evaluated lazily
    - An unreferenced import will not be evaluated
    - Unreferenced symbols from an import will not be evaluated
    - This prevents the issue of circular imports without needing to implement a macro system or `pragma`
