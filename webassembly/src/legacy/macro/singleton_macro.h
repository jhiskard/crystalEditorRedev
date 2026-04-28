#pragma once

// Generate a singleton class
// Constructor and destructor should be implemented in the .cpp file.
#define DECLARE_SINGLETON(ClassName)                 \
public:                                              \
    static ClassName& Instance() {                   \
        static ClassName s_Instance;                 \
        return s_Instance;                           \
    }                                                \
private:                                             \
    ClassName();                                     \
    ~ClassName();                                    \
    ClassName(const ClassName&) = delete;            \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete;                 \
    ClassName& operator=(ClassName&&) = delete;