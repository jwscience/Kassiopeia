#include <string>
#include <iostream>
#include <vector>
#include "variable.hpp"
#include "ast.hpp"
#include "program.hpp"





Function::Function(std::string name, bool has_input, Sequence *seq):
	name(name),
	input_present(has_input),
	entrypoint(seq->getfirst()){
	seq->setnext(NULL);
	delete seq;
}

Expression *Function::get_entrypoint(){
	return entrypoint;
}

bool Function::has_input(){
	return input_present;
}

void Function::printsubprogram(){
	std::cout << "Function " << name << "(";
	if(has_input()) std::cout << "_in_";
	std::cout << "){\n";
	entrypoint->printsubprogram();
	std::cout << "}\n";
}




Program::Program(std::string name, std::vector<Variable*> variables, Function *init, std::vector<Function*> functions):
	name(name),
	variables(variables),
	init(init),
	functions(functions),
	code(Expression::get_expressions()){
}

Program::~Program(){
	for(auto i : variables){
		delete i;
	}
	delete init;
	for(auto i : functions){
		delete i;
	}
	for(auto i : code){
		delete i;
	}
}

Function *Program::get_init(){
	return init;
}

std::vector<Function*> Program::get_functions(){
	return functions;
}

std::vector<Expression*> Program::get_code(){
	return code;
}

