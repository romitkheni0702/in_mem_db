#ifndef IMD_RENDERER_HPP
#define IMD_RENDERER_HPP

#include <string>
#include <vector>
#include <ostream>

namespace imd {

void printAscii(const std::vector<std::string>& headers,
                const std::vector<std::vector<std::string>>& rows,
                std::ostream& out);

} // namespace imd

#endif
