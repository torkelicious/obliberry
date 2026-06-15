#include "Core/Application.h"
#include "Core/Constants.h"
#include "Core/Utils.h"
#include "Scripting/Entry.h"
#include "Scripting/Lexer/Lexer.h"

int main(int argc, char *argv[]) {
    //Application app;
    //app.Run();

    ObSL::Entry entry;
    const std::string fp = PathUtils::Join(SCRIPT_PATH, "test.obsl");
    entry.runFile(fp);
    //entry.runREPL();
    return 0;
}
