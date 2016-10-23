#include <vector>
#include "z3++.h"
#include "variable.hpp"
#include "ast.hpp"
#include "program.hpp"
#include "observer.hpp"
#include "z3variablecontainer.hpp"
#include "noic3.hpp"
#include "view.hpp"




View::View(Program *prog, Observer *obs, context &ctxt, std::vector<Variable*> nonobsvars, std::vector<Variable*> obsvars, std::vector<Variable*> vars, std::vector<Variable*> activelocalscope, std::vector<Variable*> activenotlocalscope):
	program(prog),
	observer(obs),
	ctxt(ctxt),
	nonobsvars(nonobsvars),
	obsvars(obsvars),
	vars(vars),
	activelocalscope(activelocalscope),
	activenotlocalscope(activenotlocalscope),
	loc(Expression::loc()),
	autc(observer->count_states()),
	varc(vars.size()),
	obsc(obsvars.size()),
	old_variables(),
	new_variables(),
	
	pc(ctxt, loc, "pc", old_variables),
	os(ctxt, autc, "os", old_variables),
	shape_eq(ctxt, varc, "shape=", true, old_variables, &vars),
	shape_neq(ctxt, varc, "shape~", true, old_variables, &vars),
	shape_next(ctxt, varc, "shape+", false, old_variables, &vars),
	shape_reach(ctxt, varc, "shape*", false, old_variables, &vars),
	ages_eq(ctxt, 2*varc, "ages=", true, old_variables, &vars),
	ages_lt(ctxt, 2*varc, "ages<", false, old_variables, &vars),
	own(ctxt, varc, "own", old_variables, &vars),
	freed(ctxt, varc, "freed", old_variables, &vars),
	freednext(ctxt, varc, "freednext", old_variables, &vars),
	in(ctxt, obsc, "in", old_variables, &vars),
	out(ctxt, obsc, "out", old_variables, &vars),
	inseen(ctxt, obsc, "seen", old_variables, &vars),
	inv_eq(ctxt, varc, "inv=", old_variables, &vars),
	inv_next(ctxt, varc, "inv+", old_variables, &vars),
	inv_reach(ctxt, varc, "inv*", old_variables, &vars),
	sin_eq(ctxt, varc, "sin=", old_variables, &vars),
	sin_next(ctxt, varc, "sin+", old_variables, &vars),
	sin_reach(ctxt, varc, "sin*", old_variables, &vars),
	sinout(ctxt.bool_const("sinout")),
	
	Xpc(ctxt, loc, "Xpc", new_variables),
	Xos(ctxt, autc, "Xos", new_variables),
	Xshape_eq(ctxt, varc, "Xshape=", true, new_variables, &vars),
	Xshape_neq(ctxt, varc, "Xshape~", true, new_variables, &vars),
	Xshape_next(ctxt, varc, "Xshape+", false, new_variables, &vars),
	Xshape_reach(ctxt, varc, "Xshape*", false, new_variables, &vars),
	Xages_eq(ctxt, 2*varc, "Xages=", true, new_variables, &vars),
	Xages_lt(ctxt, 2*varc, "Xages<", false, new_variables, &vars),
	Xown(ctxt, varc, "Xown", new_variables, &vars),
	Xfreed(ctxt, varc, "Xfreed", new_variables, &vars),
	Xfreednext(ctxt, varc, "Xfreednext", new_variables, &vars),
	Xin(ctxt, obsc, "Xin", new_variables, &vars),
	Xout(ctxt, obsc, "Xout", new_variables, &vars),
	Xinseen(ctxt, obsc, "Xseen", new_variables, &vars),
	Xinv_eq(ctxt, varc, "Xinv=", new_variables, &vars),
	Xinv_next(ctxt, varc, "Xinv+", new_variables, &vars),
	Xinv_reach(ctxt, varc, "Xinv*", new_variables, &vars),
	Xsin_eq(ctxt, varc, "Xsin=", new_variables, &vars),
	Xsin_next(ctxt, varc, "Xsin+", new_variables, &vars),
	Xsin_reach(ctxt, varc, "Xsin*", new_variables, &vars),
	Xsinout(ctxt.bool_const("Xsinout")),
	
	XPC(),
	XOS(),
	transition(ctxt, Z3_mk_false(ctxt))
{
	sinoutid = old_variables.size();
	old_variables.push_back(sinout);
	Xsinoutid = new_variables.size();
	new_variables.push_back(Xsinout);
	
	XPC.reserve(loc+1);
	for(int i=0; i<loc+1; i++){
		expr f(ctxt, Z3_mk_true(ctxt));
		if(i < loc){
			f = Xpc(i);
		}
		for(int j=0; j<loc; j++){
			if(i == j) continue;
			f = f && !Xpc(j);
		}
		XPC.push_back(f);
	}
	
	XOS.reserve(autc);
	for(int i=0; i<autc; i++){
		expr f = Xos(i);
		for(int j=0; j<autc; j++){
			if(i == j) continue;
			f = f && !Xos(j);
		}
		XOS.push_back(f);
	}
}

int View::count_variables(){
	return old_variables.size();
}

std::vector<expr> &View::get_old_variables(){
	return old_variables;
}

std::vector<expr> &View::get_new_variables(){
	return new_variables;
}

expr &View::get_transition(){
	return transition;
}

void View::build_transition(){
	expr nopc(ctxt, Z3_mk_true(ctxt));
	expr yespc(ctxt, Z3_mk_false(ctxt));
	for(int i=0; i<loc; i++){
		nopc = nopc && !pc(i);
		yespc = yespc || pc(i);
	}
	transition = implies(nopc, trans_functioncall());
	transition = transition && implies(yespc, nochange_seen());
	
	for(auto next : program->get_code()){
		std::cout << "starting ";
		next->printline();
		
		expr formula = expr(ctxt, Z3_mk_true(ctxt));
		
		switch(next->get_subtype()){
		case MALLOC:
			formula = trans_malloc((Malloc*)next);
			break;
		case FREE:
			formula = trans_free((Free*)next);
			break;
		case AGEINCREMENT:
			break;
		case NEXTINCREMENT:
			break;
		case POINTERASSIGNMENT:
			formula = trans_pointerassignment((PointerAssignment*)next);
			break;
		case NEXTPOINTERASSIGNMENT:
			formula = trans_nextpointerassignment((NextPointerAssignment*)next);
			break;
		case POINTERNEXTASSIGNMENT:
			formula = trans_pointernextassignment((PointerNextAssignment*)next);
			break;
		case AGEASSIGNMENT:
			break;
		case INPUTASSIGNMENT:
			formula = trans_inputassignment((InputAssignment*)next);
			break;
		case OUTPUTASSIGNMENT:
			formula = trans_outputassignment((OutputAssignment*)next);
			break;
		case RETURN:
			formula = trans_return((Return*)next);
			break;
		case POINTERCOMPARISON:
			formula = trans_pointercomparison((PointerComparison*)next);
			break;
		case AGECOMPARISON:
			break;
		case CANDS:
			formula = trans_cas((CAS*)next);
			break;
		}
		
		std::cout << "finished ";
		next->printline();
		
		// implication
		transition = transition && implies(pc(next->getpc()), formula);
	}
}

expr View::pcincrement(expr &f, Statement *command){
	Expression *next = command->getnext();
	if(next != NULL){
		int nextpc = next->getpc();
		f = f && XPC[nextpc];
	}else{
		f = f && XPC[loc];
	}
	return f;
}

expr View::pcincrement(expr &f, Condition *command, expr cond){
	Expression *next_success = command->getnext(true);
	Expression *next_fail = command->getnext(false);
	
	expr branch_success(ctxt, Z3_mk_true(ctxt));
	expr branch_fail(ctxt, Z3_mk_true(ctxt));
	if(next_success != NULL){
		branch_success = XPC[next_success->getpc()];
	}else{
		branch_success = XPC[loc];
	}
	if(next_fail != NULL){
		branch_fail = XPC[next_fail->getpc()];
	}else{
		branch_fail = XPC[loc];
	}
	f = f && implies(cond, branch_success) && implies(!cond, branch_fail);
	return f;
}

expr View::osupdate(expr &f, LinearisationEvent *event){
	if(event == NULL) return nochange_os(f);
	LinearisationType type = event->gettype();
	Variable *var = vardown(event->getvar());
	Variable *cmp1 = vardown(event->getcmp1());
	Variable *cmp2 = vardown(event->getcmp2());
	
	expr guard(ctxt, Z3_mk_true(ctxt));
	if(type == IN_PCMP || type == OUT_PCMP){
		int cmp1id = cmp1->getid();
		int cmp2id = cmp2->getid();
		guard = shape_eq(cmp1id, cmp2id);
	}else if(type == IN_PACMP || type == OUT_PACMP){
		int cmp1id = cmp1->getid();
		int cmp2id = cmp2->getid();
		guard = shape_eq(cmp1id, cmp2id) && ages_eq(cmp1id, cmp2id);
	}
	
	for(int source=0; source<autc; source++){
		if(var == NULL){
			int target = observer->transition(source, event, NULL);
			f = f && implies(os(source) && guard, XOS[target]);
			f = f && implies(os(source) && !guard, XOS[source]);
		}else{
			int varid = var->getid();
			expr notwatched(ctxt, Z3_mk_true(ctxt));
			for(const auto z : obsvars){
				int target = observer->transition(source, event, varup(z));
				int zid = z->getid();
				f = f && implies(os(source) && guard && shape_eq(varid, zid), XOS[target]);
				notwatched = notwatched && !shape_eq(varid, zid);
			}
			f = f && implies(os(source) && (!guard || notwatched), XOS[source]);
		}
	}
	
	return f;
}

expr View::nochange_sinout(expr &f){
	return f && (Xsinout == sinout);
}

expr View::nochange_freed(expr &f, std::vector<Variable*> &v, Variable *except){
	for(const auto &z : v){
		if(z == except) continue;
		int zid = z->getid();
		f = f && (Xfreed(zid) == freed(zid)) && (Xfreednext(zid) == freednext(zid));
	}
	return f;
}

expr View::nochange_os(expr &f, int except){
	for(int i=0; i<autc; i++){
		if(i == except) continue;
		f = f && (Xos(i) == os(i));
	}
	return f;
}

expr View::nochange_observed_in(expr &f, std::vector<Variable*> &v){
	for(const auto &z : v){
		int zid = z->getid();
		f = f && (Xin(zid) == in(zid));
	}
	return f;
}

expr View::nochange_observed_out(expr &f, std::vector<Variable*> &v){
	for(const auto &z : v){
		int zid = z->getid();
		f = f && (Xout(zid) == out(zid));
	}
	return f;
}

expr View::nochange_shape(expr &f, std::vector<Variable*> &v, Variable *except){
	for(const auto &z1 : v){
		if(z1 == except) continue;
		int z1id = z1->getid();
		for(const auto &z2 : v){
			if(z2 == except) continue;
			int z2id = z2->getid();
			f = f && (Xshape_eq(z1id, z2id) == shape_eq(z1id, z2id)) && (Xshape_neq(z1id, z2id) == shape_neq(z1id, z2id)) && (Xshape_next(z1id, z2id) == shape_next(z1id, z2id)) && (Xshape_reach(z1id, z2id) == shape_reach(z1id, z2id));
		}
	}
	return f;
}

expr View::nochange_inv(expr &f, std::vector<Variable*> &v, Variable *except){
	for(const auto &z : v){
		if(z == except) continue;
		int zid = z->getid();
		f = f && (Xinv_eq(zid) == inv_eq(zid)) && (Xinv_next(zid) == inv_next(zid)) && (Xinv_reach(zid) == inv_reach(zid));
	}
	return f;
}

expr View::nochange_sin(expr &f, std::vector<Variable*> &v, Variable *except){
	for(const auto &z : v){
		if(z == except) continue;
		int zid = z->getid();
		f = f && (Xsin_eq(zid) == sin_eq(zid)) && (Xsin_next(zid) == sin_next(zid)) && (Xsin_reach(zid) == sin_reach(zid));
	}
	return f;
}

expr View::nochange_ages(expr &f, std::vector<Variable*> &v, Variable *except){
	for(const auto &z1 : v){
		if(z1 == except) continue;
		int z1id = z1->getid();
		int z1nextid = z1->getnextid();
		for(const auto &z2 : v){
			if(z2 == except) continue;
			int z2id = z2->getid();
			int z2nextid = z2->getnextid();
			f = f && (Xages_eq(z1id, z2id) == ages_eq(z1id, z2id)) && (Xages_eq(z1nextid, z2id) == ages_eq(z1nextid, z2id)) && (Xages_eq(z1id, z2nextid) == ages_eq(z1id, z2nextid)) && (Xages_eq(z1nextid, z2nextid) == ages_eq(z1nextid, z2nextid)) &&
			         (Xages_lt(z1id, z2id) == ages_lt(z1id, z2id)) && (Xages_lt(z1nextid, z2id) == ages_lt(z1nextid, z2id)) && (Xages_lt(z1id, z2nextid) == ages_lt(z1id, z2nextid)) && (Xages_lt(z1nextid, z2nextid) == ages_lt(z1nextid, z2nextid));
		}
	}
	return f;
}

void View::shapeconsistency(Frame &frame){
	for(const auto x : vars){
		int xid = x->getid();
		
		Clause self1(ctxt, old_variables.size());
		self1.set_variable(shape_eq(xid, xid), shape_eq.getid(xid, xid), CLAUSE_POSITIVE);
		
		Clause self2(ctxt, old_variables.size());
		self2.set_variable(shape_neq(xid, xid), shape_neq.getid(xid, xid), CLAUSE_NEGATIVE);
		
		frame.add_clause(self1);
		frame.add_clause(self2);
		
		for(const auto y : vars){
			int yid = y->getid();
			
			Clause min1(ctxt, old_variables.size());
			min1.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_POSITIVE);
			min1.set_variable(shape_next(xid, yid), shape_next.getid(xid, yid), CLAUSE_POSITIVE);
			min1.set_variable(shape_next(yid, xid), shape_next.getid(yid, xid), CLAUSE_POSITIVE);
			min1.set_variable(shape_reach(xid, yid), shape_reach.getid(xid, yid), CLAUSE_POSITIVE);
			min1.set_variable(shape_reach(yid, xid), shape_reach.getid(yid, xid), CLAUSE_POSITIVE);
			min1.set_variable(shape_neq(xid, yid), shape_neq.getid(xid, yid), CLAUSE_POSITIVE);
			
			Clause min2(ctxt, old_variables.size());
			min2.set_variable(shape_neq(xid, yid), shape_neq.getid(xid, yid), CLAUSE_NEGATIVE);
			min2.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
			
			Clause min3(ctxt, old_variables.size());
			min3.set_variable(shape_neq(xid, yid), shape_neq.getid(xid, yid), CLAUSE_NEGATIVE);
			min3.set_variable(shape_next(xid, yid), shape_next.getid(xid, yid), CLAUSE_NEGATIVE);
			
			Clause min5(ctxt, old_variables.size());
			min5.set_variable(shape_neq(xid, yid), shape_neq.getid(xid, yid), CLAUSE_NEGATIVE);
			min5.set_variable(shape_reach(xid, yid), shape_reach.getid(xid, yid), CLAUSE_NEGATIVE);
			
			
			frame.add_clause(min1, false);
			frame.add_clause(min2, false);
			frame.add_clause(min3, false);
			frame.add_clause(min5, false);
			
			
			for(const auto z : vars){
				int zid = z->getid();
				
				Clause uniquenext(ctxt, old_variables.size());
				uniquenext.set_variable(shape_next(xid, yid), shape_next.getid(xid, yid), CLAUSE_NEGATIVE);
				uniquenext.set_variable(shape_next(xid, zid), shape_next.getid(xid, zid), CLAUSE_NEGATIVE);
				uniquenext.set_variable(shape_eq(yid, zid), shape_eq.getid(yid, zid), CLAUSE_POSITIVE);
				frame.add_clause(uniquenext, false);
				
				if(x != y && x != z){
					Clause eqeq1(ctxt, old_variables.size());
					eqeq1.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
					eqeq1.set_variable(shape_eq(yid, zid), shape_eq.getid(yid, zid), CLAUSE_POSITIVE);
					eqeq1.set_variable(shape_eq(xid, zid), shape_eq.getid(xid, zid), CLAUSE_NEGATIVE);
					
					frame.add_clause(eqeq1, false);
				}
				
				if(x != y){
					Clause eqneq1(ctxt, old_variables.size());
					eqneq1.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
					eqneq1.set_variable(shape_neq(yid, zid), shape_neq.getid(yid, zid), CLAUSE_POSITIVE);
					eqneq1.set_variable(shape_neq(xid, zid), shape_neq.getid(xid, zid), CLAUSE_NEGATIVE);
					
					Clause eqnext1(ctxt, old_variables.size());
					eqnext1.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
					eqnext1.set_variable(shape_next(yid, zid), shape_next.getid(yid, zid), CLAUSE_POSITIVE);
					eqnext1.set_variable(shape_next(xid, zid), shape_next.getid(xid, zid), CLAUSE_NEGATIVE);
					
					Clause eqnext2(ctxt, old_variables.size());
					eqnext2.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
					eqnext2.set_variable(shape_next(zid, xid), shape_next.getid(zid, xid), CLAUSE_NEGATIVE);
					eqnext2.set_variable(shape_next(zid, yid), shape_next.getid(zid, yid), CLAUSE_POSITIVE);
					
					Clause eqreach1(ctxt, old_variables.size());
					eqreach1.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
					eqreach1.set_variable(shape_reach(yid, zid), shape_reach.getid(yid, zid), CLAUSE_POSITIVE);
					eqreach1.set_variable(shape_reach(xid, zid), shape_reach.getid(xid, zid), CLAUSE_NEGATIVE);
					
					Clause eqreach2(ctxt, old_variables.size());
					eqreach2.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
					eqreach2.set_variable(shape_reach(zid, xid), shape_reach.getid(zid, xid), CLAUSE_NEGATIVE);
					eqreach2.set_variable(shape_reach(zid, yid), shape_reach.getid(zid, yid), CLAUSE_POSITIVE);
					
					frame.add_clause(eqneq1, false);
					frame.add_clause(eqnext1, false);
					frame.add_clause(eqnext2, false);
					frame.add_clause(eqreach1, false);
					frame.add_clause(eqreach2, false);
				}
			}
		}
	}
}

void View::memconsistency(Frame &frame){
	for(const auto x : vars){
		int xid = x->getid();
		
		if(x->isobserver()){
			Clause gowner(ctxt, old_variables.size());
			gowner.set_variable(own(xid), own.getid(xid), CLAUSE_NEGATIVE);
			frame.add_clause(gowner);
		}else if(x->isglobal()){
			Clause gowner(ctxt, old_variables.size());
			gowner.set_variable(own(xid), own.getid(xid), CLAUSE_NEGATIVE);
			gowner.set_variable(inv_eq(xid), inv_eq.getid(xid), CLAUSE_POSITIVE);
			frame.add_clause(gowner);
		}
		
		for(const auto y : vars){
			if(y == x) continue;
			int yid = y->getid();
			
			Clause fr(ctxt, old_variables.size());
			fr.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
			fr.set_variable(freed(xid), freed.getid(xid), CLAUSE_POSITIVE);
			fr.set_variable(freed(yid), freed.getid(yid), CLAUSE_NEGATIVE);
			
			frame.add_clause(fr);
		}
	}
}

expr View::trans_functioncall(){
	expr result = !Xsinout;
	// sinout2 handled in inherited function
	
	// shape relations of local variables are removed
	for(const auto x : activelocalscope){
		int xid = x->getid();
		result = result && Xshape_eq(xid, xid) && !Xshape_neq(xid, xid) && !Xshape_next(xid, xid) && !Xshape_reach(xid, xid);
		for(const auto y : vars){
			if(y == x) continue;
			int yid = y->getid();
			result = result && !Xshape_eq(xid, yid) && Xshape_neq(xid, yid) && !Xshape_next(xid, yid) && !Xshape_next(yid, xid) && !Xshape_reach(xid, yid) && !Xshape_reach(yid, xid);
		}
	}
	result = nochange_shape(result, activenotlocalscope);
	
	// local variables are neither free nor owned, but invalid
	for(const auto x : activelocalscope){
		int xid = x->getid();
		result = result && !Xfreed(xid) && !Xfreednext(xid) && Xinv_eq(xid) && Xinv_next(xid) && Xinv_reach(xid) && !Xsin_eq(xid) && !Xsin_next(xid) && !Xsin_reach(xid) && nuke_ownership(x);
	}
	result = nochange_freed(result, activenotlocalscope);
	result = nochange_inv(result, activenotlocalscope);
	result = nochange_sin(result, activenotlocalscope);
	result = nochange_own(result, activenotlocalscope);
	
	// forget about age relations of locals (except the obvious), copy the others
	for(const auto x : activelocalscope){
		int xid = x->getid();
		int xnextid = x->getnextid();
		result = result && Xages_eq(xid, xid) && !Xages_lt(xid, xid) && Xages_eq(xnextid, xnextid) && !Xages_lt(xnextid, xnextid);
	}
	result = nochange_ages(result, activenotlocalscope);
	
	// nobody is output
	for(const auto x : obsvars){
		int xid = x->getid();
		result = result && !Xout(xid);
	}
	// out2 handled in inherited function
	
	
	// set input and pc
	expr alternatives(ctxt, Z3_mk_false(ctxt));
	for(const auto f : program->get_functions()){
		expr thisfunc = XPC[f->get_entrypoint()->getpc()];
		if(f->has_input()){
			expr inputs(ctxt, Z3_mk_false(ctxt));
			
			// input is observed
			for(const auto chosen : obsvars){
				int chid = chosen->getid();
				expr thisinput = Xin(chid);
				for(const auto others : obsvars){
					if(others == chosen) continue;
					thisinput = thisinput && !Xin(others->getid());
				}
				inputs = inputs || (thisinput && !input_seen(chosen) && see_input(chosen) && nochange_seen(chosen));
			}
			// input is not observed
			expr noinput = nochange_seen();
			for(const auto chosen : obsvars){
				int chid = chosen->getid();
				noinput = noinput && !Xin(chid);
			}
			
			inputs = inputs || noinput;
			
			thisfunc = thisfunc && inputs;
		}else{
			thisfunc = thisfunc && nochange_seen();
			for(const auto z : obsvars){
				thisfunc = thisfunc && !Xin(z->getid());
			}
		}
		alternatives = alternatives || thisfunc;
	}
	result = result && alternatives;
	// in2 and pc2 handled in inherited function
	
	result = result && nochange_os(result);
	
	return result;
}

expr View::trans_malloc(Malloc *command){
	Variable *x = vardown(command->getvar());
	int xid = x->getid();
	
	
	// 1) getting a fresh memory cell
	expr result_fresh = Xshape_eq(xid, xid) && !Xshape_neq(xid, xid) && !Xshape_next(xid, xid) && !Xshape_reach(xid, xid);
	for(const auto &z : vars){
		if(z == x) continue;
		int zid = z->getid();
		result_fresh = result_fresh && Xshape_neq(xid, zid) && !Xshape_eq(xid, zid) && !Xshape_next(xid, zid) && !Xshape_next(zid, xid) && !Xshape_reach(xid, zid) && !Xshape_reach(zid, xid);
	}
	
	result_fresh = result_fresh && !Xfreed(xid);
	// Xfreednext(xid)? We don't know.
	result_fresh = nochange_freed(result_fresh, vars, x);
	
	
	expr result = result_fresh;
	
	
	
	
	
	// 2) getting a memory cell with a dangling pointer
	for(const auto &y : vars){
		int yid = y->getid();
		
		// guard, so that this is not even considered if y is not freed
		expr result_y = freed(yid);
		
		// copy everything from y to x
		result_y = result_y && (Xshape_eq(xid, xid) == shape_eq(yid, yid)) && (Xshape_neq(xid, xid) == shape_neq(yid, yid)) && (Xshape_next(xid, xid) == shape_next(yid, yid)) && (Xshape_reach(xid, xid) == shape_reach(yid, yid));
		for(const auto &z : vars){
			if(z == x) continue;
			int zid = z->getid();
			result_y = result_y && (Xshape_eq(xid, zid) == shape_eq(yid, zid)) && (Xshape_neq(xid, zid) == shape_neq(yid, zid)) && (Xshape_next(xid, zid) == shape_next(yid, zid)) && (Xshape_next(zid, xid) == shape_next(zid, yid)) && (Xshape_reach(xid, zid) == shape_reach(yid, zid)) && (Xshape_reach(zid, xid) == shape_reach(zid, yid));
		}
		
		result_y = result_y && !Xfreed(xid);
		// Xfreednext(xid)? We don't know.
		for(const auto &z : vars){
			if(z == x) continue;
			int zid = z->getid();
			result_y = result_y && (Xfreed(zid) == (freed(zid) && !shape_eq(yid, zid)));
			// Xfreednext(zid)? We don't know (sure, ONE reason was removed)
		}
		
		result = result || result_y;
	}
	
	
	
	
	// 3) some things are the same in both 1) and 2)
	result = result && !Xinv_eq(xid) && Xinv_next(xid) && Xinv_reach(xid);
	result = result && !Xsin_eq(xid) && Xsin_next(xid) && Xsin_reach(xid);
	if(x->isglobal()){
		result = result && nuke_ownership(x);
	}else{
		result = result && claim_ownership(x);
	}
	
	result = nochange_shape(result, vars, x);
	result = nochange_inv(result, vars, x);
	result = nochange_sin(result, vars, x);
	result = nochange_own(result, vars, x);
	result = nochange_ages(result, vars);
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	result = nochange_sinout(result);
	result = pcincrement(result, command);
	result = osupdate(result, command->get_event());
	
	
	return result;
}

expr View::trans_free(Free *command){
	Variable *x = vardown(command->getvar());
	int xid = x->getid();
	
	expr result(ctxt, Z3_mk_true(ctxt));
	
	// now free
	for(const auto z : vars){
		int zid = z->getid();
		result = result && (Xfreed(zid) == (freed(zid) || shape_eq(xid, zid)));
		result = result && (Xfreednext(zid) == (freednext(zid) || shape_next(zid, xid) || shape_reach(zid, xid)));
	}
	
	// everything that points there is invalid...
	for(const auto z : vars){
		int zid = z->getid();
		result = result && (Xinv_eq(zid) == (inv_eq(zid) || shape_eq(zid, xid)));
		result = result && (Xinv_next(zid) == (inv_next(zid) || shape_next(zid, xid)));
		result = result && (Xinv_reach(zid) == (inv_reach(zid) || shape_reach(zid, xid)));
	}
	// ...but not strongly invalid (if it was not already)
	result = nochange_sin(result, vars);
	
	// nobody owns that
	for(const auto z : vars){
		int zid = z->getid();
		result = result && implies(shape_eq(xid, zid), nuke_ownership(z)) && implies(!shape_eq(xid, zid), copy_ownership(z, z));
	}
	
	result = nochange_shape(result, vars);
	result = nochange_ages(result, vars);
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	result = nochange_sinout(result);
	result = pcincrement(result, command);
	result = osupdate(result, command->get_event());
	
	return result;
}

expr View::trans_pointerassignment(PointerAssignment *command){
	Variable *x = vardown(command->getlhs());
	Variable *y = vardown(command->getrhs());
	int xid = x->getid();
	int xnextid = x->getnextid();
	int yid = y->getid();
	int ynextid = y->getnextid();
	
	expr result(ctxt, Z3_mk_true(ctxt));
	
	
	// all freed and freednext information of y is copied to x, all others stay as they were
	result = result && (Xfreed(xid) == freed(yid)) && (Xfreednext(xid) == freednext(yid));
	result = nochange_freed(result, vars, x);
	
	// all inv and sin information is copied to x, all others stay as they were
	result = result && (Xinv_eq(xid) == inv_eq(yid)) && (Xinv_next(xid) == inv_next(yid)) && (Xinv_reach(xid) == inv_reach(yid));
	result = nochange_inv(result, vars, x);
	result = result && (Xsin_eq(xid) == sin_eq(yid)) && (Xsin_next(xid) == sin_next(yid)) && (Xsin_reach(xid) == sin_reach(yid));
	result = nochange_sin(result, vars, x);
	
	// ownership
	if(x->isglobal()){
		// if y is valid, we just published its memory cell
		for(const auto z : vars){
			int zid = z->getid();
			result = result && implies(!inv_eq(yid) && shape_eq(yid, zid), nuke_ownership(z)) &&
			                   implies(inv_eq(yid) || !shape_eq(yid, zid), copy_ownership(z, z));
		}
	}else{
		//ownership of y is copied to x, all others stay as they were
		result = result && copy_ownership(y, x);
		result = nochange_own(result, vars, x);
	}
	
	// all shape information of y is copied to x, all others stay as they were
	result = result && (Xshape_eq(xid, xid) == shape_eq(yid, yid)) && (Xshape_neq(xid, xid) == shape_neq(yid, yid)) && (Xshape_next(xid, xid) == shape_next(yid, yid)) && (Xshape_reach(xid, xid) == shape_reach(yid, yid));
	for(const auto &z : vars){
		if(z == x) continue;
		int zid = z->getid();
		result = result && (Xshape_eq(xid, zid) == shape_eq(yid, zid)) && (Xshape_neq(xid, zid) == shape_neq(yid, zid)) && (Xshape_next(xid, zid) == shape_next(yid, zid)) && (Xshape_next(zid, xid) == shape_next(zid, yid)) && (Xshape_reach(xid, zid) == shape_reach(yid, zid)) && (Xshape_reach(zid, xid) == shape_reach(zid, yid));
	}
	result = nochange_shape(result, vars, x);
	
	// all age information of y is copied to x, all others stay as they were
	result = result && (Xages_eq(xid, xid) == ages_eq(yid, yid)) && (Xages_eq(xnextid, xnextid) == ages_eq(ynextid, ynextid)) && (Xages_eq(xid, xnextid) == ages_eq(yid, ynextid)) &&
	                   (Xages_lt(xid, xid) == ages_lt(yid, yid)) && (Xages_lt(xnextid, xnextid) == ages_lt(ynextid, ynextid)) && (Xages_lt(xid, xnextid) == ages_lt(yid, ynextid)) && (Xages_lt(xnextid, xid) == ages_lt(ynextid, yid));
	for(const auto &z : vars){
		if(z == x) continue;
		int zid = z->getid();
		int znextid = z->getnextid();
		result = result && (Xages_eq(xid, zid) == ages_eq(yid, zid)) &&                                              (Xages_eq(xid, znextid) == ages_eq(yid, znextid)) &&                                                      (Xages_eq(xnextid, zid) == ages_eq(ynextid, zid)) &&                                                      (Xages_eq(xnextid, znextid) == ages_eq(ynextid,znextid)) &&
		                   (Xages_lt(xid, zid) == ages_lt(yid, zid)) && (Xages_lt(zid, xid) == ages_lt(zid, yid)) && (Xages_lt(xid, znextid) == ages_lt(yid, znextid)) && (Xages_lt(znextid, xid) == ages_lt(znextid, yid)) && (Xages_lt(xnextid, zid) == ages_lt(ynextid, zid)) && (Xages_lt(zid, xnextid) == ages_lt(zid, ynextid)) && (Xages_lt(xnextid, znextid) == ages_lt(ynextid, znextid)) && (Xages_lt(znextid, xnextid) == ages_lt(znextid, ynextid));
	}
	result = nochange_ages(result, vars, x);
	
	result = pcincrement(result, command);
	result = osupdate(result, command->get_event());
	result = nochange_sinout(result);
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	
	return result;
}

expr View::trans_nextpointerassignment(NextPointerAssignment *command){
	Variable *x = vardown(command->getlhs());
	Variable *y = vardown(command->getrhs());
	int xid = x->getid();
	int xnextid = x->getnextid();
	int yid = y->getid();
	
	// age relations of y are transferred to x.next, all others stay as they were
	expr result = (Xages_eq(xnextid, xnextid) == ages_eq(yid, yid)) && (Xages_lt(xnextid, xnextid) == ages_lt(yid, yid));
	for(const auto z : vars){
		int zid = z->getid();
		int znextid = z->getid();
		result = result &&         (Xages_eq(xnextid, zid) == ages_eq(yid, zid))         && (Xages_lt(xnextid, zid) == ages_lt(yid, zid))         && (Xages_lt(zid, xnextid) = ages_lt(yid, xnextid));
		if(z != x){
			result = result && (Xages_eq(xnextid, znextid) == ages_eq(yid, znextid)) && (Xages_lt(xnextid, znextid) == ages_lt(yid, znextid)) && (Xages_lt(znextid, xnextid) == ages_lt(znextid, yid));
		}
		result = result && (Xages_eq(xid, zid) == ages_eq(xid, zid))         && (Xages_lt(xid, zid) == ages_lt(xid, zid))         && (Xages_lt(zid, xid) == ages_lt(zid, xid)) &&
		                   (Xages_eq(xid, znextid) == ages_eq(xid, znextid)) && (Xages_lt(xid, znextid) == ages_lt(xid, znextid)) && (Xages_lt(znextid, xid) == ages_lt(znextid, xid));
	}
	result = nochange_ages(result, vars, x);
	
	// who can reach an invalid pointer now
	for(const auto z : vars){
		int zid = z->getid();
		result = result && (Xinv_eq(zid) == inv_eq(zid)) && (Xsin_eq(zid) == sin_eq(zid));
		result = result && implies(shape_eq(zid, xid), (Xinv_next(zid) == inv_eq(yid)) && (Xsin_next(zid) == sin_eq(yid)));
		result = result && implies(!shape_eq(zid, xid), (Xinv_next(zid) == inv_next(zid)) && (Xsin_next(zid) == sin_next(zid)));
		result = result && implies(shape_eq(zid, xid), (Xinv_reach(zid) == (inv_next(yid) || inv_reach(yid))) && (Xsin_reach(xid) == (sin_next(yid) || sin_reach(yid))));
		result = result && implies(!shape_eq(zid, xid), (Xinv_reach(zid) == (inv_reach(zid) || ((shape_next(zid, xid) || shape_reach(zid, xid)) && (inv_eq(yid) || inv_next(yid) || inv_reach(yid))))) &&
		                                                (Xsin_reach(zid) == (sin_reach(zid) || ((shape_next(zid, xid) || shape_reach(zid, xid)) && (sin_eq(yid) || sin_next(yid) || sin_reach(yid))))));
	}
	
	// maybe we can reach a freed cell now
	for(const auto z : vars){
		int zid = z->getid();
		result = result && (Xfreed(zid) == freed(zid));
		result = result && implies(shape_eq(zid, xid), Xfreednext(zid) == (freed(yid) || freednext(yid)));
		result = result && implies(!shape_eq(zid, xid), Xfreednext(zid) == (freednext(zid) || ((shape_next(zid, xid) || shape_reach(zid, xid)) && (freed(yid) || freednext(yid)))));
	}
	
	// shape tohuwabohu
	for(const auto z : vars){
		int zid = z->getid();
		for(const auto w : vars){
			int wid = w->getid();
			result = result && (Xshape_eq(zid, wid) == shape_eq(zid, wid));
			result = result && (Xshape_next(zid, wid) == ((shape_eq(zid, xid) && shape_eq(wid, yid)) || (!shape_eq(zid, xid) && shape_next(zid, wid))));
			
			expr newreach = (shape_eq(zid, xid) && (shape_next(yid, wid) || shape_reach(yid, wid)) && ((!shape_next(yid, xid) && !shape_reach(yid, xid)) || shape_eq(wid, xid) || shape_next(wid, xid) || shape_reach(wid, xid))) ||
			                (!shape_eq(zid, xid) && (shape_next(zid, xid) || shape_reach(zid, xid)) && (shape_eq(yid, wid) || shape_next(yid, wid) || (shape_reach(yid, wid) && ((!shape_next(yid, xid) && !shape_reach(yid, xid)) || shape_eq(wid, xid) || shape_next(wid, xid) || shape_reach(wid, xid)))));
			expr xreach = (shape_eq(zid, xid) || shape_next(zid, xid) || shape_reach(zid, xid)) && (shape_next(xid, wid) || shape_reach(xid, wid));
			expr reflection = shape_eq(zid, xid) && shape_eq(wid, yid) && (shape_next(yid, xid) || shape_reach(yid, xid));
			expr stillreach = shape_reach(zid, wid) && !xreach && !(shape_eq(xid, wid) && shape_next(zid, xid) && shape_reach(xid, xid));
			result = result && (Xshape_reach(zid, wid) == (newreach || reflection || stillreach));
			
			result = result && (Xshape_neq(zid, wid) == !(Xshape_eq(zid, wid) || Xshape_next(zid, wid) || Xshape_next(wid, zid) || Xshape_reach(zid, wid) || Xshape_reach(wid, zid)));
		}
	}
	
	result = pcincrement(result, command);
	result = osupdate(result, command->get_event());
	result = nochange_own(result, vars);
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	result = nochange_sinout(result);
	
	return result;
}

expr View::trans_pointernextassignment(PointerNextAssignment *command){
	Variable *x = vardown(command->getlhs());
	Variable *y = vardown(command->getrhs());
	int xid = x->getid();
	int yid = y->getid();
	int xnextid = x->getnextid();
	int ynextid = y->getnextid();
	
	expr result = (Xinv_eq(xid) == inv_next(yid));
	result = result && (inv_reach(yid) == (Xinv_next(xid) || Xinv_reach(xid)));
	result = nochange_inv(result, vars, x);
	result = result && (Xsin_eq(xid) == (inv_eq(yid) || sin_eq(yid) || inv_next(yid)));
	result = result && (Xsin_next(xid) == (inv_next(yid) || sin_next(yid))) && (Xsin_reach(xid) == (inv_reach(yid) || sin_reach(yid)));
	result = nochange_sin(result, vars, x);
	
	// copy ages from y.next to x
	result = result && (Xages_eq(xid, xid) == ages_eq(ynextid, ynextid))  && (Xages_lt(xid, xid) == ages_lt(ynextid, ynextid)) && Xages_eq(xnextid, xnextid) && !Xages_lt(xnextid, xnextid);
	for(const auto z : vars){
		if(z == x) continue;
		int zid = z->getid();
		int znextid = z->getnextid();
		
		result = result && (Xages_eq(xid, zid) == ages_eq(ynextid, zid))         && (Xages_lt(xid, zid) == ages_lt(ynextid, zid))         && (Xages_lt(zid, xid) == ages_lt(zid, ynextid)) &&
		                   (Xages_eq(xid, znextid) == ages_eq(ynextid, znextid)) && (Xages_lt(xid, znextid) == ages_lt(ynextid, znextid)) && (Xages_lt(znextid, xid) == ages_lt(znextid, ynextid));
	}
	// NOTE: ages(x.next, z) with z =/= x.next not treated on purpose - we don't know about it
	result = nochange_ages(result, vars, x);
	
	// shape tohuwabohu
	result = result && Xshape_eq(xid, xid) && !Xshape_neq(xid, xid); // we don't know about next and reach
	for(const auto z : vars){
		if(z == x) continue;
		int zid = x->getid();
		result = result && (shape_next(yid, zid) == Xshape_eq(xid, zid));
		// side-track pointers prevent us from having equivalences here as well
		result = result && implies(shape_eq(yid, zid), Xshape_next(zid, xid));
		result = result && implies(shape_next(zid, yid) || shape_reach(zid, yid), Xshape_reach(zid, xid));
		result = result && implies(!shape_neq(yid, zid), !Xshape_neq(xid, zid));
		// but we can impose restrictions
		result = result && implies(shape_neq(yid, zid), Xshape_neq(zid, xid) || Xshape_next(zid, xid) || Xshape_reach(zid, xid));
		result = result && (Xshape_neq(zid, xid) == !(Xshape_eq(zid, xid) || Xshape_next(zid, xid) || Xshape_next(xid, zid) || Xshape_reach(zid, xid) || Xshape_reach(xid, zid)));
		// If you want to point at my successors, you have to know some things
		expr samesuc(ctxt, Z3_mk_true(ctxt));
		expr samesucplus(ctxt, Z3_mk_true(ctxt));
		for(const auto w : vars){
			int wid = w->getid();
			samesuc = samesuc && (shape_next(zid, wid) == shape_next(yid, wid)) && (shape_reach(zid, wid) == shape_reach(yid, wid));
			samesucplus = samesucplus && implies(shape_next(yid, wid) || shape_reach(yid, wid), shape_reach(zid, wid));
		}
		result = result && implies(Xshape_next(zid, xid), samesuc);
		result = result && implies(Xshape_reach(zid, xid), samesucplus);
	}
	// reaching from x is more involved. First check if there is already a pointer on y.next. We can copy that.
	expr noonenext(ctxt, Z3_mk_true(ctxt));
	for(const auto z : vars){
		int zid = z->getid();
		expr copyz = (Xshape_eq(xid, xid) == shape_eq(zid, zid)) && (Xshape_neq(xid, xid) == shape_neq(zid, zid)) && (Xshape_next(xid, xid) == shape_next(zid, zid)) && (Xshape_reach(xid, xid) == shape_reach(zid, zid));
		for(const auto w : vars){
			if(w == x) continue;
			int wid = w->getid();
			copyz = copyz && (Xshape_eq(xid, wid) == shape_eq(zid, wid)) && (Xshape_neq(xid, wid) == shape_neq(zid, wid)) && (Xshape_next(xid, wid) == shape_next(zid, wid)) && (Xshape_next(wid, xid) == shape_next(wid, zid)) && (Xshape_reach(xid, wid) == shape_reach(zid, wid)) && (Xshape_reach(wid, xid) == shape_reach(wid, zid));
		}
		copyz = copyz && (Xfreed(xid) == freed(zid)) && (Xfreednext(xid) == freednext(zid));
		result = result && implies(shape_next(yid, zid), copyz);
		noonenext = noonenext && !shape_next(yid, zid);
	}
	// Otherwise we have to find the nearest pointer. That one might or might not be next to x
	for(const auto z : vars){
		if(z == x) continue;
		int zid = z->getid();
		expr znearest(ctxt, Z3_mk_true(ctxt));
		for(const auto w : vars){
			int wid = w->getid();
			znearest = znearest && (!shape_reach(yid, wid) || shape_eq(zid, wid) || shape_next(zid, wid) || shape_reach(zid, wid));
		}
		expr close = Xshape_next(xid, zid) && !Xshape_reach(xid, zid);
		expr distant = !Xshape_next(xid, zid) && Xshape_reach(xid, zid);
		expr exe = implies(shape_reach(yid, zid) && znearest, close || distant) &&
		           implies(shape_reach(yid, zid) && !znearest, distant) &&
		           implies(!shape_reach(yid, zid), !shape_reach(xid, zid));
		exe = exe && (freednext(yid) == (Xfreed(xid) || Xfreednext(xid)));
		result = result && implies(noonenext, exe);
	}
	result = nochange_shape(result, vars, x);
	
	result = nochange_freed(result, vars, x);
	
	
	// ownership loss can be initiated or propagated
	if(x->isglobal()){
		result = result && nuke_ownership(x);
		for(const auto z : vars){
			if(z == x) continue;
			int zid = z->getid();
			
			expr cond = !inv_eq(yid) && !inv_next(yid) && shape_next(yid, zid);
			result = result && implies(cond, nuke_ownership(z));
			result = result && implies(!cond, copy_ownership(z, z));
		}
	}else{
		result = result && nibble_ownership(x, y);
	}
	
	result = pcincrement(result, command);
	result = osupdate(result, command->get_event());
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	result = nochange_sinout(result);
	
	return result;
}

expr View::trans_inputassignment(InputAssignment *command){
	Variable *x = vardown(command->getvar());
	int xid = x->getid();
	int xnextid = x->getnextid();
	
	expr result(ctxt, Z3_mk_true(ctxt));
	
	
	// if observer variable z is watching the input, it becomes a copy of x.
	// if not, it stays as it was.
	for(const auto &z : obsvars){
		int zid = z->getid();
		int znextid = z->getnextid();
		expr change(ctxt, Z3_mk_true(ctxt));
		expr stay(ctxt, Z3_mk_true(ctxt));
		
		
		//ages change
		result = result && (Xages_eq(zid, zid) == ages_eq(xid, xid)) && (Xages_eq(znextid, znextid) == ages_eq(xnextid, xnextid)) && (Xages_eq(zid, znextid) == ages_eq(xid, xnextid)) &&
		                   (Xages_lt(zid, zid) == ages_lt(xid, xid)) && (Xages_lt(znextid, znextid) == ages_lt(xnextid, xnextid)) && (Xages_lt(zid, znextid) == ages_lt(xid, xnextid)) && (Xages_lt(znextid, zid) == ages_lt(xnextid, xid));
		for(const auto &w : vars){
			if(w == z) continue;
			int wid = w->getid();
			int wnextid = w->getnextid();
			result = result && (Xages_eq(zid, wid) == ages_eq(xid, wid)) &&                                              (Xages_eq(zid, wnextid) == ages_eq(xid, wnextid)) &&                                                      (Xages_eq(znextid, wid) == ages_eq(xnextid, wid)) &&                                                      (Xages_eq(znextid, wnextid) == ages_eq(xnextid, wnextid)) &&
			                   (Xages_lt(zid, wid) == ages_lt(xid, wid)) && (Xages_lt(wid, zid) == ages_lt(wid, xid)) && (Xages_lt(zid, wnextid) == ages_lt(xid, wnextid)) && (Xages_lt(wnextid, zid) == ages_lt(wnextid, xid)) && (Xages_lt(znextid, wid) == ages_lt(xnextid, wid)) && (Xages_lt(wid, znextid) == ages_lt(wid, xnextid)) && (Xages_lt(znextid, wnextid) == ages_lt(xnextid, wnextid)) && (Xages_lt(wnextid, znextid) == ages_lt(wnextid, xnextid));
		}
		
		//ages stay
		for(const auto &w : vars){
			int wid = w->getid();
			int wnextid = w->getnextid();
			stay = stay && (Xages_eq(zid, wid) == ages_eq(zid, wid)) &&                                              (Xages_eq(znextid, wid) == ages_eq(znextid, wid)) &&                                                      (Xages_eq(zid, wnextid) == ages_eq(zid, wnextid)) &&                                                      (Xages_eq(znextid, wnextid) == ages_eq(znextid, wnextid)) &&
			               (Xages_lt(zid, wid) == ages_lt(zid, wid)) && (Xages_lt(wid, zid) == ages_lt(wid, zid)) && (Xages_lt(znextid, wid) == ages_lt(znextid, wid)) && (Xages_lt(wid, znextid) == ages_lt(wid, znextid)) && (Xages_lt(zid, wnextid) == ages_lt(zid, wnextid)) && (Xages_lt(wnextid, zid) == ages_lt(wnextid, zid)) && (Xages_lt(znextid, wnextid) == ages_lt(znextid, wnextid)) && (Xages_lt(wnextid, znextid) == ages_lt(wnextid, znextid));
		}
		
		// shape change
		change = change && (Xshape_eq(zid, zid) == shape_eq(xid, xid)) && (Xshape_neq(zid, zid) == shape_neq(xid, xid)) && (Xshape_next(zid, zid) == shape_next(xid, xid)) && (Xshape_reach(zid, zid) == shape_reach(xid, xid));
		for(const auto &w : vars){
			if(w == z) continue;
			int wid = w->getid();
			change = change && (Xshape_eq(zid, wid) == shape_eq(xid, wid)) && (Xshape_neq(zid, wid) == shape_neq(xid, wid)) && (Xshape_next(zid,wid) == shape_next(xid, wid)) && (Xshape_next(wid, zid) == shape_next(wid, xid)) && (Xshape_reach(zid, wid) == shape_reach(xid, wid)) && (Xshape_reach(wid, zid) == shape_reach(wid, xid));
		}
		
		// shape stay
		for(const auto &w : vars){
			int wid = w->getid();
			stay = stay && (Xshape_eq(zid, wid) == shape_eq(zid, wid)) && (Xshape_neq(zid, wid) == shape_neq(zid, wid)) && (Xshape_next(zid, wid) == shape_next(zid, wid)) && (Xshape_next(wid, zid) == shape_next(wid, zid)) && (Xshape_reach(zid, wid) == shape_reach(zid, wid)) && (Xshape_reach(wid, zid) == shape_reach(wid, zid));
		}
		
		// freed change
		change = change && (Xfreed(zid) == freed(xid)) && (Xfreednext(zid) == freednext(xid));
		
		// freed stay
		stay = stay && (Xfreed(zid) == freed(zid)) && (Xfreednext(zid) == freednext(zid));
		
		// inv+sin change
		change = change && (Xinv_eq(zid) == inv_eq(xid)) && (Xinv_next(zid) == inv_next(xid)) && (Xinv_reach(zid) == inv_reach(xid)) &&
		                   (Xsin_eq(zid) == sin_eq(xid)) && (Xsin_next(zid) == sin_next(xid)) && (Xsin_reach(zid) == sin_reach(xid));
		
		// inv+sin stay
		stay = stay && (Xinv_eq(zid) == inv_eq(zid)) && (Xinv_next(zid) == inv_next(zid)) && (Xinv_reach(zid) == inv_reach(zid)) &&
		               (Xsin_eq(zid) == sin_eq(zid)) && (Xsin_next(zid) == sin_next(zid)) && (Xsin_reach(zid) == sin_reach(zid));
		
		
		
		result = result && implies(in(zid), change) && implies(!in(zid), stay);
		result = result && copy_ownership(z, z);
	}
	
	
	
	// all not-observer pointers stay as they were
	result = nochange_own(result, nonobsvars);
	result = nochange_ages(result, nonobsvars);
	result = nochange_shape(result, nonobsvars);
	result = nochange_freed(result, nonobsvars);
	result = nochange_inv(result, nonobsvars);
	result = nochange_sin(result, nonobsvars);
	
	// on some things everything stays as it was
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	result = nochange_sinout(result);
	
	
	result = pcincrement(result, command);
	result = osupdate(result, command->get_event());
	
	return result;
}

expr View::trans_outputassignment(OutputAssignment *command){
	Variable *x = vardown(command->getvar());
	int xid = x->getid();
	
	expr result = (Xsinout == inv_eq(xid));
	for(const auto z : obsvars){
		int zid = z->getid();
		result = result && (Xout(zid) == (shape_eq(xid, zid) && !inv_eq(xid)));
	}
	// sinout2 and out2 handled in inherited function
	
	result = pcincrement(result, command);
	result = osupdate(result, command->get_event());
	result = nochange_freed(result, vars);
	result = nochange_observed_in(result, obsvars);
	result = nochange_shape(result, vars);
	result = nochange_inv(result, vars);
	result = nochange_sin(result, vars);
	result = nochange_ages(result, vars);
	result = nochange_own(result, vars);
	
	return result;
}

expr View::trans_return(Return *command){
	expr result(ctxt, Z3_mk_true(ctxt));
	
	result = pcincrement(result, command);
	result = osupdate(result, command->get_event());
	result = nochange_freed(result, vars);
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	result = nochange_shape(result, vars);
	result = nochange_ages(result, vars);
	result = nochange_inv(result, vars);
	result = nochange_sin(result, vars);
	result = nochange_own(result, vars);
	result = nochange_sinout(result);
	
	return result;
}

expr View::trans_pointercomparison(PointerComparison *command){
	Variable *x = vardown(command->getlhs());
	Variable *y = vardown(command->getrhs());
	int xid = x->getid();
	int yid = y->getid();
	
	expr result(ctxt, Z3_mk_true(ctxt));
	
	
	// we go whereever the comparison leads us
	result = pcincrement(result, command, shape_eq(xid, yid));
	
	// we might lose ownership of variables that point to x or y, if compared to an invalid pointer
	for(const auto z : vars){
		int zid = z->getid();
		expr cond = (shape_eq(xid, zid) && inv_eq(yid)) || (shape_eq(yid, zid) && inv_eq(xid));
		result = result && implies(cond, lose_ownership(z)) && implies(!cond, copy_ownership(z, z));
	}
	
	result = osupdate(result, command->get_event());
	result = nochange_sinout(result);
	result = nochange_freed(result, vars);
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	result = nochange_shape(result, vars);
	result = nochange_inv(result, vars);
	result = nochange_sin(result, vars);
	result = nochange_ages(result, vars);
	
	
	return result;
}

expr View::trans_cas(CAS *command){
	Variable *v = vardown(command->getvictim());
	Variable *c = vardown(command->getcompare());
	Variable *r = vardown(command->getreplace());
	int vid = v->getid();
	int cid = c->getid();
	
	
	expr success = trans_cas_success(v, c, r);
	expr fail = trans_cas_fail(v, c, r);
	
	
	expr condition = shape_eq(vid, cid) && ages_eq(vid, cid);
	// expr condition = shape_eq(vid, cid);
	expr result = implies(condition, success) && implies(!condition, fail);
	result = pcincrement(result, command, condition);
	result = osupdate(result, command->get_event());
	
	return result;
}

expr View::trans_cas_success(Variable *v, Variable *c, Variable *r){
	int vid = v->getid();
	int cid = c->getid();
	int rid = r->getid();
	
	expr result = (Xages_eq(vid, vid) == ages_eq(vid, vid)) && (Xages_lt(vid, vid) == ages_lt(vid, vid));
	for(const auto z : vars){
		if(z == v) continue;
		int zid = z->getid();
		int znextid = z->getnextid();
		// the equal ones get smaller, the smaller ones stay smaller
		result = result && implies(ages_eq(zid, vid) || ages_lt(zid, vid), Xages_lt(zid, vid) && !Xages_lt(vid, zid) && !Xages_eq(vid, zid));
		result = result && implies(ages_eq(znextid, vid) || ages_lt(znextid, vid), Xages_lt(znextid, vid) && !Xages_lt(vid, znextid) && !Xages_eq(znextid, vid));
		// the smallest of the greater ones can get equal, the other greater ones stay greater
		expr zsmallestbig(ctxt, Z3_mk_true(ctxt));
		expr znextsmallestbig(ctxt, Z3_mk_true(ctxt));
		for(const auto w : vars){
			int wid = w->getid();
			int wnextid = w->getnextid();
			zsmallestbig = zsmallestbig && (ages_lt(zid, wid) || ages_eq(zid, wid) || !ages_lt(vid, wid));
			zsmallestbig = zsmallestbig && (ages_lt(zid, wnextid) || ages_eq(zid, wnextid) || !ages_lt(vid, wnextid));
			znextsmallestbig = znextsmallestbig && (ages_lt(znextid, wid) || ages_eq(znextid, wid) || !ages_lt(vid, wid));
			znextsmallestbig = znextsmallestbig && (ages_lt(znextid, wnextid) || ages_eq(znextid, wnextid) || !ages_lt(vid, wnextid));
		}
		expr getequal = Xages_eq(vid, zid) && !Xages_lt(vid, zid) && !Xages_lt(zid, vid);
		expr staygreater = Xages_lt(vid, zid) && !Xages_eq(vid, zid) && !Xages_lt(zid, vid);
		expr nextgetequal = Xages_eq(vid, znextid) && !Xages_lt(vid, znextid) && !Xages_lt(znextid, vid);
		expr nextstaygreater = Xages_lt(vid, znextid) && !Xages_eq(vid, znextid) && !Xages_lt(znextid, vid);
		result = result && implies(ages_lt(vid, zid) && zsmallestbig, getequal || staygreater);
		result = result && implies(ages_lt(vid, zid) && !zsmallestbig, staygreater);
		result = result && implies(ages_lt(vid, znextid) && znextsmallestbig, nextgetequal || nextstaygreater);
		result = result && implies(ages_lt(vid, znextid) && !znextsmallestbig, nextstaygreater);
	}
	result = nochange_ages(result, vars, v);
	
	result = result && (Xfreed(vid) == freed(rid)) && (Xfreednext(vid) == freednext(rid));
	result = nochange_freed(result, vars, v);
	
	result = result && (Xinv_eq(vid) == inv_eq(rid)) && (Xinv_next(vid) == inv_next(rid)) && (Xinv_reach(vid) == inv_reach(rid));
	result = nochange_inv(result, vars, v);
	result = result && (Xsin_eq(vid) == sin_eq(rid)) && (Xsin_next(vid) == sin_next(rid)) && (Xsin_reach(vid) == sin_reach(rid));
	result = nochange_sin(result, vars, v);
	
	if(v->isglobal()){
		for(const auto &z : vars){
			int zid = z->getid();
			expr condinvcmp = (shape_eq(vid, zid) && inv_eq(cid)) || (shape_eq(cid, zid) && inv_eq(vid));
			result = result && implies((!inv_eq(rid) && shape_eq(rid, zid)), nuke_ownership(z)) &&
			                   implies(condinvcmp, lose_ownership(z)) &&
			                   implies((inv_eq(rid) || !shape_eq(rid, zid)) && !condinvcmp, copy_ownership(z, z));
		}
	}else{
		result = result && copy_ownership(r, v);
		for(const auto &z : vars){
			if(z == r) continue;
			int zid = z->getid();
			expr cond = (shape_eq(vid, zid) && inv_eq(cid)) || (shape_eq(cid, zid) && inv_eq(vid));
			result = result && implies(cond, lose_ownership(z)) && implies(!cond, copy_ownership(z, z));
		}
	}
	
	result = result && (Xshape_eq(vid, vid) == shape_eq(rid, rid)) && (Xshape_neq(vid, vid) == shape_neq(rid, rid)) && (Xshape_next(vid, vid) == shape_next(rid, rid)) && (Xshape_reach(vid, vid) == shape_reach(rid, rid));
	for(const auto &z : vars){
		if(z == v) continue;
		int zid = z->getid();
		result = result && (Xshape_eq(vid, zid) == shape_eq(rid, zid)) && (Xshape_neq(vid, zid) == shape_neq(rid, zid)) && (Xshape_next(vid, zid) == shape_next(rid, zid)) && (Xshape_next(zid, vid) == shape_next(zid, rid)) && (Xshape_reach(vid, zid) == shape_reach(rid, zid)) && (Xshape_reach(zid, vid) == shape_reach(zid, rid));
	}
	result = nochange_shape(result, vars, v);
	
	result = nochange_sinout(result);
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	
	
	return result;
}

expr View::trans_cas_fail(Variable *v, Variable *c, Variable *r){
	expr result(ctxt, Z3_mk_true(ctxt));
	
	int vid = v->getid();
	int cid = c->getid();
	
	// we might lose ownership of variables that point to v or c, if compared to an invalid pointer
	for(const auto &z : vars){
		int zid = z->getid();
		expr cond = (shape_eq(vid, zid) && inv_eq(cid)) || (shape_eq(cid, zid) && inv_eq(vid));
		result = result && implies(cond, lose_ownership(z)) && implies(!cond, copy_ownership(z, z));
	}
	
	result = nochange_sinout(result);
	result = nochange_freed(result, vars);
	result = nochange_observed_in(result, obsvars);
	result = nochange_observed_out(result, obsvars);
	result = nochange_shape(result, vars);
	result = nochange_inv(result, vars);
	result = nochange_sin(result, vars);
	result = nochange_ages(result, vars);
	
	return result;
}

std::vector<int> View::get_variables_single(){
	return std::vector<int>({sinoutid});
}

std::vector<Z3VariableContainer1D*> View::get_variables_1did(){
	return std::vector<Z3VariableContainer1D*>({&pc, &os});
}

std::vector<Z3VariableContainer1D*> View::get_variables_1dvar(){
	return std::vector<Z3VariableContainer1D*>({&own, &freed, &freednext, &inv_eq, &inv_next, &inv_reach, &sin_eq, &sin_next, &sin_reach});
}

std::vector<Z3VariableContainer1D*> View::get_variables_1dobsvar(){
	return std::vector<Z3VariableContainer1D*>({&in, &out, &inseen});
}

std::vector<Z3VariableContainer2D*> View::get_variables_2dvar(){
	return std::vector<Z3VariableContainer2D*>({&shape_eq, &shape_neq, &shape_next, &shape_reach});
}

std::vector<Z3VariableContainer2D*> View::get_variables_2dnextvar(){
	return std::vector<Z3VariableContainer2D*>({&ages_eq, &ages_lt});
}





OneThreadView::OneThreadView(Program *prog, Observer *obs, context &ctxt, std::vector<Variable*> nonobsvars, std::vector<Variable*> obsvars, std::vector<Variable*> vars, std::vector<Variable*> activelocalscope, std::vector<Variable*> activenotlocalscope):
	View(prog, obs, ctxt, nonobsvars, obsvars, vars, activelocalscope, activenotlocalscope),
	initial(ctxt),
	safety(ctxt)
{
}

Variable *OneThreadView::vardown(Variable *var){
	return var;
}

Variable *OneThreadView::varup(Variable *var){
	return var;
}

expr OneThreadView::nochange_own(expr &f, std::vector<Variable*> &v, Variable *except){
	for(const auto &z : v){
		if(z == except) continue;
		int zid = z->getid();
		f = f && (Xown(zid) == own(zid));
	}
	return f;
}

expr OneThreadView::claim_ownership(Variable *x){
	int xid = x->getid();
	return Xown(xid);
}

expr OneThreadView::lose_ownership(Variable *x){
	int xid = x->getid();
	return !Xown(xid);
}

expr OneThreadView::nuke_ownership(Variable *x){
	int xid = x->getid();
	return !Xown(xid);
}

expr OneThreadView::copy_ownership(Variable *source, Variable *dest){
	int sid = source->getid();
	int did = dest->getid();
	return (Xown(did) == own(sid));
}

expr OneThreadView::nibble_ownership(Variable *nibbler, Variable *pointer){
	int nid = nibbler->getid();
	int pid = pointer->getid();
	
	expr cond = !own(pid) && !inv_eq(pid) && !inv_next(pid);
	expr result = implies(cond, !Xown(nid));
	result = result && implies(!cond, Xown(nid) == own(pid));
	
	for(const auto z : vars){
		if(z == nibbler) continue;
		int zid = z->getid();
		
		expr zcond = cond && shape_next(pid, zid);
		result = result && implies(zcond, !Xown(zid));
		result = result && implies(!zcond, Xown(zid) == own(zid));
	}
	
	return result;
}

expr OneThreadView::nochange_seen(Variable *except){
	expr result(ctxt, Z3_mk_true(ctxt));
	for(const auto z : obsvars){
		if(z == except) continue;
		int zid = z->getid();
		result = result && (Xinseen(zid) == inseen(zid));
	}
	return result;
}

expr OneThreadView::input_seen(Variable *z){
	int zid = z->getid();
	return inseen(zid);
}

expr OneThreadView::see_input(Variable *z){
	return Xinseen(z->getid());
}

void OneThreadView::build_safety(){
	// never go to the accepting state
	Clause c(ctxt, old_variables.size());
	c.set_variable(os(autc-1), os.getid(autc-1), CLAUSE_NEGATIVE);
	safety.add_clause(c, false);
	
	// strong pointer races
	for(auto next : program->get_code()){
		int pcid = next->getpc();
		expression_subtype st = next->get_subtype();
		
		if(st == MALLOC){
			// no SPR
		}else if(st == FREE){
			Variable *x = ((Free*)next)->getvar();
			int xid = x->getid();
			
			// SPR iff x is invalid and has a data binding
			for(auto z : obsvars){
				int zid = z->getid();
				Clause c(ctxt, old_variables.size());
				c.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
				c.set_variable(inv_eq(xid), inv_eq.getid(xid), CLAUSE_NEGATIVE);
				c.set_variable(shape_eq(xid, zid), shape_eq.getid(xid, zid), CLAUSE_NEGATIVE);
				safety.add_clause(c, false);
			}
			/*/
			// SPR iff x is invalid
			Clause c(ctxt, old_variables.size());
			c.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c.set_variable(inv_eq(xid), inv_eq.getid(xid), CLAUSE_NEGATIVE);
			safety.add_clause(c);
			//*/
		}else if(st == POINTERASSIGNMENT){
			// no SPR
		}else if(st == NEXTPOINTERASSIGNMENT){
			Variable *x = ((NextPointerAssignment*)next)->getlhs();
			int xid = x->getid();
			// SPR iff x is invalid or strongly invalid
			Clause c1(ctxt, old_variables.size());
			c1.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c1.set_variable(inv_eq(xid), inv_eq.getid(xid), CLAUSE_NEGATIVE);
			Clause c2(ctxt, old_variables.size());
			c2.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c2.set_variable(sin_eq(xid), sin_eq.getid(xid), CLAUSE_NEGATIVE);
			safety.add_clause(c1, false);
			safety.add_clause(c2, false);
		}else if(st == POINTERNEXTASSIGNMENT){
			Variable *y = ((PointerNextAssignment*)next)->getrhs();
			int yid = y->getid();
			// SPR iff y is strongly invalid
			Clause c(ctxt, old_variables.size());
			c.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c.set_variable(sin_eq(yid), sin_eq.getid(yid), CLAUSE_NEGATIVE);
			safety.add_clause(c, false);
		}else if(st == INPUTASSIGNMENT){
			Variable *x = ((IOAssignment*)next)->getvar();
			int xid = x->getid();
			// SPR iff x is invalid or strongly invalid
			Clause c1(ctxt, old_variables.size());
			c1.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c1.set_variable(inv_eq(xid), inv_eq.getid(xid), CLAUSE_NEGATIVE);
			Clause c2(ctxt, old_variables.size());
			c2.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c2.set_variable(sin_eq(xid), sin_eq.getid(xid), CLAUSE_NEGATIVE);
			safety.add_clause(c1, false);
			safety.add_clause(c2, false);
		}else if(st == OUTPUTASSIGNMENT){
			Variable *x = ((IOAssignment*)next)->getvar();
			int xid = x->getid();
			// SPR iff x is strongly invalid
			Clause c(ctxt, old_variables.size());
			c.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c.set_variable(sin_eq(xid), sin_eq.getid(xid), CLAUSE_NEGATIVE);
			safety.add_clause(c, false);
		}else if(st == RETURN){
			// no SPR
		}else if(st == POINTERCOMPARISON || st == CANDS){
			Variable *x = ((PointerComparison*)next)->getlhs();
			Variable *y = ((PointerComparison*)next)->getrhs();
			int xid = x->getid();
			int yid = y->getid();
			// SPR iff x or y are strongly invalid
			Clause c1(ctxt, old_variables.size());
			c1.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c1.set_variable(sin_eq(xid), sin_eq.getid(xid), CLAUSE_NEGATIVE);
			Clause c2(ctxt, old_variables.size());
			c2.set_variable(pc(pcid), pc.getid(pcid), CLAUSE_NEGATIVE);
			c2.set_variable(sin_eq(yid), sin_eq.getid(yid), CLAUSE_NEGATIVE);
			safety.add_clause(c1, false);
			safety.add_clause(c2, false);
		}
	}
	
	
	/*
	here are some constraints that are always valid by initiation and
	transition but we should include them here to exclude them from
	(unsuccessful but time-consuming) counterexample finding
	*/
	
	// never be at two pcs at the same time
	for(int i=0; i<loc; i++){
		for(int j=i+1; j<loc; j++){
			Clause c(ctxt, old_variables.size());
			c.set_variable(pc(i), pc.getid(i), CLAUSE_NEGATIVE);
			c.set_variable(pc(j), pc.getid(j), CLAUSE_NEGATIVE);
			safety.add_clause(c, false);
		}
	}
	
	// never be at two observer states at the same time but always at one
	Clause oneobs(ctxt, old_variables.size());
	for(int i=0; i<autc; i++){
		oneobs.set_variable(os(i), os.getid(i), CLAUSE_POSITIVE);
		for(int j=i+1; j<autc; j++){
			Clause c(ctxt, old_variables.size());
			c.set_variable(os(i), os.getid(i), CLAUSE_NEGATIVE);
			c.set_variable(os(j), os.getid(j), CLAUSE_NEGATIVE);
			safety.add_clause(c, false);
		}
	}
	safety.add_clause(oneobs);
	
	// input and output cannot be watched by more than one variable
	for(const auto z1 : obsvars){
		int z1id = z1->getid();
		for(const auto z2 : obsvars){
			if(z2 == z1) continue;
			int z2id = z2->getid();
			
			Clause i(ctxt, old_variables.size());
			i.set_variable(in(z1id), in.getid(z1id), CLAUSE_NEGATIVE);
			i.set_variable(in(z2id), in.getid(z2id), CLAUSE_NEGATIVE);
			safety.add_clause(i);
			
			Clause o(ctxt, old_variables.size());
			o.set_variable(out(z1id), out.getid(z1id), CLAUSE_NEGATIVE);
			o.set_variable(out(z2id), out.getid(z2id), CLAUSE_NEGATIVE);
			safety.add_clause(o);
		}
	}
	
	// consistency
	shapeconsistency(safety);
	memconsistency(safety);
	// */
}

Frame &OneThreadView::get_initial(){
	return initial;
}

Frame &OneThreadView::get_safety(){
	return safety;
}





TwoThreadView::TwoThreadView(Program *prog, Observer *obs, context &ctxt, std::vector<Variable*> nonobsvars, std::vector<Variable*> obsvars, std::vector<Variable*> vars, std::vector<Variable*> activelocalscope, std::vector<Variable*> activenotlocalscope, std::vector<Variable*> passivelocalscope, std::vector<Variable*> passivenotlocalscope):
	View(prog, obs, ctxt, nonobsvars, obsvars, vars, activelocalscope, activenotlocalscope),
	passivelocalscope(passivelocalscope),
	passivenotlocalscope(passivenotlocalscope),
	pc2(ctxt, loc, "pc2", old_variables),
	own2(ctxt, varc, "own2", old_variables, &vars),
	in2(ctxt, obsc, "in2", old_variables, &vars),
	out2(ctxt, obsc, "out2", old_variables, &vars),
	inseen2(ctxt, obsc, "seen2", old_variables, &vars),
	sinout2(ctxt.bool_const("sinout2")),
	
	Xpc2(ctxt, loc, "Xpc2", new_variables),
	Xown2(ctxt, varc, "Xown2", new_variables, &vars),
	Xin2(ctxt, obsc, "Xin2", new_variables, &vars),
	Xout2(ctxt, obsc, "Xout2", new_variables, &vars),
	Xinseen2(ctxt, obsc, "Xseen2", new_variables, &vars),
	Xsinout2(ctxt.bool_const("Xsinout2")),
	
	XPC2(),
	intconstraints(ctxt)
{
	sinout2id = old_variables.size();
	old_variables.push_back(sinout2);
	Xsinout2id = new_variables.size();
	new_variables.push_back(Xsinout2);
	
	XPC2.reserve(loc+1);
	for(int i=0; i<loc+1; i++){
		expr f(ctxt, Z3_mk_true(ctxt));
		if(i < loc){
			f = Xpc2(i);
		}
		for(int j=0; j<loc; j++){
			if(i == j) continue;
			f = f && !Xpc2(j);
		}
		XPC2.push_back(f);
	}
}

Variable *TwoThreadView::vardown(Variable *var){
	if(var == NULL) return NULL;
	return var->get_twin(1);
}

Variable *TwoThreadView::varup(Variable *var){
	if(var == NULL) return NULL;
	return var->get_parent();
}

void TwoThreadView::build_intconstraints(){
	// Data Independence
	for(const auto z : obsvars){
		int zid = z->getid();
		
		Clause input(ctxt, old_variables.size());
		input.set_variable(in(zid), in.getid(zid), CLAUSE_NEGATIVE);
		input.set_variable(in2(zid), in2.getid(zid), CLAUSE_NEGATIVE);
		
		Clause output(ctxt, old_variables.size());
		output.set_variable(out(zid), out.getid(zid), CLAUSE_NEGATIVE);
		output.set_variable(out2(zid), out2.getid(zid), CLAUSE_NEGATIVE);
		
		intconstraints.add_clause(input);
		intconstraints.add_clause(output);
	}
	
	// Shape cf. p47
	// consistency (C1)
	shapeconsistency(intconstraints);
	// relations of locals (C6n)
	for(const auto x : activelocalscope){
		int xid = x->getid();
		for(const auto y : passivelocalscope){
			int yid = y->getid();
			
			Clause constr_xeq(ctxt, old_variables.size());
			constr_xeq.set_variable(own(xid), own.getid(xid), CLAUSE_NEGATIVE);
			constr_xeq.set_variable(shape_eq(xid, yid), shape_eq.getid(xid, yid), CLAUSE_NEGATIVE);
			constr_xeq.set_variable(inv_eq(yid), inv_eq.getid(yid), CLAUSE_POSITIVE);
			constr_xeq.set_variable(sin_eq(yid), sin_eq.getid(yid), CLAUSE_POSITIVE);
			
			Clause constr_xnext(ctxt, old_variables.size());
			constr_xnext.set_variable(own(xid), own.getid(xid), CLAUSE_NEGATIVE);
			constr_xnext.set_variable(shape_next(yid, xid), shape_next.getid(yid, xid), CLAUSE_NEGATIVE);
			constr_xnext.set_variable(inv_next(yid), inv_next.getid(yid), CLAUSE_POSITIVE);
			constr_xnext.set_variable(sin_next(yid), sin_next.getid(yid), CLAUSE_POSITIVE);
			
			Clause constr_xreach(ctxt, old_variables.size());
			constr_xreach.set_variable(own(xid), own.getid(xid), CLAUSE_NEGATIVE);
			constr_xreach.set_variable(shape_reach(yid, xid), shape_reach.getid(yid, xid), CLAUSE_NEGATIVE);
			constr_xreach.set_variable(inv_reach(yid), inv_reach.getid(yid), CLAUSE_POSITIVE);
			constr_xreach.set_variable(sin_reach(yid), sin_reach.getid(yid), CLAUSE_POSITIVE);
			
			Clause constr_yeq(ctxt, old_variables.size());
			constr_yeq.set_variable(own2(yid), own2.getid(yid), CLAUSE_NEGATIVE);
			constr_yeq.set_variable(shape_eq(yid, xid), shape_eq.getid(yid, xid), CLAUSE_NEGATIVE);
			constr_yeq.set_variable(inv_eq(xid), inv_eq.getid(xid), CLAUSE_POSITIVE);
			constr_yeq.set_variable(sin_eq(xid), sin_eq.getid(xid), CLAUSE_POSITIVE);
			
			Clause constr_ynext(ctxt, old_variables.size());
			constr_ynext.set_variable(own2(yid), own2.getid(yid), CLAUSE_NEGATIVE);
			constr_ynext.set_variable(shape_next(xid, yid), shape_next.getid(xid, yid), CLAUSE_NEGATIVE);
			constr_ynext.set_variable(inv_next(xid), inv_next.getid(xid), CLAUSE_POSITIVE);
			constr_ynext.set_variable(sin_next(xid), sin_next.getid(xid), CLAUSE_POSITIVE);
			
			Clause constr_yreach(ctxt, old_variables.size());
			constr_yreach.set_variable(own2(yid), own2.getid(yid), CLAUSE_NEGATIVE);
			constr_yreach.set_variable(shape_reach(xid, yid), shape_reach.getid(xid, yid), CLAUSE_NEGATIVE);
			constr_yreach.set_variable(inv_reach(xid), inv_reach.getid(xid), CLAUSE_POSITIVE);
			constr_yreach.set_variable(sin_reach(xid), sin_reach.getid(xid), CLAUSE_POSITIVE);
			
			intconstraints.add_clause(constr_xeq);
			intconstraints.add_clause(constr_xnext);
			intconstraints.add_clause(constr_xreach);
			intconstraints.add_clause(constr_yeq);
			intconstraints.add_clause(constr_ynext);
			intconstraints.add_clause(constr_yreach);
		}
	}
	
	memconsistency(intconstraints);
}

Frame &TwoThreadView::get_intconstraints(){
	return intconstraints;
}

expr TwoThreadView::pcincrement(expr &f, Statement *command){
	// in addition to incrementing pc1, pc2 must stay as it was
	f = View::pcincrement(f, command);
	for(int i=0; i<loc; i++){
		f = f && (pc2(i) == Xpc2(i));
	}
	return f;
}

expr TwoThreadView::pcincrement(expr &f, Condition *command, expr cond){
	// in addition to redirecting pc1, pc2 must stay as it was
	f = View::pcincrement(f, command, cond);
	for(int i=0; i<loc; i++){
		f = f && (pc2(i) == Xpc2(i));
	}
	return f;
}

expr TwoThreadView::nochange_sinout(expr &f){
	f = View::nochange_sinout(f);
	f = f && (Xsinout2 == sinout2);
	return f;
}

expr TwoThreadView::nochange_observed_in(expr &f, std::vector<Variable*> &v){
	f = View::nochange_observed_in(f, v);
	for(const auto &z : v){
		int zid = z->getid();
		f = f && (Xin2(zid) == in2(zid));
	}
	return f;
}

expr TwoThreadView::nochange_observed_out(expr &f, std::vector<Variable*> &v){
	f = View::nochange_observed_out(f, v);
	for(const auto &z : v){
		int zid = z->getid();
		f = f && (Xout2(zid) == out2(zid));
	}
	return f;
}

expr TwoThreadView::nochange_own(expr &f, std::vector<Variable*> &v, Variable *except){
	for(const auto &z : v){
		if(z == except) continue;
		int zid = z->getid();
		f = f && (Xown(zid) == own(zid)) && (Xown2(zid) == own2(zid));
	}
	return f;
}

expr TwoThreadView::claim_ownership(Variable *x){
	int xid = x->getid();
	return Xown(xid) && !Xown2(xid);
}

expr TwoThreadView::lose_ownership(Variable *x){
	int xid = x->getid();
	return !Xown(xid) && (Xown2(xid) == own2(xid));
}

expr TwoThreadView::nuke_ownership(Variable *x){
	int xid = x->getid();
	return !Xown(xid) && !Xown2(xid);
}

expr TwoThreadView::copy_ownership(Variable *source, Variable *dest){
	int sid = source->getid();
	int did = dest->getid();
	return (Xown(did) == own(sid)) && (Xown2(did) == own2(sid));
}

expr TwoThreadView::nibble_ownership(Variable *nibbler, Variable *pointer){
	int nid = nibbler->getid();
	int pid = pointer->getid();
	
	expr cond1 = !own(pid) && !inv_eq(pid) && !inv_next(pid);
	expr result = implies(cond1, !Xown(nid));
	result = result && implies(!cond1, Xown(nid) == own(pid));
	
	expr cond2 = !own2(pid) && !inv_eq(pid) && !inv_next(pid);
	result = result && implies(cond2, !Xown2(nid));
	result = result && implies(!cond2, Xown2(nid) == own2(pid));
	
	for(const auto z : vars){
		if(z == nibbler) continue;
		int zid = z->getid();
		
		expr zcond1 = cond1 && shape_next(pid, zid);
		result = result && implies(zcond1, !Xown(zid));
		result = result && implies(!zcond1, Xown(zid) == own(zid));
		
		expr zcond2 = cond2 && shape_next(pid, zid);
		result = result && implies(zcond2, !Xown2(zid));
		result = result && implies(!zcond2, Xown2(zid) == own2(zid));
	}
	
	return result;
}

expr TwoThreadView::nochange_seen(Variable *except){
	expr result(ctxt, Z3_mk_true(ctxt));
	for(const auto z : obsvars){
		if(z == except) continue;
		int zid = z->getid();
		result = result && (Xinseen(zid) == (inseen(zid) || inseen2(zid))) && (Xinseen2(zid) == (inseen(zid) || inseen2(zid)));
	}
	return result;
}

expr TwoThreadView::input_seen(Variable *z){
	int zid = z->getid();
	return inseen(zid) || inseen2(zid);
}

expr TwoThreadView::see_input(Variable *z){
	int zid = z->getid();
	return Xinseen(zid) && Xinseen2(zid);
}

void TwoThreadView::memconsistency(Frame &frame){
	View::memconsistency(frame);
	
	for(const auto x : vars){
		int xid = x->getid();
		
		if(x->isobserver()){
			Clause gowner(ctxt, old_variables.size());
			gowner.set_variable(own2(xid), own2.getid(xid), CLAUSE_NEGATIVE);
			frame.add_clause(gowner);
		}else if(x->isglobal()){
			Clause gowner(ctxt, old_variables.size());
			gowner.set_variable(own2(xid), own2.getid(xid), CLAUSE_NEGATIVE);
			gowner.set_variable(inv_eq(xid), inv_eq.getid(xid), CLAUSE_POSITIVE);
			frame.add_clause(gowner);
		}
		
		Clause doubleown(ctxt, old_variables.size());
		doubleown.set_variable(own(xid), own.getid(xid), CLAUSE_NEGATIVE);
		doubleown.set_variable(own2(xid), own2.getid(xid), CLAUSE_NEGATIVE);
		frame.add_clause(doubleown);
	}
}

expr TwoThreadView::trans_functioncall(){
	expr result = View::trans_functioncall();
	
	result = result && (Xsinout2 == sinout2);
	for(const auto z : obsvars){
		int zid = z->getid();
		result = result && (Xout2(zid) == out2(zid)) && (Xin2(zid) == in2(zid));
	}
	for(int i=0; i<loc; i++){
		result = result && (Xpc2(i) == pc2(i));
	}
	
	return result;
}

expr TwoThreadView::trans_outputassignment(OutputAssignment *command){
	expr result = View::trans_outputassignment(command);
	
	result = result && (Xsinout2 == sinout2);
	for(const auto z : obsvars){
		int zid = z->getid();
		result = result && (Xout2(zid) == out2(zid));
	}
	
	return result;
}

std::vector<int> TwoThreadView::get_variables2_single(){
	return std::vector<int>({sinout2id});
}

std::vector<Z3VariableContainer1D*> TwoThreadView::get_variables2_1did(){
	return std::vector<Z3VariableContainer1D*>({&pc2, &os});
}

std::vector<Z3VariableContainer1D*> TwoThreadView::get_variables2_1dvar(){
	return std::vector<Z3VariableContainer1D*>({&own2, &freed, &freednext, &inv_eq, &inv_next, &inv_reach, &sin_eq, &sin_next, &sin_reach});
}

std::vector<Z3VariableContainer1D*> TwoThreadView::get_variables2_1dobsvar(){
	return std::vector<Z3VariableContainer1D*>({&in2, &out2, &inseen2});
}

std::vector<Z3VariableContainer2D*> TwoThreadView::get_variables2_2dvar(){
	return std::vector<Z3VariableContainer2D*>({&shape_eq, &shape_neq, &shape_next, &shape_reach});
}

std::vector<Z3VariableContainer2D*> TwoThreadView::get_variables2_2dnextvar(){
	return std::vector<Z3VariableContainer2D*>({&ages_eq, &ages_lt});
}


