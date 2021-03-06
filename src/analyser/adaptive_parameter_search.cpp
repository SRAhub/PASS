#include "pass_bits/analyser/adaptive_parameter_search.hpp"
#include "pass_bits/optimiser/parallel_swarm_search.hpp"
#include "pass_bits/helper/random.hpp"
#include <iostream> // std::cout

void pass::search_parameters(const pass::problem &problem, const bool benchmark)
{
  // Our algorithm
  pass::parallel_swarm_search algorithm;

  // Time to run a black box problem
  const arma::uword time_in_seconds = 10;

  // Global Winner
  arma::uword global_winner;
  arma::mat global_best_runtimes;

  // Check if the problem is a benchmark problem or not
  if (benchmark == true)
  {
    algorithm.maximal_evaluations = 2000000;
    algorithm.maximal_duration = std::chrono::seconds(time_in_seconds);
    algorithm.acceptable_fitness_value = pass::precision;
  }
  else if (benchmark == false)
  {
    algorithm.maximal_evaluations = 2000000;
    algorithm.maximal_duration = std::chrono::seconds(time_in_seconds);
  }
  else
  {
    throw std::runtime_error(
        "benchmark: Please check your benchmark value, as we got not a valid value");
  }

  // Output file
  std::string file_name;
  file_name = "adaptive_parameter_";
  file_name += problem.name;
  file_name += "_d" + std::to_string(problem.dimension());
  file_name += ".pass";

  // Output information
  std::cout << " ==================== Start Parameter Optimisation ================== " << std::endl;
  std::cout << "                                                                      " << std::endl;
  std::cout << " Your Algorithm:              " << algorithm.name << std::endl;
  std::cout << " Your Problem:                " << problem.name << std::endl;
  std::cout << " Dimension:                   " << problem.dimension() << std::endl;
  if (benchmark == true)
  {
    std::cout << " Benchmark?                   "
              << "Yes" << std::endl;
  }
  else
  {
    std::cout << " Benchmark?                   "
              << "No" << std::endl;
  }
  std::cout << " ==================================================================== " << std::endl;
  std::cout << "                                                                      " << std::endl;

  arma::vec input(5);
  // Check if the file exists
  bool ok = input.load("./" + file_name);

  // Output information
  std::cout << " ============================== Check  ============================== " << std::endl;
  std::cout << "                                                                      " << std::endl;
  if (ok == true)
  {
    std::cout << " - Adaptive Parameters exists!                                      " << std::endl;
    std::cout << "                                                                    " << std::endl;
    // Output information
    std::cout << " ======================== Adaptive Parameters ======================= " << std::endl;
    std::cout << "                                                                      " << std::endl;
    std::cout << " Swarm Size:                  " << input(0) << std::endl;
    std::cout << " Neighbourhood Probability:   " << input(1) << std::endl;
    std::cout << " Inertia:                     " << input(2) << std::endl;
    std::cout << " Cognitive Acceleration:      " << input(3) << std::endl;
    std::cout << " Social Acceleration:         " << input(4) << std::endl;
    std::cout << " ==================================================================== " << std::endl;
    std::cout << "                                                        " << std::endl;
    std::cout << " Filename: '" << file_name << "'" << std::endl;
    std::cout << "                                                        " << std::endl;
    std::cout << " =============================== DONE =============================== " << std::endl;
    return;
  }

  std::cout
      << " - Adaptive Parameters does not exist!                              " << std::endl;
  std::cout << "                                                                    " << std::endl;

  // ================================ Default Parameters ================================

  // Output information
  std::cout << " ========================= Default Parameters ======================= " << std::endl;
  std::cout << "                                                                      " << std::endl;
  std::cout << " Swarm Size:                  " << algorithm.swarm_size << std::endl;
  std::cout << " Neighbourhood Probability:   " << algorithm.neighbourhood_probability << std::endl;
  std::cout << " Inertia:                     " << algorithm.inertia << std::endl;
  std::cout << " Cognitive Acceleration:      " << algorithm.cognitive_acceleration << std::endl;
  std::cout << " Social Acceleration:         " << algorithm.social_acceleration << std::endl;
  std::cout << " ==================================================================== " << std::endl;
  std::cout << "                                                                      " << std::endl;

  arma::mat default_runtimes = parameter_evaluate(algorithm, problem);
  global_best_runtimes = default_runtimes;

  arma::vec default_nonZeroIndices = arma::nonzeros(default_runtimes.col(1));
  double default_success = static_cast<double>(default_nonZeroIndices.size()) / static_cast<double>(pass::parameter_setting_number_of_runs) * 100.00;
  double default_evaluations = std::numeric_limits<double>::max();
  double default_fitness_value = std::numeric_limits<double>::max();

  if (default_nonZeroIndices.size() == 0)
  {
    default_fitness_value = arma::median(default_runtimes.col(0));
  }
  else
  {
    default_evaluations = arma::median(default_nonZeroIndices);
  }

  // Output information
  std::cout << " ================ Done Optimising Default Parameters ================ " << std::endl;
  std::cout << "                                                                      " << std::endl;
  if (benchmark == true)
  {
    std::cout << " Success:                     " << default_success << " % " << std::endl;
  }
  if (default_nonZeroIndices.size() == 0)
  {
    std::cout << " Fitness Value:               " << default_fitness_value << std::endl;
  }
  else
  {
    std::cout << " Evaluations:                 " << default_evaluations << std::endl;
  }
  std::cout << " ==================================================================== " << std::endl;
  std::cout << "                                                                      " << std::endl;

  // ================================ End Default Parameters ================================
  //
  //
  //
  //
  // ====================================== Swarm Size ======================================

  std::cout << " ==================== Optimising Parameters......  ================== " << std::endl;
  std::cout << "                                                                      " << std::endl;
  std::cout << " - Start: Search for Population " << std::endl;

  arma::uword p_min = 10;
  arma::uword p_max = 200;

  arma::uword p_granularity = 1;
  arma::mat p_best_segment_runtimes;

  while (p_granularity <= 4)
  {
    arma::uword p_middle = (p_max - p_min) / 2 + p_min;

    arma::uword first_segment = pass::random_integer_uniform_in_range(p_min, p_middle);
    algorithm.swarm_size = first_segment;
    arma::mat first_segment_runtimes = parameter_evaluate(algorithm, problem);

    arma::uword second_segment = pass::random_integer_uniform_in_range(p_middle, p_max);
    algorithm.swarm_size = second_segment;
    arma::mat second_segment_runtimes = parameter_evaluate(algorithm, problem);

    arma::uword winner = compare_segments(first_segment_runtimes, second_segment_runtimes);

    if (winner == 1)
    {
      p_max = p_middle;
      algorithm.swarm_size = first_segment;
      p_best_segment_runtimes = first_segment_runtimes;
    }
    else
    {
      p_min = p_middle;
      algorithm.swarm_size = second_segment;
      p_best_segment_runtimes = second_segment_runtimes;
    }

    p_granularity++;
  }

  // Compare with default
  global_winner = compare_segments(p_best_segment_runtimes, global_best_runtimes);

  // If the population search doesn't find a better value than the default
  if (global_winner == 1)
  {
    global_best_runtimes = p_best_segment_runtimes;
  }
  else
  {
    algorithm.swarm_size = 40;
  }

  std::cout << " - Finished: Search for Population " << std::endl;
  std::cout << "                                                                      " << std::endl;

  // ================================ End Swarm Size ================================
  //
  //
  //
  //
  // ======================= Search for Neighbourhood Probability =======================

  std::cout << " - Start: Search for Neighbourhood Probability " << std::endl;

  arma::uword np_min = 0;
  arma::uword np_max = 1000000;

  arma::uword np_granularity = 1;
  arma::mat np_best_segment_runtimes;

  while (np_granularity <= 6)
  {
    arma::uword np_middle = (np_max - np_min) / 2 + np_min;

    arma::uword first_segment = pass::random_integer_uniform_in_range(np_min, np_middle);
    algorithm.neighbourhood_probability = static_cast<double>(first_segment) / 1000000;
    arma::mat first_segment_runtimes = parameter_evaluate(algorithm, problem);

    arma::uword second_segment = pass::random_integer_uniform_in_range(np_middle, np_max);
    algorithm.neighbourhood_probability = static_cast<double>(second_segment) / 1000000;
    arma::mat second_segment_runtimes = parameter_evaluate(algorithm, problem);

    arma::uword winner = compare_segments(first_segment_runtimes, second_segment_runtimes);

    if (winner == 1)
    {
      np_max = np_middle;
      algorithm.neighbourhood_probability = static_cast<double>(first_segment) / 1000000;
      np_best_segment_runtimes = first_segment_runtimes;
    }
    else
    {
      np_min = np_middle;
      algorithm.neighbourhood_probability = static_cast<double>(second_segment) / 1000000;
      np_best_segment_runtimes = second_segment_runtimes;
    }
    np_granularity++;
  }

  // Compare with best
  global_winner = compare_segments(np_best_segment_runtimes, global_best_runtimes);

  if (global_winner == 1)
  {
    global_best_runtimes = np_best_segment_runtimes;
  }
  else
  {
    algorithm.neighbourhood_probability = 1.0 -
                                          std::pow(1.0 - 1.0 / static_cast<double>(40), 3.0);
  }

  std::cout << " - Finished: Search for Neighbourhood Probability " << std::endl;
  std::cout << "                                                                      " << std::endl;

  // ======================= End Search for Neighbourhood Probability ======================
  //
  //
  //
  //
  // ================================ Search for Inertia ===================================

  std::cout << " - Start: Search for Inertia " << std::endl;

  arma::sword w_min = -1000000;
  arma::sword w_max = 1000000;

  arma::uword w_granularity = 1;
  arma::mat w_best_segment_runtimes;

  while (w_granularity <= 7)
  {
    arma::sword w_middle = (w_max - w_min) / 2 + w_min;

    arma::sword first_segment = pass::random_integer_uniform_in_range(w_min, w_middle);
    algorithm.inertia = static_cast<double>(first_segment) / 1000000;
    arma::mat first_segment_runtimes = parameter_evaluate(algorithm, problem);

    arma::sword second_segment = pass::random_integer_uniform_in_range(w_middle, w_max);
    algorithm.inertia = static_cast<double>(second_segment) / 1000000;
    arma::mat second_segment_runtimes = parameter_evaluate(algorithm, problem);

    arma::uword winner = compare_segments(first_segment_runtimes, second_segment_runtimes);

    if (winner == 1)
    {
      w_max = w_middle;
      algorithm.inertia = static_cast<double>(first_segment) / 1000000;
      w_best_segment_runtimes = first_segment_runtimes;
    }
    else
    {
      w_min = w_middle;
      algorithm.inertia = static_cast<double>(second_segment) / 1000000;
      w_best_segment_runtimes = second_segment_runtimes;
    }
    w_granularity++;
  }

  // Compare with best
  global_winner = compare_segments(w_best_segment_runtimes, global_best_runtimes);

  if (global_winner == 1)
  {
    global_best_runtimes = w_best_segment_runtimes;
  }
  else
  {
    algorithm.inertia = 1.0 / (2.0 * std::log(2.0));
  }

  std::cout << " - Finished: Search for Inertia " << std::endl;
  std::cout << "                                                                      " << std::endl;

  // ================================ End Search for Inertia ===============================
  //
  //
  //
  //
  // =============================== Search for C1 and C2 =================================

  std::cout << " - Start: Search for Acceleration " << std::endl;

  arma::uword c_min = 0;
  arma::uword c_max = 2500000;

  arma::uword c_granularity = 1;
  arma::mat c_best_segment_runtimes;

  while (c_granularity <= 7)
  {
    arma::uword c_middle = (c_max - c_min) / 2 + c_min;

    arma::uword first_segment = pass::random_integer_uniform_in_range(c_min, c_middle);
    algorithm.cognitive_acceleration = static_cast<double>(first_segment) / 1000000;
    algorithm.social_acceleration = algorithm.cognitive_acceleration;
    arma::mat first_segment_runtimes = parameter_evaluate(algorithm, problem);

    arma::uword second_segment = pass::random_integer_uniform_in_range(c_middle, c_max);
    algorithm.cognitive_acceleration = static_cast<double>(second_segment) / 1000000;
    algorithm.social_acceleration = algorithm.cognitive_acceleration;
    arma::mat second_segment_runtimes = parameter_evaluate(algorithm, problem);

    arma::uword winner = compare_segments(first_segment_runtimes, second_segment_runtimes);

    if (winner == 1)
    {
      c_max = c_middle;
      algorithm.cognitive_acceleration = static_cast<double>(first_segment) / 1000000;
      algorithm.social_acceleration = algorithm.cognitive_acceleration;
      c_best_segment_runtimes = first_segment_runtimes;
    }
    else
    {
      c_min = c_middle;
      algorithm.cognitive_acceleration = static_cast<double>(second_segment) / 1000000;
      algorithm.social_acceleration = algorithm.cognitive_acceleration;
      c_best_segment_runtimes = second_segment_runtimes;
    }
    c_granularity++;
  }

  // Compare with best
  global_winner = compare_segments(c_best_segment_runtimes, global_best_runtimes);

  if (global_winner == 1)
  {
    global_best_runtimes = c_best_segment_runtimes;
  }
  else
  {
    algorithm.cognitive_acceleration = 0.5 + std::log(2.0);
    algorithm.social_acceleration = algorithm.cognitive_acceleration;
  }

  std::cout << " - Finished: Search for Acceleration " << std::endl;
  std::cout << "                                                                      " << std::endl;

  // ============================= End Search for C1 and C2 ===============================
  //
  //
  //
  //
  // File where the parameters are saved
  arma::vec output(5);
  output(0) = algorithm.swarm_size;
  output(1) = algorithm.neighbourhood_probability;
  output(2) = algorithm.inertia;
  output(3) = algorithm.cognitive_acceleration;
  output(4) = algorithm.social_acceleration;

  output.save("./" + file_name, arma::raw_ascii);

  // Output information
  std::cout << " ======================== Adaptive Parameters ======================= " << std::endl;
  std::cout << "                                                                      " << std::endl;
  std::cout << " Swarm Size:                  " << algorithm.swarm_size << std::endl;
  std::cout << " Neighbourhood Probability:   " << algorithm.neighbourhood_probability << std::endl;
  std::cout << " Inertia:                     " << algorithm.inertia << std::endl;
  std::cout << " Cognitive Acceleration:      " << algorithm.cognitive_acceleration << std::endl;
  std::cout << " Social Acceleration:         " << algorithm.social_acceleration << std::endl;
  std::cout << " ==================================================================== " << std::endl;
  std::cout << "                                                        " << std::endl;

  // =============================== Final Evaluations =================================

  arma::vec final_nonZeroIndices = arma::nonzeros(global_best_runtimes.col(1));
  double final_success = static_cast<double>(final_nonZeroIndices.size()) / static_cast<double>(pass::parameter_setting_number_of_runs) * 100.00;
  double final_evaluations = std::numeric_limits<double>::max();
  double final_fitness_value = std::numeric_limits<double>::max();

  if (final_nonZeroIndices.size() == 0)
  {
    final_fitness_value = arma::median(global_best_runtimes.col(0));
  }
  else
  {
    final_evaluations = arma::median(final_nonZeroIndices);
  }

  // Output information
  std::cout << " ================ Done Optimising Adaptive Parameters =============== " << std::endl;
  std::cout << "                                                                      " << std::endl;
  if (benchmark == true)
  {
    std::cout << " Success:                     " << final_success << " % " << std::endl;
  }
  if (final_nonZeroIndices.size() == 0)
  {
    std::cout << " Fitness Value:               " << final_fitness_value << std::endl;
  }
  else
  {
    std::cout << " Evaluations:                 " << final_evaluations << std::endl;
  }
  std::cout << " ==================================================================== " << std::endl;
  std::cout << "                                                                      " << std::endl;

  // Output information
  std::cout << " =========================== Improvement ============================ " << std::endl;
  std::cout << "                                                                      " << std::endl;
  if (benchmark == true)
  {
    std::cout << " Improvement Success:         " << final_success - default_success << " % more solved" << std::endl;
  }
  if (final_nonZeroIndices.size() == 0)
  {
    std::cout << " Improvement Fitness Value:   " << default_fitness_value / final_fitness_value << " times better" << std::endl;
  }
  else
  {
    if (default_success > 0.0)
    {
      std::cout << " Speedup Evaluations:         " << default_evaluations / final_evaluations << std::endl;
    }
  }
  std::cout << " ==================================================================== " << std::endl;
  std::cout << "                                                                      " << std::endl;
  std::cout << " The adaptive parameters are saved in " << file_name << std::endl;
  std::cout << "                                                        " << std::endl;
  std::cout << " =============================== DONE =============================== " << std::endl;

  // =============================== End Final Evaluations =================================
}

arma::mat pass::parameter_evaluate(pass::optimiser &optimiser, const pass::problem &problem)
{
  arma::mat runtimes(pass::parameter_setting_number_of_runs, 2);

  for (arma::uword r = 0; r < pass::parameter_setting_number_of_runs; ++r)
  {
    pass::optimise_result results = optimiser.optimise(problem);

    if (results.solved())
    {
      runtimes(r, 0) = results.fitness_value;
      runtimes(r, 1) = results.evaluations;
    }
    else
    {
      runtimes(r, 0) = results.fitness_value;
      runtimes(r, 1) = 0;
    }
  }

  return runtimes;
}

arma::uword pass::compare_segments(const arma::mat first_segment_runtimes, const arma::mat second_segment_runtimes)
{
  arma::uword winner = arma::datum::nan;

  assert(first_segment_runtimes.n_elem == second_segment_runtimes.n_elem &&
         "`first_segment_runtimes` and `second_segment_runtimes` must have the "
         "same dimension");

  // first segment
  arma::vec first_segment_nonZeroIndices = arma::nonzeros(first_segment_runtimes.col(1));
  double first_segment_success = static_cast<double>(first_segment_nonZeroIndices.size()) / static_cast<double>(pass::parameter_setting_number_of_runs) * 100.00;
  double first_segment_evaluations = std::numeric_limits<double>::max();
  double first_segment_fitness_value = std::numeric_limits<double>::max();

  if (first_segment_nonZeroIndices.size() == 0)
  {
    first_segment_fitness_value = arma::median(first_segment_runtimes.col(0));
  }
  else
  {
    first_segment_evaluations = arma::median(first_segment_nonZeroIndices);
  }

  // second segment
  arma::vec second_segment_nonZeroIndices = arma::nonzeros(second_segment_runtimes.col(1));
  arma::uword second_segment_success = static_cast<double>(second_segment_nonZeroIndices.size()) / static_cast<double>(pass::parameter_setting_number_of_runs) * 100.00;
  double second_segment_evaluations = std::numeric_limits<double>::max();
  double second_segment_fitness_value = std::numeric_limits<double>::max();

  if (second_segment_nonZeroIndices.size() == 0)
  {
    second_segment_fitness_value = arma::median(second_segment_runtimes.col(0));
  }
  else
  {
    second_segment_evaluations = arma::median(second_segment_nonZeroIndices);
  }

  // Determine Score
  double score_first;
  double score_second;

  if (first_segment_success != second_segment_success && first_segment_evaluations != second_segment_evaluations)
  {
    score_first = std::abs(first_segment_evaluations - second_segment_evaluations) / first_segment_evaluations + first_segment_success / std::abs(first_segment_success - second_segment_success);
    score_second = std::abs(second_segment_evaluations - first_segment_evaluations) / second_segment_evaluations + second_segment_success / std::abs(second_segment_success - first_segment_success);
  }
  else if (first_segment_success == second_segment_success)
  {
    score_first = std::abs(first_segment_evaluations - second_segment_evaluations) / first_segment_evaluations;
    score_second = std::abs(second_segment_evaluations - first_segment_evaluations) / second_segment_evaluations;
  }
  else
  {
    score_first = std::abs(first_segment_success - second_segment_success) / first_segment_success;
    score_second = std::abs(second_segment_success - first_segment_success) / second_segment_success;
  }

  // Determine Winner
  if (first_segment_nonZeroIndices.size() == 0 && second_segment_nonZeroIndices.size() != 0)
  {
    winner = 2;
  }
  else if (first_segment_nonZeroIndices.size() != 0 && second_segment_nonZeroIndices.size() == 0)
  {
    winner = 1;
  }
  else if (first_segment_nonZeroIndices.size() == 0 && second_segment_nonZeroIndices.size() == 0)
  {
    if (first_segment_fitness_value <= second_segment_fitness_value)
    {
      winner = 1;
    }
    else
    {
      winner = 2;
    }
  }
  else if (first_segment_nonZeroIndices.size() != 0 && second_segment_nonZeroIndices.size() != 0)
  {
    if (score_first > score_second)
    {
      winner = 1;
    }
    else
    {
      winner = 2;
    }
  }
  assert((winner == 1 || winner == 2) && "something went wrong with the 'winner' variable. It should be 1 or 2");

  return winner;
}
