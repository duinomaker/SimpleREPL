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

        [[nodiscard]] inline bool closed() const {
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
            return std::move(oss.str());
        }
    };

    std::vector<std::string> unpack_commands(const std::string &str);

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

    WaitHandler register_repl_service(
            const std::function<void(const std::vector<std::string>)> &command_dispatcher);

    extern _LIBCPP_FUNC_VIS REPL repl;

}

#endif //LABRESV_REPL_H
