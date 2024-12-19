#pragma once

// NOTE: this functions assumes we are using no multi-threading
// TODO: check if catch2 uses multithreading or use synchronization primitive
template<typename T>
T const& make_const_ref(T t) {
    static T temp = t;
    temp = t;
    return temp;
}
