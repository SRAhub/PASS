#include "pass_bits/analyser/openmp.hpp"
#include "pass_bits/problem/space_mission/gtoc1.hpp"
#include "pass_bits/problem/optimisation_benchmark/de_jong_function.hpp"
#include "pass_bits/helper/evaluation_time_stall.hpp"
#include "pass_bits/optimiser/parallel_swarm_search.hpp"
#include "pass_bits/optimiser/particle_swarm_optimisation.hpp"
#include "pass_bits/analyser/problem_evaluation_time.hpp"
#include "pass_bits/helper/regression.hpp"
#include "pass_bits/helper/random.hpp"
#include <unistd.h>

bool pass::enable_openmp(const pass::problem &problem)
{
  // Output information
  std::cout << " =========================== Start openMP Analyse ========================= " << std::endl;
  std::cout << "                                                                            " << std::endl;
  std::cout << " Your Problem:                " << problem.name << std::endl;
  std::cout << " Dimension:                   " << problem.dimension() << std::endl;
  std::cout << " Number Threads:              " << pass::number_of_threads() << std::endl;
  std::cout << " Maximum Speedup:             " << pass::number_of_threads() << std::endl;
  std::cout << "                                                                            " << std::endl;

  std::cout << " ============================= Start Evaluation =========================== " << std::endl;

  double your_time = pass::problem_evaluation_time(problem);

  std::cout << "                                                                            " << std::endl;
  std::cout << " Evaluation time: " << your_time * 1e-6 << " milliseconds." << std::endl;
  std::cout << "                                                                            " << std::endl;
  std::cout << " ============================= End Evaluation  ============================ " << std::endl;
  std::cout << "                                                                            " << std::endl;

  arma::mat summary;
  arma::rowvec model;

  // Check if the file exists
  bool ok = model.load("./openmp_model.pass");

  if (ok == false)
  {
    std::cout << " - Model does not exist                                                     " << std::endl;
    std::cout << "                                                                            " << std::endl;
    // Start training the data
    summary = train(30);
    model = build_model(summary);
  }
  else
  {
    std::cout << "                                                                            " << std::endl;
    std::cout << " - Model exists                                                             " << std::endl;
    std::cout << "                                                                            " << std::endl;
  }

  std::cout << " ========================= Start SpeedUp Prediction ======================= " << std::endl;

  // Generating a regression object
  pass::regression r;

  if (model.n_elem == 3)
  {
    double predict_linear = r.predict_linear(your_time, model);

    if (predict_linear > 0.9 * pass::number_of_threads())
    {
      predict_linear = 0.9 * pass::number_of_threads();
    }
    std::cout << "                                                                            " << std::endl;
    std::cout << " Your speedUp would be approximately: " << predict_linear << std::endl;

    std::cout << "                                                                            " << std::endl;
    std::cout << " ========================== Done SpeedUp Prediction ======================= " << std::endl;

    if (predict_linear > pass::number_of_threads() / 2) // efficienty is more than 50 %
    {
      std::cout << "                                                                            " << std::endl;
      std::cout << " You should activate openMP!                                                " << std::endl;
      std::cout << "                                                                            " << std::endl;
      std::cout << " =========================  Done openMP Analyse  ========================== " << std::endl;
      return true;
    }
    if (predict_linear < 1) // is efficienty is more than 50 %
    {
      std::cout << "                                                                            " << std::endl;
      std::cout << " You should NOT activate openMP!                                            " << std::endl;
      std::cout << "                                                                            " << std::endl;
      std::cout << " =========================  Done openMP Analyse  ========================== " << std::endl;
      return false;
    }
    if (predict_linear > 1 && predict_linear < pass::number_of_threads() / 2) // efficienty is less than 50 %
    {
      std::cout << "                                                                            " << std::endl;
      std::cout << " You should decide yourself if to activate openMP or not!                   " << std::endl;
      std::cout << "                                                                            " << std::endl;
    }
  }
  else
  {
    double predict_poly = r.predict_poly(your_time, model);

    if (predict_poly > 0.9 * pass::number_of_threads())
    {
      predict_poly = 0.9 * pass::number_of_threads();
    }
    std::cout << "                                                                            " << std::endl;
    std::cout << " Your speedUp would be approximately: " << predict_poly << std::endl;
    std::cout << "                                                                            " << std::endl;
    std::cout << " ========================== Done SpeedUp Prediction ======================= " << std::endl;

    if (predict_poly > pass::number_of_threads() / 2) // is efficienty is more than 50 %
    {
      std::cout << "                                                                            " << std::endl;
      std::cout << " You should activate openMP!                                                " << std::endl;
      std::cout << "                                                                            " << std::endl;
      std::cout << " =========================  Done openMP Analyse  ========================== " << std::endl;
      return true;
    }
    if (predict_poly < 1) // is efficienty is more than 50 %
    {
      std::cout << "                                                                            " << std::endl;
      std::cout << " You should NOT activate openMP!                                            " << std::endl;
      std::cout << "                                                                            " << std::endl;
      std::cout << " =========================  Done openMP Analyse  ========================== " << std::endl;
      return false;
    }
    if (predict_poly > 1 && predict_poly < pass::number_of_threads() / 2) // is efficienty is more than 50 %
    {
      std::cout << "                                                                            " << std::endl;
      std::cout << " You should decide yourself if to activate openMP or not!                   " << std::endl;
      std::cout << "                                                                            " << std::endl;
    }
  }

  std::cout << "                                                                            " << std::endl;
  std::cout << " =========================  Done openMP Analyse  ========================== " << std::endl;

  return true;
}

arma::mat pass::train(const int &examples)
{
  // define the maximum of runs
  arma::uword alg_runs = 2;

  // Array including all alg runtime, we want to test
  //std::array<int, 30> repetitions = {1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 14, 15, 17, 20, 25, 28, 30, 33, 36,
  //                                   40, 45, 50, 60, 70, 80, 100, 120, 140, 160};

  arma::rowvec repetitions = pass::integers_uniform_in_range(50, 200, examples);

  // Output information
  std::cout << " ============================= Start Trainining =========================== " << std::endl;
  std::cout << "                                                                            " << std::endl;

  arma::vec serial(alg_runs);
  arma::vec parallel(alg_runs);

  arma::mat summary(2, repetitions.size());

  std::srand(time(0));
  int count = 0;

  pass::particle_swarm_optimisation algorithm_serial;
  algorithm_serial.maximal_duration = std::chrono::seconds(5);

  pass::parallel_swarm_search algorithm_parallel;
  algorithm_serial.maximal_duration = std::chrono::seconds(5);

  for (arma::uword repetition : repetitions)
  {
    std::cout << "Repetition " << repetition << std::endl;

    // Problem initialisation
    pass::gtoc1 test_problem;
    pass::evaluation_time_stall simulated_problem(test_problem);
    simulated_problem.repetitions = repetition;

    double ev_time = pass::problem_evaluation_time(simulated_problem);
    summary(0, count) = ev_time;

    // Do the evaluation for serial and parallel for all the evaluations values
    for (arma::uword serial_run = 0; serial_run < alg_runs; ++serial_run)
    {
      pass::optimise_result serial_alg = algorithm_serial.optimise(simulated_problem);
      serial(serial_run) = serial_alg.evaluations;
      usleep(1000000);
    }

    for (arma::uword parallel_run = 0; parallel_run < alg_runs; ++parallel_run)
    {
      optimise_result parallel_alg = algorithm_parallel.optimise(simulated_problem);
      parallel(parallel_run) = parallel_alg.evaluations;
      usleep(1000000);
    }

    summary(1, count) = arma::mean(parallel) / arma::mean(serial);

    std::cout << "Summary: \n"
              << summary.col(count) << std::endl;

    count++;

    // load bar

    double temp_count = static_cast<double>(examples) / static_cast<double>(count);
    std::cout << " \r " << 100.0 / temp_count << " % completed." << std::flush;
  }

  std::cout << std::endl
            << std::endl
            << " Training completed successfully.\n"
            << std::flush;
  std::cout << "                                                                            " << std::endl;
  std::cout << " ===========================  End Training  =============================== " << std::endl;
  std::cout << "                                                                            " << std::endl;

  return summary;
}

arma::rowvec pass::build_model(const arma::mat &training_points)
{
  // File to save the model
  std::string file_name;
  file_name = "openmp_model.pass";

  // Generating a regression object
  pass::regression r;

  // Getting the data for the model
  arma::rowvec x_values = training_points.row(0);
  arma::rowvec y_values = training_points.row(1);

  // Output information
  std::cout << " =========================== Start Building Models ======================== " << std::endl;
  std::cout << "                                                                            " << std::endl;
  std::cout << " Building linear model for the training data.                               " << std::endl;
  std::cout << "                                                                            " << std::endl;

  // Generating the linear model

  arma::rowvec linear_model = r.linear_model(x_values, y_values);

  if (linear_model(2) >= 0.9)
  {
    std::cout << " Finished building linear model.                                            " << std::endl;
    std::cout << "                                                                            " << std::endl;
    std::cout << " Linear Model is suitable.                                                  " << std::endl;
    std::cout << "                                                                            " << std::endl;
    std::cout << " ========================= Done Building Models  ========================== " << std::endl;
    std::cout << "                                                                            " << std::endl;

    linear_model.save("./" + file_name, arma::raw_ascii);

    return linear_model;
  }

  std::cout << " Finished building linear model.                                            " << std::endl;
  std::cout << "                                                                            " << std::endl;
  std::cout << " Linear Model is NOT suitable.                                              " << std::endl;
  std::cout << "                                                                            " << std::endl;
  std::cout << " Building polynomial model for the training data.                           " << std::endl;
  std::cout << "                                                                            " << std::endl;

  arma::rowvec poly_model = r.poly_model(x_values, y_values, 3);
  std::cout << " ========================= Done Building Models  ========================== " << std::endl;
  std::cout << "                                                                            " << std::endl;

  poly_model.save("./" + file_name, arma::raw_ascii);

  return poly_model;
}
