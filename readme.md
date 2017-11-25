QL Language
===========

QL is a script language with object oriented capabilities.
The lifetime of a script is described as follows:

1) *.ql script is compiled to internal bytecode
2) optionally bytecode can be written to a *.qob file
3) bytecode can be interpreted
4) various bytecode object files can be loaded and linked before execution

The script language data types have been designed to encompass:

a) integer   (you guessed it: 32 bits signed integer)
b) string    (as in a string of characters)
c) bcd       binary decimal floating point up to 40 decimal places
d) database  A pointer to a database object
c) query     A pointer to a query object
d) variant   A result from a database query (can hold any database datatype)
e) file      A file pointer
f) array     An array of elementary datatypes (integer, string, bcd)

See also de definition file: QL_in_BNF.txt
for a definition of the language
