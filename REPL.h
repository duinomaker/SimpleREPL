#ifndef REPL_H
#define REPL_H

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
        explicit REPL(std::istream &in = std::cin,
                      std::ostream &out = std::cout,
                      std::ostream &log = std::clog)
                : m_in(in), m_out(out), m_log(log), requesting_input(false), is_closed(false) {}

        std::string read_line();

        std::string read_line_no_interrupt();

        template<typename ...Args>
        void put(const Args &...args) {
            put<const std::string &>(join_to_string(args...));
        }

        template<>
        void put<const std::string &>(const std::string &val) {
            if (closed())
                throw std::runtime_error("putting to a closed REPL");
            mtx.lock();
            m_out << val << std::endl;
            mtx.unlock();
        }

        template<typename ...Args>
        void log(const Args &...args) {
            log<const std::string &>(join_to_string(args...));
        }

        template<>
        void log<const std::string &>(const std::string &val) {
            if (closed())
                throw std::runtime_error("putting to a closed REPL");
            mtx.lock();
            if (requesting_input) {
                m_log << " (input interrupted)\n"
                      << val
                      << "\n>> " << std::flush;
            } else {
                m_log << val << std::endl;
            }
            mtx.unlock();
        }

        inline void close() {
            is_closed = true;
        }

        [[nodiscard]]
        inline bool closed() const {
            return is_closed || m_in.eof();
        }

    private:
        std::mutex mtx;
        std::istream &m_in;
        std::ostream &m_out;
        std::ostream &m_log;
        bool requesting_input;
        bool is_closed;

        static void join_to_string_impl(const std::ostringstream &oss) {}

        template<typename T, typename ...Ts>
        static void join_to_string_impl(std::ostringstream &oss,
                                        const T &val, const Ts &...args) {
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
                const std::function<void(const std::vector<std::string> &)> &>> &il);

        void operator()(const std::vector<std::string> &commands) const;

    private:
        std::unordered_map<const std::pair<std::string, std::size_t>,
                const std::function<void(const std::vector<std::string> &)> &> workers;
    };

    class WaitHandler {
    public:
        explicit WaitHandler(std::unique_ptr<std::thread> thread)
                : thread(std::move(thread)) {}

        inline void wait() const {
            thread->join();
        }

    private:
        std::unique_ptr<std::thread> thread;
    };

    bool check_format(const std::string &str);

    std::vector<std::string> unpack_commands(const std::string &str);

    WaitHandler register_repl_service(
            std::function<void(const std::vector<std::string> &)> command_dispatcher,
            bool no_interrupt = false);

    extern _LIBCPP_FUNC_VIS REPL repl;

}

#endif //REPL_H
