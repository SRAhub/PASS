#include "pass_bits/optimiser/parallel_swarm_search.hpp"
#include "pass_bits/helper/random.hpp"
#include <cmath> // std::pow

pass::parallel_swarm_search::parallel_swarm_search() noexcept
    : optimiser("Parallel_Swarm_Search"),
      swarm_size(40),
      inertia(1.0 / (2.0 * std::log(2.0))),
      cognitive_acceleration(0.5 + std::log(2.0)),
      social_acceleration(cognitive_acceleration),
      neighbourhood_probability(1.0 -
                                std::pow(1.0 - 1.0 / static_cast<double>(swarm_size), 3.0)) {}

pass::optimise_result pass::parallel_swarm_search::optimise(
    const pass::problem &problem)
{
  assert(inertia >= 0.0 && "'inertia' should be greater or equal than 0.0");
  assert(cognitive_acceleration >= 0.0 && "'cognitive_acceleration' should be greater or equal than 0.0");
  assert(social_acceleration >= 0.0 && "'social_acceleration' should be greater or equal than 0.0");
  assert(neighbourhood_probability > 0.0 && neighbourhood_probability <= 1.0 &&
         "'neighbourhood_probability' should be a value between 0.0 and 1.0");
  assert(swarm_size > 0 && "Can't generate 0 agents");

  // Variables used to analyse the behavior of a particle
  arma::mat verbose(maximal_iterations + 1, 4);
  arma::vec best_agent_velocity(problem.dimension());

  pass::stopwatch stopwatch;
  stopwatch.start();

  // Variables needed
  pass::optimise_result result(problem, acceptable_fitness_value);

  arma::mat positions;
  arma::mat velocities(problem.dimension(), swarm_size);

  arma::mat personal_best_positions;
  arma::rowvec personal_best_fitness_values(swarm_size);

  arma::umat topology(swarm_size, swarm_size);

  arma::vec local_best_position;
  double local_best_fitness_value;

  arma::vec attraction_center;
  arma::vec weighted_personal_attraction;
  arma::vec weighted_local_attraction;

  double fitness_value;

  // Initialise the positions and the velocities
  // Particle data, stored column-wise.
  positions = problem.normalised_hammersley_agents(swarm_size);

  for (arma::uword col = 0; col < swarm_size; ++col)
  {
    for (arma::uword row = 0; row < problem.dimension(); ++row)
    {
      velocities(row, col) = random_double_uniform_in_range(
          0.0 - positions(row, col),
          1.0 - positions(row, col));
    }
  }

  personal_best_positions = positions;

  // Evaluate the initial positions.
  // Compute the fitness.
  // Begin with the previous best set to this initial position
  for (arma::uword n = 0; n < swarm_size; ++n)
  {
    fitness_value = problem.evaluate_normalised(positions.col(n));
    personal_best_fitness_values(n) = fitness_value;

    if (fitness_value <= result.fitness_value)
    {
      result.normalised_agent = positions.col(n);
      best_agent_velocity = velocities.col(n);
      result.fitness_value = fitness_value;
    }
  }
  ++result.iterations;

  /**
   * +------------+---------------+----------+----------+
   * | Iterations | Fitness Value | Position | Velocity |
   * +------------+---------------+----------+----------+
   * Each Dimension is independent. So, the analysis can be performed
   * on just one dimension
   */
  if (pass::is_verbose)
  {
    if (maximal_iterations != std::numeric_limits<arma::uword>::max() && maximal_iterations > 0)
    {
      verbose(result.iterations, 0) = result.iterations;
      verbose(result.iterations, 1) = result.fitness_value;
      verbose(result.iterations, 2) = result.agent().at(0);
      verbose(result.iterations, 3) = best_agent_velocity[0];
    }
    else
    {
      throw std::runtime_error(
          "Please set - maximal_iterations - to a valid number to analyse the behaviour of the algorithm.");
    }
  }
  //end initialisation

  bool randomize_topology = true;

  // termination criteria.
  while (stopwatch.get_elapsed() < maximal_duration &&
         result.iterations < maximal_iterations && !result.solved())
  {
    if (randomize_topology)
    {
      topology = (arma::mat(swarm_size, swarm_size,
                            arma::fill::randu) < neighbourhood_probability);
      // When searching for the best neighbour, we begin with the particles
      // personal best value; We don't need to visit it twice.
      topology.diag().fill(0);
    }
    randomize_topology = true;

#if defined(SUPPORT_OPENMP)
#pragma omp parallel for proc_bind(spread) num_threads(pass::number_of_threads()) private(local_best_position, local_best_fitness_value, attraction_center, weighted_personal_attraction, weighted_local_attraction, fitness_value) schedule(static)
#endif

    // iterate over the particles
    for (arma::uword n = 0; n < swarm_size; ++n)
    {
      // l_i^t
      local_best_position = personal_best_positions.col(n);
      local_best_fitness_value = personal_best_fitness_values(n);

      // check the topology to identify with which particle you communicate
      for (arma::uword i = 0; i < swarm_size; i++)
      {
        if (topology(n, i) && personal_best_fitness_values(i) < local_best_fitness_value)
        {
          local_best_fitness_value = personal_best_fitness_values(i);
          local_best_position = personal_best_positions.col(i);
        }
      }

        /**
         * Compute the new velocity
         * If OpenMP is activated, make sure the random numbers are thread safe
         */
#if defined(SUPPORT_OPENMP)
      arma::arma_rng::set_seed_random();
#endif
      //p_i
      weighted_personal_attraction = positions.col(n) +
                                     random_double_uniform_in_range(0.0, cognitive_acceleration) *
                                         (personal_best_positions.col(n) - positions.col(n));

      // l_i
      weighted_local_attraction = positions.col(n) +
                                  random_double_uniform_in_range(0.0, social_acceleration) *
                                      (local_best_position - positions.col(n));

      // If the best informant is the particle itself, define the gravity center G as the middle of x-p'
      if (personal_best_fitness_values(n) == local_best_fitness_value)
      {
        attraction_center = 0.5 * (positions.col(n) + weighted_personal_attraction);
      }
      else
      {
        attraction_center = (positions.col(n) + weighted_personal_attraction + weighted_local_attraction) / 3.0;
      }

      velocities.col(n) = inertia * velocities.col(n) +
                          random_neighbour(attraction_center, 0.0, arma::norm(attraction_center - positions.col(n))) -
                          positions.col(n);

      // move by applying this new velocity to the current position
      positions.col(n) = positions.col(n) + velocities.col(n);

      // stay inside the bounds
      for (arma::uword k = 0; k < problem.dimension(); ++k)
      {
        if (positions(k, n) < 0)
        {
          positions(k, n) = 0;
          velocities(k, n) = -0.5 * velocities(k, n);
        }
        else if (positions(k, n) > 1)
        {
          positions(k, n) = 1;
          velocities(k, n) = -0.5 * velocities(k, n);
        }
      }

      // evaluate the new position
      fitness_value = problem.evaluate_normalised(positions.col(n));
      if (fitness_value < personal_best_fitness_values(n))
      {
        personal_best_positions.col(n) = positions.col(n);
        personal_best_fitness_values(n) = fitness_value;

        if (fitness_value < result.fitness_value)
        {
#if defined(SUPPORT_OPENMP)
#pragma omp critical
          {
#endif
            result.normalised_agent = positions.col(n);
            best_agent_velocity = velocities.col(n);
            result.fitness_value = fitness_value;
            randomize_topology = false;
#if defined(SUPPORT_OPENMP)
          }
#endif
        }
      }
    }
    ++result.iterations;

    /**
     * +------------+---------------+----------+----------+
     * | Iterations | Fitness Value | Position | Velocity |
     * +------------+---------------+----------+----------+
     * Each Dimension is independent. So, the analysis can be performed
     * on just one dimension
     */
    if (pass::is_verbose)
    {
      verbose(result.iterations, 0) = result.iterations;
      verbose(result.iterations, 1) = result.fitness_value;
      verbose(result.iterations, 2) = result.agent().at(0);
      verbose(result.iterations, 3) = best_agent_velocity[0];
    }
  }
  result.duration = stopwatch.get_elapsed();

  // Save the file
  if (pass::is_verbose)
  {
    verbose.shed_row(0);
    verbose.save("Verbose_Optimiser_" + name + "_Problem_" + problem.name + "_Dim_" +
                     std::to_string(problem.dimension()) +
                     "_Run_" + std::to_string(pass::number_of_runs),
                 arma::raw_ascii);
  }

  return result;
}