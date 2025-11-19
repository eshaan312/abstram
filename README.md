<p align="center">
<img width="200" height="230" alt="image" src="https://github.com/xmm16/xmm16.github.io/blob/main/nisse.png?raw=true" />
</p>
<p align="center"><b>Nisse</b> - a convenient systems programming language<br></p>

#### Info:
- I made Nisse because I didn't see the point of a lot of the complexity in languages like C++ and Rust.
- I'm also a big fan of zero-cost abstractions so you'll see plenty of that (not built into the language, of course).

#### Design:
- Nisse is designed to be very similar to C in terms of performance and syntax.
- However, Nisse adds a few things that I think will make code easier to read and more enjoyable to write.

#### Memory:
- **Dependency-checking:**
	- Nisse's main feature is memory-safety using "dependency-checking" instead of borrow-checking.
	- At compile-time, each heap-allocated data structure in this language has a value that tells the compiler what scope it needs to be freed at (AKA the lifetime).
	- By default, this value is set to the scope it was defined in, but when you, for example, assign the value to a global array, it'll end up being freed at the end of the program.
  - This system gives you memory-safety without the difficulty and occasionally annoying rules that borrow-checking requires.
  - **TL;DR:** Dependency-checking lets you to treat the language like it's garbage-collected but with basically no runtime cost.

- **Potential concerns:**
  - **Q:** If 'A' owns 'B', 'B' owns 'C', and 'C' owns 'A', what will happen?    
    **A:** Ownership isn't really a thing using dependency-checking, but, if 'B' depends on 'A' and 'C' depends on 'B' and 'A' depends on 'C', it'll be freed at the highest lifetime value in the group. So if 'C' has global scope and both 'A' and 'B' have local scopes, both 'A' and 'B' will recieve extended lifetimes to match 'C'. 
           
  - **Q:** If a function runs a complex calculation to determine if it'll make a heap allocation, how do you know if it's ends up being made or not, and when to free it?    
    **A:** In these situations, a static array is created for the scope, and at the end of the scope, it'll loop through the array and free all addresses inside it (this process has the same runtime cost as its typical implementation in memory-unsafe languages). If no potential heap allocations are detected, neither the static array or the loop will exist in the resulting compilation.
          
  - **Q:** How is aliasing done?    
    **A:** Aliases are identified by the compiler and do not have a lifetime value. However, if another piece of data depends on a heap-allocated alias, it will modify the lifetime of the original allocation accordingly.

#### Credits:
- README image from [Hilda](https://en.wikipedia.org/wiki/Hilda_(TV_series)) (a Netflix show)
- Nisse language by [xmm16](https://github.com/xmm16)
