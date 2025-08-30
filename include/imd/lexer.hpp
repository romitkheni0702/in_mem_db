#ifndef IMD_LEXER_HPP
#define IMD_LEXER_HPP

#include <string>
#include <stdexcept>

namespace imd {

enum class TokType {
    End,
    Ident,
    Number,
    String, // String: double-quoted only, no escaping
    Comma,
    LParen,
    RParen,
    Semicolon,
    Star,
    Equal,
    NotEqual, // =, !=
    Less,
    LessEq,
    Greater,
    GreaterEq // <, <=, >, >=   <-- added
};

struct Token {
    TokType type{TokType::End};
    std::string text;
    int line{1};
    int col{1};
};

class Lexer {
  public:
    explicit Lexer(std::string src);
    Token next();

  private:
    std::string s_;
    size_t i_ = 0;
    int line_ = 1, col_ = 1;

    bool eof() const {
        return i_ >= s_.size();
    }
    char peek(size_t k = 0) const {
        return (i_ + k < s_.size()) ? s_[i_ + k] : '\0';
    }
    char get();
    void skipSpaces();
    Token readString(); // "..."
    Token readNumber(); // [-]?[0-9]+
    Token readIdent();  // [A-Za-z_][A-Za-z0-9_]*
};

bool isUpperKeyword(const std::string& w); // CREATE/TABLE/INSERT/INTO/VALUES/SELECT/FROM/WHERE/DELETE/UPDATE/SET
bool isTypeWord(const std::string& w);     // int / str (lowercase per spec)

} // namespace imd

#endif
