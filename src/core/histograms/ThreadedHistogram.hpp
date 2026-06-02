/**
 * @file
 * @brief Threaded histogram interface class
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_THREADED_HISTOGRAM_H
#define ALLPIX_THREADED_HISTOGRAM_H

#include <cmath>
#include <concepts>
#include <optional>
#include <string>
#include <utility>

#include <ROOT/TThreadedObject.hxx>
#include <TAxis.h>
#include <TH1.h>
#include <TObject.h>

#include "core/module/ThreadPool.hpp"
#include "core/utils/log.h"

namespace allpix {
    // NOLINTBEGIN(readability-identifier-naming)

    /**
     * @brief Base class for ThreadedHistograms
     *
     * Main right to exist is for the HistogramManager to be able to store pointers to these objects in a container.
     * No inherent functionality.
     */
    class BaseHistogram {
    public:
        BaseHistogram() = default;
        virtual ~BaseHistogram() = default;
        BaseHistogram(const BaseHistogram&) = default;
        BaseHistogram& operator=(const BaseHistogram&) = default;
        BaseHistogram(BaseHistogram&&) noexcept = default;
        BaseHistogram& operator=(BaseHistogram&&) noexcept = default;

        virtual std::string GetName() = 0;
        virtual void write() = 0;
        virtual void SetOption(std::string) = 0;
        virtual void setAdjustAxisRanges() = 0;
        virtual void setAdjustAxisDivisions() = 0;
        virtual void SetMinimum(double) = 0;
        virtual double GetMinimum() const = 0;
        virtual void SetMaximum(double) = 0;
        virtual double GetMaximum() const = 0;
    };

    /**
     * @brief A re-implementation of ROOT::TThreadedObject
     *
     * This class is a re-implementation of TThreadedObject for histograms and profiles, providing better scalability and an
     * additional thin wrapper to commonly used histogram functions such as Fill() or SetBinContent(). Furthermore, it also
     * does not depend on ROOT implementation changes that have happened to the original class between minor ROOT versions.
     * This class scales to an arbitrary number of thread, irrespective of the underlying ROOT version.
     *
     * Enables filling histograms in parallel and makes sure an empty instance will exist if not filled.
     */
    template <typename T> requires std::derived_from<T, TH1> class ThreadedHistogram : public BaseHistogram {
        friend class HistogramManager;

    public:
        template <class... ARGS> explicit ThreadedHistogram(ARGS&&... args) { this->init(std::forward<ARGS>(args)...); }

        /**
         * @brief An easy way to fill a histogram
         */
        template <class... ARGS> Int_t Fill(ARGS&&... args) { return this->Get()->Fill(std::forward<ARGS>(args)...); }

        /**
         * @brief An easy way to set bin contents
         */
        template <class... ARGS> void SetBinContent(ARGS&&... args) {
            this->Get()->SetBinContent(std::forward<ARGS>(args)...);
        }

        /**
         * @brief Extract the name of the histogram
         */
        std::string GetName() override { return this->Get()->GetName(); }

        /**
         * @brief Store a draw option for this histogram
         * @param option Draw option parsed as string
         */
        void SetOption(std::string option) override { drawing_options_.emplace(option); }

        /**
         * @brief Sets a tag that will invoke an axis range adjustment at the time of writing
         */
        void setAdjustAxisRanges() override { auto_adjust_axis_ranges_ = true; }

        /**
         * @brief Sets a tag that will invoke an axis divisions adjustment at the time of writing
         */
        void setAdjustAxisDivisions() override { auto_adjust_axis_divisions_ = true; }

        /**
         * @brief Set minimum value for plotting range
         * @param minimum Minimum value for plotting range
         */
        void SetMinimum(double minimum) override { draw_minimum_ = minimum; }

        /**
         * @brief Set maximum value for plotting range
         * @param maximum Maximum value for plotting range
         */
        void SetMaximum(double maximum) override { draw_maximum_ = maximum; }

        /**
         * @brief Get minimum value for plotting range
         */
        double GetMinimum() const override { return objects_[0]->GetMinimum(); }

        /**
         * @brief Get maximum value for plotting range
         */
        double GetMaximum() const override { return objects_[0]->GetMaximum(); }

        /**
         * @brief Get the thread local instance of the histogram
         *
         * Based on get in https://root.cern/doc/master/classROOT_1_1TThreadedObject.html, optimized for faster retrieval.
         */
        std::shared_ptr<T> Get();

    private:
        /**
         * @brief Initialize the threaded histogram
         *
         * Based on initialization in https://root.cern/doc/master/classROOT_1_1TThreadedObject.html, modified to
         * initialize based on number of preregistered threads.
         */
        template <class... ARGS> void init(ARGS&&... args);

        /**
         * @brief An easy way to write a histogram. Applies draw & modification options before.
         */
        void write() override;

        // NOLINTEND(readability-identifier-naming)

        /**
         * @brief Merge the threaded histograms into final object
         *
         * Based on merging in https://root.cern/doc/master/classROOT_1_1TThreadedObject.html.
         */
        void merge();

        std::unique_ptr<T> model_;
        std::vector<std::shared_ptr<T>> objects_;
        std::vector<TDirectory*> directories_;
        bool is_merged_{false};
        std::unordered_set<std::string> drawing_options_;
        bool auto_adjust_axis_ranges_{false};
        bool auto_adjust_axis_divisions_{false};
        std::optional<double> draw_minimum_;
        std::optional<double> draw_maximum_;
    };

    template <class T> using Histogram = std::shared_ptr<ThreadedHistogram<T>>;
} // namespace allpix

// Include template members
#include "ThreadedHistogram.tpp"

#endif /* ALLPIX_THREADED_HISTOGRAM_H */
