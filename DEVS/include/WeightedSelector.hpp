#ifndef __WEIGHTED_SELECTOR_HPP__
#define __WEIGHTED_SELECTOR_HPP__

// Strategy interface: chooses a server index for each job.
class WeightedSelector {
public:
	virtual ~WeightedSelector() {}
	virtual int Generate() = 0;
};

#endif	// __WEIGHTED_SELECTOR_HPP__
