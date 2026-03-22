# Filtering

Filters let you show only the log lines that match conditions on named fields.

## Filter syntax

```
<FieldName> <Operator> <Value>
```

Example:

```
Level EQUAL ERROR
```

## Operators

| Operator | Description |
|----------|-------------|
| `EQUAL` | Field value equals the given value |
| `CONTAINS` | Field value contains the given string |
| `BEGINS_WITH` | Field value starts with the given string |
| `ENDS_WITH` | Field value ends with the given string |
| `GREATER` | Field value is greater than the given value |
| `SMALLER` | Field value is less than the given value |

<!-- TODO: clarify numeric vs. string comparison for GREATER/SMALLER -->

## Combining filters

Multiple filters can be combined with boolean operators:

| Operator | Behaviour |
|----------|-----------|
| `AND` | Both conditions must match |
| `OR` | Either condition must match |
| `XOR` | Exactly one condition must match |
| `NOR` | Neither condition must match |

Example:

```
Level EQUAL ERROR AND Time GREATER 085339
```

<!-- TODO: document precedence rules and grouping, if any -->

## Applying and removing filters

<!-- TODO: describe the UI workflow for entering, toggling, and clearing filters -->
