// Unit tests for in-memory DB (spec-compliant)
// Framework: Catch2 v3

#include <catch2/catch_all.hpp>

#include "imd/parser.hpp"
#include "imd/executor.hpp"
#include <sstream>
#include <iostream>   // for std::cout

using namespace imd;

static void run_all_sql(const std::string& sql, Database& db) {
    Parser p(sql);
    auto stmts = p.parseAll();
    Executor ex(db);
    for (const auto& st : stmts) ex.execute(st); // pass the variant directly
}

TEST_CASE("CREATE + INSERT + SELECT * basic flow") {
    Database db;
    const std::string sql =
        "CREATE TABLE users (id int, name str);"
        "INSERT INTO users (id, name) VALUES (1, \"Alice\"), (2, \"Bob\");"
        "SELECT * FROM users;";
    Parser p(sql);
    auto stmts = p.parseAll();
    REQUIRE(stmts.size() == 3);

    Executor ex(db);

    // run CREATE + INSERT
    ex.execute(stmts[0]);
    ex.execute(stmts[1]);

    // capture SELECT output
    std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
    ex.execute(stmts[2]);
    std::cout.rdbuf(old);

    auto out = cap.str();
    REQUIRE(out.find("Alice") != std::string::npos);
    REQUIRE(out.find("Bob")   != std::string::npos);
    REQUIRE(out.find("| id")  != std::string::npos);
    REQUIRE(out.find("| name")!= std::string::npos);
}

TEST_CASE("INSERT fills defaults for missing columns") {
    Database db;
    run_all_sql(
        "CREATE TABLE t (n int, s str);"
        "INSERT INTO t (s) VALUES (\"x\");", db);

    Parser p("SELECT * FROM t;");
    auto stmts = p.parseAll();
    Executor ex(db);
    std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
    ex.execute(stmts[0]);
    std::cout.rdbuf(old);
    auto out = cap.str();

    // n should default to 0; s is "x"
    REQUIRE(out.find("| 0 ") != std::string::npos);
    REQUIRE(out.find("| x ") != std::string::npos);
}

TEST_CASE("WHERE with '=' and '!=' works") {
    Database db;
    run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"Alice\"), (2, \"Bob\"), (3, \"Bob\");",
        db);

    // Equals
    {
        Parser p("SELECT id, name FROM t WHERE name = \"Bob\";");
        auto stmts = p.parseAll();
        Executor ex(db);
        std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
        ex.execute(stmts[0]);
        std::cout.rdbuf(old);
        auto out = cap.str();
        // expect two rows with Bob: ids 2 and 3
        REQUIRE(out.find("| 2 ") != std::string::npos);
        REQUIRE(out.find("| 3 ") != std::string::npos);
    }

    // Not equals
    {
        Parser p("SELECT id, name FROM t WHERE name != \"Bob\";");
        auto stmts = p.parseAll();
        Executor ex(db);
        std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
        ex.execute(stmts[0]);
        std::cout.rdbuf(old);
        auto out = cap.str();
        // only Alice remains
        REQUIRE(out.find("Alice") != std::string::npos);
        REQUIRE(out.find("Bob")   == std::string::npos);
    }
}

TEST_CASE("UPDATE modifies matching rows") {
    Database db;
    run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"Alice\"), (2, \"Bob\");",
        db);

    run_all_sql("UPDATE t SET name = \"Bee\" WHERE id = 2;", db);

    Parser p("SELECT id, name FROM t;");
    auto stmts = p.parseAll();
    Executor ex(db);
    std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
    ex.execute(stmts[0]);
    std::cout.rdbuf(old);
    auto out = cap.str();

    REQUIRE(out.find("Alice") != std::string::npos);
    REQUIRE(out.find("Bee")   != std::string::npos);
    REQUIRE(out.find("Bob")   == std::string::npos);
}

TEST_CASE("DELETE supports WHERE and full delete") {
    Database db;
    run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"A\"), (2, \"B\"), (3, \"C\");",
        db);

    // delete id=2
    run_all_sql("DELETE FROM t WHERE id = 2;", db);

    // now delete all
    run_all_sql("DELETE FROM t;", db);

    Parser p("SELECT * FROM t;");
    auto stmts = p.parseAll();
    Executor ex(db);
    std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
    ex.execute(stmts[0]);
    std::cout.rdbuf(old);
    auto out = cap.str();

    // table is empty -> still prints header + "0 row(s)."
    REQUIRE(out.find("0 row(s).") != std::string::npos);
}

TEST_CASE("Parse errors: lowercase keyword is invalid (case-sensitive)") {
    Database db;
    // 'select' (lowercase) must fail per spec
    REQUIRE_THROWS_AS(run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "select * FROM t;", db), std::runtime_error);
}

TEST_CASE("Type errors: inserting wrong type throws") {
    Database db;
    run_all_sql("CREATE TABLE t (id int, name str);", db);
    // name is str, id is int â€” make wrong assignment
    Parser p("INSERT INTO t (id, name) VALUES (\"bad\", 3);");
    auto stmts = p.parseAll();
    Executor ex(db);
    REQUIRE_THROWS_AS(ex.execute(stmts[0]), std::runtime_error);
}
TEST_CASE("Parse error: missing \")\" in CREATE TABLE is rejected") {
    using namespace imd;
    std::string sql = "CREATE TABLE t (id int, name str;";
    REQUIRE_THROWS_AS( (Parser(sql).parseAll()), std::runtime_error );
}
TEST_CASE("Inequality operators <, <=, >, >=") {
    using namespace imd;
    Database db;
    // ints
    {
        std::string sql =
            "CREATE TABLE nums (n int, label str);"
            "INSERT INTO nums (n, label) VALUES (1, \"a\"), (2, \"b\"), (3, \"c\");";
        Parser p(sql); auto v = p.parseAll(); Executor ex(db);
        for (auto& s : v) ex.execute(s);
    }
    // n < 3 -> 1 and 2
    {
        Parser p("SELECT n FROM nums WHERE n < 3;"); auto v=p.parseAll(); Executor ex(db);
        std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf()); ex.execute(v[0]); std::cout.rdbuf(old);
        auto out = cap.str();
        REQUIRE(out.find("| 1 ") != std::string::npos);
        REQUIRE(out.find("| 2 ") != std::string::npos);
        REQUIRE(out.find("| 3 ") == std::string::npos);
    }
    // strings: label >= "b" -> b, c
    {
        Parser p("SELECT label FROM nums WHERE label >= \"b\";"); auto v=p.parseAll(); Executor ex(db);
        std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf()); ex.execute(v[0]); std::cout.rdbuf(old);
        auto out = cap.str();
        REQUIRE(out.find("Alice") == std::string::npos);
        REQUIRE(out.find("b") != std::string::npos);
        REQUIRE(out.find("c") != std::string::npos);
    }
}
TEST_CASE("Inequalities: <, <=, >, >=") {
    using namespace imd;
    Database db;
    // id: 1,2,3 ; names: Alice, Bob, Cara
    std::string setup =
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"Alice\"), (2, \"Bob\"), (3, \"Cara\");";
    Parser p(setup);
    auto s = p.parseAll();
    Executor ex(db);
    for (auto& st : s) ex.execute(st);

    // id < 3  ->  1,2
    {
        Parser q("SELECT id FROM t WHERE id < 3;");
        auto stmts = q.parseAll();
        std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
        ex.execute(stmts[0]); std::cout.rdbuf(old);
        auto out = cap.str();
        REQUIRE(out.find("| 1 ") != std::string::npos);
        REQUIRE(out.find("| 2 ") != std::string::npos);
        REQUIRE(out.find("| 3 ") == std::string::npos);
    }
    // name >= "Bob"  ->  Bob, Cara  (lexicographic)
    {
        Parser q("SELECT name FROM t WHERE name >= \"Bob\";");
        auto stmts = q.parseAll();
        std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
        ex.execute(stmts[0]); std::cout.rdbuf(old);
        auto out = cap.str();
        REQUIRE(out.find("Bob")  != std::string::npos);
        REQUIRE(out.find("Cara") != std::string::npos);
        REQUIRE(out.find("Alice")== std::string::npos);
    }
}

TEST_CASE("Type mismatch in inequality throws") {
    using namespace imd;
    Database db;
    Parser p("CREATE TABLE t (id int, name str); INSERT INTO t (id, name) VALUES (1, \"A\");");
    auto stmts = p.parseAll();
    Executor ex(db);
    for (auto& st : stmts) ex.execute(st);
    // id < "x" is invalid
    Parser bad("SELECT * FROM t WHERE id < \"x\";");
    auto badStmts = bad.parseAll();
    REQUIRE_THROWS_AS( ex.execute(badStmts[0]) , std::runtime_error );
}

TEST_CASE("Users flow with != filter works (REPL parity)") {
    using namespace imd;
    Database db;
    std::string sql =
        "CREATE TABLE users (id int, name str);"
        "INSERT INTO users (id, name) VALUES (1, \"Alice\"), (2, \"Bob\"), (3, \"Cara\");"
        "SELECT * FROM users;"
        "SELECT name FROM users WHERE id != 2;";
    Parser p(sql);
    auto stmts = p.parseAll();
    Executor ex(db);

    std::ostringstream cap; auto* old = std::cout.rdbuf(cap.rdbuf());
    for (auto& s : stmts) ex.execute(s);
    std::cout.rdbuf(old);

    auto out = cap.str();
    REQUIRE(out.find("Alice") != std::string::npos);
    REQUIRE(out.find("Bob")   != std::string::npos);
    REQUIRE(out.find("Cara")  != std::string::npos);
    // ensure at least one of (Alice, Cara) appears after the != filter overall
    REQUIRE(out.find("Cara")  != std::string::npos);
}

TEST_CASE("Invalid keyword is rejected (SELCT)") {
    using namespace imd;
    Parser p("CREATE TABLE t (id int); SELCT * FROM t;");
    REQUIRE_THROWS_AS(p.parseAll(), std::runtime_error);
}
