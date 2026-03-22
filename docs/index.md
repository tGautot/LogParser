# Logram

> A fast, terminal-based log viewer that actually understands your logs.

`grep` and `tail` treat your logs as plain text. Logram lets you **define the structure** of your log lines, then navigate, filter, and search based on what the fields *mean* — not just what they look like.

## Quick example

Given log lines like:

```
0322 085338 TRACE  :......connect: Attempting connection to 192.168.1.10
0322 085339 INFO   :......connect: Connection established
0322 085340 WARN   :......retry: Timeout, retrying...
0322 085341 ERROR  :......retry: Max retries exceeded
```

You define the format once:

```
{INT:Date} {INT:Time} {STR:Level} {CHR:, ,1}:{STR:Source}: {STR:Message}
```

Then filter instantly:

```
Level EQUAL ERROR AND Time GREATER 085050
```

## Next steps

- [Getting Started](getting-started.md) — build and run LogParser
- [Format Strings](format-strings.md) — teach LogParser your log structure
- [Filtering](filtering.md) — filter by field value and combine conditions
- [Keybindings](keybindings.md) — navigate the interface
