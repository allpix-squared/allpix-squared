/**
 * Access to the global random generator
 *
 * NOTE: should only be used to generate the seeds for other random generators
 *
 * FIXME: do this better in a later stage
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_RANDOM_H
#define ALLPIX_RANDOM_H

#include <chrono>
#include <climits>
#include <initializer_list>
#include <random>
#include <thread>

#include <iostream>

namespace allpix {
    // get the seed_seq object
    inline std::mt19937_64& get_random_seeder(std::initializer_list<uint64_t> list = std::initializer_list<uint64_t>()) {
        // FIXME: seed_seq is a bit broken
        static std::seed_seq seed_seq(list);
        static std::mt19937_64 random_generator(seed_seq);

        // NOTE: not thread safe
        if(seed_seq.size() == 0) {
            throw std::invalid_argument("random seeder is not initialized before first call");
        }

        return random_generator;
    }

    // initialize the randomizer
    // NOTE: should be called before any call to get_random_seed()
    inline void random_init(uint64_t init_seed = UINT64_MAX) {
        if(init_seed == UINT64_MAX) {
            // use the clock
            auto clock_seed = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            // use memory location local variable
            auto mem_seed = reinterpret_cast<uint64_t>(&init_seed); // NOLINT
            // use thread id
            std::hash<std::thread::id> thrd_hasher;
            auto thread_seed = thrd_hasher(std::this_thread::get_id());
            get_random_seeder({clock_seed, mem_seed, thread_seed});
        } else {
            get_random_seeder({init_seed});
        }
    }

    // return a random seed
    inline uint64_t get_random_seed() { return get_random_seeder()(); }
}

#endif /* ALLPIX_FILE_H */
