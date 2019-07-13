# SIN-Language

SIN is a custom procedural programming language based on C, C++, and Python, designed as an exercise in computer language design. It is named after the Spanish word meaning "without", because if anyone were to ask you about the language, you would reply that it is without any real utility or purpose. This repository contains the entire toolchain for using the SIN language, including a Lexer, Parser, Interpreter, Compiler, Assembler, Linker, and Virtual Machine to execute SIN programs. It can currently be compiled into working binaries for the SIN Virtual Machine, a 16-bit simulated processor based on the [MOS 6502](https://en.wikipedia.org/wiki/MOS_Technology_6502).

## Goal of the Project

This project is a hands-on way of learning how parsers, compilers, and interpreters operate. The purpose of this project is not to create a modern, efficient, lightweight, and useful programming language or compiler; this is a first attempt, and as such it will be mediocre at best. The goal is to serve as a learning exercise in compiler writing and solving various programming problems in the C++ language.

## Getting Started

### Installation / Build

There are currently no working binaries for the toolchain; you must compile and build it yourself. Note that the program has only been built and debugged on Windows 10 using Microsoft Visual Studio 17; because of this, *NIX and OSX builds may face compilation issues due to compatibility differences between platforms.

### Using SIN

Consult the [wiki](https://github.com/truffly/SIN-Language/wiki) for a reference to the language and the [pages](https://truffly.github.io/SIN-Language) for more information. It is small and simple, but will continually grow in complexity as I expand it.
_The program currently only supports ASCII-encoded files_, it will not be able to parse Unicode or other encoding standards. This is something I intend to change once the toolchain is more or less functional, but it is not currently a priority.

## Future Goals

I hope to use this project as a stepping stone to develop other language and explore other features, such as compilers for object-oriented programming. For this project specifically, I hope to add in:
* Compiler optimizations
* Support for more target architectures (specifically x86)
* Debugging support
