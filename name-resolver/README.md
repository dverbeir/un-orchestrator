# Name-resolver

The name-resolver is a module that returns the list of all existing implementations
of a network function.


## Compile the name-resolver

We assume here that you already followed the general steps (e.g., required 
libraries) that are detailed in the general [../README_COMPILE.md](../README_COMPILE.md)
page.

Your are now ready to compile the name-resolver.

	$ cd name-resolver

	; Choose among possible compilation options
	$ ccmake .

The previous command allows you to select some configuration parameters for the
name-resolver, such as the logging level. When you're finished, exit from the
`ccmake` interface by *generating the configuration files* (press 'c' and 'g')
and type the following commands:

	; Create makefile scripts based on the previously selected options
	$ cmake .

	; Compile and create the executable
	$ make

# How to run the name-resolver

The full list of command line parameters for the name-resolver can be
retrieved by the following command:

	$ sudo ./name-resolver --h

Please refer to the help provided by the name-resolver itself in order to
understand how to use the different options.

Please check [config/example.xml](config/example.xml) to understand the configuration 
file required by the name-resolver. This file represents a database containing 
information on all the possible implementations for each available network function.
