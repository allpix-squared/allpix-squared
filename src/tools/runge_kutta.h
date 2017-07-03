/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_RUNGE_KUTTA_H
#define ALLPIX_RUNGE_KUTTA_H

#include <functional>

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace allpix {

    template <typename T, int S, int D = 3> class RungeKutta {
    public:
        // FIXME: is this a good return type...
        class Step {
        public:
            Eigen::Matrix<T, D, 1> value;
            Eigen::Matrix<T, D, 1> error;
        };

        using StepFunction = std::function<Eigen::Matrix<T, D, 1>(T, Eigen::Matrix<T, D, 1>)>;

        // constructor
        RungeKutta(Eigen::Matrix<T, S + 2, S> tableau,
                   StepFunction function,
                   T step_size,
                   Eigen::Matrix<T, D, 1> initial_y,
                   T initial_t = 0)
            : tableau_(std::move(tableau)), function_(std::move(function)), h_(std::move(step_size)),
              y_(std::move(initial_y)), t_(std::move(initial_t)) {
            error_.setZero();
        }

        // change parameters
        void setTimeStep(T step_size) { h_ = std::move(step_size); }
        T getTimeStep() { return h_; }

        // set value (if some extra operations are done)
        void setValue(Eigen::Matrix<T, D, 1> y) { y_ = std::move(y); }

        // get result
        Eigen::Matrix<T, D, 1> getValue() { return y_; }
        Eigen::Matrix<T, D, 1> getError() { return error_; }
        T getTime() { return t_; }

        // execute runge kutta step
        Step step() {
            Step step;
            Eigen::Matrix<T, D, 1> ys;
            Eigen::Matrix<T, D, 1> error;
            ys.setZero();
            error.setZero();

            // compute step
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
                error += h_ * (tableau_(S, i) - tableau_(S + 1, i)) * k.row(i);
            }

            // update step
            y_ += ys;
            t_ += h_;

            // return step
            step.value = ys;
            step.error = error;
            return step;
        }

        // FIXME: call this steps?
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
        T h_; // step size

        Eigen::Matrix<T, D, 1> y_;     // vector that changes
        Eigen::Matrix<T, D, 1> error_; // error vector
        T t_;                          // time
    };

    // clang-format off
    namespace tableau {
        // WARNING: no error function
        static const auto RK3((Eigen::Matrix<double, 5, 3>() <<
            0, 0, 0,
            1.0/2, 0, 0,
            -1, 2, 0,
            1.0/6, 2.0/3, 1.0/6,
            0, 0, 0).finished());
        // WARNING: no error function
        static const auto RK4((Eigen::Matrix<double, 6, 4>() <<
            0, 0, 0, 0,
            1.0/2, 0, 0, 0,
            0, 1.0/2, 0, 0,
            0, 0, 1, 0,
            1.0/6, 1.0/3, 1.0/3, 1.0/6,
            0, 0, 0, 0).finished());
        static const auto RK5((Eigen::Matrix<double, 8, 6>() <<
            0, 0, 0, 0, 0, 0,
            1.0/4, 0, 0, 0, 0, 0,
            3.0/32, 9.0/32, 0, 0, 0, 0,
            1932.0/2197, -7200.0/2197, 7296.0/2197, 0, 0, 0,
            439.0/216, -8, 3680.0/513, -845.0/4104, 0, 0,
            -8.0/27, 2, -3544.0/2565, 1859.0/4104, -11.0/40, 0,
            16.0/135, 0, 6656.0/12825, 28561.0/56430, -9.0/50, 2.0/55,
            25.0/216, 0, 1408.0/2565, 2197.0/4104, -1.0/5, 0).finished());
    }
    // clang-format on

    // FIXME: better name
    template <typename T, int S, int D = 3>
    RungeKutta<T, S, D> make_runge_kutta(Eigen::Matrix<T, S + 2, S> tableau,
                                         typename RungeKutta<T, S, D>::StepFunction function,
                                         T step_size,
                                         Eigen::Matrix<T, D, 1> initial_y,
                                         T initial_t = 0) {
        return RungeKutta<T, S, D>(
            std::move(tableau), std::move(function), std::move(step_size), std::move(initial_y), std::move(initial_t));
    }
} // namespace allpix

#endif /* ALLPIX_RUNGE_KUTTA_H */
