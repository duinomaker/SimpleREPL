#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "REPL.h"

namespace simple_repl {

    REPL repl;

    std::vector<std::string> unpack_arguments(const std::string &str) {
        std::vector<std::string> arguments;
        std::smatch sm;
        std::string matched;
        std::regex parts_regex(R"("(?:[^\\"]|\\")*?"|\S+)");
        auto search_begin = str.cbegin();
        while (std::regex_search(search_begin, str.cend(), sm, parts_regex)) {
            matched = sm[0];
            if (matched[0] == '"') {
                std::string temp;
                std::size_t length = matched.length();
                temp.reserve(length - 2);
                for (std::size_t i = 1, i_ = matched.length() - 1; i != i_; ++i) {
                    if (matched[i] == '\\' && matched[i + 1] == '"') {
                        temp.push_back('"');
                        ++i;
                    } else {
                        temp.push_back(matched[i]);
                    }
                }
                temp.shrink_to_fit();
                arguments.emplace_back(std::move(temp));
            } else {
                arguments.emplace_back(std::move(matched));
            }
            search_begin = sm.suffix().first;
        }
        return arguments;
    }

    WaitHandler register_repl_service(
            std::function<void(const std::vector<std::string> &)> dispatcher) {
        static bool registered = false;
        static auto thread = std::make_unique<std::thread>([dispatcher = std::move(dispatcher)] {
            while (!repl.closed()) {
                std::string line(repl.read_line());
                std::vector<std::string> arguments = unpack_arguments(line);
                dispatcher(arguments);
            }
        });
        if (registered)
            throw std::runtime_error("REPL service registered twice");
        registered = true;
        return WaitHandler(std::move(thread));
    }

}
