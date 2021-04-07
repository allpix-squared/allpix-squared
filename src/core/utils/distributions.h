/**
 * @file
 * @brief Random distributions, copied from
 * @copyright Copyright (c) 2007-2018 Free Software Foundation, Inc.
 *
 * This file is part of the GNU ISO C++ Library.  This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 */

#ifndef ALLPIX_RANDOM_DISTRIBUTIONS_H
#define ALLPIX_RANDOM_DISTRIBUTIONS_H

#include <boost/random/normal_distribution.hpp>

namespace allpix {
    template <typename T> using normal_distribution = boost::normal_distribution<T>;
} // namespace allpix

#endif // ALLPIX_RANDOM_DISTRIBUTIONS_H
