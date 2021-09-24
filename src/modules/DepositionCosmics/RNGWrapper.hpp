/**
 * @file
 * @brief Definition of Geant4 deposition module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_COSMICS_DEPOSITION_MODULE_CRYRNG_H
#define ALLPIX_COSMICS_DEPOSITION_MODULE_CRYRNG_H

#include <Randomize.hh>

namespace allpix {
    template <class T> class RNGWrapper {
    public:
        static thread_local void set(T* object, double (T::*func)(void));
        static thread_local double rng(void);

    private:
        static thread_local T* m_obj;
        static thread_local double (T::*m_func)(void);
    };

    template <class T> thread_local T* RNGWrapper<T>::m_obj;

    template <class T> thread_local double (T::*RNGWrapper<T>::m_func)(void);

    template <class T> void RNGWrapper<T>::set(T* object, double (T::*func)(void)) {
        m_obj = object;
        m_func = func;
    }

    template <class T> double RNGWrapper<T>::rng(void) { return (m_obj->*m_func)(); }

} // namespace allpix

#endif /* ALLPIX_COSMICS_DEPOSITION_MODULE_CRYRNG_H */
