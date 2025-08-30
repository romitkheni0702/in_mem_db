# Minimal wrapper Makefile for your CMake project (MinGW / MSYS2)

BDIR   ?= build
GEN    ?= MinGW Makefiles
CONFIG ?= Release

.PHONY: all configure build db tests test run demo clean rebuild

all: build

configure:
	cmake -S . -B $(BDIR) -G "$(GEN)" -DCMAKE_BUILD_TYPE=$(CONFIG)

build: configure
	cmake --build $(BDIR) --config $(CONFIG)

db: build

tests: build

# Run all Catch2 tests
test: build
	ctest --test-dir $(BDIR) -V

# Interactive run (finish with Ctrl+Z then Enter)
run: db
	cd $(BDIR) && .\db.exe

# Demo: generate SQL and feed it to the CLI
demo: db
	cmake -E make_directory $(BDIR)
	cmake -E echo "CREATE TABLE items (name str, qty int);" >  $(BDIR)/demo.sql
	cmake -E echo "INSERT INTO items (name, qty) VALUES (\"a\", 2), (\"b\", 14), (\"c\", -2);" >> $(BDIR)/demo.sql
	cmake -E echo "SELECT * FROM items;" >> $(BDIR)/demo.sql
	cmake -E echo "SELECT qty, name FROM items WHERE name = \"b\";" >> $(BDIR)/demo.sql
	cmake -E echo "UPDATE items SET name = \"bee\" WHERE qty = 14;" >> $(BDIR)/demo.sql
	cmake -E echo "SELECT * FROM items;" >> $(BDIR)/demo.sql
	cmake -E echo "DELETE FROM items WHERE qty < 0;" >> $(BDIR)/demo.sql
	cmake -E echo "SELECT * FROM items;" >> $(BDIR)/demo.sql
	cd $(BDIR) && .\db.exe < demo.sql

clean:
	- cmake --build $(BDIR) --target clean --config $(CONFIG)
	cmake -E rm -rf $(BDIR)

rebuild: clean all
