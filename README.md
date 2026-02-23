<h2>Propositions</h2>

A proposition is any statement that could be true or false.

For example in English these:
- clouds are white
- clouds are purple
- 20 is greater than 5
- 7 is greater than 18
- i like penguins

are all propositions.

You can use the 'implies' operator ( ⟶ ) to form a new proposition from two propositions.<br/>A ⟶ B means "if A is true then B must also be true".
<br/>In this language propositions are represented as expressions.


<br/>
<h2>Expressions</h2>

An expression is one of these three things:
- An identifier (the name of a function, atom, or variable)
- `[expression] [expression]`, the right-hand side expression is applied to the left-hand side to form a larger expression
- `forany [variable list]: [expression]`, one or more variables are introduced that can be used as identifiers in the inner expression

Parentheses can be used to disambiguate.

BusLang has one built-in identifier: `IMPLIES`.<br/>It represents the 'implies' operator in propositions:&nbsp;&nbsp;&nbsp; `(IMPLIES a) b` &nbsp;&nbsp;means &nbsp;&nbsp;a ⟶ b


<br/>
<h2>Functions and atoms</h2>

```
atom foo, bar;
```

An atom is just a declared identifier. This syntax exists to help detect typos and to let you explicitly set the scope of an identifier.

```
define Func a b = foo a b (bar b a);
```

Functions serve three purposes:
- Letting you write common long expressions as shorter ones
- Letting you write recursive expressions (functions can be defined in terms of themselves)
- Letting you write functional expressions that generalize over different functions which may have wildly different innards

<br/>
<h2>Proofs</h2>

A proof is an argument that some proposition is true.<br/>
This language has three pairs of syntaxes to represent these arguments:

<table>
<tr>
<td>

```
assume [proof name] proves [expression];
[proof]
```

This introduces an implication. Within the inner proof the proof name represents a proof of the expression. This syntax yields a proof of `[expression] ⟶ [result of inner proof]`


</td>
<td>

```
[proof] > [proof]
```

This resolves an implication. If the left-hand side is a proof of `A` and the right-hand side is a proof of `A ⟶ B` then this syntax yields a proof of `B`

</td>
</tr>
<tr></tr>
<tr>
<td>
<br/>

```
forany [variable list]:
[proof]
```


This introduces one or more variables that can be used as identifiers in the inner proof. This syntax yields a proof of `forany [variables]: [result of inner proof]`
</td>
<td>
<br/>

```
substitute [variable name] = [expression] in [proof]
```

This resolves a variable in the inner proof. If the inner proof proves `forany x: F x` then `substitute x=test in [proof]` yields a proof of `F test`

</td>
</tr>
<tr></tr>
<tr>
<td>
<br/>

```
wrap [function name] [proof]
```

This rewrites the proven proposition as a function.

</td>
<td>
<br/>

```
unwrap [proof]
```

This rewrites a proven proposition as the definition of the function it is written in

</td>
</tr>
<tr><td></td><td></td></tr>
</table>
<br/>


There are two additional syntaxes that can be used in a proof:

```
require [proof name] proves [expression] by [proof];
```

This checks whether the proof actually proves the given proposition. If it does, the proof is given a name that can then be used as a proof in subsequent proofs within the current scope.


```
syntax [syntax specification] = [expression], precedence [int], associativity [left/right/noassoc];
```

This lets you define a custom syntax. There are no expression operators built into the language.


<br/>
<h2>Examples</h2>

`stdlib.bus` contains a set of basic definitions and proofs regarding unsigned integers, and finally a proof that there are infinitely many prime numbers.

Some basic example code:

```
syntax (a -> b) = IMPLIES a b, precedence 100, associativity right;
atom AND;
syntax (a && b) = AND a b, precedence 150, associativity left;
assume and_left  proves forany a,b: (a && b) -> a;
assume and_right proves forany a,b: (a && b) -> b;


atom FOO, BAR, KEZ;
assume foobar proves FOO && BAR;
require foo    proves FOO   by foobar > and_left;   // Correct proof
require bar    proves BAR   by foobar > and_right;  // Correct proof
//require nope proves BAR   by foobar > and_left;   // <-- Proof error: Mismatch between FOO and BAR

assume  test1 proves forany a, b:  FOO a b   ->  KEZ b a;
require test2 proves forany a:     FOO a BAR ->  KEZ BAR a
  by substitute b=BAR in test1;

atom ULF;
assume  test3 proves forany a: FOO ULF a;
require test4 proves forany a: KEZ a ULF
  by test3 > test1;

atom JIM;
require test5 proves forany a: FOO JIM a  ->  KEZ a JIM
  by (
    forany a,b:
    assume hypoth proves FOO JIM a;
    hypoth > test1
  );
```





