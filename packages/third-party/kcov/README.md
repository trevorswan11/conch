# Kcov
The build rules in this directory sometimes build specified configurations of associated libraries. Specifically, it is impossible to produce a windows kcov binary from these build rules without manual intervention. This has the consequence of coverage instrumentation not working on windows, though this is a limitation of kcov itself. 
