# Pika

Pika is a dynamic, multi-paradigm programming language. 

## Features Include

* First class functions and Closures
* Class based object system
* Automatic memory managment through an incremental, mark and sweep, garbage collector
* Packages
* Properties with get and set accessor functions
* Native API for binding with C/C++
* Unified import system works with scripts and native C/C++ modules
* Exception handling
* Block finalizers
* Lambda expressions
* Anonymous function expressions
* Default values for functions
* Variable argument functions
* Keyword arguments
* Multiple return and yield values
* Coroutines (*aka cooperative threads*)
* Generators (*light weight coroutines*)
* Annotations for functions, classes and packages
* Mersenne Twister based pseudorandom number generator

## Some Examples

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
    
    (* Generators can also created from instance methods. *)
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

    rangeMultiplier = RangeMultiplier.new(scalar:2)
    multIter = rangeMultiplier.range(1, 6)

    while multIter
        print multIter()
    end
    
## Get Pika

This document will explain how to build and install the Pika library, interpreter and modules from source using the CMake build system.

### First Get CMake


(If CMake version 2.6 or greater is already installed go to step 1.)
Go to the CMake home page at http://www.cmake.org and follow the instructions for installing CMake for your platform. If you are using a *nix platform if might be preferable to use the system's package manager. On Windows you will probably want to use the installer. Under Mac OS X you can use fink, Mac Ports or the installer.

Now open up the terminal and navigate to the project's root directory.

At the terminal type: 
note: (don't type the > character)

    > mkdir build && cd build
    > cmake ../../pika
    > make
    > sudo make install

The CMake option `-DCMAKE_INSTALL_PREFIX=INSTALL_PATH`  can be used to specify a different location. You might want to point it to your user directory if you do not have administrator privileges.

    cmake -DCMAKE_INSTALL_PREFIX=/home/myhome ../../pika

To build without modules use the following

    cmake ../../pika -DPIKA_NO_MODULES=1

### Module Dependencies

re - Depends on PCRE which is now included in the code base. The project's home page is http://www.pcre.org/. I use PCRE version 8.10, so any version compatible with that release should work. Specify `RE_USE_EXTERNAL_PCRE` to search for the system's PCRE.
