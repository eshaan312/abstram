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
- Nisse's main feature is memory-safety using "dependency-checking" (DC) instead of borrow-checking.
- At compile-time, each heap-allocated data structure in this language has a value that tells the compiler what     
  scope it needs to be freed at.
- By default, this value is set to the scope it was defined in, but when you, for example, assign the value to a       
  global array, its scope to be freed will be at the end of the program.
-  

#### Credits:
- README image from [Hilda](https://en.wikipedia.org/wiki/Hilda_(TV_series)) (a Netflix show)
- Nisse language by [xmm16](https://github.com/xmm16)
