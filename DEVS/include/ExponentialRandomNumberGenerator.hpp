#ifndef __EXPONENTIAL_RANDOM_NUMBER_GENERATOR_HPP__
#define __EXPONENTIAL_RANDOM_NUMBER_GENERATOR_HPP__

#include <random>

// Inter-arrival times of a Poisson process are exponentially distributed.
class ExponentialRandomNumberGenerator {
private:
	std::random_device seed_gen;
	std::default_random_engine engine;
	std::exponential_distribution<double> exponential;
public:
	// mean = expected inter-arrival time; the rate is lambda = 1 / mean.
	ExponentialRandomNumberGenerator(double mean)
		: engine(seed_gen()), exponential(1.0 / mean) { }
	double Generate();
};

#endif	// __EXPONENTIAL_RANDOM_NUMBER_GENERATOR_HPP__
