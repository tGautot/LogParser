# Format Strings

A format string tells LogParser how to parse each line of your log file into named, typed fields.

## Field types

| Type | Syntax | Matches |
|------|--------|---------|
| Integer | `{INT:Name}` | A sequence of digits |
| Float | `{DBL:Name}` | A decimal number |
| String | `{STR:Name}` | Any text up to the next delimiter |
| Character-delimited | `{CHR:delim,Name}` | Text up to a specific character |

Field names are optional, but required if you want to filter on that field.

## How parsing works

<!-- TODO: explain the parsing model — literal separators, greedy vs. delimited, etc. -->

## Examples

### Simple space-separated fields

```
0322 085338 TRACE  :......connect: Attempting connection
```

```
{INT:Date} {INT:Time} {STR:Level} {CHR:, ,1}:{STR:Source}: {STR:Message}
```

### Zookeeper-style timestamp

```
2015-07-29 17:41:41,536 - INFO  [MainThread:Server@42] - Connection accepted
```

```
{INT:Year}-{INT:Month}-{INT:Day} {INT:Hour}:{INT:Min}:{INT:Sec},{INT:Ms} - {STR:Level} [{STR:Thread}] - {STR:Message}
```

## Setting the format

<!-- TODO: describe how to enter the format string in the UI -->
