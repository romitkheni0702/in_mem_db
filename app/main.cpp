#include "imd/parser.hpp"
#include "imd/executor.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <iterator>
#include <cctype>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
[[maybe_unused]] static bool isatty_stdin() {
    return _isatty(_fileno(stdin)) != 0;
}
static bool isatty_stdout() {
    return _isatty(_fileno(stdout)) != 0;
}
static void enableVT() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode))
        return;
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#else
#include <unistd.h>
[[maybe_unused]] static bool isatty_stdin() {
    return isatty(fileno(stdin)) != 0;
}
static bool isatty_stdout() {
    return isatty(fileno(stdout)) != 0;
}
static void enableVT() {}
#endif

// ---- Banner: MINISQL (Q has a clear tail) ----
// clang-format off
static const char* B0 = R"( __  __ ___ _   _ ___  ____    ___    _     )";
static const char* B1 = R"(|  \/  |_ _| \ | |_ _|/ ___|  / __\  | |     )";
static const char* B2 = R"(| |\/| || ||  \| || | \___ \ | | | | | |    )";
static const char* B3 = R"(| |  | || || |\  || |  ___) || |_| | | |___  )";
static const char* B4 = R"(|_|  |_|___|_| \_|___||____/  \__\ \ |_____| )";
//                                      
static const char* B5 = R"(W E L C O M E   T O   M I N I   S Q L     )";
// clang-format on

static void printBannerOnce(bool forceColor = false) {
    const bool colorOK = (forceColor || (isatty_stdout()));
    if (colorOK)
        enableVT();
    auto c = [&](int code) { return colorOK ? ("\x1b[38;5;" + std::to_string(code) + "m") : std::string(); };
    const std::string cyan = c(51), teal = c(44), green = c(46), dim = c(37), reset = colorOK ? "\x1b[0m" : "";
    using namespace std::chrono_literals;
    if (colorOK) {
        std::cout << dim << B0 << reset << "\n"
                  << dim << B1 << reset << "\n"
                  << dim << B2 << reset << "\n"
                  << dim << B3 << reset << "\n"
                  << dim << B4 << reset << "\n"
                  << dim << B5 << reset << "\n";
        std::this_thread::sleep_for(100ms);
        std::cout << "\x1b[6A";
    }
    std::cout << cyan << B0 << reset << "\n"
              << teal << B1 << reset << "\n"
              << cyan << B2 << reset << "\n"
              << teal << B3 << reset << "\n"
              << cyan << B4 << reset << "\n"
              << green << B5 << reset << "\n\n";
}

// ------------ REPL helpers ------------
static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a]))
        ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1]))
        --b;
    return s.substr(a, b - a);
}
static std::string upper_nowhitespace_nosemi(std::string s) {
    std::string t;
    t.reserve(s.size());
    for (char ch : s) {
        if (ch == ';')
            continue;
        if (!std::isspace((unsigned char)ch))
            t.push_back((char)std::toupper((unsigned char)ch));
    }
    return t;
}
static void exec_sql_blob(const std::string& sql) {
    imd::Database db;
    imd::Parser p(sql);
    auto stmts = p.parseAll();
    imd::Executor ex(db);
    for (const auto& st : stmts)
        ex.execute(st);
}
static void repl() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    imd::Database db;
    imd::Executor ex(db);
    auto exec_one = [&](const std::string& stmt) {
        std::string t = trim(stmt);
        std::string u = upper_nowhitespace_nosemi(t);
        if (u == "EXIT" || u == "QUIT" || u == ".QUIT")
            return false;
        try {
            imd::Parser p(t);
            auto v = p.parseAll();
            for (auto& s : v)
                ex.execute(s);
        } catch (const std::exception& e) {
            std::cerr << "Parse/exec error: " << e.what() << "\n";
        }
        return true;
    };
    bool inStr = false;
    std::string buf;
    std::cout << "mini> " << std::flush;
    for (std::string line; std::getline(std::cin, line);) {
        buf += line;
        buf.push_back('\n');
        size_t start = 0;
        for (size_t i = 0; i < buf.size(); ++i) {
            char ch = buf[i];
            if (ch == '"')
                inStr = !inStr;
            if (!inStr && ch == ';') {
                std::string stmt = buf.substr(start, i - start + 1);
                if (!exec_one(stmt))
                    return;
                start = i + 1;
            }
        }
        if (start > 0)
            buf.erase(0, start);
        std::cout << "mini> " << std::flush;
    }
    if (!trim(buf).empty())
        std::cerr << "Parse/exec error: missing ';' before end of input\n";
}

int main(int argc, char** argv) {
    bool showBanner = true, forceColor = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--no-banner")
            showBanner = false;
        if (a == "--banner")
            showBanner = true;
        if (a == "--banner-color")
            forceColor = true;
    }
    if (showBanner)
        printBannerOnce(forceColor);
    if (isatty_stdin()) {
        repl();
    } else {
        try {
            std::string sql((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
            exec_sql_blob(sql);
        } catch (const std::exception& e) {
            std::cerr << "Parse/exec error: " << e.what() << "\n";
            return 1;
        }
    }
    return 0;
}
