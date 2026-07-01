#ifndef __WEIGHTED_RANDOM_NUMBER_GENERATOR_HPP__
#define __WEIGHTED_RANDOM_NUMBER_GENERATOR_HPP__

#include <random>
#include <initializer_list>

#include "WeightedSelector.hpp"

// Picks an index in [0, N) with probability weight[i] / sum(weights).
class WeightedRandomNumberGenerator : public WeightedSelector {
private:
	std::random_device seed_gen;
	std::default_random_engine engine;
	std::discrete_distribution<int> weighted;
public:
	WeightedRandomNumberGenerator(std::initializer_list<double> weights)
		: engine(seed_gen()), weighted(weights) { }
	int Generate() override;
};

#endif	// __WEIGHTED_RANDOM_NUMBER_GENERATOR_HPP__
