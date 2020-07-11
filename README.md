A compiler for mini_C

It parses the mini_C code and generate object code in eeyore, tigger or riscv64 asm

## requirement

- sudo apt-get install gcc make  bison flex  
- to test riscv64C, get riscv-gnu-toolchain from https://github.com/riscv/riscv-gnu-toolchain first 

## Test

To test eeyore  

```
cd eeyore
make
./eeyore ../testcase/00_main.c out
./Eeyore_simulator out
```

To test tigger

```
cd tigger
make
./tiggerC ../testcase/00_main.c out
./Tigger_simulator out
```

To test riscv64

```
cd riscv64
make
./riscv64C ../testcase/00_main.c out.s
/opt/riscv/bin/riscv64-unknown-elf-gcc link.c -c
/opt/riscv/bin/riscv64-unknown-elf-gcc out.s link.o -o out
/opt/riscv/bin/qemu-riscv64 out
```

