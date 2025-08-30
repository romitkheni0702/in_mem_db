#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <iostream>
#include "imd/parser.hpp"
#include "imd/executor.hpp"

using namespace imd;

static void run_all_sql(const std::string& sql, Database& db) {
    Parser p(sql);
    auto stmts = p.parseAll();
    Executor ex(db);
    for (auto& st : stmts) ex.execute(st);
}

static std::string run_select(const std::string& sql, Database& db) {
    Parser p(sql);
    auto stmts = p.parseAll();
    Executor ex(db);
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    ex.execute(stmts[0]);
    std::cout.rdbuf(old);
    return cap.str();
}

TEST(MiniSQL, BasicCreateInsertSelectAll) {
    Database db;
    run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"Alice\"), (2, \"Bob\"), (3, \"Cara\");",
        db
    );
    auto out = run_select("SELECT * FROM t;", db);
    EXPECT_NE(out.find("Alice"), std::string::npos);
    EXPECT_NE(out.find("Bob"),   std::string::npos);
    EXPECT_NE(out.find("Cara"),  std::string::npos);
}

TEST(MiniSQL, InsertFillsDefaults) {
    Database db;
    run_all_sql("CREATE TABLE t (n int, s str); INSERT INTO t (s) VALUES (\"x\");", db);
    auto out = run_select("SELECT * FROM t;", db);
    EXPECT_NE(out.find("| 0 "), std::string::npos);   // int default 0
    EXPECT_NE(out.find("| x "), std::string::npos);   // str "x"
}

TEST(MiniSQL, WhereEqAndNeq) {
    Database db;
    run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"Alice\"), (2, \"Bob\"), (3, \"Cara\");",
        db
    );
    {
        auto out = run_select("SELECT id, name FROM t WHERE name = \"Bob\";", db);
        EXPECT_NE(out.find("| 2 "), std::string::npos);
        EXPECT_NE(out.find("Bob"),  std::string::npos);
    }
    {
        auto out = run_select("SELECT id FROM t WHERE name != \"Bob\";", db);
        EXPECT_NE(out.find("| 1 "), std::string::npos);
        EXPECT_NE(out.find("| 3 "), std::string::npos);
        EXPECT_EQ(out.find("| 2 "), std::string::npos);
    }
}

TEST(MiniSQL, UpdateModifiesRows) {
    Database db;
    run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"Alice\"), (2, \"Bob\");",
        db
    );
    run_all_sql("UPDATE t SET name = \"Bee\" WHERE id = 2;", db);
    auto out = run_select("SELECT * FROM t;", db);
    EXPECT_NE(out.find("Bee"), std::string::npos);
    EXPECT_EQ(out.find("Bob"), std::string::npos);
}

TEST(MiniSQL, DeleteWhereAndFullDelete) {
    Database db;
    run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"A\"), (2, \"B\"), (3, \"C\");",
        db
    );
    run_all_sql("DELETE FROM t WHERE id = 2;", db);
    run_all_sql("DELETE FROM t;", db);
    auto out = run_select("SELECT * FROM t;", db);
    EXPECT_NE(out.find("0 row(s)."), std::string::npos);
}

TEST(MiniSQL, CaseSensitiveKeywords) {
    Parser p("select * from t;"); // wrong case on purpose
    EXPECT_THROW({ auto stmts = p.parseAll(); (void)stmts; }, std::runtime_error);
}

TEST(MiniSQL, TypeErrorOnInsert) {
    Database db;
    run_all_sql("CREATE TABLE t (n int);", db);
    Parser p("INSERT INTO t (n) VALUES (\"oops\");"); // string into int
    auto stmts = p.parseAll();
    Executor ex(db);
    EXPECT_THROW(ex.execute(stmts[0]), std::runtime_error);
}

TEST(MiniSQL, InequalitiesWork) {
    Database db;
    run_all_sql(
        "CREATE TABLE t (id int, name str);"
        "INSERT INTO t (id, name) VALUES (1, \"Alice\"), (2, \"Bob\"), (3, \"Cara\");",
        db
    );
    {
        auto out = run_select("SELECT id FROM t WHERE id < 3;", db);
        EXPECT_NE(out.find("| 1 "), std::string::npos);
        EXPECT_NE(out.find("| 2 "), std::string::npos);
        EXPECT_EQ(out.find("| 3 "), std::string::npos);
    }
    {
        auto out = run_select("SELECT name FROM t WHERE name >= \"Bob\";", db);
        EXPECT_EQ(out.find("Alice"), std::string::npos);
        EXPECT_NE(out.find("Bob"),   std::string::npos);
        EXPECT_NE(out.find("Cara"),  std::string::npos);
    }
}

TEST(MiniSQL, InequalityTypeMismatchThrows) {
    Database db;
    run_all_sql("CREATE TABLE t (id int, name str); INSERT INTO t (id, name) VALUES (1, \"A\");", db);
    Parser bad("SELECT * FROM t WHERE id < \"x\";");
    auto stmts = bad.parseAll();
    Executor ex(db);
    EXPECT_THROW(ex.execute(stmts[0]), std::runtime_error);
}

TEST(MiniSQL, ParseErrorMissingParenInCreate) {
    Parser p("CREATE TABLE t (id int, name str;");
    EXPECT_THROW({ auto s = p.parseAll(); (void)s; }, std::runtime_error);
}
