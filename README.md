# SimpleREPL

A simple REPL module I created for the Programming Week.

## Basic Usage

``` c++
#include "REPL.h"

using simple_repl::repl;

int main() {
    repl.put(12, "34", '5'); // Prints `12345`.
}
```

## Register as a Service ...

``` c++
#include <string>
#include <vector>

#include "REPL.h"

using simple_repl::repl;
using simple_repl::register_repl_service;

void dispatcher(const std::vector<std::string> &commands) {
    if (commands.empty())
        return;
    std::string action = commands[0];
    if (action == "exit") {
        repl.put("bye");
        // Close the REPL service, it will close the thread.
        repl.close();
    } else if (action == "say") {
        if (commands.size() > 1) {
            repl.put(commands[1]);
        } else {
            repl.put("nothing to say");
        }
    } else {
        repl.put("[ERROR] Unknown action");
    }
}

int main() {
    auto handler = register_repl_service(dispatcher);
    handler.wait();

    // You can also quote parts of the input. Try typing `say "Hello, world"`.
}
```

## ... with a Dispatcher

The dispatcher needs to know name of each action and the number of arguments they require. The following example implements the same functionality as above.

``` c++
#include "REPL.h"

using simple_repl::repl;
using simple_repl::register_repl_service;
using simple_repl::Dispatcher;

int main() {
    Dispatcher dispatcher({{{"say",     1}, [](auto args) { repl.put(args[0]); }},
                           {{"say",     0}, [](auto args) { repl.put("nothing to say"); }},
                           {{"exit",    0}, [](auto args) { repl.put("bye"), repl.close(); }},
                           {{"UNKNOWN", 0}, [](auto args) { repl.put("[ERROR] Unknown action"); }}});
    auto handler = register_repl_service(dispatcher);
    handler.wait();
}
```
