# proximal

##i A C++ template that compares floating point values for near equality in a robust manner

This template provides a robust mechanism for comparing floating point
values for effective equality, or, being *close enough*. It is primarily intended 
to mitigate the effects of rounding errors caused by floating point 
representations. The template allows a user to specify a margin (the 
threshold of difference within which equality is conferred) in terms of
*ulps* (units in the last place). Ulps scale with the magnitude of the 
values being compared in a way that reflects the source of 
representational rounding errors.

The boolean expression for effective equality is:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;close_enough(a, b) := &#124;a - b&#124; <= margin(a, b)

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;margin(a, b) := 2<sup>N</sup> * ulp(max(&#124;a&#124;, &#124;b&#124;), p<sub>rep</sub>) 

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;ulp(x,p<sub>rep</sub>) := 2<sup> max( &lfloor; log<sub>2</sub>(x) &rfloor; - p<sub>rep</sub>, exp<sub>min</sub> - p<sub>rep</sub>)  </sup>

where N is a user-defined template parameter, p<sub>rep</sub> is the number of 
fractional bits in the significand of the 
floating-point representation of x, and exp<sub>min</sub> 
is the exponent of the smallest normal value that can be represented (forcing exp<sub>min</sub> - p<sub>rep</sub> as a 
lower limit of the exponent ensures that the template works properly for denormal values of x.)

The above definition of ulp(x) differs from the accepted definition, which is:

>ulp(x) is the gap between the two finite floating-point numbers nearest x,  even if x is one of them.<sup>[1](#kahan)</sup>

Unfortunately, this definition is computationally problematic, as it depends on the abstract value x, which may not
be representable as a finite floating point value. Hence the above approximation, which could be stated:

>ulp(x) is the value of the least significant bit in the floating point representation of the value x.<sup>[2](#me)</sup>

### About the implementation

The definitions above are abstract. The implementation doesn't use heavyweight floating point calculations for logarithms and exponentiation. 

There are two implementations included, selectable at compile time with macro definitions. The first implementation
is a single template that uses the std::numeric_limits traits template, and standard library functions exp2() and ilogb(). It is generic, 
and should work portably for diverse floating-point representations.

The second implementation provides template specializations that use representation-specific bitwise operations instead of exp2() and ilogb(). 
The specializations are for float (single precision IEEE 754 format), double (double precision IEEE 754 format), and long double 
(x86 extended precision format). These specializations are about twice as fast as the generic template when compiled with -O2 or -O3
optimization levels.

The proximal.h header includes the following defines:

```` cpp
#define __USE_FLOAT_IEEE754_SPECIALIZATION__ 1
#define __USE_DOUBLE_IEEE754_SPECIALIZATION__ 1
#define __USE_LONG_DOUBLE_X86_EXTENDED_SPECIALIZATION__ 1
````
To select the generic template implementation for a floating point type, set the corresponding definition to 0.

### How to use it

Instantiate the template with a small number N for the parameter, and use 
it to compare values of some floating point type.

```` cpp
#include <proximal.h>

double a = 0.1;
double b = 1.0 - 0.9;

if (a == b) // will be false
{
	do_something();
}

utils::proximal<1> close_enough;
	
// the comparison is implemented as the function call operator:

if (close_enough(a, b)) // will be true
{
	do_something();
}
````

### What value should I use for the template parameter?

When you instantiate the template with some value N, it's roughly equivalent
to saying, "please ignore the last (that is, least significant) N + 1 bits 
when comparing numbers using this instantiation."

A comprehensive answer to the question posed is beyond the scope of this
discussion. The magnitude of representational noise that may accumulate
depends on the specific computation involved.<sup>[3](#goldberg)</sup>

As a rule of thumb, I find it useful to be conservative, so I set N to 1 
(margin = 2 ulps). Comparisons that are legitimately "close enough" may fail, 
in which case I analyze the source and magnitude of the error, and adjust N accordingly.
To some extent, this forces me to be more aware of the causes
of noise in floating point computations than I might otherwise be.

It can be useful to know the value of ulp(x) or the value of margin<N>(x).
The template provides methods for this:
```` cpp
#include <proximal.h>
	
utils::proximal<1> close_enough;
double x = ...
double m = close_enough.margin(x);
double u = close_enough.ulp(x);
````
There are also standalone function templates for ulp and margin:

```` cpp
float x = ...;
float m = utils::margin<2>(x); // template parameter same as class template
float u = utils::ulp(x);
```	

### Miscellany

This template will behave properly for comparisons involving denormal 
numbers, zeros of opposite sign, NaN values, and infinite values (as 
defined by floating point representation standards). Specifically:

* Comparing -0.0 with +0.0 yields true.

* For any legitimate normal or denormal value x, comparing x with a bitwise-identical value will yield true.

* Comparing two infinite numbers of the same sign will evaluate to true.

* Comparing infinity (regardless of sign) to any non-infinite value will yield false.

* A NaN value compared with anything, including another NaN value with bitwise-identical contents, will evaluate to false.

The template should work with any floating point type with binary exponents 
and significands. It won't work with decimal representations. A single 
template instantiation can be used to compare pairs of any floating point type:
```` cpp 
proximal<1> close_enough;

float a = ...; float b = ...;
if (close_enough(a, b)) {  ... }

long double d = ...; long double e = ...;
if (close_enough(d, e) { ... }
````
and so on. An attempt to compare dissimilar floating point will generate 
compile errors:

```` cpp
	if (close_enough(a, d) { ... }  // compile error
```
Arguments will not be implicitly type-converted.

### To do

* Provide comprehensive test cases.

* Test with more diverse floating point representations.


<a name="kahan">1</a>: Kahan, W. 2004. [A logarithm too clever by half](http://http.cs.berkeley.edu/
∼wkahan/LOG10HAF.TXT)

<a name="me">2</a>: Me. 2016. This document.

<a name="goldberg">3</a>: For an excellent, throrough treatement of practical issues with floating point arithmetic, see 
Goldberg, D. 1991. [What every computer scientist should know about floating-point arithmetic](http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html)

