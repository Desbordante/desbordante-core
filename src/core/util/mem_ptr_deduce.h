#pragma once

namespace util {
template <typename T>
struct MemPtrDeduce;

template <typename MemberOf, typename Type>
struct MemPtrDeduce<Type MemberOf::*> {
    using ReturnType = Type;
    using Class = MemberOf;
};
}  // namespace util
