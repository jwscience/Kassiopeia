#include <vector>
#include "z3++.h"
#include "program.hpp"
#include "observer.hpp"
#include "noic3.hpp"
#include "analyzer.hpp"




// link together with the (program + observer + initial) you want to test
Program *testprogram();
Observer *testobserver();





int main(){
	Program *prog = testprogram();
	Observer *obs = testobserver();
	
	// test AST
	std::cout << "We have " << Expression::loc() << " lines of code\n";
	prog->get_init()->printsubprogram();
	std::vector<Function*> list = prog->get_functions();
	
	for(auto i=list.begin(); i!=list.end(); i++){
		(*i)->printsubprogram();
	}
	
	for(const auto v : SingleVariable::get_variables()){
		std::cout << v->getname() << " |-> " << v->getid() << "\n";
	}
	// */
	
	
	context ctxt;
	
	Analyzer ayz(prog, obs, ctxt);
	ayz.prepare();
	
	std::cout << "Starting analysis\n";
	bool result = ayz.analyze();
	if(result){
		std::cout << "System safe\n";
	}else{
		std::cout << "System unsafe\n";
	}
	// */
	
	
	delete prog;
	delete obs;
	
	return 0;
}








