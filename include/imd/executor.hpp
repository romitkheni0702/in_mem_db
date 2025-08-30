#ifndef IMD_EXECUTOR_HPP
#define IMD_EXECUTOR_HPP

#include "ast.hpp"

namespace imd {

class Executor {
  public:
    explicit Executor(Database& db) : db_(db) {}
    void execute(const Statement& st);

  private:
    Database& db_;
    void exec(const CreateStmt& s);
    void exec(const InsertStmt& s);
    void exec(const DeleteStmt& s);
    void exec(const UpdateStmt& s);
    void exec(const SelectStmt& s);

    static Value defaultFor(ColType t);
    static bool rowMatches(const Table& t, const Row& r, const Condition& c);
    static void ensureTableExists(const Database& db, const std::string& name);
};

} // namespace imd

#endif
