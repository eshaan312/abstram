# abstram
I want to implement a type system that I've been thinking up for a while which would switch variables from statically typed to dynamically typed at runtime. It would only be useful if you had a limited amount of variables but I think it would be cool to make.      

# MVP scope
I want to build a small compiler with 2 types, integer and float. It'll target x86 Assembly and likely won't include an advanced optimizer, just a front-end, type checker, and code generator.

# Schedule
- Set up the Git repo
- Set up a small static linker with dd and smgk
- Write the lexer
- Create the runtime register type table
- Add variable initialization


