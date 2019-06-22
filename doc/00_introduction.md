# Introduction

Modern programming takes many forms. There's web development, desktop application development, mobile development and more. Embedded programming is one of the areas of programming, and can be radically different from the others. Embedded programming means programming for a computer that's mainly intended to be embedded within a larger system, and the embedded computer is usually responsible for a particular task, instead of being a general-purpose computing device. The system where it's embedded might be a simple pocket calculator, or an industrial robot, or a spaceship. Some embedded devices are microcontrollers with very little memory and low frequencies, others are more powerful.

Embedded computers may be running a fully-fleged operating system, or a minimalistic system that just provides some scheduling of real-time functions. In cases when there's no operating system at all, the computer is said to be *bare metal*, and consequently *bare metal programming* is programming directly for a (micro-)computer that lacks an operating system. Bare metal programming can be both highly challenging and very different from other types of programming. Code interfaces directly with the underlying hardware, and common abstractions aren't available. There are no files, processes or command lines. You cannot even get the simplest C code to work without some preparatory steps. And, in one of the biggest challenges, failures tend to be absolute and mysterious. It's not uncommon to see embedded developers break out tools such as voltmeters and oscilloscopes to debug their software.

Modern embedded hardware comes in very many types, but the field is dominated by CPUs implementing an ARM architecture. Smartphones and other mobile devices often run Qualcomm Snapdragon or Apple A-series CPUs, which are all based on the ARM architecture. Among microcontrollers, ARM Cortex-M and Cortex-R series CPU cores are very popular. The ARM architectures play a very significant role in modern computing.

The subject of this ebook is bare-metal programming in C for an ARM system. Specifically, the ARMv7-A architecture is used, which is the last purely 32-bit ARM architecture, unlike the newer ARMv8/AArch64. The -A suffix in ARMv7-A indicates the A profile, which is intended for more resource-intensive applications. The corresponding microcontroller architecture is ARMv7-M.

Note that this is not a tutorial on how to write an OS. Some of the topics covered in this ebook are relevant for OS development, but there are many OS-specific aspects that are not covered here.

## Target audience

This ebook is aimed at people who have an interest in low-level programming, and in seeing how to build a system from the ground up. Topics covered include system startup, driver development and low-level memory management. For the most part, the chapters cover things from a practical perspective, by building something, although background theory is provided.

The reader should be familiar with C programming. This is not a C tutorial, and even though there are occasional notes on the language, the ebook is probably difficult to follow without having programmed in C. Some minimal exposure to an assembly language and understanding of computer architecture are very useful, though the assembly code presented is explained line by line as it's introduced.

It's also helpful to be familiar with Linux on a basic level. You should be able to navigate directories, run shell scripts and do basic troubleshooting if something doesn't work - fortunately, even for an inexperienced Linux user, a simple online search is often enough to solve a problem. The ebook assumes all development is done on Linux, although it should be possible to do it on OS X and even on Windows with some creativity.

Experienced embedded developers are unlikely to find much value in this text.

## Formatting and terminology

The ebook tries to for the most part follow usual conventions for a programming-related text. Commands, bits of code or references to variables are `formatted like code`, with bigger code snippets presented separately like this:

```
void do_amazing_things(void) {
    int answer = 42;
    /* A lot happens here! */
}
```

If you are reading the PDF version, note that longer lines of code have to get wrapped to fit within the page, but the indentation and line numbers inside each code block should help keep things clear.

Due to some unfortunate historical legacy, there are two different definitions for data sizes in common use. There's the binary definition, where a kilobyte is 1024 bytes, and the metric definition, where a kilobyte is 1000 bytes. Throughout this ebook, all references to data quantities are in the binary sense.

## Source code

For each chapter, the corresponding source code is available. If you're reading this on GitHub, you can explore the repository and check its readme file for more information. If you're reading the PDF version or other standalone copy, you can head to the [GitHub repository](https://github.com/umanovskis/baremetal-arm/) to access the source code, and perhaps updates to this ebook. The repository URL is `https://github.com/umanovskis/baremetal-arm/`.

## Licensing

The ebook is licensed under Creative Commons Attribution-Share Alike (CC-BY-SA) [license](http://creativecommons.org/licenses/by-nc-sa/4.0/) - see the Git repository for more details on licensing.

## Credits and acknowledgments

The PDF version of this ebook is typeset using the [Eisvogel LaTeX template](https://github.com/Wandmalfarbe/pandoc-latex-template) by Pascal Wagler.
