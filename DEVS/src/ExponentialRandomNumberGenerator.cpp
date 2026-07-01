#include "../include/ExponentialRandomNumberGenerator.hpp"

double ExponentialRandomNumberGenerator::Generate() {
	return exponential(engine);
}
