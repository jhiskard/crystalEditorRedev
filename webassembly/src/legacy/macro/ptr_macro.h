#pragma once

// Generate smart pointer typedefs for a class
// UPtr: unique pointer
// Ptr: shared pointer
// WPtr: weak pointer
#define DECLARE_PTR(ClassName)                           \
class ClassName;                                         \
using ClassName ## UPtr = std::unique_ptr<ClassName>;    \
using ClassName ## Ptr = std::shared_ptr<ClassName>;     \
using ClassName ## WPtr = std::weak_ptr<ClassName>;

// Define a smart pointer type for a template class
// UPtr<T>: unique pointer
// Ptr<T>: shared pointer
// WPtr<T>: weak pointer
#define DECLARE_TEMPLATE_PTR(ClassName)                  \
template <typename T> class ClassName;                   \
template <typename T>                                    \
using ClassName ## UPtr = std::unique_ptr<ClassName<T>>; \
template <typename T>                                    \
using ClassName ## Ptr = std::shared_ptr<ClassName<T>>;  \
template <typename T>                                    \
using ClassName ## WPtr = std::weak_ptr<ClassName<T>>;   