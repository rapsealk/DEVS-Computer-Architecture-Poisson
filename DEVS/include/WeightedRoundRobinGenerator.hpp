#ifndef __WEIGHTED_ROUND_ROBIN_GENERATOR_HPP__
#define __WEIGHTED_ROUND_ROBIN_GENERATOR_HPP__

#include <vector>
#include <initializer_list>

#include "WeightedSelector.hpp"

// Deterministic smooth weighted round-robin (the nginx algorithm):
// over each full cycle every server is chosen exactly weight[i] times,
// interleaved rather than in bursts.
class WeightedRoundRobinGenerator : public WeightedSelector {
private:
	std::vector<double> weights;
	std::vector<double> current;
	double total;
public:
	WeightedRoundRobinGenerator(std::initializer_list<double> w)
		: weights(w), current(w.size(), 0.0), total(0.0) {
		for (double x : weights) total += x;
	}
	int Generate() override;
};

#endif	// __WEIGHTED_ROUND_ROBIN_GENERATOR_HPP__
