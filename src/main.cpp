#include "Core/Application.h"
#include "Core/Constants.h"
#include "Core/Utils.h"
#include "Scripting/Entry.h"
#include "Scripting/Lexer/Lexer.h"

int main(int argc, char *argv[]) {
    // Application app;
    // app.Run();

    ObSL::Entry entry;
    // mockup standard command line arguments

    //const char *test_argv[] = {
    //    "obsl_interpreter",
    //    "assets/scripts/test.obsl"
    //};
    //
    //entry.exec(2, const_cast<char **>(test_argv));

    entry.exec(argc, argv);

    return 0;
}
