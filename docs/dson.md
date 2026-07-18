# JSON And DSON

Both formats parse into and write from the same format-neutral `Value` tree.
Derived code therefore does not depend on either wire syntax.

## JSON

JSON is intended for interchange. Serdde preserves exact signed and unsigned
integer values while parsing, validates UTF-8, and emits finite floating-point
values with locale-independent conversion.

JSON cannot spell separate signed and unsigned categories. Typed conversion
checks the destination range when decoding.

## DSON

DSON is a deterministic typed text format used for exact roundtrips and
conformance testing. Its grammar is intentionally small:

```text
null
bool(true)
i64(-2)
u64(2)
f64(2.5)
str(3:Ada)
seq(3:[i64(1),u64(2),f64(3.0)])
obj(2:{str(2:id)=u64(7),str(4:name)=str(3:Ada)})
```

String lengths and sequence/object counts are byte counts or item counts,
respectively. Parsers reject mismatches, duplicate object keys, malformed
numbers, invalid UTF-8, trailing input, and excessive nesting.

DSON is not a replacement for an established interoperable format. It proves
that Serdde's derive and conversion layers are format-neutral and provides a
stable exact representation for tests and debugging.
