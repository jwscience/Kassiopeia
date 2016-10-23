#include <vector>
#include "z3++.h"
#include "noic3.hpp"


using namespace z3;




NextConverter::NextConverter(context &ctxt, std::vector<expr> old, std::vector<expr> next):
	ctxt(ctxt),
	old_variables(old),
	new_variables(next),
	old_z3ast(ast_vector_tpl<expr>(ctxt)),
	new_z3ast(ast_vector_tpl<expr>(ctxt)){
	for(auto i : old){
		old_z3ast.push_back(i);
	}
	for(auto i : next){
		new_z3ast.push_back(i);
	}
}

Clause NextConverter::convert(Clause &oldclause){
	return Clause(ctxt, new_variables, oldclause.get_dirac());
}

Frame NextConverter::convert(Frame &oldframe){
	Frame result(ctxt);
	std::vector<Clause> clauses = oldframe.get_clauses();
	for(auto i : clauses){
		Clause j = convert(i);
		result.add_clause(j);
	}
	return result;
}



Mediator::Mediator(context &ctxt, std::vector<expr> &single_variables, std::vector<expr> &double_variables):
	ctxt(ctxt),
	single_variables(single_variables),
	double_variables(double_variables),
	table1(),
	table2()
{
	int size = single_variables.size();
	table1.reserve(size);
	table2.reserve(size);
	for(int i=0; i<size; i++){
		table1.push_back(-1);
		table2.push_back(-1);
	}
	size = double_variables.size();
	rtable1.reserve(size);
	rtable2.reserve(size);
	for(int i=0; i<size; i++){
		rtable1.push_back(-1);
		rtable2.push_back(-1);
	}
}

void Mediator::make_entry(int s, int d1, int d2){
	table1[s] = d1;
	table2[s] = d2;
	rtable1[d1] = s;
	rtable2[d2] = s;
}

void Mediator::add(int singleid, int double1id, int double2id){
	make_entry(singleid, double1id, double2id);
}

void Mediator::add_id(Z3VariableContainer1D &single, Z3VariableContainer1D &double1, Z3VariableContainer1D &double2){
	int size = single.size();
	for(int i=0; i<size; i++){
		make_entry(single.getid(i), double1.getid(i), double2.getid(i));
	}
}

void Mediator::add_1dvar(Z3VariableContainer1D &single, Z3VariableContainer1D &double1, Z3VariableContainer1D &double2, std::vector<Variable*> &vars){
	for(const auto x : vars){
		int xid = x->getid();
		int xtwin1id = x->get_twin(1)->getid();
		int xtwin2id = x->get_twin(2)->getid();
		
		make_entry(single.getid(xid), double1.getid(xtwin1id), double2.getid(xtwin2id));
	}
}

void Mediator::add_2dvar(Z3VariableContainer2D &single, Z3VariableContainer2D &double1, Z3VariableContainer2D &double2, std::vector<Variable*> &vars){
	for(const auto x : vars){
		int xid = x->getid();
		int xtwin1id = x->get_twin(1)->getid();
		int xtwin2id = x->get_twin(2)->getid();
		for(const auto y : vars){
			int yid = y->getid();
			int ytwin1id = y->get_twin(1)->getid();
			int ytwin2id = y->get_twin(2)->getid();
			
			make_entry(single.getid(xid, yid), double1.getid(xtwin1id, ytwin1id), double2.getid(xtwin2id, ytwin2id));
		}
	}
}

void Mediator::add_2dnextvar(Z3VariableContainer2D &single, Z3VariableContainer2D &double1, Z3VariableContainer2D &double2, std::vector<Variable*> &vars){
	for(const auto x : vars){
		int xid = x->getid();
		int xnextid = x->getnextid();
		int xtwin1id = x->get_twin(1)->getid();
		int xtwin1nextid = x->get_twin(1)->getnextid();
		int xtwin2id = x->get_twin(2)->getid();
		int xtwin2nextid = x->get_twin(2)->getnextid();
		for(const auto y : vars){
			int yid = y->getid();
			int ynextid = y->getnextid();
			int ytwin1id = y->get_twin(1)->getid();
			int ytwin1nextid = y->get_twin(1)->getnextid();
			int ytwin2id = y->get_twin(2)->getid();
			int ytwin2nextid = y->get_twin(2)->getnextid();
			
			make_entry(single.getid(xid, yid), double1.getid(xtwin1id, ytwin1id), double2.getid(xtwin2id, ytwin2id));
			make_entry(single.getid(xid, ynextid), double1.getid(xtwin1id, ytwin1nextid), double2.getid(xtwin2id, ytwin2nextid));
			make_entry(single.getid(xnextid, yid), double1.getid(xtwin1nextid, ytwin1id), double2.getid(xtwin2nextid, ytwin2id));
			make_entry(single.getid(xnextid, ynextid), double1.getid(xtwin1nextid, ytwin1nextid), double2.getid(xtwin2nextid, ytwin2nextid));
		}
	}
}

void Mediator::check_complete(){
	int size1 = table1.size();
	for(int i=0; i<size1; i++){
		if(table1[i] == -1 || table2[i] == -1){
			std::cout << "You missed a spot there\n";
		}
	}
	int size2 = rtable1.size();
	for(int i=0; i<size2; i++){
		if(rtable1[i] == -1){
			for(int j=0; j<size1; j++){
				if(table1[j] == i){
					std::cout << "You missed a reverse spot there\n";
				}
			}
		}
		if(rtable2[i] == -1){
			for(int j=0; j<size1; j++){
				if(table2[j] == i){
					std::cout << "You missed a reverse spot there\n";
				}
			}
		}
	}
}

Clause Mediator::evolve(Clause &original, int type){
	std::vector<int> &contained = original.get_contained();
	Clause result(ctxt, double_variables.size());
	
	for(auto j : contained){
		int target = -1;
		if(type == 1) target = table1[j];
		if(type == 2) target = table2[j];
		expr lit = double_variables[target];
		result.set_variable(lit, target, original.get_literal(j));
	}
	return result;
}

Frame Mediator::evolve(Frame &original, int type){
	std::vector<Clause> &clauses = original.get_clauses();
	Frame result(ctxt);
	for(auto &clause : clauses){
		singleadd(result, clause, type);
	}
	return result;
}

Frame Mediator::mitosis(Frame &original){
	std::vector<Clause> &clauses = original.get_clauses();
	Frame result(ctxt);
	for(auto &clause : clauses){
		doubleadd(result, clause);
	}
	return result;
}

void Mediator::cytokinesis(Clause &original, Clause &result1, Clause &result2){
	std::vector<int> &contained = original.get_contained();
	for(auto j : contained){
		int target1 = rtable1[j];
		int target2 = rtable2[j];
		clausevar status = original.get_literal(j);
		if(target1 != -1){
			expr lit = single_variables[target1];
			result1.set_variable(lit, target1, status);
		}
		if(target2 != -1){
			expr lit = single_variables[target2];
			result2.set_variable(lit, target2, status);
		}
	}
}

void Mediator::singleadd(Frame &frame, Clause &clause, int type){
	std::vector<int> &contained = clause.get_contained();
	Clause c(ctxt, double_variables.size());
	
	for(auto j : contained){
		int target = -1;
		if(type == 1) target = table1[j];
		if(type == 2) target = table2[j];
		expr lit = double_variables[target];
		c.set_variable(lit, target, clause.get_literal(j));
	}
	frame.add_clause(c);
}

void Mediator::doubleadd(Frame &frame, Clause &clause){
	std::vector<int> &contained = clause.get_contained();
	Clause c1(ctxt, double_variables.size());
	Clause c2(ctxt, double_variables.size());
	
	for(auto j : contained){
		int target1 = table1[j];
		int target2 = table2[j];
		expr lit1 = double_variables[target1];
		expr lit2 = double_variables[target2];
		
		c1.set_variable(lit1, target1, clause.get_literal(j));
		c2.set_variable(lit2, target2, clause.get_literal(j));
	}
	frame.add_clause(c1, false);
	frame.add_clause(c2, false);
}





Clause::Clause(context &ctxt, std::vector<expr> &variables, std::vector<clausevar> stati):
	num_vars(variables.size()),
	dirac(),
	contained(),
	formula(ctxt, Z3_mk_false(ctxt)),
	coformula(ctxt, Z3_mk_true(ctxt)){
	dirac.reserve(num_vars);
	for(int i=0; i<num_vars; i++){
		clausevar &st = stati[i];
		dirac.push_back(st);
		if(st == CLAUSE_POSITIVE){
			formula = formula || variables[i];
			coformula = coformula && !variables[i];
			contained.push_back(i);
		}else if(st == CLAUSE_NEGATIVE){
			formula = formula || !variables[i];
			coformula = coformula && variables[i];
			contained.push_back(i);
		}
	}
}

Clause::Clause(context &ctxt, std::vector<expr> &variables, model &m):
	// the state contained in the model will become the coformula,
	// the negated state will become the formula / clause.
	num_vars(variables.size()),
	dirac(),
	contained(),
	formula(ctxt, Z3_mk_false(ctxt)),
	coformula(ctxt, Z3_mk_true(ctxt)){
	dirac.reserve(num_vars);
	for(int i=0; i<num_vars; i++){
		expr val = m.eval(variables[i]);
		Z3_lbool boolval = Z3_get_bool_value(ctxt, val);
		if(boolval == Z3_L_TRUE){
			dirac.push_back(CLAUSE_NEGATIVE);
			formula = formula || !variables[i];
			coformula = coformula && variables[i];
			contained.push_back(i);
		}else if(boolval == Z3_L_FALSE){
			dirac.push_back(CLAUSE_POSITIVE);
			formula = formula || variables[i];
			coformula = coformula && !variables[i];
			contained.push_back(i);
		}else{
			dirac.push_back(CLAUSE_DONTCARE);
		}
	}
}

Clause::Clause(context &ctxt, int size):
	num_vars(size),
	dirac(),
	contained(),
	formula(expr(ctxt, Z3_mk_false(ctxt))),
	coformula(expr(ctxt, Z3_mk_true(ctxt))){
	dirac.reserve(size);
	for(int i=0; i<num_vars; i++){
		dirac.push_back(CLAUSE_DONTCARE);
	}
}

clausevar Clause::get_literal(int id){
	return dirac[id];
}

void Clause::set_variable(expr var, int var_id, clausevar status){
	// NOTE: Only use this function to change a variable status from DONTCARE to something.
	// Doing otherwise might leave you with a destroyed vector.
	// Calls that do not change the status are allowed.
	if(dirac[var_id] == status) return;
	dirac[var_id] = status;
	if(status == CLAUSE_POSITIVE){
		formula = formula || var;
		coformula = coformula && !var;
		contained.push_back(var_id);
	}else if(status == CLAUSE_NEGATIVE){
		formula = formula || !var;
		coformula = coformula && var;
		contained.push_back(var_id);
	}
}

std::vector<clausevar> &Clause::get_dirac(){
	return dirac;
}

std::vector<int> &Clause::get_contained(){
	return contained;
}

expr Clause::get_formula(){
	return formula;
}

expr Clause::get_coformula(){
	return coformula;
}

bool Clause::contains(int literalid){
	return dirac[literalid] != CLAUSE_DONTCARE;
}

bool Clause::compare(Clause &other){
	// TODO: use <contained> for comparison
	// shift from 2*num_vars to 2*contained1 + 2*contained2
	std::vector<clausevar> &cmp = other.get_dirac();
	for(int i=0; i<num_vars; i++){
		if(dirac[i] != cmp[i]) return false;
	}
	return true;
}

Clause Clause::remove_literal(context &ctxt, std::vector<expr> &variables, int id){
	std::vector<clausevar> newvector = dirac;
	newvector[id] = CLAUSE_DONTCARE;
	return Clause(ctxt, variables, newvector);
}

Clause Clause::intersect(context &ctxt, std::vector<expr> &variables, Clause &other){
	int size = variables.size();
	Clause result(ctxt, size);
	for(int i=0; i<size; i++){
		clausevar v = dirac[i];
		if(v == other.get_literal(i)){
			result.set_variable(variables[i], i, v);
		}
	}
	return result;
}



Frame::Frame(context &ctxt):
	clauses(std::vector<Clause>()),
	formula(expr(ctxt, Z3_mk_true(ctxt))),
	notpropagated(std::vector<Clause>()),
	propagated_change(false){
}

bool Frame::contains_clause(Clause &clause){
	for(auto i : clauses){
		if(clause.compare(i)) return true;
	}
	return false;
}

void Frame::add_clause(Clause clause, bool propagate){
	if(contains_clause(clause)) return;
	clauses.push_back(clause);
	formula = formula && clause.get_formula();
	if(propagate){
		notpropagated.push_back(clause);
		propagated_change = true;
	}
}

std::vector<Clause> &Frame::get_clauses(){
	return clauses;
}

expr &Frame::get_formula(){
	return formula;
}

void Frame::fill_solver(solver &s){
	for(auto i : clauses){
		s.add(i.get_formula());
	}
}

bool Frame::has_propagation(){
	return propagated_change;
}

std::vector<Clause> &Frame::get_propagation(){
	propagated_change = false;
	return notpropagated;
}

void Frame::set_propagation(std::vector<Clause> newlist){
	notpropagated = newlist;
}




ProofObligation::ProofObligation(Clause ob, ProofObligation *back):
	obligation(ob),
	fulfilled(false),
	failed(false),
	twin(NULL),
	backtrack(back){
}

void ProofObligation::set_twin(ProofObligation *tw){
	twin = tw;
}

void ProofObligation::set_fulfilled(){
	fulfilled = true;
	if(twin != NULL) twin->fulfilled = true;
}

void ProofObligation::set_failed(){
	failed = true;
	if(twin != NULL){
		twin->twin = NULL;
	}else if(backtrack != NULL){
		backtrack->set_failed();
	}
}

bool ProofObligation::is_fulfilled(){
	return fulfilled;
}

bool ProofObligation::is_failed(){
	return failed;
}

Clause ProofObligation::get_clause(){
	return obligation;
}





NoIC3::NoIC3(context &ctxt, std::vector<expr> &seq_old_variables, std::vector<expr> &seq_new_variables, std::vector<expr> &int_old_variables, std::vector<expr> &int_new_variables, Mediator &mediator, Frame &initial, expr &transition, expr &inttransition, Frame &intconstraints, Frame &safety):
	ctxt(ctxt),
	seq_old_variables(seq_old_variables),
	seq_new_variables(seq_new_variables),
	int_old_variables(int_old_variables),
	int_new_variables(int_new_variables),
	seqconverter(ctxt, seq_old_variables, seq_new_variables),
	intconverter(ctxt, int_old_variables, int_new_variables),
	mediator(mediator),
	initial(initial),
	double_initial(mediator.mitosis(initial)),
	transition(transition),
	inttransition(inttransition),
	intconstraints(intconstraints),
	safety(safety),
	double_safety(mediator.mitosis(safety)),
	intsafety(mediator.evolve(safety, 2)),
	Xsafety(seqconverter.convert(safety)),
	Xintsafety(intconverter.convert(intsafety)),
	frames(),
	doubleframes(),
	frontier_level(0)
{
}


bool NoIC3::prove(){
	solver solv(ctxt);
	initial.fill_solver(solv);
	
	// zero steps
	solv.push();
	solv.add(!safety.get_formula());
	if(solv.check() == sat) return false;
	solv.pop();
	
	// one step (sequential)
	solv.add(transition);
	solv.add(!Xsafety.get_formula());
	if(solv.check() == sat) return false;
	
	// one step (interference)
	solv.reset();
	double_initial.fill_solver(solv);
	intconstraints.fill_solver(solv);
	solv.add(inttransition);
	solv.add(!Xintsafety.get_formula());
	if(solv.check() == sat) return false;
	
	frames.push_back(initial);
	doubleframes.push_back(double_initial);
	frames.push_back(safety);
	doubleframes.push_back(double_safety);
	frontier_level = 1;
	std::cout << "initiated induction, frontier is at level " << frontier_level << "\n";
	propagateClauses();
	
	while(true){
		if(!extendFrontier()){
			return false;
		}
		frontier_level += 1;
		std::cout << "extended frontier to level " << frontier_level << "\n";
		propagateClauses();
		for(int i=0; i<=frontier_level-1; i++){
			// check if frames i and i+1 are equal
			expr nequality = ! (frames[i].get_formula() == frames[i+1].get_formula());
			solv.reset();
			solv.add(nequality);
			if(solv.check() == unsat){
				return true;
			}
		}
	}
	
	return true;
}



bool NoIC3::extendFrontier(){
	solver seqsolv(ctxt);
	solver intsolv(ctxt);
	
	frames.push_back(safety);
	doubleframes.push_back(double_safety);
	
	// sequential step
	seqsolv.add(transition);
	seqsolv.add(!Xsafety.get_formula());
	while(true){
		seqsolv.push();
		frames[frontier_level].fill_solver(seqsolv);
		if(seqsolv.check() == unsat) break;
		
		model m = seqsolv.get_model();
		Clause witness(ctxt, seq_old_variables, m);
		
		std::cout << "Extending frontier interrupted because of witness " << witness.get_coformula() << "\n";
		std::cout << "It leads to bad state " << Clause(ctxt, seq_new_variables, m).get_coformula() << "\n";
		if(!removeCTI(witness)){
			return false;
		}
		seqsolv.pop();
	}
	
	// interference step
	intconstraints.fill_solver(intsolv);
	intsolv.add(inttransition);
	intsolv.add(!Xintsafety.get_formula());
	while(true){
		intsolv.push();
		doubleframes[frontier_level].fill_solver(intsolv);
		if(intsolv.check() == unsat) break;
		
		model m = intsolv.get_model();
		Clause cex = Clause(ctxt, int_old_variables, m);
		Clause witness1(ctxt, seq_old_variables.size());
		Clause witness2(ctxt, seq_old_variables.size());
		mediator.cytokinesis(cex, witness1, witness2);
		std::cout << "Extending frontier interrupted because of interference witness\n" << cex.get_coformula() << "\n";
		std::cout << "It leads to bad state " << Clause(ctxt, int_new_variables, m).get_coformula() << "\n";
		if(!removeCTI(witness1) && !removeCTI(witness2)){
			return false;
		}
		intsolv.pop();
	}
	
	return true;
}


void NoIC3::propagateClauses(){
	solver seqsolv(ctxt);
	solver intsolv(ctxt);
	seqsolv.add(transition);
	intconstraints.fill_solver(intsolv);
	intsolv.add(inttransition);
	for(int i=0; i<=frontier_level-1; i++){
		Frame &now = frames[i];
		Frame &doublenow = doubleframes[i];
		// at this time this already exists
		Frame &next = frames[i+1];
		Frame &doublenext = doubleframes[i+1];
		if(!now.has_propagation()) continue;
		
		std::vector<Clause> &list = now.get_propagation();
		std::vector<Clause> newlist;
		std::cout << "propagating " << list.size() << " clauses from level " << i << "\n";
		for(auto clause : list){
			if(next.contains_clause(clause)) continue;
			seqsolv.push();
			now.fill_solver(seqsolv);
			seqsolv.add(seqconverter.convert(clause).get_coformula());
			
			if(seqsolv.check() == unsat){
				intsolv.push();
				doublenow.fill_solver(intsolv);
				Clause intclause = mediator.evolve(clause, 2);
				intsolv.add(intconverter.convert(intclause).get_coformula());
				if(intsolv.check() == unsat){
					// unreachable -> propagate and forget about it
					next.add_clause(clause);
					mediator.doubleadd(doublenext, clause);
				}
				intsolv.pop();
			}else{
				// for now leave it open
				newlist.push_back(clause);
			}
			seqsolv.pop();
		}
		now.set_propagation(newlist);
	}
}


bool NoIC3::removeCTI(Clause &witness){
	std::cout << "Removing CTI\n";
	// Note: witness is a conjunction, so its negation is a clause
	solver seqsolv(ctxt);
	solver seqinitsolv(ctxt);
	solver intsolv(ctxt);
	// considering frames 0, ..., frontier_level
	//std::vector<ProofObligation*>  obligations[frontier_level+1];
	std::vector<ProofObligation*> *obligations = new std::vector<ProofObligation*>[frontier_level+1];
	ProofObligation *rootobligation = new ProofObligation(witness, NULL);
	obligations[frontier_level].push_back(rootobligation);
	
	bool result = false;
	int min_level = 0;
	seqsolv.add(transition);
	frames[0].fill_solver(seqinitsolv);
	intconstraints.fill_solver(intsolv);
	intsolv.add(inttransition);
	while(true){
		while(min_level <= frontier_level && obligations[min_level].size() == 0){
			min_level++;
		}
		if(min_level > frontier_level){
			result = true;
			break;
		}
		
		ProofObligation *topobl = obligations[min_level].back();
		obligations[min_level].pop_back();
		if(topobl->is_fulfilled()){
			delete topobl;
			continue;
		}
		if(topobl->is_failed()){
			if(topobl == rootobligation){
				delete topobl;
				result = true;
				break;
			}
			delete topobl;
			continue;
		}
		Clause state = topobl->get_clause();
		Clause Xstate = seqconverter.convert(state);
		Clause statepart1 = mediator.evolve(state, 1);
		Clause statepart2 = mediator.evolve(state, 2);
		Clause Xstatepart2 = intconverter.convert(statepart2);
		
		std::cout << "handling proof obligation (" << state.get_coformula() << ", " << min_level << ")\n";
		
		seqinitsolv.push();
		seqinitsolv.add(state.get_coformula());
		if(seqinitsolv.check() == sat){
			std::cout << "contained in the initial states!\n";
			topobl->set_failed();
			if(rootobligation->is_failed()){
				result = false;
				break;
			}
			delete topobl;
			seqinitsolv.pop();
			continue;
		}
		seqinitsolv.pop();
		
		
		seqsolv.push();
		seqsolv.add(state.get_formula());
		seqsolv.add(Xstate.get_coformula());
		
		seqsolv.push();
		frames[0].fill_solver(seqsolv);
		if(seqsolv.check() == sat){
			std::cout << "not sequential-inductive to initial frame!\n";
			topobl->set_failed();
			if(rootobligation->is_failed()){
				result = false;
				break;
			}
			delete topobl;
			seqsolv.pop();
			seqsolv.pop();
			continue;
		}
		std::cout << "is at least sequential-inductive to initial frame\n";
		seqsolv.pop();
		
		
		intsolv.push();
		intsolv.add(statepart1.get_formula());
		intsolv.add(statepart2.get_formula());
		intsolv.add(Xstatepart2.get_coformula());
		
		intsolv.push();
		doubleframes[0].fill_solver(intsolv);
		if(intsolv.check() == sat){
			std::cout << "not interference-inductive to initial frame!\n";
			topobl->set_failed();
			if(rootobligation->is_failed()){
				result = false;
				break;
			}
			delete topobl;
			intsolv.pop();
			intsolv.pop();
			continue;
		}
		std::cout << "is at least interference-inductive to initial frame\n";
		intsolv.pop();
		
		
		bool cex_is_int = false;
		int maxsafe = frontier_level-1;
		// Dummy value because reference -.-
		Clause newseqwitness(ctxt, 0);
		// seqsolv contains transition, !state and Xstate
		while(maxsafe > 0){
			seqsolv.push();
			frames[maxsafe].fill_solver(seqsolv);
			if(seqsolv.check() == sat){
				model m = seqsolv.get_model();
				newseqwitness = Clause(ctxt, seq_old_variables, m);
				maxsafe--;
				seqsolv.pop();
			}else{
				seqsolv.pop();
				break;
			}
		}
		std::cout << "Found sequential-safe frame at level " << maxsafe << "\n";
		
		// Dummy values because reference -.-
		Clause newintwitness1(ctxt, 0);
		Clause newintwitness2(ctxt, 0);
		// intsolv contains intconstraints, inttransition, !statepart1, !statepart2 and Xstatepart2
		while(maxsafe > 0){
			intsolv.push();
			doubleframes[maxsafe].fill_solver(intsolv);
			if(intsolv.check() == sat){
				model m = intsolv.get_model();
				Clause wit = Clause(ctxt, int_old_variables, m);
				newintwitness1 = Clause(ctxt, seq_old_variables.size());
				newintwitness2 = Clause(ctxt, seq_old_variables.size());
				mediator.cytokinesis(wit, newintwitness1, newintwitness2);
				cex_is_int = true;
				maxsafe--;
				intsolv.pop();
			}else{
				intsolv.pop();
				break;
			}
		}
		std::cout << "Found interference-safe frame at level " << maxsafe << "\n";
		
		
		
		std::cout << "Trying to generalize state w.r.t. frame " << maxsafe << "\n"; //\n" << frames[maxsafe].get_formula() << "\n";
		Clause strongerstate = state;
		seqsolv.pop();
		intsolv.pop();
		// seqsolv and intsolv now only contain their respective transitions (and intconstraints)
		seqsolv.push();
		intsolv.push();
		frames[maxsafe].fill_solver(seqsolv);
		doubleframes[maxsafe].fill_solver(intsolv);
		for(unsigned int i=0; i<seq_old_variables.size(); i++){
			if(! strongerstate.contains(i)) continue;
			Clause newstrongerstate = strongerstate.remove_literal(ctxt, seq_old_variables, i);
			Clause Xnewstrongerstate = seqconverter.convert(newstrongerstate);
			Clause newstrongerstatepart1 = mediator.evolve(newstrongerstate, 1);
			Clause newstrongerstatepart2 = mediator.evolve(newstrongerstate, 2);
			Clause Xnewstrongerstatepart2 = intconverter.convert(newstrongerstatepart2);
			
			seqinitsolv.push();
			seqinitsolv.add(newstrongerstate.get_coformula());
			if(seqinitsolv.check() == unsat){
				// state not contained in initial states
				seqsolv.push();
				seqsolv.add(newstrongerstate.get_formula());
				seqsolv.add(Xnewstrongerstate.get_coformula());
				if(seqsolv.check() == unsat){
					// sequential-inductive
					intsolv.push();
					intsolv.add(newstrongerstatepart1.get_formula());
					intsolv.add(newstrongerstatepart2.get_formula());
					intsolv.add(Xnewstrongerstatepart2.get_coformula());
					if(intsolv.check() == unsat){
						//interference-inductive
						strongerstate = newstrongerstate;
					}
					intsolv.pop();
				}
				seqsolv.pop();
			}
			seqinitsolv.pop();
		}
		std::cout << "Result: " << strongerstate.get_coformula() << "\n";
		
		for(int l=0; l<=maxsafe+1; l++){
			frames[l].add_clause(strongerstate, l==maxsafe+1);
			mediator.doubleadd(doubleframes[l], strongerstate);
		}
		// proof obligation fulfilled?
		if(maxsafe >= min_level-1){
			if(topobl == rootobligation){
				delete topobl;
				result = true;
				break;
			}
			topobl->set_fulfilled();
			delete topobl;
		}else{
			if(cex_is_int){
				ProofObligation *newobl1 = new ProofObligation(newintwitness1, topobl);
				ProofObligation *newobl2 = new ProofObligation(newintwitness2, topobl);
				newobl1->set_twin(newobl2);
				newobl2->set_twin(newobl1);
				obligations[maxsafe+1].push_back(newobl1);
				obligations[maxsafe+1].push_back(newobl2);
			}else{
				ProofObligation *newobl = new ProofObligation(newseqwitness, topobl);
				obligations[maxsafe+1].push_back(newobl);
			}
			
			obligations[min_level].push_back(topobl);
			if(maxsafe+1 < min_level) min_level = maxsafe + 1;
		}
		seqsolv.pop();
		intsolv.pop();
	}
	
	// cleanup
	for(int i=0; i<= frontier_level; i++){
		while(obligations[i].size() > 0){
			ProofObligation *obl = obligations[i].back();
			delete obl;
			obligations[i].pop_back();
		}
	}
	delete[] obligations;
	
	
	return result;
}
