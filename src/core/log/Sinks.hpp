/**
 * @file
 * @brief Custom spdlog sinks for ALlpix Squared log style
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LOG_SINKS_H
#define ALLPIX_LOG_SINKS_H

#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/base_sink.h>

#include "Level.hpp"

namespace allpix {
    /**
     * @brief Sink output style
     */
    enum class SinkStyle {
        PLAIN, ///< Plain text (log files etc)
        COLOR, ///< ANSI colors, hidden cursor (interactive console)
    };

    /**
     * @brief Sink writing to an arbitrary `std::ostream`
     * @note The caller must make sure the wrapped stream stays valid for as long as the sink may be used.
     */
    class StreamSink final : public spdlog::sinks::base_sink<std::mutex> {
    private:
        /**
         * @brief Allpix Squared level, colored and right-aligned or single-character short form
         */
        class LevelFormatter final : public spdlog::custom_flag_formatter {
        public:
            LevelFormatter(bool short_form, bool colored);
            void
            format(const spdlog::details::log_msg& msg, const std::tm& /*tm_time*/, spdlog::memory_buf_t& dest) override;
            std::unique_ptr<custom_flag_formatter> clone() const override;

        private:
            bool short_form_;
            bool colored_;
        };

        /**
         * @brief Formatter for "(Event N) ", short "(E: N) "
         */
        class EventFormatter final : public spdlog::custom_flag_formatter {
        public:
            explicit EventFormatter(bool short_form);
            void
            format(const spdlog::details::log_msg& /*msg*/, const std::tm& /*tm_time*/, spdlog::memory_buf_t& dest) override;
            std::unique_ptr<custom_flag_formatter> clone() const override;

        private:
            bool short_form_;
        };

        /**
         * @brief "[STAGE:topic] " section header, empty for framework logger
         */
        class SectionFormatter final : public spdlog::custom_flag_formatter {
        public:
            explicit SectionFormatter(bool colored);
            void
            format(const spdlog::details::log_msg& msg, const std::tm& /*tm_time*/, spdlog::memory_buf_t& dest) override;
            std::unique_ptr<custom_flag_formatter> clone() const override;

        private:
            bool colored_;
        };

        /**
         * @brief Formatter for the log message body, replacing spdlog's `%v`. This implements the re-indentation of new
         *        lines of the same log message to the begin of the message, skipping the sink pattern
         */
        class MessageFormatter final : public spdlog::custom_flag_formatter {
        public:
            void
            format(const spdlog::details::log_msg& msg, const std::tm& /*tm_time*/, spdlog::memory_buf_t& dest) override;
            std::unique_ptr<custom_flag_formatter> clone() const override;

        private:
            std::string strip_ansi_codes(std::string_view input);
        };

    public:
        /**
         * @brief Construct a sink around an existing stream
         * @param stream Stream to write formatted log messages to
         * @param style Style of this sink, color or plain
         * @param format Format of the sink
         */
        explicit StreamSink(std::ostream& stream, SinkStyle style, Format format = Format::DEFAULT);
        ~StreamSink() override;

        /**
         * @brief Return the style this sink was constructed with
         */
        SinkStyle getStyle() const { return style_; }

        /**
         * @brief Reconfigure this sink's formatting (keeping its \ref SinkStyle)
         * @param format New format to render with
         */
        void setFormat(Format format);

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override;
        void flush_() override;

    private:
        /**
         * @brief Build the default Allpix pattern_formatter for a given \ref Format and \ref SinkStyle
         *
         * @param format Log format
         * @param style Log style (color, plain)
         * @return New pattern_formatter for a sink
         */
        std::unique_ptr<spdlog::pattern_formatter> make_formatter(Format format, SinkStyle style);

    private:
        std::ostream& stream_; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        SinkStyle style_;
        bool progress_active_ = false;
    };

} // namespace allpix

#endif /* ALLPIX_LOG_SINKS_H */
