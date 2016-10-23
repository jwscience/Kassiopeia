#include "variable.hpp"
#include "observer.hpp"






Observer *onevalobserver(){
	Variable *z = new SingleVariable("z", true, true);
	
	Observer *obs = new Observer(std::vector<Variable*>({z}), 4);
	// normal transitions
	obs->add_transition(0, 1, IN, z);
	obs->add_transition(1, 2, OUT, z);
	// failure transitions
	obs->add_transition(0, 3, OUT, z);
	obs->add_transition(1, 3, OUT, NULL);
	obs->add_transition(2, 3, OUT, z);
	
	return obs;
}






