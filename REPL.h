#ifndef LABRESV_REPL_H
#define LABRESV_REPL_H

#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Utilities.h"

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
            return std::move(line);
        }

        template<typename ...Args>
        void put(const Args &...args) {
            put<const std::string &>(utilities::join_to_string(args...));
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

        [[nodiscard]] inline bool closed() const {
            return is_closed || in.eof();
        }

    private:
        std::mutex mtx;
        std::istream &in;
        std::ostream &out;
        bool requesting_input;
        bool is_closed;
    };

    std::vector<std::string> unpack_commands(const std::string &str);

    std::unique_ptr<std::thread> register_repl_service(
            const std::function<void(const std::vector<std::string>)> &command_dispatcher);

    extern _LIBCPP_FUNC_VIS REPL repl;

}

#endif //LABRESV_REPL_H
