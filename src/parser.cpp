#include "imd/parser.hpp"
#include <stdexcept>

namespace imd {

Parser::Parser(std::string src) : lx_(std::move(src)) {
    cur_ = lx_.next();
}

bool Parser::accept(TokType t) {
    if (cur_.type == t) { advance(); return true; }
    return false;
}
void Parser::expect(TokType t, const char* msg) {
    if (!accept(t)) throw std::runtime_error(msg);
}

bool Parser::acceptWord(const char* w) {
    return (cur_.type == TokType::Ident && cur_.text == w) ? (advance(), true) : false;
}
void Parser::expectWord(const char* w, const char* msg) {
    if (!acceptWord(w)) throw std::runtime_error(msg);
}

std::string Parser::parseIdent(const char* what) {
    if (cur_.type == TokType::Ident && !isUpperKeyword(cur_.text) && !isTypeWord(cur_.text)) {
        std::string s = cur_.text; advance(); return s;
    }
    throw std::runtime_error(std::string("Expected identifier for ") + what);
}

Value Parser::parseLiteral() {
    if (cur_.type == TokType::Number) {
        long long x = std::stoll(cur_.text);
        advance(); return Value::makeInt(x);
    }
    if (cur_.type == TokType::String) {
        std::string s = cur_.text;
        advance(); return Value::makeStr(std::move(s));
    }
    throw std::runtime_error("Expected literal (number or \"string\")");
}

CreateStmt Parser::parseCreate() {
    expectWord("CREATE","Expected CREATE");
    expectWord("TABLE","Expected TABLE");
    CreateStmt s;
    s.table = parseIdent("table");
    expect(TokType::LParen, "Expected '('");

    // First column
    {
        std::string cname = parseIdent("column");
        if (!(cur_.type==TokType::Ident && isTypeWord(cur_.text)))
            throw std::runtime_error("Expected type int or str after column name");
        ColType ct = (cur_.text=="int") ? ColType::INT : ColType::STR;
        advance();
        s.columns.push_back({std::move(cname), ct});

        // Additional columns
        while (accept(TokType::Comma)) {
            cname = parseIdent("column");
            if (!(cur_.type==TokType::Ident && isTypeWord(cur_.text)))
                throw std::runtime_error("Expected type int or str after column name");
            ct = (cur_.text=="int") ? ColType::INT : ColType::STR;
            advance();
            s.columns.push_back({std::move(cname), ct});
        }
    }

    // Require closing ')'
    expect(TokType::RParen, "Expected ')' after column list");
    return s;
}

InsertStmt Parser::parseInsert() {
    expectWord("INSERT","Expected INSERT");
    expectWord("INTO","Expected INTO");
    InsertStmt s;
    s.table = parseIdent("table");
    expect(TokType::LParen, "Expected '(' after table");
    do {
        s.cols.push_back(parseIdent("column"));
    } while (accept(TokType::Comma));
    expect(TokType::RParen, "Expected ')'");
    expectWord("VALUES","Expected VALUES");
    do {
        expect(TokType::LParen, "Expected '(' before row");
        std::vector<Value> row;
        row.push_back(parseLiteral());
        while (accept(TokType::Comma)) row.push_back(parseLiteral());
        expect(TokType::RParen, "Expected ')'");
        s.rows.push_back(std::move(row));
    } while (accept(TokType::Comma));
    return s;
}

DeleteStmt Parser::parseDelete() {
    expectWord("DELETE","Expected DELETE");
    expectWord("FROM","Expected FROM");
    DeleteStmt s;
    s.table = parseIdent("table");
    if (acceptWord("WHERE")) s.where = parseCondition();
    return s;
}

UpdateStmt Parser::parseUpdate() {
    expectWord("UPDATE","Expected UPDATE");
    UpdateStmt s;
    s.table = parseIdent("table");
    expectWord("SET","Expected SET");
    do {
        std::string cname = parseIdent("column");
        expect(TokType::Equal, "Expected '=' in SET");
        Value v = parseLiteral();
        s.assignments.push_back({std::move(cname), std::move(v)});
    } while (accept(TokType::Comma));
    if (acceptWord("WHERE")) s.where = parseCondition();
    return s;
}

SelectStmt Parser::parseSelect() {
    expectWord("SELECT","Expected SELECT");
    SelectStmt s;
    if (accept(TokType::Star)) {
        s.selectAll = true;
    } else {
        s.selectAll = false;
        s.cols.push_back(parseIdent("column"));
        while (accept(TokType::Comma)) s.cols.push_back(parseIdent("column"));
    }
    expectWord("FROM","Expected FROM");
    s.table = parseIdent("table");
    if (acceptWord("WHERE")) s.where = parseCondition();
    return s;
}

Condition Parser::parseCondition() {
    Condition c;
    c.column = parseIdent("WHERE column");
    if (accept(TokType::Equal))        { c.op = CmpOp::EQ; }
    else if (accept(TokType::NotEqual)){ c.op = CmpOp::NE; }
    else if (accept(TokType::LessEq))  { c.op = CmpOp::LE; }
    else if (accept(TokType::GreaterEq)){ c.op = CmpOp::GE; }
    else if (accept(TokType::Less))    { c.op = CmpOp::LT; }
    else if (accept(TokType::Greater)) { c.op = CmpOp::GT; }
    else throw std::runtime_error("Expected comparison operator (=, !=, <, <=, >, >=) in WHERE");
    c.literal = parseLiteral();
    return c;
}

std::vector<Statement> Parser::parseAll() {
    std::vector<Statement> out;
    while (cur_.type != TokType::End) {
        if (cur_.type != TokType::Ident || !isUpperKeyword(cur_.text))
            throw std::runtime_error("Expected a statement keyword (CREATE/INSERT/DELETE/SELECT/UPDATE)");
        std::string kw = cur_.text;
        if (kw=="CREATE") out.push_back(parseCreate());
        else if (kw=="INSERT") out.push_back(parseInsert());
        else if (kw=="DELETE") out.push_back(parseDelete());
        else if (kw=="UPDATE") out.push_back(parseUpdate());
        else if (kw=="SELECT") out.push_back(parseSelect());
        else throw std::runtime_error("Unsupported statement");
        expect(TokType::Semicolon, "Expected ';' after statement");
    }
    return out;
}

} // namespace imd
