#include "Core/Application.h"
#include "Core/ProjectConfig.h"

/*
  todo:
 * SCRIPTS:
 * lsp
 * docs
 * enginelib
 *  ===
 * ENGINE:
 * MSAA?
 * multithreading
 * gui generation (via ObSL?)
 * editor
 * convert some stuff to ObSL
 */

int main(int argc, char *argv[]) {
    const ProjectConfig config = ProjectConfig::Deserialize("project.json");

    Application app(config);
    app.Run();
    return 0;
}
