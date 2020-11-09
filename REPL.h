#ifndef LABRESV_REPL_H
#define LABRESV_REPL_H

#include <exception>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "HashHelper.h"

#ifndef _LIBCPP_FUNC_VIS
#define _LIBCPP_FUNC_VIS __attribute__ ((__visibility__("default")))
#endif

namespace simple_repl {

    class REPL {
    public:
        explicit REPL(std::istream &in = std::cin, std::ostream &out = std::cout)
                : in(in), out(out), requesting_input(false), is_closed(false) {}

        std::string read_line() {
            if (closed())
                throw std::runtime_error("requesting from a closed REPL");
            std::string line;
            mtx.lock();
            requesting_input = true;
            out << ">> " << std::flush;
            mtx.unlock();
            getline(in, line);
            mtx.lock();
            requesting_input = false;
            mtx.unlock();
            return line;
        }

        template<typename ...Args>
        void put(const Args &...args) {
            put<const std::string &>(join_to_string(args...));
        }

        template<>
        void put<const std::string &>(const std::string &val) {
            if (closed())
                throw std::runtime_error("putting to a closed REPL");
            mtx.lock();
            if (requesting_input) {
                out << " (input interrupted)\n"
                    << val
                    << "\n>> " << std::flush;
            } else {
                out << val << std::endl;
            }
            mtx.unlock();
        }

        inline void close() {
            is_closed = true;
        }

        [[nodiscard]]
        inline bool closed() const {
            return is_closed || in.eof();
        }

    private:
        std::mutex mtx;
        std::istream &in;
        std::ostream &out;
        bool requesting_input;
        bool is_closed;

        static void join_to_string_impl(const std::ostringstream &oss) {}

        template<typename T, typename ...Ts>
        static void join_to_string_impl(std::ostringstream &oss, const T &val, const Ts &...args) {
            oss << val;
            join_to_string_impl(oss, args...);
        }

        template<typename ...Args>
        static std::string join_to_string(const Args &...args) {
            std::ostringstream oss;
            join_to_string_impl(oss, args...);
            return oss.str();
        }
    };

    class Dispatcher {
    public:
        Dispatcher(const std::initializer_list<std::pair<const std::pair<std::string, std::size_t>,
                const std::function<void(const std::vector<std::string> &)>>> &il)
                : workers(il) {
            if (workers.find({"UNKNOWN", 0}) == workers.cend())
                throw std::runtime_error("an `UNKNOWN` action with no parameter wasn't given");
        }

        void operator()(const std::vector<std::string> &commands) const {
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

    private:
        std::unordered_map<const std::pair<std::string, std::size_t>,
                const std::function<void(const std::vector<std::string> &)>> workers;
    };

    class WaitHandler {
    public:
        explicit WaitHandler(std::unique_ptr<std::thread> thread)
                : thread(std::move(thread)) {}

        void wait() {
            thread->join();
        }

    private:
        std::unique_ptr<std::thread> thread;
    };

    std::vector<std::string> unpack_commands(const std::string &str);

    WaitHandler register_repl_service(
            std::function<void(const std::vector<std::string> &)> command_dispatcher);

    extern _LIBCPP_FUNC_VIS REPL repl;

}

#endif //LABRESV_REPL_H
