#ifndef __WEIGHTED_RANDOM_NUMBER_GENERATOR_HPP__
#define __WEIGHTED_RANDOM_NUMBER_GENERATOR_HPP__

#include <random>
#include <initializer_list>

// Picks an index in [0, N) with probability weight[i] / sum(weights).
class WeightedRandomNumberGenerator {
private:
	std::random_device seed_gen;
	std::default_random_engine engine;
	std::discrete_distribution<int> weighted;
public:
	WeightedRandomNumberGenerator(std::initializer_list<double> weights)
		: engine(seed_gen()), weighted(weights) { }
	int Generate();
};

#endif	// __WEIGHTED_RANDOM_NUMBER_GENERATOR_HPP__
