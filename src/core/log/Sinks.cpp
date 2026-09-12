/**
 * @file
 * @brief Implementation of custom spdlog sinks and pattern flags
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Sinks.hpp"

#include <iostream>
#include <memory>
#include <string>

#include <unistd.h>

#include "core/utils/enum.h"

#include "LogContext.hpp"

using namespace allpix;

StreamSink::LevelFormatter::LevelFormatter(bool short_form, bool colored) : short_form_(short_form), colored_(colored) {}

void StreamSink::LevelFormatter::format(const spdlog::details::log_msg& msg,
                                        const std::tm& /*tm_time*/,
                                        spdlog::memory_buf_t& dest) {
    auto level = from_spdlog_level(msg.level);

    if(colored_) {
        std::string_view color;
        switch(level) {
        case Level::ERROR:
            color = "\x1B[31;1m"; // RED
            break;
        case Level::WARNING:
            color = "\x1B[33;1m"; // YELLOW
            break;
        case Level::STATUS:
            color = "\x1B[32;1m"; // GREEN
            break;
        case Level::TRACE:
        case Level::DEBUG:
            color = "\x1B[36m"; // NON-BOLD CYAN
            break;
        case Level::PRNG:
            color = "\x1B[90m"; // NON-BOLD GREY
            break;
        default:
            color = "\x1B[36;1m"; // CYAN
            break;
        }
        dest.append(color.data(), color.data() + color.size());
    }

    std::string badge;
    if(short_form_) {
        badge = "(";
        badge += enum_name(level).substr(0, 1);
        badge += ") ";
    } else {
        badge = "(";
        badge += enum_name(level);
        badge += ")";
        if(badge.size() < 9) {
            badge.insert(badge.begin(), 9 - badge.size(), ' ');
        }
        badge += " ";
    }
    dest.append(badge.data(), badge.data() + badge.size());

    if(colored_) {
        static constexpr std::string_view reset = "\x1B[0m";
        dest.append(reset.data(), reset.data() + reset.size());
    }
}

std::unique_ptr<spdlog::custom_flag_formatter> StreamSink::LevelFormatter::clone() const {
    return spdlog::details::make_unique<LevelFormatter>(short_form_, colored_);
}

StreamSink::EventFormatter::EventFormatter(bool short_form) : short_form_(short_form) {}

void StreamSink::EventFormatter::format(const spdlog::details::log_msg& /*msg*/,
                                        const std::tm& /*tm_time*/,
                                        spdlog::memory_buf_t& dest) {
    auto event_num = log_context::event_num();
    if(event_num == 0) {
        return;
    }
    std::string text = short_form_ ? "(E: " : "(Event ";
    text += std::to_string(event_num);
    text += ") ";
    dest.append(text.data(), text.data() + text.size());
}

std::unique_ptr<spdlog::custom_flag_formatter> StreamSink::EventFormatter::clone() const {
    return spdlog::details::make_unique<EventFormatter>(short_form_);
}

StreamSink::SectionFormatter::SectionFormatter(bool colored) : colored_(colored) {}

void StreamSink::SectionFormatter::format(const spdlog::details::log_msg& msg,
                                          const std::tm& /*tm_time*/,
                                          spdlog::memory_buf_t& dest) {
    if(msg.logger_name.size() == 0) {
        return;
    }
    if(colored_) {
        static constexpr std::string_view bold = "\x1B[1m";
        dest.append(bold.data(), bold.data() + bold.size());
    }
    dest.push_back('[');
    const char stage = log_context::stage();
    if(stage != '\0') {
        dest.push_back(stage);
        dest.push_back(':');
    }
    dest.append(msg.logger_name.data(), msg.logger_name.data() + msg.logger_name.size());
    dest.push_back(']');
    dest.push_back(' ');
    if(colored_) {
        static constexpr std::string_view reset = "\x1B[0m";
        dest.append(reset.data(), reset.data() + reset.size());
    }
}

std::unique_ptr<spdlog::custom_flag_formatter> StreamSink::SectionFormatter::clone() const {
    return spdlog::details::make_unique<SectionFormatter>(colored_);
}

void StreamSink::MessageFormatter::format(const spdlog::details::log_msg& msg,
                                          const std::tm& /*tm_time*/,
                                          spdlog::memory_buf_t& dest) {
    const std::string_view payload(msg.payload.data(), msg.payload.size());
    if(payload.find('\n') == std::string_view::npos) {
        dest.append(payload.data(), payload.data() + payload.size());
        return;
    }

    auto indent_count = strip_ansi_codes(std::string_view(dest.data(), dest.size())).size();
    std::string indented(payload);
    std::string spcs(indent_count + 1, ' ');
    spcs.at(0) = '\n';
    size_t pos = 0;
    while((pos = indented.find('\n', pos)) != std::string::npos) {
        indented.replace(pos, 1, spcs);
        pos += spcs.length();
    }
    dest.append(indented.data(), indented.data() + indented.size());
}

std::unique_ptr<spdlog::custom_flag_formatter> StreamSink::MessageFormatter::clone() const {
    return spdlog::details::make_unique<MessageFormatter>();
}

std::string StreamSink::MessageFormatter::strip_ansi_codes(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    size_t prev = 0;
    size_t pos = 0;
    while((pos = input.find("\x1B[", prev)) != std::string_view::npos) {
        out += input.substr(prev, pos - prev);
        auto end_pos = input.find('m', pos);
        if(end_pos == std::string_view::npos) {
            prev = pos;
            break;
        }
        prev = end_pos + 1;
    }
    out += input.substr(prev);
    return out;
}

std::unique_ptr<spdlog::pattern_formatter> StreamSink::make_formatter(Format format, SinkStyle style) {
    const bool colored = (style == SinkStyle::COLOR);
    const std::string bold = colored ? "\x1B[1m" : "";
    const std::string reset = colored ? "\x1B[0m" : "";

    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<LevelFormatter>('k', format == Format::SHORT, colored);
    formatter->add_flag<EventFormatter>('j', format == Format::SHORT);
    formatter->add_flag<SectionFormatter>('q', colored);
    formatter->add_flag<MessageFormatter>('w');

    switch(format) {
    case Format::SHORT:
        formatter->set_pattern("%k%j%q%w");
        break;
    case Format::LONG:
        formatter->set_pattern(bold + "|%H:%M:%S.%e| " + reset + bold + "=%t= " + reset + "%k%j%q" + bold + "<%s/%!:L%#> " +
                               reset + "%w");
        break;
    case Format::DEFAULT:
    default:
        formatter->set_pattern(bold + "|%H:%M:%S.%e| " + reset + "%k%j%q%w");
        break;
    }
    return formatter;
}

StreamSink::StreamSink(std::ostream& stream, SinkStyle style, Format format) : stream_(stream), style_(style) {
    set_formatter_(make_formatter(format, style_));
    if(style_ == SinkStyle::COLOR) {
        // Hide cursor
        stream_ << "\x1B[?25l";
    }
}

StreamSink::~StreamSink() {
    if(style_ == SinkStyle::COLOR) {
        // Restore cursor on destruction
        stream_ << "\x1B[?25h";
        stream_.flush();
    }
}

void StreamSink::setFormat(Format format) { set_formatter(make_formatter(format, style_)); }

void StreamSink::sink_it_(const spdlog::details::log_msg& msg) {
    spdlog::memory_buf_t formatted;
    formatter_->format(msg, formatted);

    if(style_ != SinkStyle::COLOR) {
        stream_.write(formatted.data(), static_cast<std::streamsize>(formatted.size()));
        return;
    }

    // Progress updates - if the current line is also a progress update, overwrite it
    if(progress_active_) {
        // Set cursor up one line because spdlog always ends messages with end-of-line:
        static constexpr std::string_view erase = "\x1B[1A\x1B[2K\r";
        stream_.write(erase.data(), static_cast<std::streamsize>(erase.size()));
    }
    stream_.write(formatted.data(), static_cast<std::streamsize>(formatted.size()));
    progress_active_ = log_context::is_progress();
}

void StreamSink::flush_() { stream_.flush(); }
