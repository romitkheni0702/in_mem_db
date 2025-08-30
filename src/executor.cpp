#include "imd/executor.hpp"
#include "imd/renderer.hpp"
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace imd {

Value Executor::defaultFor(ColType t) {
    if (t == ColType::INT) return Value::makeInt(0);
    return Value::makeStr("");
}

void Executor::ensureTableExists(const Database& db, const std::string& name) {
    if (!db.tables.count(name)) throw std::runtime_error("No such table: " + name);
}

static void typeCheckAssign(const Column& col, const Value& v) {
    if (col.type == ColType::INT && !v.isInt())
        throw std::runtime_error("Type error: expected int for column '" + col.name + "'");
    if (col.type == ColType::STR && !v.isStr())
        throw std::runtime_error("Type error: expected str for column '" + col.name + "'");
}

static bool equalValues(const Value& a, const Value& b) {
    if (a.data.index() != b.data.index()) return false;
    if (a.isInt()) return a.asInt() == b.asInt();
    return a.asStr() == b.asStr();
}

// Tri-state comparator for Value (clean, no warnings)
static int compareValues(const Value& a, const Value& b) {
    if (a.isInt() && b.isInt()) {
        long long x = a.asInt();
        long long y = b.asInt();
        if (x < y) return -1;
        if (x > y) return 1;
        return 0;
    }
    if (a.isStr() && b.isStr()) {
        const std::string& x = a.asStr();
        const std::string& y = b.asStr();
        if (x < y) return -1;
        if (x > y) return 1;
        return 0;
    }
    throw std::runtime_error("Type mismatch in comparison");
}

bool Executor::rowMatches(const Table& t, const Row& r, const Condition& c) {
    int j = t.indexOf(c.column);
    if (j < 0) throw std::runtime_error("Unknown column in WHERE: " + c.column);
    const Value& cell = r[j];

    switch (c.op) {
        case CmpOp::EQ: return equalValues(cell, c.literal);
        case CmpOp::NE: return !equalValues(cell, c.literal);
        case CmpOp::LT: return compareValues(cell, c.literal) < 0;
        case CmpOp::LE: return compareValues(cell, c.literal) <= 0;
        case CmpOp::GT: return compareValues(cell, c.literal) > 0;
        case CmpOp::GE: return compareValues(cell, c.literal) >= 0;
    }
    return false;
}

void Executor::exec(const CreateStmt& s) {
    if (db_.tables.count(s.table)) throw std::runtime_error("Table already exists: " + s.table);
    Table t;
    t.name = s.table;
    for (size_t i=0;i<s.columns.size();++i) {
        t.columns.push_back({s.columns[i].first, s.columns[i].second});
        t.colIndex[t.columns.back().name] = static_cast<int>(i);
    }
    db_.tables.emplace(t.name, std::move(t));
}

void Executor::exec(const InsertStmt& s) {
    ensureTableExists(db_, s.table);
    Table& t = db_.tables[s.table];

    std::vector<int> pos;
    pos.reserve(s.cols.size());
    for (const auto& cn : s.cols) {
        int j = t.indexOf(cn);
        if (j < 0) throw std::runtime_error("Unknown column: " + cn);
        pos.push_back(j);
    }

    for (const auto& values : s.rows) {
        if (values.size() != pos.size())
            throw std::runtime_error("VALUES count does not match column list");

        Row r;
        r.reserve(t.columns.size());
        for (const auto& col : t.columns) r.push_back(defaultFor(col.type));

        for (size_t k=0;k<pos.size();++k) {
            int j = pos[k];
            const Column& col = t.columns[j];
            const Value& v = values[k];
            typeCheckAssign(col, v);
            r[j] = v;
        }
        t.rows.push_back(std::move(r));
    }
}

void Executor::exec(const DeleteStmt& s) {
    ensureTableExists(db_, s.table);
    Table& t = db_.tables[s.table];
    if (!s.where) {
        t.rows.clear();
        return;
    }
    const Condition& c = *s.where;
    t.rows.erase(std::remove_if(t.rows.begin(), t.rows.end(),
        [&](const Row& r){ return rowMatches(t, r, c); }), t.rows.end());
}

void Executor::exec(const UpdateStmt& s) {
    ensureTableExists(db_, s.table);
    Table& t = db_.tables[s.table];

    std::vector<int> idx;
    idx.reserve(s.assignments.size());
    for (const auto& [cn, v] : s.assignments) {
        int j = t.indexOf(cn);
        if (j<0) throw std::runtime_error("Unknown column in SET: " + cn);
        typeCheckAssign(t.columns[j], v);
        idx.push_back(j);
    }

    for (auto& r : t.rows) {
        if (s.where && !rowMatches(t, r, *s.where)) continue;
        for (size_t k=0;k<idx.size();++k) {
            r[idx[k]] = s.assignments[k].second;
        }
    }
}

void Executor::exec(const SelectStmt& s) {
    ensureTableExists(db_, s.table);
    const Table& t = db_.tables[s.table];

    std::vector<int> proj;
    std::vector<std::string> headers;
    if (s.selectAll) {
        proj.reserve(t.columns.size());
        for (size_t j=0;j<t.columns.size();++j) { proj.push_back((int)j); headers.push_back(t.columns[j].name); }
    } else {
        proj.reserve(s.cols.size());
        for (const auto& cn : s.cols) {
            int j = t.indexOf(cn);
            if (j < 0) throw std::runtime_error("Unknown column: " + cn);
            proj.push_back(j);
            headers.push_back(t.columns[j].name);
        }
    }

    std::vector<std::vector<std::string>> outRows;
    for (const auto& r : t.rows) {
        if (s.where && !rowMatches(t, r, *s.where)) continue;
        std::vector<std::string> line;
        line.reserve(proj.size());
        for (int j : proj) line.push_back(r[j].toString());
        outRows.push_back(std::move(line));
    }

    printAscii(headers, outRows, std::cout);
}

void Executor::execute(const Statement& st) {
    std::visit([&](auto&& s){ exec(s); }, st);
}

} // namespace imd
