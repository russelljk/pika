# Pika

Pika is a dynamic, multi-paradigm programming language. 

## Features Include

 * First class functions and Closures.
 * Class based object system.
 * Automatic memory managment through an incremental, mark and sweep, garbage collector.
 * Packages.
 * Properties with get and set accessor functions.
 * Native API for binding with C/C++.
 * Unified import system works with scripts and native C/C++ modules.
 * Exception handling.
 * Block finalizers.
 * Lambda expressions.
 * Anonymous function expressions.
 * Default values for functions.
 * Variable argument functions.
 * Keyword arguments.
 * Lexically scoped.
 * Multiple return and yield values.
 * Coroutines (*aka cooperative threads*).
 * Generators (*light weight coroutines*).
 * Annotations for functions, classes and packages.
 * Array comprehension. `[ i for x = 1 to 20 if x mod 2 ]`

## Some Examples

See the samples subdirectory for a complete list of sample scripts. 

You can import them *once installed* by doing the following:

    # Install sample file classes
    import "samples/classes.pika" 

Or run them from the command line:

    pika samples/classes.pika

###Classes

    class Person
        function init(name, age)
            self.name = name
            self.age = age
        end
        
        function toString()
            return 'Person {self.name} {self.age}'
        end
    end
    
    class Employee: Person
        function init(name, age, occupation)
            super(name, age)
            self.occupation = occupation
        end
        
        function toString()
            return 'Employee {self.name} {self.age}, {self.occupation}'
        end
    end
    
    jane = Person.new('Jane Doe', 33)
    john = Employee.new('John Smith', 35, 'Programmer')
    
    print jane is Person #=> true
    print john is Person #=> true    

    print jane is Employee #=> false
    print john is Employee #=> true
    
    print jane
    print john

###Generators

    function fib()
        a, b = 0, 1
        loop
            yield a
            a, b = b, a + b
        end
    end
    
    fibIterator = fib()
    
    print fibIterator()
    print fibIterator()
    print fibIterator()
    
    {* Generators can also created from instance methods. *}
    class RangeMultiplier
        function init(scalar)
            self.scalar = scalar
        end
        
        function range(a, b)
            for i = a to b
                yield i * self.scalar
            end
        end
    end
    
    # Create a RangeMultiplier instance
    rangeMultiplier = RangeMultiplier.new(scalar=2)
    
    # Create the generator bound to the instance
    multIter = rangeMultiplier.range(1, 6)

    while multIter
        print multIter()
    end
    
## Get Pika

This document will explain how to build and install the Pika library, interpreter and modules from source using the CMake build system.

### First Get CMake


(If CMake is already installed skip step 1.)

####Step 1

Go to the CMake home page at http://www.cmake.org and follow the instructions for installing CMake for your platform.

 * On **Windows** you will probably want to use the **CMake installer**.
 * On **Mac OS X** you can use **fink**, **Mac Ports**, **Homebrew** or the **CMake installer**.
 * On a ***nix platform** if might be preferable to use the system's package manager. 

####Step 2

Now open up the terminal and navigate to the project's root directory.

####Step 3

At the terminal type: 
note: (don't type the > character)

    > mkdir build && cd build
    > cmake ../../pika
    > make
    > sudo make install

The CMake option `-DCMAKE_INSTALL_PREFIX=INSTALL_PATH`  can be used to specify a different location. You might want to point it to your user directory if you do not have administrator privileges.

    cmake -DCMAKE_INSTALL_PREFIX=/home/myhome ../../pika

To build without modules use the following

    cmake -DPIKA_NO_MODULES=1 ../../pika

### Modules

**base64**

 * A base 64 encoder and decoder, meant to be an example module.
 
**bignum**

 * Arbitrary precision arithmetic for integers and real numbers (*floating point numbers*).
 * Depends on GMP and MPRF for the BigInteger and BigReal classes respectively.
 
**curses**

 * Bindings for curses/ncurses library.
 * Depends on curses/ncurses, no support for pdcurses at this time.

**datetime**

 * Provides Date, DateTime, Time, Timezone and TimeSpan classes and date manipulation.
 
**event**

 * Bindings for LibEvent for use with the Socket and File classes.
 * Depends on LibEvent version `2.0.1 alpha` and up.
 
**json**
 
 * JSON encoder and decoder.
 
**random**
 
 * Pseudorandom number generator based on Mersenne Twister (included).

**re**

 * Regular expressions module.
 * Depends on PCRE which is now included in the code base. The project's home page is http://www.pcre.org/. I use PCRE version 8.10, so any version compatible with that release should work. Specify `RE_USE_EXTERNAL_PCRE` to search for the system's PCRE.

**socket**

 * Berkley Sockets library.
 * Depends on Posix Socket support, not ported to Winsock yet.

**unittest**

 * Unittest module. 
 * Depends on the `re` module.