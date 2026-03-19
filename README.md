# LogParser

> A fast, terminal-based log viewer that actually understands your logs.

`grep` and `tail` are powerful, but they treat your logs as plain text. LogParser lets you **define the structure** of your log lines, then navigate, filter, and search based on what the fields *mean* — not just what they look like.

---

## The problem

You're staring at thousands of log lines like this:

```
0322 085338 TRACE  :......connect: Attempting connection to 192.168.1.10
0322 085339 INFO   :......connect: Connection established
0322 085340 WARN   :......retry: Timeout, retrying...
0322 085341 ERROR  :......retry: Max retries exceeded
```

You want to see **only the ERRORs from the `retry` function**. With grep, you're writing fragile regexes. With a log aggregator, you're copy-pasting into a browser. With LogParser, you define the format once and filter by field.

---

## How it works

**1. Define your log format**

Tell LogParser what your lines look like using a simple format string:

```
{INT:Date} {INT:Time} {STR:Level} {CHR:, ,1}:{STR:Source}: {STR:Message}
```

This extracts `Date`, `Time`, `Level`, `Source`, and `Message` as typed, named fields.

**2. Navigate your file**

Use vim-style keys (`hjkl`, `gg`, `G`, `:42`) to move through the file at any scale. LogParser uses memory-mapped file access — large files are no problem.

**3. Filter by field**

Apply filters like:
- `Level EQUAL ERROR`
- `Source BEGINS_WITH retry`
- `Time GREATER 085339`

Combine them with `AND`, `OR`, `XOR` logic. Only matching lines are shown.

**4. Search within results**

Use `/pattern` to find text within your filtered view. `n`/`N` jump to next/previous match.

---

## Features

- **Custom format definitions** — supports `INT`, `DBL`, `STR`, and character-delimited (`CHR`) fields
- **Field-based filtering** — `EQUAL`, `CONTAINS`, `BEGINS_WITH`, `ENDS_WITH`, `GREATER`, `SMALLER`, and more
- **Boolean filter composition** — combine multiple filters with `AND`, `OR`, `XOR`, `NOR`
- **Vim keybindings** — `hjkl`, `gg`, `G`, `:q`, `/search`, `n`/`N`
- **Memory-mapped I/O** — handles large log files efficiently without loading them into RAM
- **Persistent per-file config** — format and settings are remembered per log file in `~/.lr`
- **ANSI color support** — customize colors for background, text, and selection
- **Hide malformed lines** — optionally suppress lines that don't match the expected format

---

## Getting started

### Build

```bash
git clone <repo-url>
cd LogParser
mkdir build && cd build
cmake ..
make
```

Requires: C++17, CMake 3.21+, a Unix/Linux system.

### Run

```bash
./build/bin/lp_term path/to/your.log
```

On first open, LogParser creates a profile for this file in `~/.lr`. Set your format string, and it will be remembered next time.

---

## Format string syntax

A format string is a sequence of **typed fields** separated by literal characters:

| Type | Syntax | Matches |
|------|--------|---------|
| Integer | `{INT:Name}` | A sequence of digits |
| Float | `{DBL:Name}` | A decimal number |
| String | `{STR:Name}` | Any text up to the next delimiter |
| Character-delimited | `{CHR:delim,Name}` | Text up to a specific character |

Field names are optional but required for filtering.

**Example** — a Zookeeper-style log line:
```
2015-07-29 17:41:41,536 - INFO  [MainThread:Server@42] - Connection accepted
```
Format:
```
{INT:Year}-{INT:Month}-{INT:Day} {INT:Hour}:{INT:Min}:{INT:Sec},{INT:Ms} - {STR:Level} [{STR:Thread}] - {STR:Message}
```

---

## Keybindings

| Key | Action |
|-----|--------|
| `h` / `←` | Scroll left |
| `l` / `→` | Scroll right |
| `j` / `↓` | Move down |
| `k` / `↑` | Move up |
| `gg` | Jump to start of file |
| `G` | Jump to end of file |
| `:N` | Jump to line N |
| `/` | Enter search mode |
| `n` / `N` | Next / previous match |
| `:q` | Quit |

---

## Why not just use `grep`?

| | grep / tail | LogParser |
|---|-------------|-----------|
| Understands log structure | No | Yes |
| Filter by field value | No | Yes |
| Combine multiple filters | Awkward | Yes |
| Navigate large files interactively | No | Yes |
| Remembers your format per file | No | Yes |
| Handles GB-scale files | Partially | Yes |

LogParser is not a replacement for log aggregation platforms — it's a replacement for the ad-hoc grep sessions you run when you're already on the machine and need answers fast.

---

## License

[MIT](LICENSE)
