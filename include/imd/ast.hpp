#ifndef IMD_AST_HPP
#define IMD_AST_HPP

#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <optional>

namespace imd {

// ----- Types and values -----
enum class ColType { INT, STR };

struct Value {
    std::variant<long long, std::string> data;

    static Value makeInt(long long x) {
        return Value{x};
    }
    static Value makeStr(std::string s) {
        return Value{std::move(s)};
    }

    bool isInt() const {
        return std::holds_alternative<long long>(data);
    }
    bool isStr() const {
        return std::holds_alternative<std::string>(data);
    }

    long long asInt() const {
        return std::get<long long>(data);
    }
    const std::string& asStr() const {
        return std::get<std::string>(data);
    }

    std::string toString() const {
        if (isInt())
            return std::to_string(asInt());
        return asStr();
    }
};

struct Column {
    std::string name;
    ColType type;
};

using Row = std::vector<Value>;

struct Table {
    std::string name;
    std::vector<Column> columns;
    std::unordered_map<std::string, int> colIndex; // exact (case-sensitive) names
    std::vector<Row> rows;

    int indexOf(const std::string& col) const {
        auto it = colIndex.find(col);
        return (it == colIndex.end()) ? -1 : it->second;
    }
};

struct Database {
    std::unordered_map<std::string, Table> tables; // exact names
};

// ----- WHERE condition -----
// Extended to support <, >, <=, >= (assignment extension)
enum class CmpOp { EQ, NE, LT, LE, GT, GE };
struct Condition {
    std::string column;
    CmpOp op{CmpOp::EQ};
    Value literal; // int or string
};

// ----- Statements -----
struct CreateStmt {
    std::string table;
    std::vector<std::pair<std::string, ColType>> columns;
};
struct InsertStmt {
    std::string table;
    std::vector<std::string> cols;
    std::vector<std::vector<Value>> rows;
};
struct DeleteStmt {
    std::string table;
    std::optional<Condition> where;
};
struct UpdateStmt {
    std::string table;
    std::vector<std::pair<std::string, Value>> assignments;
    std::optional<Condition> where;
};
struct SelectStmt {
    bool selectAll{false};
    std::vector<std::string> cols; // ignored if selectAll==true
    std::string table;
    std::optional<Condition> where;
};

using Statement = std::variant<CreateStmt, InsertStmt, DeleteStmt, UpdateStmt, SelectStmt>;

} // namespace imd

#endif
