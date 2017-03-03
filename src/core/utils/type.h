/*
 * Layer for type demangling
 *
 */

#ifndef ALLPIX_TYPE_H
#define ALLPIX_TYPE_H

#include <cstdlib>
#include <memory>
#include <cxxabi.h>

namespace allpix{
#ifdef __GNUG__
    inline std::string demangle(const char* name, bool keep_allpix = false) {
        // some arbitrary value to eliminate the compiler warning
        int status = -4;

        // enable c++11 by passing the flag -std=c++11 to g++
        std::unique_ptr<char, void(*)(void*)> res {
            abi::__cxa_demangle(name, NULL, NULL, &status),
            std::free
        };

        if (status == 0){
            std::string str = res.get();
            if(!keep_allpix && str.find("allpix::") == 0){
                return str.substr(8);
            }
            return str;
        }
        return name;
    }

#else

    // does nothing if not g++
    inline std::string demangle(const char* name, bool keep_allpix = false) {
        return name;
    }
#endif

}

#endif /* ALLPIX_TYPE_H */
