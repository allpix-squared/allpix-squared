/**
 * @file
 * @brief Threaded histogram interface class implementation
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "ThreadedHistogram.hpp" // NOLINT(misc-header-include-cycle)

#include <concepts>
#include <string>
#include <utility>

#include <ROOT/TThreadedObject.hxx>
#include <TAxis.h>
#include <TH1.h>
#include <TObject.h>

#include "core/module/ThreadPool.hpp"
#include "core/utils/log.h"

namespace allpix {

    template <typename T> requires std::derived_from<T, TH1> std::shared_ptr<T> ThreadedHistogram<T>::Get() {
        auto idx = ThreadPool::threadNum();
        auto& object = objects_[idx];
        if(!object) {
            object.reset(ROOT::Internal::TThreadedObjectUtils::Cloner<T>::Clone(model_.get(), directories_[idx]));
        }
        return object;
    }

    template <typename T>
    requires std::derived_from<T, TH1> template <class... ARGS>
    void ThreadedHistogram<T>::init(ARGS&&... args) {
        const auto num_slots = ThreadPool::threadCount();
        objects_.resize(num_slots);

#if ROOT_VERSION_CODE < ROOT_VERSION(6, 22, 0)
        directories_ = ROOT::Internal::TThreadedObjectUtils::DirCreator<T>::Create(num_slots);
#else
        // create at least one directory (we need it for the model), plus others as needed by the size of objects
        directories_.emplace_back(ROOT::Internal::TThreadedObjectUtils::DirCreator<T>::Create());
        for(auto i = 1U; i < num_slots; ++i) {
            directories_.emplace_back(ROOT::Internal::TThreadedObjectUtils::DirCreator<T>::Create());
        }
#endif

        TDirectory::TContext ctxt(directories_[0]);
        model_.reset(ROOT::Internal::TThreadedObjectUtils::Detacher<T>::Detach(new T(std::forward<ARGS>(args)...)));

        // initialize at least the base object
        objects_[0].reset(ROOT::Internal::TThreadedObjectUtils::Cloner<T>::Clone(model_.get(), directories_[0]));
    }

    /**
     * @brief An easy way to write a histogram. Applies draw & modification options before.
     */
    template <typename T> requires std::derived_from<T, TH1> void ThreadedHistogram<T>::write() {
        this->merge();

        if(draw_minimum_) {
            objects_[0]->SetMinimum(draw_minimum_.value());
        }

        if(draw_maximum_) {
            objects_[0]->SetMaximum(draw_maximum_.value());
        }

        for(const auto& option : drawing_options_) {
            objects_[0]->SetOption(option.c_str());
        }

        auto adjust_axis_divisions = [&](TAxis* axis) {
            if(static_cast<int>(axis->GetXmax()) < 10) {
                axis->SetNdivisions(static_cast<int>(axis->GetXmax()) + 1, 0, 0, true);
            }
        };

        if(auto_adjust_axis_ranges_) {
            auto adjust_axis_range = [&](int axis_id, TAxis* axis) {
                auto xmax = std::ceil(axis->GetBinCenter(objects_[0]->FindLastBinAbove(0, axis_id)) + 1);
                axis->SetRangeUser(0, xmax);
                adjust_axis_divisions(axis);
            };

            auto ndim = objects_[0]->GetDimension();
            if(ndim >= 1) {
                adjust_axis_range(1, objects_[0]->GetXaxis());
            }
            if(ndim >= 2) {
                adjust_axis_range(2, objects_[0]->GetYaxis());
            }
            if(ndim >= 3) {
                adjust_axis_range(3, objects_[0]->GetZaxis());
            }
        }

        if(auto_adjust_axis_divisions_) {

            auto ndim = objects_[0]->GetDimension();
            if(ndim >= 1) {
                adjust_axis_divisions(objects_[0]->GetXaxis());
            }
            if(ndim >= 2) {
                adjust_axis_divisions(objects_[0]->GetYaxis());
            }
            if(ndim >= 3) {
                adjust_axis_divisions(objects_[0]->GetZaxis());
            }
        }

        objects_[0]->Write();
    }

    template <typename T> requires std::derived_from<T, TH1> void ThreadedHistogram<T>::merge() {
        ROOT::TThreadedObjectUtils::MergeFunctionType<T> mergeFunction = ROOT::TThreadedObjectUtils::MergeTObjects<T>;
        if(is_merged_) {
            return;
        }
        mergeFunction(objects_[0], objects_);
        is_merged_ = true;
    }
} // namespace allpix
