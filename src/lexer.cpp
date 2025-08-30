#include "imd/lexer.hpp"
#include <cctype>

namespace imd {

Lexer::Lexer(std::string src) : s_(std::move(src)) {}

char Lexer::get() {
    char c = peek();
    if (!c)
        return c;
    ++i_;
    if (c == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return c;
}

void Lexer::skipSpaces() {
    while (std::isspace(static_cast<unsigned char>(peek())))
        get();
}

Token Lexer::readString() {
    Token t;
    t.type = TokType::String;
    t.line = line_;
    t.col = col_;
    std::string out;
    while (!eof()) {
        char c = get();
        if (c == '"') {
            t.text = std::move(out);
            return t;
        }
        out.push_back(c); // no escaping per spec
    }
    throw std::runtime_error("Unterminated string literal");
}

Token Lexer::readNumber() {
    Token t;
    t.type = TokType::Number;
    t.line = line_;
    t.col = col_;
    std::string out;
    char c0 = s_[i_ - 1];
    out.push_back(c0);
    while (std::isdigit(static_cast<unsigned char>(peek())))
        out.push_back(get());
    if (out == "-" || out.empty())
        throw std::runtime_error("Invalid integer literal");
    t.text = std::move(out);
    return t;
}

Token Lexer::readIdent() {
    Token t;
    t.type = TokType::Ident;
    t.line = line_;
    t.col = col_;
    std::string out;
    char c0 = s_[i_ - 1];
    out.push_back(c0);
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
        out.push_back(get());
    t.text = std::move(out);
    return t;
}

Token Lexer::next() {
    skipSpaces();
    Token t;
    t.line = line_;
    t.col = col_;

    if (eof()) {
        t.type = TokType::End;
        return t;
    }
    char c = get();

    switch (c) {
    case ',':
        t.type = TokType::Comma;
        t.text = ",";
        return t;
    case '(':
        t.type = TokType::LParen;
        t.text = "(";
        return t;
    case ')':
        t.type = TokType::RParen;
        t.text = ")";
        return t;
    case ';':
        t.type = TokType::Semicolon;
        t.text = ";";
        return t;
    case '*':
        t.type = TokType::Star;
        t.text = "*";
        return t;
    case '"':
        return readString();
    case '=':
        t.type = TokType::Equal;
        t.text = "=";
        return t;
    case '!':
        if (peek() == '=') {
            get();
            t.type = TokType::NotEqual;
            t.text = "!=";
            return t;
        }
        break;
    case '<':
        if (peek() == '=') {
            get();
            t.type = TokType::LessEq;
            t.text = "<=";
            return t;
        }
        t.type = TokType::Less;
        t.text = "<";
        return t;
    case '>':
        if (peek() == '=') {
            get();
            t.type = TokType::GreaterEq;
            t.text = ">=";
            return t;
        }
        t.type = TokType::Greater;
        t.text = ">";
        return t;
    default:
        break;
    }

    if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && std::isdigit(static_cast<unsigned char>(peek())))) {
        return readNumber();
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return readIdent();
    }

    throw std::runtime_error("Unexpected character");
}

static bool isUpper(const std::string& s) {
    for (char c : s)
        if (!(c >= 'A' && c <= 'Z'))
            return false;
    return !s.empty();
}

bool isUpperKeyword(const std::string& w) {
    if (!isUpper(w))
        return false;
    return (w == "CREATE" || w == "TABLE" || w == "INSERT" || w == "INTO" || w == "VALUES" || w == "SELECT" ||
            w == "FROM" || w == "WHERE" || w == "DELETE" || w == "UPDATE" || w == "SET");
}

bool isTypeWord(const std::string& w) {
    return (w == "int" || w == "str"); // exactly lowercase per spec
}

} // namespace imd
