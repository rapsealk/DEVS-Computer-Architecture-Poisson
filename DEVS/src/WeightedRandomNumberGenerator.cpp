#include "../include/WeightedRandomNumberGenerator.hpp"

int WeightedRandomNumberGenerator::Generate() {
	return weighted(engine);
}
