#include "./GarbageCollector.h"
#include "Interpreter/Interpreter.h"
#include "Parser/ast.h"

namespace ObSL {
    void mark_value(const Value &val) {
        if (const auto obj = std::get_if<ObSLObject *>(&val); obj && *obj) {
            (*obj)->mark();
        } else if (const auto arr = std::get_if<ObSLArray *>(&val); arr && *arr) {
            (*arr)->mark();
        } else if (const auto callable = std::get_if<ObSLCallable *>(&val); callable && *callable) {
            (*callable)->mark();
        }
    }

    void GarbageCollector::collect() {
        interpreter->mark_roots();

        GCObject **obj = &first_obj;
        while (*obj != nullptr) {
            if (!(*obj)->is_marked) {
                const GCObject *unreached = *obj;
                *obj = unreached->next;
                delete unreached;
                allocated_objs--;
            } else {
                (*obj)->is_marked = false;
                obj = &(*obj)->next;
            }
        }
        gc_threshold = allocated_objs * 2;
    }
} // namespace ObSL
