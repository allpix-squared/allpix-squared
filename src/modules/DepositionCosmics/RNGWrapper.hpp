/**
 * @file
 * @brief Definition of the CRY RNG wrapper class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_COSMICS_DEPOSITION_MODULE_CRYRNG_H
#define ALLPIX_COSMICS_DEPOSITION_MODULE_CRYRNG_H

#include <Randomize.hh>

namespace allpix {
    /**
     * @brief This class is a wrapper that allows to pass the per-thread random number engine from Geant4 to CRY.
     * The Geant4 engine is seeded by the Allpix Squared framework for every event, and separately per thread, so using this
     * engine in CRY ensures a reproducible and thread-safe simulation.
     */
    template <class T> class RNGWrapper {
    public:
        /**
         * @brief Setter function for the static thread-local object
         * @param object  Pointer to the random number engine
         * @param func    Pointer to the method to be called on the object to obtain a random number
         */
        static thread_local void set(T* object, double (T::*func)(void));

        /**
         * @brief Wrapped call to the configured method of the stored object
         * @return Pseudo-random number
         */
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
