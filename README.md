# MiniSQL

A small in-memory SQL database written in C++17. It has a hand-written lexer and
recursive-descent parser, an executor that works on in-memory tables, and a CLI that
prints results as ASCII tables.

```
mini> CREATE TABLE users (id int, name str);
mini> INSERT INTO users (id, name) VALUES (1, "Alice"), (2, "Bob"), (3, "Cara");
mini> SELECT name FROM users WHERE id != 2;
+-------+
| name  |
+-------+
| Alice |
| Cara  |
+-------+
2 row(s).
```

## Supported SQL

| Statement | Example |
|-----------|---------|
| `CREATE TABLE` | `CREATE TABLE items (name str, qty int);` |
| `INSERT INTO ... VALUES` | `INSERT INTO items (name, qty) VALUES ("a", 2), ("b", 14);` |
| `SELECT` | `SELECT * FROM items;` / `SELECT qty, name FROM items WHERE name = "b";` |
| `UPDATE ... SET` | `UPDATE items SET name = "bee" WHERE qty = 14;` |
| `DELETE FROM` | `DELETE FROM items WHERE qty < 0;` |

- Column types: `int` and `str`.
- `WHERE` supports `=`, `!=`, `<`, `<=`, `>`, `>=` against a literal.
- Keywords are case-sensitive and must be uppercase.
- Columns left out of an `INSERT` get a default value (`0` or `""`).
- Type mismatches and syntax errors are reported as errors. In the interactive shell the
  session continues; when a script is piped in, the whole script is parsed first, so one
  invalid statement rejects the script.

## Build

Requires CMake 3.16+ and a C++17 compiler. GoogleTest is downloaded automatically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

On Windows with MinGW, the included `Makefile` wraps the same steps (`make`, `make test`, `make demo`).

## Run

Interactive shell (type `EXIT` or `QUIT` to leave):

```sh
./build/db
```

Run a script file:

```sh
./build/db < script.sql
```

Options: `--no-banner` hides the startup banner, `--banner-color` forces colored output.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

The test suite (`tests/test_gtest.cpp`) covers create/insert/select, default values,
`WHERE` comparisons, update, delete, keyword case sensitivity, and error handling.

## Project structure

```
include/imd/   headers: AST, lexer, parser, executor, renderer
src/           implementation
app/main.cpp   CLI / REPL
tests/         GoogleTest suite
```

## Authors

Romit Jayeshbhai Kheni and Rajkumar Pambhar
