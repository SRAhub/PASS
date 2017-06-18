#include "../../include/pass_bits/optimiser/particle_swarm_optimisation.hpp"

// pass::random_number_generator(), pass::random_neighbour()
#include "../../include/pass_bits/helper/random.hpp"

// std::accumulate
#include <algorithm>

// std::pow
#include <cmath>

// assert
#include <cassert>

pass::particle_swarm_optimisation::particle_swarm_optimisation() noexcept
    : optimiser(),
      initial_velocity(0.5),
      maximal_acceleration(1.0 / (2.0 * std::log(2.0))),
      maximal_local_attraction(0.5 + std::log(2.0)),
      maximal_global_attraction(maximal_local_attraction) {}

pass::optimise_result pass::particle_swarm_optimisation::optimise(
    const pass::problem& problem) {
  const arma::uword dimension = problem.dimension();
  const arma::uword particle_count = dimension * 20;

  assert(initial_velocity >= 0.0);
  assert(maximal_acceleration >= 0.0);
  assert(maximal_local_attraction >= 0.0);
  assert(maximal_global_attraction >= 0.0);

  auto start_time = std::chrono::steady_clock::now();
  pass::optimise_result result(dimension);

  // Particle data, stored column-wise.
  arma::mat positions = problem.random_parameters(particle_count);

  arma::mat velocities(dimension, particle_count, arma::fill::randu);
  velocities *= 2 * initial_velocity;
  velocities.for_each([this](auto& elem) { elem -= initial_velocity; });

  arma::mat best_found_parameters = positions;
  arma::rowvec best_found_values(particle_count);

  // Evaluate the initial positions.
  for (arma::uword n = 0; n < particle_count; ++n) {
    const auto& parameter = positions.col(n);
    const double objective_value = best_found_values(n) =
        problem.evaluate(parameter);

    ++result.evaluations;
    result.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - start_time);

    if (objective_value <= result.objective_value) {
      result.parameter = parameter;
      result.objective_value = objective_value;

      if (result.objective_value <= acceptable_objective_value) {
        return result;
      }
    }

    if (result.evaluations >= maximal_evaluations) {
      return result;
    } else if (result.duration >= maximal_duration) {
      return result;
    }
  }
  ++result.iterations;

  // Evaluate a single particle per loop iteration.
  while (result.duration < maximal_duration &&
         result.evaluations < maximal_evaluations &&
         result.objective_value > acceptable_objective_value) {
    const auto n = result.evaluations % particle_count;

    const auto& position = positions.col(n);
    const auto& velocity = velocities.col(n);
    const auto& best_found_parameter = best_found_parameters.col(n);
    const double best_found_value = best_found_values(n);

    const arma::vec weighted_local_attraction =
        random_uniform_in_range(0.0, maximal_local_attraction) *
        (best_found_parameter - position);
    const arma::vec weighted_global_attraction =
        random_uniform_in_range(0.0, maximal_global_attraction) *
        (result.parameter - position);
    const arma::vec acceleration =
        (weighted_local_attraction + weighted_global_attraction) / 3.0;
    const arma::vec displaced_acceleration =
        random_neighbour(acceleration, 0.0, arma::norm(acceleration));

    const double inertia = random_uniform_in_range(0, maximal_acceleration);
    velocities.col(n) = inertia * velocity + displaced_acceleration;
    positions.col(n) += velocity;
    for (arma::uword k = 0; k < dimension; ++k) {
      if (position(k) < problem.lower_bounds(k)) {
        positions(k, n) = problem.lower_bounds(k);
        velocities(k, n) *= -0.5;
      } else if (position(k) > problem.upper_bounds(k)) {
        positions(k, n) = problem.upper_bounds(k);
        velocities(k, n) *= -0.5;
      }
    }

    const double objective_value = problem.evaluate(position);
    ++result.evaluations;
    if (n == particle_count - 1) {
      ++result.iterations;
    }
    result.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - start_time);

    if (objective_value < best_found_value) {
      best_found_parameters.col(n) = position;
      best_found_values(n) = objective_value;
    }
    if (objective_value < result.objective_value) {
      result.parameter = position;
      result.objective_value = objective_value;
    }
  }

  return result;
}
