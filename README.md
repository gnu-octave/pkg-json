# Octave JSON package

JSON support by Matlab compatible (jsondecode / jsonencode) functions.

The code is courtesy of [@Abdallah-Elshamy](https://github.com/Abdallah-Elshamy/)
from his 2020 Google Summer of Code (GSoC) project, which will be part of Octave 7.


## Installation

From the Octave command-line:

```octave
pkg install "https://github.com/gnu-octave/pkg-json/archive/v1.4.0.tar.gz"
```


## jsondecode

```
OBJECT = jsondecode (JSON_TXT)
OBJECT = jsondecode (..., "ReplacementStyle", RS)
OBJECT = jsondecode (..., "Prefix", PFX)
OBJECT = jsondecode (..., "makeValidName", TF)
```
Decode text that is formatted in JSON.

The input `JSON_TXT` is a string that contains JSON text.

The output `OBJECT` is an Octave object that contains the result of
decoding `JSON_TXT`.

For more information about the options `"ReplacementStyle"` and
`"Prefix"`, see `matlab.lang.makeValidName`.

If the value of the option `makeValidName` is false then names will not be
changed by `matlab.lang.makeValidName` and the `"ReplacementStyle"` and
`"Prefix"` options will be ignored.

NOTE: Decoding and encoding JSON text is not guaranteed to
reproduce the original text as some names may be changed by
`'matlab.lang.makeValidName'`.

This table shows the conversions from JSON data types to Octave data types:

JSON data type                     | Octave data type
-----------------------------------|--------------------------------------
Boolean                            | scalar logical
Number                             | scalar double
String                             | vector of characters
Object                             | scalar struct (field names of the struct may be different from the keys of the JSON object due to `matlab_lang_makeValidName`)
null, inside a numeric array       | 'NaN'
null, inside a non-numeric array   | empty double array '[]'
Array, of different data types     | cell array
Array, of Booleans                 | logical array
Array, of Numbers                  | double array
Array, of Strings                  | cell array of character vectors ('cellstr')
Array of Objects, same field names | struct array
Array of Objects, different field  | cell array of scalar structs names


### Examples:

```
jsondecode ('[1, 2, null, 3]')
    => ans =

      1
      2
    NaN
      3


jsondecode ('["foo", "bar", ["foo", "bar"]]')
    => ans =
        {
          [1,1] = foo
          [2,1] = bar
          [3,1] =
          {
            [1,1] = foo
            [2,1] = bar
          }

        }


jsondecode ('{"nu#m#ber": 7, "s#tr#ing": "hi"}', ...
            'ReplacementStyle', 'delete')
    => scalar structure containing the fields:

          number = 7
          string = hi


jsondecode ('{"nu#m#ber": 7, "s#tr#ing": "hi"}', ...
            'makeValidName', false)
    => scalar structure containing the fields:

         nu#m#ber = 7
         s#tr#ing = hi


jsondecode ('{"1": "one", "2": "two"}', 'Prefix', 'm_')
    => scalar structure containing the fields:

          m_1 = one
          m_2 = two
```

## jsonencode

```
JSON_TXT = jsonencode (OBJECT)
JSON_TXT = jsonencode (..., "ConvertInfAndNaN", TF)
JSON_TXT = jsonencode (..., "PrettyPrint", TF)
```

Encode Octave data types into JSON text.

The input `OBJECT` is an Octave variable to encode.

The output `JSON_TXT` is the JSON text that contains the result of
encoding `OBJECT`.

If the value of the option `"ConvertInfAndNaN"` is true then `'NaN'`,
`'NA'`, `'-Inf'`, and `'Inf'` values will be converted to `"null"` in the
output.  If it is false then they will remain as their original
values.  The default value for this option is true.

If the value of the option `"PrettyPrint"` is true, the output text
will have indentations and line feeds.  If it is false, the output
will be condensed and written without whitespace.  The default
value for this option is false.


### Programming Notes:

- Complex numbers are not supported.

- classdef objects are first converted to structs and then encoded.

- To preserve escape characters (e.g., `"\n"`), use single-quoted strings.

- Every character after the null character (`"\0"`) in a
  double-quoted string will be dropped during encoding.

- Encoding and decoding an array is not guaranteed to preserve
  the dimensions of the array.  In particular, row vectors will
  be reshaped to column vectors.

- Encoding and decoding is not guaranteed to preserve the Octave
  data type because JSON supports fewer data types than Octave.
  For example, if you encode an 'int8' and then decode it, you
  will get a 'double'.

This table shows the conversions from Octave data types to JSON data types:

Octave data type           | JSON data type
---------------------------|--------------------------------------
logical scalar             | Boolean
logical vector             | Array of Boolean, reshaped to row vector
logical array              | nested Array of Boolean
numeric scalar             | Number
numeric vector             | Array of Number, reshaped to row vector
numeric array              | nested Array of Number
'NaN', 'NA', 'Inf', '-Inf' | "null" (1)
'NaN', 'NA', 'Inf', '-Inf' | "NaN", "NaN", "Infinity", "-Infinity" (2)
empty array                | "[]"
character vector           | String
character array            | Array of String
empty character array      | ""
cell scalar                | Array
cell vector                | Array, reshaped to row vector
cell array                 | Array, flattened to row vector
struct scalar              | Object
struct vector              | Array of Object, reshaped to row vector
struct array               | nested Array of Object
classdef object            | Object

(1) When `"ConvertInfAndNaN" = true`.
(2) When `"ConvertInfAndNaN" = false`.


### Examples:

```
jsonencode ([1, NaN; 3, 4])
=> [[1,null],[3,4]]

jsonencode ([1, NaN; 3, 4], "ConvertInfAndNaN", false)
=> [[1,NaN],[3,4]]

## Escape characters inside a single-quoted string
jsonencode ('\0\a\b\t\n\v\f\r')
=> "\\0\\a\\b\\t\\n\\v\\f\\r"

## Escape characters inside a double-quoted string
jsonencode ("\a\b\t\n\v\f\r")
=> "\u0007\b\t\n\u000B\f\r"

jsonencode ([true; false], "PrettyPrint", true)
=> ans = [
      true,
      false
    ]

jsonencode (['foo', 'bar'; 'foo', 'bar'])
=> ["foobar","foobar"]

jsonencode (struct ('a', Inf, 'b', [], 'c', struct ()))
=> {"a":null,"b":[],"c":{}}

jsonencode (struct ('structarray', struct ('a', {1; 3}, 'b', {2; 4})))
=> {"structarray":[{"a":1,"b":2},{"a":3,"b":4}]}

jsonencode ({'foo'; 'bar'; {'foo'; 'bar'}})
=> ["foo","bar",["foo","bar"]]

jsonencode (containers.Map({'foo'; 'bar'; 'baz'}, [1, 2, 3]))
=> {"bar":2,"baz":3,"foo":1}
```
