#include "EngineLib.h"

/*
 *todo:
 * gui stuff? -- wip
 * wait corutine type thingy (wait for specific time without pausing entire thread type thingy)
 */

void Scripting::EngineLib::register_modules(ObSL::Interpreter &interpreter) {
    register_core_modules(interpreter);
    register_registry_modules(interpreter);
    register_input_modules(interpreter);
    register_camera_modules(interpreter);
    register_map_modules(interpreter);
    register_audio_modules(interpreter);
    register_scene_management_modules(interpreter);
    register_time_modules(interpreter);
    register_gui_modules(interpreter);
}
