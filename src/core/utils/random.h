/**
 * @file
 * @brief Global random generator that should be used to seed all the internal random number generators
 * @copyright MIT License
 */

#ifndef ALLPIX_RANDOM_H
#define ALLPIX_RANDOM_H

#include <chrono>
#include <climits>
#include <initializer_list>
#include <random>
#include <thread>

// TODO [doc]: should be in a separate namespace or available as static class

namespace allpix {
    /**
     * @brief Returns the random generator that should be used for seeding
     * @param list List of entropy sources that is used to initialize the seeder (only used during the first call!)
     * @return Random generator object
     * @throws std::invalid_argument If the first call does not give the entropy sources to initialize the generator
     */
    // TODO [doc]: should move to a anonymous namespace / private function
    // TODO [doc]: always reinitialize if entropy list is not empty
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

    /**
     * @brief Initializes the random generator
     * @param init_seed An optional seed to use for the generator. Internal entropy is used otherwise to initialize.
     * @warning This function should be called before any call to \ref get_random_seed()
     */
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

    /**
     * @brief Return a random seed
     * @return The random number
     *
     * This method should not be used for generating sets of random numbers, but only to initialize other generators.
     */
    inline uint64_t get_random_seed() { return get_random_seeder()(); }
}

#endif /* ALLPIX_FILE_H */
