#include "ScriptEntry.h"

int main(const int argc, char *argv[]) {
    ObSL::ScriptEntry entry_point;
    entry_point.exec(argc, argv);
}
