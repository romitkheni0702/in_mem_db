#ifndef IMD_PARSER_HPP
#define IMD_PARSER_HPP

#include "ast.hpp"
#include "lexer.hpp"
#include <vector>

namespace imd {

class Parser {
  public:
    explicit Parser(std::string src);
    std::vector<Statement> parseAll();

  private:
    Lexer lx_;
    Token cur_;

    void advance() {
        cur_ = lx_.next();
    }
    bool accept(TokType t);
    void expect(TokType t, const char* msg);
    bool acceptWord(const char* w); // exact case-sensitive match (Identifier token)
    void expectWord(const char* w, const char* msg);

    std::string parseIdent(const char* what);
    imd::Value parseLiteral(); // number or string

    CreateStmt parseCreate();
    InsertStmt parseInsert();
    DeleteStmt parseDelete();
    UpdateStmt parseUpdate();
    SelectStmt parseSelect();

    Condition parseCondition(); // <ident> ( '=' | '!=' ) <literal>
};

} // namespace imd

#endif
