# Brian

Brian is a term rewriter -based language and its interpreter written in C.

## The Language
In general, the syntax of Brian is based on Terms, much like any algebraic expression is made of terms.  
A word starting with a lower case letter is a Constant.  
A word starting with an upper case letter is a Variable.
A word starting with a minus or digit which contains only digits (and optionally a single period) is a number.   
A list is a comma separated sequence of terms within opening and closing brackets ([ and ]) or curly brackets ({ and }).  
A string is any sequence of printable or non-printable characters contained within double quotes ("). Strings are turned into lists of single character Constants; note this is the only way to have a Constant which is an upper case letter or a symbol (or white space).  
Constants, Variables, Numbers, and Lists are terms and any application of a Binary Operator to a term on the left side and a term on the right side is also a term.  
Parenthesis should be used to establish operator precedence. 
Predefined Binary Operators include implication (->), application (@), and right associative sequencing (,). All Binary Operators are left associative, except the comma. No unary operators are provided.

More formally...

### EBNF

    lower::= "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z" ;  
    upper::= "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z" ;  
    digit::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;  
    punctuation::= "~" | "`" | "!" | "$" | "%" | "^" | "&" | "*" | "_" | "-" | "+" | "=" | "|" | "\" | ":" | ";" | "'" | "<" | ">" | "?" | "/" ;  
    constant ::= lower , { lower | upper | digit } | digit , { lower | upper | digit } ;
    variable ::= upper , { lower | upper | digit } ;
    number ::= [ "-" ] , digit , { digit } , [ "." , { digit} ] ;
    binaryop ::= "," | "@" | punctuation , { punctuation } ;
    string ::= "\"" , {any character except "} , "\"" ;
    list ::= "[" , term , { "," , term } , "]" | "{" , term , { "," , term } , "}" ;
    term ::= constant | variable | number | list | {"("} , term , binaryop , term , {")"};
    rule ::= term , "->" , term , ".";
    statement ::= term , ".";

## Status
Work in progress.

TODOs
- Garbage Collection
  - Tokens (done)
  - Unused astNodes
  - Unused char lists
- Side effects
  - output and input
    - console
    - file
  - environments with variables and closures  
  - math
