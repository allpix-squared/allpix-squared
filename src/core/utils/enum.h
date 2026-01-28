/**
 * @file
 * @brief Enum helpers
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_ENUM_H
#define ALLPIX_ENUM_H

#include <concepts>
#include <optional>
#include <string>
#include <string_view>

#if __has_include(<magic_enum/magic_enum.hpp>)
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>
#else
#include <magic_enum.hpp>
#include <magic_enum_flags.hpp>
#endif

namespace allpix {

    template <typename E> requires std::is_enum_v<E> constexpr auto enum_cast(std::underlying_type_t<E> value) noexcept {
        return magic_enum::enum_cast<E>(value);
    }

    template <typename E>
    requires std::is_enum_v<E> constexpr auto enum_cast(std::string_view value, bool case_insensitive = true) noexcept {
        std::optional<E> retval{};
        retval = case_insensitive ? magic_enum::enum_cast<E>(value, magic_enum::case_insensitive)
                                  : magic_enum::enum_cast<E>(value);
        // If unscoped enum and no value, try cast as flag
        if constexpr(magic_enum::is_unscoped_enum_v<E>) {
            if(!retval.has_value()) {
                retval = case_insensitive ? magic_enum::enum_flags_cast<E>(value, magic_enum::case_insensitive)
                                          : magic_enum::enum_flags_cast<E>(value);
            }
        }
        return retval;
    }

    template <typename E> requires std::is_enum_v<E> auto enum_name(E enum_val) noexcept {
        auto retval = std::string(magic_enum::enum_name<E>(enum_val));
        // If unscoped enum and no value, try as flag
        if constexpr(magic_enum::is_unscoped_enum_v<E>) {
            if(retval.empty()) {
                retval = magic_enum::enum_flags_name<E>(enum_val);
            }
        }
        return retval;
    }

    template <typename E> requires std::is_enum_v<E> constexpr auto enum_names() noexcept {
        return magic_enum::enum_names<E>();
    }

} // namespace allpix

#endif /* ALLPIX_ENUM_H */
