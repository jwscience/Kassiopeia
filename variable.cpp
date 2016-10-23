#include <string>
#include <vector>
#include "variable.hpp"




Variable::Variable(std::string name, bool global, bool observer):
	name(name),
	global(global),
	observer(observer)
{
}

Variable::~Variable(){
}

std::string Variable::getname(){
	return name;
}

int Variable::getid(){
	return id;
}

void Variable::setid(int id){
	this->id = id;
}

bool Variable::isglobal(){
	return global;
}

bool Variable::isobserver(){
	return observer;
}

TwinVariable *Variable::get_twin(int number){
	return NULL;
}

SingleVariable *Variable::get_parent(){
	return NULL;
}





int SingleVariable::obscount = 0;
std::vector<Variable*> SingleVariable::variables;

SingleVariable::SingleVariable(std::string name, bool global, bool observer):
	Variable(name, global, observer),
	twin1(NULL),
	twin2(NULL)
{
	id = variables.size();
	variables.push_back(this);
	if(observer){
		obscount++;
		// make the observer variables have the lowest ids.
		// because we have lists of only observer variables and the ids have to match there
		// AND the lists have to stay ordered by id
		int c = obscount - 1;
		
		Variable *other = variables[c];
		variables[c] = this;
		variables[variables.size()-1] = other;
		
		int swap = other->getid();
		other->setid(id);
		id = swap;
	}
	deliver_twins();
}

SingleVariable::~SingleVariable(){
}

std::vector<Variable*> SingleVariable::get_variables(){
	return variables;
}

std::vector<Variable*> SingleVariable::get_obsvariables(){
	std::vector<Variable*> result;
	result.reserve(obscount);
	for(int i=0; i<obscount; i++){
		result.push_back(variables[i]);
	}
	return result;
}

std::vector<Variable*> SingleVariable::get_nonobsvariables(){
	int total = variables.size();
	int count = total - obscount;
	std::vector<Variable*> result;
	result.reserve(count);
	for(int i=obscount; i<total; i++){
		result.push_back(variables[i]);
	}
	return result;
}

std::vector<Variable*> SingleVariable::get_twins(int number){
	std::vector<Variable*> result;
	result.reserve(variables.size());
	for(const auto v : variables){
		result.push_back(v->get_twin(number));
	}
	return result;
}

int SingleVariable::getnextid(){
	return id + variables.size();
}

void SingleVariable::deliver_twins(){
	if(global){
		twin1 = new TwinVariable(name, global, observer, this);
		twin2 = twin1;
	}else{
		twin1 = new TwinVariable(name+"$", global, observer, this);
		twin2 = new TwinVariable(name+"#", global, observer, this);
	}
}

TwinVariable *SingleVariable::get_twin(int number){
	if(number == 1) return twin1;
	if(number == 2) return twin2;
	return NULL;
}






int TwinVariable::obscount = 0;
std::vector<Variable*> TwinVariable::variables;

TwinVariable::TwinVariable(std::string name, bool global, bool observer, SingleVariable *parent):
	Variable(name, global, observer),
	parent(parent)
{
	id = variables.size();
	variables.push_back(this);
	if(observer){
		obscount++;
		// make the observer variables have the lowest ids.
		// because we have lists of only observer variables and the ids have to match there
		// AND the lists have to stay ordered by id
		int c = obscount - 1;
		
		Variable *other = variables[c];
		variables[c] = this;
		variables[variables.size()-1] = other;
		
		int swap = other->getid();
		other->setid(id);
		id = swap;
	}
}

TwinVariable::~TwinVariable(){
}

std::vector<Variable*> TwinVariable::get_variables(){
	return variables;
}

std::vector<Variable*> TwinVariable::get_obsvariables(){
	std::vector<Variable*> result;
	result.reserve(obscount);
	for(int i=0; i<obscount; i++){
		result.push_back(variables[i]);
	}
	return result;
}

std::vector<Variable*> TwinVariable::get_nonobsvariables(){
	int total = variables.size();
	int count = total - obscount;
	std::vector<Variable*> result;
	result.reserve(count);
	for(int i=obscount; i<total; i++){
		result.push_back(variables[i]);
	}
	return result;
}

int TwinVariable::getnextid(){
	return id + variables.size();
}

SingleVariable *TwinVariable::get_parent(){
	return parent;
}

