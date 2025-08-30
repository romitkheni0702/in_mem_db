#include "imd/renderer.hpp"
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>
#include <vector>

namespace imd {

static std::vector<size_t> colWidths(const std::vector<std::string>& headers,
                                     const std::vector<std::vector<std::string>>& rows) {
    std::vector<size_t> w(headers.size(), 0);
    for (size_t j = 0; j < headers.size(); ++j)
        w[j] = headers[j].size();
    for (const auto& r : rows) {
        for (size_t j = 0; j < headers.size(); ++j) {
            if (j < r.size())
                w[j] = std::max(w[j], r[j].size());
        }
    }
    return w;
}

static void printBorder(const std::vector<size_t>& w, std::ostream& os) {
    os.put('+');
    for (size_t width : w) {
        os << std::string(width + 2, '-') << '+'; // +2 for left & right padding
    }
    os << '\n';
}

static void printOneRow(const std::vector<std::string>& row, const std::vector<size_t>& w, std::ostream& os) {
    os.put('|');
    for (size_t j = 0; j < w.size(); ++j) {
        const std::string cell = (j < row.size() ? row[j] : std::string());
        // EXACTLY one space left and right of the cell content
        os << ' ' << std::left << std::setw(static_cast<int>(w[j])) << cell
           << ' ' // <-- the missing trailing space that fixes alignment
           << '|';
    }
    os << '\n';
}

void printAscii(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows,
                std::ostream& os) {
    const auto w = colWidths(headers, rows);

    printBorder(w, os);          // top
    printOneRow(headers, w, os); // header
    printBorder(w, os);          // header separator
    for (const auto& r : rows)   // data rows
        printOneRow(r, w, os);
    printBorder(w, os); // bottom
    os << rows.size() << " row(s)." << '\n';
}

static std::string csvEscape(const std::string& s) {
    if (s.find_first_of(",\"\n\r") == std::string::npos)
        return s;
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (char c : s) {
        if (c == '"')
            out += "\"\"";
        else
            out.push_back(c);
    }
    out.push_back('"');
    return out;
}

void printCsv(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows,
              std::ostream& os) {
    for (size_t j = 0; j < headers.size(); ++j) {
        if (j)
            os.put(',');
        os << csvEscape(headers[j]);
    }
    os << '\n';
    for (const auto& r : rows) {
        for (size_t j = 0; j < headers.size(); ++j) {
            if (j)
                os.put(',');
            os << csvEscape(j < r.size() ? r[j] : std::string());
        }
        os << '\n';
    }
}

} // namespace imd
