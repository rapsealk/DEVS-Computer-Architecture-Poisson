#ifndef __POISSON_RANDOM_NUMBER_GENERATOR_HPP__
#define __POISSON_RANDOM_NUMBER_GENERATOR_HPP__

#include <random>

class PoissonRandomNumberGenerator {
private:
	std::random_device seed_gen;
	std::default_random_engine engine;
	std::poisson_distribution<int> poisson;
public:
	PoissonRandomNumberGenerator(double mean)
		: engine(seed_gen()), poisson(mean) { }
	int Generate();
};

#endif	// __POISSON_RANDOM_NUMBER_GENERATOR_HPP__