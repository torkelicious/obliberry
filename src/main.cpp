#include "Core/Application.h"
#include "Scripting/repl.h"
#include "Scripting/Lexer/Lexer.h"

int main() {
    //Application app;
    //app.Run();

    Scripting::start_repl();
    return 0;
}
