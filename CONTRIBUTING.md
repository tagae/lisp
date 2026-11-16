Contributing
============

Code is self-contained
----------------------

A source file explains itself. Its comments state the rules it implements, the constraints it works
under and the reasons for them, in full. They never refer to a document (`DESIGN.md`, `README.md`,
`NAMING.md`) or to a part of one (a section, a milestone, a step of the plan): whoever reads the
file should not need another file to understand it, and a document can be reorganized without the
code noticing, which leaves the reference dangling or wrong.

A source file may refer to another source file (`see lexer.h`), since the two travel together. This
holds for build files and scripts as well: the `Makefile` and the shell scripts explain themselves
too.

The documents may refer to the code. When a rule is written in both, as the syntax of Lisp-0 is, a
change updates both, and the checks that `make test` runs pin down what the code does.
