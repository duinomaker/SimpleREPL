#include <cstddef>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <regex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "REPL.h"

namespace simple_repl {

    REPL repl;

    std::string REPL::read_line() {
        if (closed())
            throw std::runtime_error("requesting from a closed REPL");
        std::string line;
        mtx.lock();
        requesting_input = true;
        mtx.unlock();
        std::getline(m_in, line);
        mtx.lock();
        requesting_input = false;
        mtx.unlock();
        return line;
    }

    std::string REPL::read_line_no_interrupt() {
        if (closed())
            throw std::runtime_error("requesting from a closed REPL");
        std::string line;
        mtx.lock();
        std::getline(m_in, line);
        mtx.unlock();
        return line;
    }

    Dispatcher::Dispatcher(const std::initializer_list<std::pair<const std::pair<std::string, std::size_t>,
            const std::function<void(const std::vector<std::string> &)>>> &il)
            : workers(il) {
        if (workers.find({"UNKNOWN", 0}) == workers.cend())
            throw std::runtime_error("an `UNKNOWN` action with no parameter wasn't given");
    }

    void Dispatcher::operator()(const std::vector<std::string> &commands) const {
        if (commands.empty())
            return;
        std::string action = commands[0];
        auto iter = workers.find({action, commands.size() - 1});
        if (iter == workers.cend()) {
            workers.at({"UNKNOWN", 0})({});
            return;
        }
        std::vector<std::string> args(++commands.cbegin(), commands.cend());
        iter->second(args);
    }

    // The number of unescaped quotes must be even; space(s) must be present
    // between two pieces of quoted text.
    bool check_format(const std::string &str) {
        std::size_t length = str.length();
        std::size_t count = 0;
        if (str[0] == '"')
            ++count;
        for (std::size_t i = 1; i != length; ++i) {
            if (str[i - 1] != '\\' && str[i] == '"') {
                ++count;
                if (!(count & 1u) && i + 1 != length && str[i + 1] == '"') {
                    return false;
                }
            }
        }
        return !(count & 1u);
    }

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
            std::function<void(const std::vector<std::string> &)> dispatcher,
            bool no_interrupt) {
        static bool registered = false;
        static auto read_line = no_interrupt ? [] { return repl.read_line_no_interrupt(); }
                                             : [] { return repl.read_line(); };
        static auto thread = std::make_unique<std::thread>([dispatcher = std::move(dispatcher)] {
            while (!repl.closed()) {
                std::string line(read_line());
                if (check_format(line)) {
                    dispatcher(unpack_arguments(line));
                } else {
                    dispatcher({"UNKNOWN"});
                }
            }
        });
        if (registered)
            throw std::runtime_error("REPL service registered twice");
        registered = true;
        return WaitHandler(std::move(thread));
    }

}
