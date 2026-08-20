# ABSTRAM
This is the Github repository for the Abstram programming language!
# What it is
Abstram is a super low-level programming language. You don't have variables: instead you use genuine hardware registers (eax, ebx, ... & xmm0, xmm1, ...) 
and fields of memory. It's designed to be a more convenient version of x86 Netwide Assembly.    
      
When you build the program, you
combine your compiled code with the core of the 32-bit protected mode [SMGK kernel](https://github.com/eshaan312/smgk) to produce a full operating system you can run
in QEMU!     

# Type system
My favorite part of this project is the type system. Since the way the language works is so different (it's limited to only using registers in operations and things like that),
 you can do some pretty interesting things. The type system is different for every register.       
        
 At the beginning for every register, the type system is completely static. A type is assigned to a register at its' initialization. However,
 if a register's type is changed and not changed back during a conditional that may or may not happen, it's switched to using the dynamic type system. A byte
 is stored on static memory to keep track of its type.      
       
You might see a potential problem here. If a dynamically typed register (I'm going to call these dynamic registers from now on) was copied
into a static register, wouldn't the static register be converted to a dynamic register? Nope! At runtime, there's a check that makes sure
the two types are the same before moving, so the dynamic-ness doesn't spread.

How do loops work then? It's actually really simple. For each conditional loop which contains an un-reversed type cast, two versions of the loop are
made. One where the register is statically typed, and another where the register is dynamically typed.

Since this is bare metal, how do runtime errors work? For now, while the language is still in development, I'm using the QEMU debug address (0xe9)
to output simple error messages. However, eventually, the language will be able to render text to the screen for true runtime error messages!
