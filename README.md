
This language has three pairs of symmetric operators that operate on proofs.


<table>
<tr>
<td>

```assume [proof name] proves [expression];```

This introduces an implication


</td>
<td>

`[proof] > [proof]`

This resolves an implication

</td>
</tr>
<tr></tr>
<tr>
<td>

`forany [variable name]:`

This introduces a variable
</td>
<td>

`substitute [variable name] = [expression] in [proof]`

This resolves a variable

</td>
</tr>
<tr></tr>
<tr>
<td>

`wrap [function name] [proof]`

This rewrites the proven proposition as a function

</td>
<td>

`unwrap [proof]`

This rewrites a proven proposition as the definition of the function it is written in

</td>
</tr>
</table>
<br/>


There are four additional syntaxes:

<table>
<tr>
<td>

`define [function name] [function parameter names] = [expression];`

This defines a function

</td>
</tr>
<tr></tr>
<tr>
<td>

`atom [atom name];`

This declares an identifier

</td>
</tr>
<tr></tr>
<tr>
<td>

`require [proof name] proves [expression] by [proof];`

This checks whether the proof actually proves the given proposition. If it does, the proof is given a name. The name can then be used in subsequent proofs.

</td>
</tr>
<tr></tr>
<tr>
<td>

`syntax [syntax specification] = [expression], precedence [int], associativity [left,right,noassoc];`

This lets you define a custom syntax. There are no expression operators built into the language.

</td>
</tr>
</table>
<br/>


All expressions are either `[identifier]` or `[expression] [expression]`. Parentheses can be used to disambiguate.

The `IMPLIES` identifier is built into the language. The `>` / `<` operator in proofs must point towards a proof of a proposition of the form `IMPLIES a b`

`stdlib.bus` contains a set of basic definitions and proofs regarding unsigned integers, and finally a proof that there are infinitely many prime numbers.

`assume` statements in global scope are axioms.


A basic example:

```
syntax (a -> b) = IMPLIES a b, precedence 100, associativity right;
atom AND;
syntax (a && b) = AND a b, precedence 150, associativity left;
assume and_left  proves forany a,b: (a && b) -> a;
assume and_right proves forany a,b: (a && b) -> b;


atom FOO, BAR;
assume foobar proves FOO && BAR;
require foo   proves FOO   by foobar > and_left;   // Correct proof
require bar   proves BAR   by foobar > and_right;  // Correct proof
require nope  proves BAR   by foobar > and_left;   // <-- Proof error: Mismatch between FOO and BAR
```







