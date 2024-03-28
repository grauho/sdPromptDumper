# sdPromptDumper, stable-diffusion.cpp Prompt Dumper

This program is a simple tool to take the metadata information that is
encoded into images produced by stable-diffusion.cpp and use it to construct 
a valid stable-diffusion.cpp invocation. Naturally due to the limitations of 
the information encoded this merely produces the closest invocation that it 
can get with the information provided. 

I made this tool because I often want to go back to previously generated images 
and experiment with tweaking various settings but it was annoying to copy and
paste each option from a PNG metadata inspector program. This way the user just 
has to run a single program and copy and paste a single command that they 
have the option of editing beforehand.

I considered just writing this in Perl but figured access to a working C99
compiler was a lower bar to clear than having a Perl interpreter, 
especially those on platforms where Perl is not immediately available. Plus, 
this was a fun excuse to write a little non-destructive tokenizer and dig into 
the PNG specification a little bit.

Tested and works on images produced by the following programs:
* stable-diffusion.cpp
* civitai.com (mostly)

## Building

This program has no external dependencies beyond the C standard library. It 
requires at least a C99 compliant compiler due to its inclusion of stdint. The
adventurous can provide an alternative header with definitions for those 
specific bit-width types should they wish to compile it with a C90 compiler.

A Makefile is provided for those with access to POSIX make, the program can be 
built as follows:

``` shell
make
```

Otherwise, it should suffice to simply build the object files and link them
together manually:

``` shell
cc -Wall -pedantic -O2 -c -o main.o main.c
cc -Wall -pedantic -O2 -c -o stiTokenizer.o stiTokenizer.c
cc -Wall -pedantic -O2 -c -o pngProcessing.o pngProcessing.c
cc -Wall -pedantic -O2 -o sdPromptDump main.o stiTokenizer.o pngProcessing.o
```

Notes: 

* PNG files contain values encoded using Big-Endian byte order, this program
addresses this already for those on little-endian systems. If for whatever
reason all PNG files are failing to process try running the program with the -e
switch and checking that the result agrees with the system byte order. See the
information below for more information.

## Command Line Options

    -M, --model <DIR PATH> : Prepends this path to the encoded model name 
    -L, --lora  <DIR PATH> : Passes this path directly to --lora-model-dir
    -B, --bin   <DIR PATH> : Prepends this path to the sd executable 
    -V, --vae  <FILE PATH> : Passes this file path directly to --vae
    -a, --abrv             : Uses abreviated switch names in the output
    -e, --endian           : Prints assumed endian form and exits
    -h, --help             : Prints a help message much like this one

Notes:

* By default no path is prepended "sd" in the result unless set with -B, this
is to accommodate those who have it installed somewhere like /usr/local/bin. 
This program is well suited to an alias that sets -M, -L, and -B.

* Should the endian switch suggest a different byte order than what is known
to be the system order the behavior can be forced by defining either 
PORTEGG\_LITTLE\_ENDIAN\_SYSTEM or PORTEGG\_BIG\_ENDIAN\_SYSTEM either using
a compiler switch or simply before portegg.h is included in the main.h header.

* This program was written to work with images produced using txt2img and lacks
handling for other modes or more advanced workflows that don't have their 
information encoded into the resulting output image. It should also go without
saying that this program will only work with programs generated using 
stable-diffusion.cpp or ones using the same metadata encoding style that have
not had their metadata stripped.

## Example Invocation

``` shell
./sdPromptDumper --model ~/.sd/models/ --vae myVAE.safetensors foo.png bar.png
```

## Bugs

* Please report any bugs or issues to the associated github issues page for 
this repository.

## License

License information can be found in the LICENSE.txt file in this repository.
