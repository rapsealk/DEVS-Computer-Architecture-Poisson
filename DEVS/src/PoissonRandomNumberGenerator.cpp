#include "../include/PoissonRandomNumberGenerator.hpp"

int PoissonRandomNumberGenerator::Generate() {
	return poisson(engine);
}