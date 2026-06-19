#ifndef OBLIBERRY_GARBAGECOLLECTOR_H
#define OBLIBERRY_GARBAGECOLLECTOR_H
#include <stdio.h>
#include <utility>

namespace ObSL {
    class Interpreter;


    struct GCObject {
        bool is_marked = false;
        GCObject *next = nullptr;

        virtual ~GCObject() = default;

        virtual void mark() = 0;
    };

    class GarbageCollector {
    private:
        GCObject *first_obj = nullptr;
        size_t allocated_objs = 0;
        size_t gc_threshold = 1000;
        Interpreter *interpreter;

    public:
        explicit GarbageCollector(Interpreter *interpreter) : interpreter(interpreter) {
        }

        ~GarbageCollector() {
            const GCObject *obj = first_obj;
            while (obj != nullptr) {
                const GCObject *next = obj->next;
                delete obj;
                obj = next;
            }
        }

        template<typename T, typename... Args>
        T *allocate(Args &&... args) {
            if (allocated_objs >= gc_threshold) {
                collect();
            }
            T *object = new T(std::forward<Args>(args)...);
            object->next = first_obj;
            first_obj = object;
            allocated_objs++;
            return object;
        }

        void collect();
    };
} // ObSL

#endif //OBLIBERRY_GARBAGECOLLECTOR_H
