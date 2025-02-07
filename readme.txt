Virtual Nightmare

Virtual Nighmare is a simple virtual machine and C compiler, with the aim
that eventually it can be self-hosting. It's 16-bit and uses word-oriented
memory, giving a total of 64kwords or 128kb available. The compiler is by
no means complete, and instead implements a subset of C.

It's currently a work in progress, with several bugs to be ironed out before
it can compile a functioning version of itself.

The files vcc.c, vdasm.c and vnight.c correspond to the compiler, the
disassembler and the runtime respectively. For more information about the
virtual machine, take a look at iset.

