#include "../include/WeightedRoundRobinGenerator.hpp"

int WeightedRoundRobinGenerator::Generate() {
	if (weights.empty()) return 0;
	int best = 0;
	for (int i = 0; i < (int) weights.size(); i++) {
		current[i] += weights[i];
		if (current[i] > current[best]) best = i;
	}
	current[best] -= total;
	return best;
}
