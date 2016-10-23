#include "variable.hpp"
#include "observer.hpp"






Observer *stackobserver(){
	Variable *z1 = new SingleVariable("z1", true, true);
	Variable *z2 = new SingleVariable("z2", true, true);
	
	Observer *obs = new Observer(std::vector<Variable*>({z1, z2}), 6);
	// normal transitions
	obs->add_transition(0, 1, IN, z1);
	obs->add_transition(1, 2, IN, z2);
	obs->add_transition(1, 4, OUT, z1);
	obs->add_transition(2, 3, OUT, z2);
	obs->add_transition(3, 4, OUT, z1);
	// failure transitions
	obs->add_transition(0, 5, OUT, z1);
	obs->add_transition(1, 5, OUT, NULL);
	obs->add_transition(2, 5, OUT, z1);
	obs->add_transition(2, 5, OUT, NULL);
	obs->add_transition(3, 5, OUT, NULL);
	obs->add_transition(4, 5, OUT, z1);
	
	return obs;
}






