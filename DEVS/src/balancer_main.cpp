#include <string>

#include "../kernel/include/Log.hpp"
#include "../kernel/include/Entstr.hpp"

#include "../include/Generator.hpp"
#include "../include/Transducer.hpp"
#include "../include/Process.hpp"
#include "../include/Balancer.hpp"

int main(int argc, char** argv)
{
	// Strategy: "rr" selects weighted round-robin, otherwise weighted-random.
	Balancer::Strategy strategy = Balancer::Strategy::WeightedRandom;
	if (argc > 1 && std::string(argv[1]) == "rr")
		strategy = Balancer::Strategy::WeightedRoundRobin;

	EntStr *efp = new EntStr("ef-p");

	// Weighted dispatcher fanning out to three servers (weights 5:3:2 = 50% / 30% / 20%).
	efp->AddItem(new Balancer("Balancer", { 5.0, 3.0, 2.0 }, strategy));
	efp->AddItem(new Process("Process-0"));
	efp->AddItem(new Process("Process-1"));
	efp->AddItem(new Process("Process-2"));
	efp->AddItem(new Digraph("ef"));

	// Arrivals (from ef) -> balancer -> one of the three servers.
	efp->AddCouple("ef", "Balancer", "OUT", "in");
	efp->AddCouple("Balancer", "Process-0", "out0", "in");
	efp->AddCouple("Balancer", "Process-1", "out1", "in");
	efp->AddCouple("Balancer", "Process-2", "out2", "in");

	// Completions from every server -> ef -> transducer "solved".
	efp->AddCouple("Process-0", "ef", "out", "IN");
	efp->AddCouple("Process-1", "ef", "out", "IN");
	efp->AddCouple("Process-2", "ef", "out", "IN");

	efp->SetCurrentItem("ef");
	efp->AddItem(new Generator("genr"));
	efp->AddItem(new Transducer("transd"));
	efp->AddCouple("ef", "transd", "IN", "solved");
	efp->AddCouple("transd", "genr", "out", "stop");

	efp->AddCouple("genr", "ef", "out", "OUT");
	efp->AddCouple("genr", "transd", "out", "arriv");

	efp->Restart();

	return 0;
}
