#include <vector>
#include <iostream>
#include "variable.hpp"
#include "observer.hpp"




LinearisationEvent::LinearisationEvent(LinearisationType type, Variable *var):
	LinearisationEvent(type, var, NULL, NULL)
{
}

LinearisationEvent::LinearisationEvent(LinearisationType type, Variable *var, Variable *cmp1, Variable *cmp2):
	type(type),
	var(var),
	cmp1(cmp1),
	cmp2(cmp2)
{
}

LinearisationType LinearisationEvent::gettype(){
	return type;
}

Variable *LinearisationEvent::getvar(){
	return var;
}

Variable *LinearisationEvent::getcmp1(){
	return cmp1;
}

Variable *LinearisationEvent::getcmp2(){
	return cmp2;
}

void LinearisationEvent::print(){
	std::cout << "***";
	if(type == IN_PCMP || type == IN_PACMP || type == OUT_PCMP || type == OUT_PACMP){
		std::cout << "IF (" << cmp1->getname() << " ";
		if(type == IN_PACMP || type == OUT_PACMP){
			std::cout << "===";
		}else{
			std::cout << "==";
		}
		std::cout << " " << cmp2->getname() << ") ";
	}
	
	if(type == IN || type == IN_PCMP || type == IN_PACMP){
			std::cout << "in(";
		}else if(type == OUT || type == OUT_PCMP || type == OUT_PACMP){
			std::cout << "out(";
		}
		if(var != NULL){
			std::cout << var->getname() << ".data";
		}else{
			std::cout << "/";
		}
		std::cout << ")";
	
	std::cout << "***";
}





Observer::Observer(std::vector<Variable*> variables, int states):
	variables(variables),
	intransitions(),
	outtransitions()
{
	intransitions.reserve(states);
	outtransitions.reserve(states);
	int v = variables.size();
	for(int i=0; i<states; i++){
		std::vector<int> list;
		list.reserve(v+1);
		for(int j=0; j<=v; j++){
			list.push_back(i);
		}
		intransitions.push_back(list);
		outtransitions.push_back(list);
	}
}

Observer::~Observer(){
	for(const auto z : variables){
		delete z;
	}
}

void Observer::add_transition(int source, int target, LinearisationType eventtype, Variable *var){
	int varid = -1;
	if(var == NULL){
		varid = variables.size();
	}else{
		varid = var->getid();
	}
	
	if(eventtype == IN || eventtype == IN_PCMP || eventtype == IN_PACMP){
		intransitions[source][varid] = target;
	}else if(eventtype == OUT || eventtype == OUT_PCMP || eventtype == OUT_PACMP){
		outtransitions[source][varid] = target;
	}else{
		std::cout << "OMGWTFAYFKM\n";
	}
}

int Observer::transition(int source, LinearisationEvent *event, Variable *var){
	LinearisationType type = event->gettype();
	int varid = -1;
	if(var == NULL){
		varid = variables.size();
	}else{
		varid = var->getid();
	}
	
	if(type == IN || type == IN_PCMP || type == IN_PACMP){
		return intransitions[source][varid];
	}else if(type == OUT || type == OUT_PCMP || type == OUT_PACMP){
		return outtransitions[source][varid];
	}else{
		std::cout << "OMGWTFAYFKMIdonteven\n";
		return -1;
	}
}

int Observer::count_states(){
	return intransitions.size();
}



