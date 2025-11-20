<p align="center">
<img width="200" height="230" alt="image" src="https://github.com/xmm16/xmm16.github.io/blob/main/nisse.png?raw=true" /></p>
<p align="center"><b>Nisse</b> - a convenient systems programming language<br></p>

#### Design:
- Nisse is designed to be very similar to C in terms of performance and syntax.
- However, Nisse adds a few things that I think will make code easier to read and more enjoyable to write.

#### Memory:
- **Region inference:**
  - Nisse's main feature is memory-safety using region inference.
    - At compile-time, each heap-allocated data structure in this language has a value that tells the compiler what point it needs to be freed at (AKA the lifetime).
    - By default, this value is set to the point that all dependent data stops being accessed in the scope it was defined in, but when you, for example, assign the value to a global array that's accessed right before the the main function returns, it'll end up being freed at the end of the program.
           
  - You can also drop a heap allocation manually, which would tell the compiler that there is a 0% chance of that allocation getting referenced later on.
    - If this is obviously not true (e.g. when you reference a variable right after dropping it) you'll get a compile-time error, but if nothing that depends on the dropped allocation is used after the drop, there will be no error.
    - You should only drop if you're extremely confident since it would result in a UAF error at runtime if you're wrong. If you don't drop the allocation, it'll exist until all dependencies are thoroughly settled.
           
  - Sometimes developers can accidentally change the lifetimes of heap allocations even when they didn't mean to, which can result in a potentially massive runtime cost.
    - Therefore, every time a lifetime is changed, the developer needs to use the `sign` keyword which takes two arguments: the maximum amount of blocks on the heap that'll be given raised lifetimes, and the higher-scope variable that depends on the heap allocation.
    - Clarification: signs don't actually do anything at runtime.
            
      - They don't give the user control over memory management or lifetimes: they're just an acknowledgement of the cost of raising the lifetime of a heap allocation.
      - If the developer finds that cost to be too high, the sign helps them realize that they need to change something in their program. 
    - This feature is only used for compile-time confirmation and can be turned off when you compile using the `--no-sign` flag (the resulting executable will be the exact same without signs).
    - When you compile your code and the compiler realizes that you forgot to sign an allocation, it'll tell you exactly where to put the sign and what arguments to sign it with
           
  - **TL;DR:** Dependency-checking lets you mostly treat the language like it's garbage-collected, even though it isn't.

- **Potential concerns:**
  - If a function runs a complex calculation to determine if it'll make a heap allocation, how do you know if it's ends up being made or not, and when to free it?    
    - In these situations, a static array is created for the scope, and at the end of the scope, it'll loop through the array and free all addresses inside it (this process has the same runtime cost as its typical implementation in memory-unsafe languages). If no potential heap allocations are detected, neither the static array or the loop will exist in the resulting compilation.
          
  - How is aliasing done?    
    - Aliases are identified by the compiler and do not have a lifetime value. However, if another piece of data depends on a heap-allocated alias, it will modify the lifetime of the original allocation accordingly.

#### Credits:
- README image from [Hilda](https://en.wikipedia.org/wiki/Hilda_(TV_series)) (a Netflix show)
- Nisse language by [xmm16](https://github.com/xmm16)
