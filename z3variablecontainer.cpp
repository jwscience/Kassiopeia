#include <string>
#include <vector>
#include "z3++.h"
#include "variable.hpp"
#include "z3variablecontainer.hpp"


using namespace z3;





Z3VariableContainer1D::Z3VariableContainer1D(context &ctxt, int size, std::string basename, std::vector<expr> &globallist, std::vector<Variable*> *varlist){
	variables.reserve(size);
	ids.reserve(size);
	int varc = -1;
	if(varlist != NULL) varc = varlist->size();
	
	int id = globallist.size();
	for(int i=0; i<size; i++){
		std::string name = std::string(basename);
		if(varlist == NULL){
			name.append("(");
			name.append(std::to_string(i));
			name.append(")");
		}else{
			name.append("(");
			if(i >= varc){
				name.append(std::string((*varlist)[i-varc]->getname()));
				name.append(".next");
			}else{
				name.append(std::string((*varlist)[i]->getname()));
			}
			name.append(")");
		}
		expr v = ctxt.bool_const(name.c_str());
		
		variables.push_back(v);
		ids.push_back(id);
		globallist.push_back(v);
		id++;
	}
}

int Z3VariableContainer1D::size(){
	return variables.size();
}

expr Z3VariableContainer1D::operator() (unsigned int index){
	return variables[index];
}

int Z3VariableContainer1D::getid(unsigned int index){
	return ids[index];
}





Z3VariableContainer2D::Z3VariableContainer2D(context &ctxt, int size, std::string basename, bool symmetric, std::vector<expr> &globallist, std::vector<Variable*> *varlist){
	variables.reserve(size);
	ids.reserve(size);
	int varc = -1;
	if(varlist != NULL) varc = varlist->size();
	
	int id = globallist.size();
	for(int i=0; i<size; i++){
		variables.push_back(std::vector<expr>());
		ids.push_back(std::vector<int>());
		variables[i].reserve(size);
		ids[i].reserve(size);
		for(int j=0; j<size; j++){
			if(symmetric && i > j){
				variables[i].push_back(variables[j][i]);
				ids[i].push_back(ids[j][i]);
			}else{
				std::string name = std::string(basename);
				if(varlist == NULL){
					name.append("(");
					name.append(std::to_string(i));
					name.append(",");
					name.append(std::to_string(j));
					name.append(")");
				}else{
					name.append("(");
					if(i >= varc){
						name.append(std::string((*varlist)[i-varc]->getname()));
						name.append(".next");
					}else{
						name.append(std::string((*varlist)[i]->getname()));
					}
					name.append(",");
					if(j >= varc){
						name.append(std::string((*varlist)[j-varc]->getname()));
						name.append(".next");
					}else{
						name.append(std::string((*varlist)[j]->getname()));
					}
					name.append(")");
				}
				expr v = ctxt.bool_const(name.c_str());
				
				variables[i].push_back(v);
				ids[i].push_back(id);
				globallist.push_back(v);
				id++;
			}
		}
	}
}

int Z3VariableContainer2D::size(){
	return variables.size();
}

expr Z3VariableContainer2D::operator() (unsigned int index1, unsigned int index2){
	return variables[index1][index2];
}

int Z3VariableContainer2D::getid(unsigned int index1, unsigned int index2){
	return ids[index1][index2];
}








