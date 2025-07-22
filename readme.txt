Virtual Nightmare

Virtual Nighmare is a simple virtual machine and C compiler, built with the
goal eventually it would be self-hosting. It's 16-bit and uses word-oriented
memory, giving a total of 64kwords or 128kb available. The compiler is by
no means complete, and instead implements a subset of C.

There are probably plenty of bugs to work out, but it _can_ in fact compile
itself, which was the main goal of the project. I might come back to it here
and there, but I'm pretty happy with how it turned out.

The files vcc.c, vdasm.c and vnight.c correspond to the compiler, the
disassembler and the runtime respectively. For more information about the
virtual machine, take a look at iset.

