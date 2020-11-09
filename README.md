# SimpleREPL

A simple REPL service I created for a program to interact with users.

## Basic Usage

``` c++
#include "REPL.h"

using simple_repl::repl;

int main() {
    repl.put(12, "34", '5', 6.0); // Displays `123456`.
}
```

## Register as a Service

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
        repl.put("Invalid action: `", action, "`");
    }
}

int main() {
    auto handler = register_repl_service(dispatcher);
    handler.wait();

    // Try typing `say "Hello, world"`, and `exit`.
}
```
