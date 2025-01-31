/**
 * @file
 * @brief Utility to execute Runge-Kutta integration using Eigen
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_RUNGE_KUTTA_H
#define ALLPIX_RUNGE_KUTTA_H

#include <functional>

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace allpix {

    /**
     * @brief Class to perform arbitrary Runge-Kutta integration
     *
     * Class can be provided a Runge-Kutta tableau (optionally with an error function), together with the dimension of the
     * equations and a step function to integrate a step of the equation. Both the result, error and timestep can be
     * retrieved and changed during the integration.
     */
    template <typename T, int S, int D = 3> class RungeKutta {
    public:
        /**
         * @brief Utility type to return both the value and the error at every step
         */
        // FIXME Is this a appropriate return type or should pairs be preferred
        class Step {
        public:
            Eigen::Matrix<T, D, 1> value;
            Eigen::Matrix<T, D, 1> error;
        };

        /**
         * @brief Stepping function to integrate a single step of the equations
         */
        using StepFunction = std::function<Eigen::Matrix<T, D, 1>(T, Eigen::Matrix<T, D, 1>)>;

        /**
         * @brief Construct a Runge-Kutta integrator
         * @param tableau One of the possible Runge-Kutta tables (see \ref allpix::tableau should be preferred)
         * @param function Step function to perform integration
         * @param step_size Time step of the integration
         * @param initial_y Start values of the vector to perform integration on
         * @param initial_t Initial time at the start of the integration
         */
        RungeKutta(Eigen::Matrix<T, S + 2, S> tableau,
                   StepFunction function,
                   T step_size,
                   Eigen::Matrix<T, D, 1> initial_y,
                   T initial_t = 0)
            : tableau_(std::move(tableau)), function_(std::move(function)), h_(std::move(step_size)),
              y_(std::move(initial_y)), t_(std::move(initial_t)) {
            error_.setZero();
        }

        /**
         * @brief Changes the time step
         * @param step_size New time step of the integration
         */
        void setTimeStep(T step_size) { h_ = std::move(step_size); }
        /**
         * @brief Return the time step
         * @return Current time step of the integration
         */
        T getTimeStep() const { return h_; }

        /**
         * @brief Changes the current value during integration
         * @note Can be used to add additional processes during the integration
         */
        void setValue(Eigen::Matrix<T, D, 1> y) { y_ = std::move(y); }

        /**
         * @brief Get the value to integrate
         * @return Current value
         */
        Eigen::Matrix<T, D, 1> getValue() const { return y_; }
        /**
         * @brief Get the total integration error
         * @return Total integrated error
         */
        Eigen::Matrix<T, D, 1> getError() const { return error_; }
        /**
         * @brief Get the time during integration
         * @return Current time
         */
        T getTime() const { return t_; }
        /**
         * @brief Advance the time of the integration
         * @param t Time step to advance the integration by
         */
        void advanceTime(double t) { t_ += t; }

        /**
         * @brief Execute a single time step of the integration
         * @return Combination of the current value and the error in this single step
         */
        Step step() {
            // Initialize values
            Step step;
            Eigen::Matrix<T, D, 1> ys;
            Eigen::Matrix<T, D, 1> yse;
            ys.setZero();
            yse.setZero();

            // Compute step
            Eigen::Matrix<T, S, D> k;
            for(int i = 0; i < S; ++i) {
                Eigen::Matrix<T, D, 1> yt = y_;
                T tt = t_;
                for(int j = 0; j < i; ++j) {
                    yt += h_ * tableau_(i, j) * k.row(j);
                    tt += tableau_(i, j);
                }
                k.row(i) = function_(tt, yt);

                ys += h_ * tableau_(S, i) * k.row(i);
                yse += h_ * tableau_(S + 1, i) * k.row(i);
            }

            // Update values with new step
            y_ += ys;
            t_ += h_;
            error_ += ys - yse;

            // Return step information
            step.value = ys;
            step.error = ys - yse;
            return step;
        }

        /**
         * @brief Execute multiple time steps of the integration
         * @param amount Number of steps to combine
         * @return Combination of the current value and the total error in all the steps
         */
        Step step(int amount) {
            Step result;
            result.value.setZero();
            result.error.setZero();
            for(int i = 0; i < amount; ++i) {
                Step single = step();
                result.value += single.value;
                result.error += single.error;
            }
            return result;
        }

    private:
        const Eigen::Matrix<T, S + 2, S> tableau_;
        StepFunction function_;
        // Step size
        T h_;

        // Vector to integrate
        Eigen::Matrix<T, D, 1> y_;
        // Total error vector
        Eigen::Matrix<T, D, 1> error_;
        // Current time
        T t_;
    };

    // clang-format off
    namespace tableau {
        /**
         * @brief Kutta's third order method
         * @warning Without error function
         */
        static const auto RK3((Eigen::Matrix<double, 5, 3>() <<
            0, 0, 0,
            1.0/2, 0, 0,
            -1, 2, 0,
            1.0/6, 2.0/3, 1.0/6,
            0, 0, 0).finished());
        /**
         * @brief Classic original Runge-Kutta method
         * @warning Without error function
         */
        static const auto RK4((Eigen::Matrix<double, 6, 4>() <<
            0, 0, 0, 0,
            1.0/2, 0, 0, 0,
            0, 1.0/2, 0, 0,
            0, 0, 1, 0,
            1.0/6, 1.0/3, 1.0/3, 1.0/6,
            0, 0, 0, 0).finished());
        /**
         * @brief Runge-Kutta-Fehlberg method
         * Values from https://ntrs.nasa.gov/citations/19680027281, p.13, Table III
         */
        static const auto RK5((Eigen::Matrix<double, 8, 6>() <<
            0, 0, 0, 0, 0, 0,
            1.0/4, 0, 0, 0, 0, 0,
            3.0/32, 9.0/32, 0, 0, 0, 0,
            1932.0/2197, -7200.0/2197, 7296.0/2197, 0, 0, 0,
            439.0/216, -8, 3680.0/513, -845.0/4104, 0, 0,
            -8.0/27, 2, -3544.0/2565, 1859.0/4104, -11.0/40, 0,
            16.0/135, 0, 6656.0/12825, 28561.0/56430, -9.0/50, 2.0/55,
            25.0/216, 0, 1408.0/2565, 2197.0/4104, -1.0/5, 0).finished());
        /**
         * @brief Runge-Kutta-Cash-Karp method
         */
        static const auto RKCK((Eigen::Matrix<double, 8, 6>() <<
            0, 0, 0, 0, 0, 0,
            1.0/5, 0, 0, 0, 0, 0,
            3.0/40, 9.0/40, 0, 0, 0, 0,
            3.0/10, -9.0/10, 6.0/5, 0, 0, 0,
            -11.0/54, 5.0/2, -70.0/27, 35.0/27, 0, 0,
            1631.0/55296, 175.0/512, 575.0/13824, 44275.0/110592, 253.0/4096, 0,
            37.0/378, 0, 250.0/621, 125.0/594, 0, 512.0/1771,
            2825.0/27648, 0, 18575.0/48384, 13525.0/55296, 277.0/14336, 1.0/4).finished());
    }
    // clang-format on

    /**
     * @brief Utility function to create RungeKutta class using template deduction
     * @param tableau One of the possible Runge-Kutta tableaus (see \ref allpix::tableau)
     * @param args Other forwarded arguments to the \ref RungeKutta::RungeKutta constructor
     * @return Instantiation of \ref RungeKutta class with the forwarded arguments
     */
    template <typename T, int S, int D = 3, class... Args>
    RungeKutta<T, S, D> make_runge_kutta(const Eigen::Matrix<T, S + 2, S>& tableau, Args&&... args) {
        return RungeKutta<T, S, D>(tableau, std::forward<Args>(args)...);
    }
} // namespace allpix

#endif /* ALLPIX_RUNGE_KUTTA_H */
