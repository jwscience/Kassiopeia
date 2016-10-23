#include "variable.hpp"
#include "ast.hpp"
#include "program.hpp"
#include "observer.hpp"
#include "view.hpp"


/*
global NULL, flag;
local fool;

function init(){
	flag = NULL;
}

function cool(){
	do{
		flag = fool;
	}while(flag == fool);
	fool = NULL;
	fool.next = NULL;
}
*/




Program *testprogram(){
	Variable *null = new SingleVariable("NULL", true, false);
	Variable *flag = new SingleVariable("flag", true, false);
	Variable *fool = new SingleVariable("fool", false, false);
	
	
	Function *init = new Function(
		"init",
		false,
		new Sequence(std::vector<Block*>({
			new PointerAssignment(flag, null)
		}))
	);
	
	Function *cool = new Function(
		"cool",
		false,
		new Sequence(std::vector<Block*>({
			new Footloop(
				new PointerComparison(flag, fool, false),
				new Sequence(std::vector<Block*>({
					new PointerAssignment(flag, fool)
				}))
			),
			new PointerAssignment(fool, null),
			new NextPointerAssignment(fool, null)
		}))
	);
	
	Program *mad = new Program(
		"Concurrent madness",
		std::vector<Variable*>({null, flag, fool}),
		init,
		std::vector<Function*>({cool})
	);
	
	return mad;
}



Observer *testobserver(){
	// observer with two states, no variables and no transitions
	Observer *dum = new Observer(std::vector<Variable*>({}), 2);
	return dum;
}


void OneThreadView::build_initial(){
	int tosid = 1, nullid = 0;
	
	// We start in Nirvana
	for(int i=0; i<loc; i++){
		Clause c(ctxt, old_variables.size());
		clausevar pcval = CLAUSE_NEGATIVE;
		c.set_variable(pc(i), pc.getid(i), pcval);
		initial.add_clause(c);
	}
	for(int i=0; i<autc; i++){
		Clause c(ctxt, old_variables.size());
		clausevar osval = (i == 0) ? CLAUSE_POSITIVE : CLAUSE_NEGATIVE;
		c.set_variable(os(i), os.getid(i), osval);
		initial.add_clause(c);
	}
	
	// no shape relations
	for(const auto &i : vars){
		int iid = i->getid();
		for(const auto &j : vars){
			int jid = j->getid();
			
			if((iid == tosid && jid == nullid) || (iid == nullid && jid == tosid)) continue;
			
			clausevar eqval = (i == j ? CLAUSE_POSITIVE : CLAUSE_NEGATIVE);
			clausevar neqval = (i == j ? CLAUSE_NEGATIVE : CLAUSE_POSITIVE);
			
			Clause eq(ctxt, old_variables.size());
			eq.set_variable(shape_eq(iid, jid), shape_eq.getid(iid, jid), eqval);
			initial.add_clause(eq);
			
			Clause neq(ctxt, old_variables.size());
			neq.set_variable(shape_neq(iid, jid), shape_neq.getid(iid, jid), neqval);
			initial.add_clause(neq);
			
			Clause next(ctxt, old_variables.size());
			next.set_variable(shape_next(iid, jid), shape_next.getid(iid, jid), CLAUSE_NEGATIVE);
			initial.add_clause(next);
			
			Clause reach(ctxt, old_variables.size());
			reach.set_variable(shape_reach(iid, jid), shape_reach.getid(iid, jid), CLAUSE_NEGATIVE);
			initial.add_clause(reach);
		}
	}
	
	// nobody freed, nobody owned
	for(const auto i : vars){
		int iid = i->getid();
		
		Clause self(ctxt, old_variables.size());
		self.set_variable(freed(iid), freed.getid(iid), CLAUSE_NEGATIVE);
		initial.add_clause(self);
		
		Clause next(ctxt, old_variables.size());
		next.set_variable(freednext(iid), freednext.getid(iid), CLAUSE_NEGATIVE);
		initial.add_clause(next);
		
		Clause owner(ctxt, old_variables.size());
		owner.set_variable(own(iid), own.getid(iid), CLAUSE_NEGATIVE);
		initial.add_clause(owner);
	}
	
	// everybody invalid, nobody strongly invalid
	for(const auto i : vars){
		int iid = i->getid();
		
		Clause invalid_eq(ctxt, old_variables.size());
		invalid_eq.set_variable(inv_eq(iid), inv_eq.getid(iid), CLAUSE_POSITIVE);
		initial.add_clause(invalid_eq);
		
		Clause invalid_next(ctxt, old_variables.size());
		invalid_next.set_variable(inv_next(iid), inv_next.getid(iid), CLAUSE_POSITIVE);
		initial.add_clause(invalid_next);
		
		Clause invalid_reach(ctxt, old_variables.size());
		invalid_reach.set_variable(inv_reach(iid), inv_reach.getid(iid), CLAUSE_POSITIVE);
		initial.add_clause(invalid_reach);
		
		Clause sinvalid_eq(ctxt, old_variables.size());
		sinvalid_eq.set_variable(sin_eq(iid), sin_eq.getid(iid), CLAUSE_NEGATIVE);
		initial.add_clause(sinvalid_eq);
		
		Clause sinvalid_next(ctxt, old_variables.size());
		sinvalid_next.set_variable(sin_next(iid), sin_next.getid(iid), CLAUSE_NEGATIVE);
		initial.add_clause(sinvalid_next);
		
		Clause sinvalid_reach(ctxt, old_variables.size());
		sinvalid_reach.set_variable(sin_reach(iid), sin_reach.getid(iid), CLAUSE_NEGATIVE);
		initial.add_clause(sinvalid_reach);
	}
	
	// Nobody input, nobody output and I haven't seen anything
	for(const auto z : obsvars){
		int zid = z->getid();
		
		Clause i(ctxt, old_variables.size());
		i.set_variable(in(zid), in.getid(zid), CLAUSE_NEGATIVE);
		initial.add_clause(i);
		
		Clause o(ctxt, old_variables.size());
		o.set_variable(out(zid), out.getid(zid), CLAUSE_NEGATIVE);
		initial.add_clause(o);
		
		Clause c(ctxt, old_variables.size());
		c.set_variable(inseen(zid), inseen.getid(zid), CLAUSE_NEGATIVE);
		initial.add_clause(c);
	}
	
	Clause i1(ctxt, old_variables.size());
	i1.set_variable(shape_eq(tosid, nullid), shape_eq.getid(tosid, nullid), CLAUSE_POSITIVE);
	initial.add_clause(i1);
	
	Clause i2(ctxt, old_variables.size());
	i2.set_variable(shape_neq(tosid, nullid), shape_neq.getid(tosid, nullid), CLAUSE_NEGATIVE);
	initial.add_clause(i2);
	
	Clause i6(ctxt, old_variables.size());
	i6.set_variable(shape_next(tosid, nullid), shape_next.getid(tosid, nullid), CLAUSE_NEGATIVE);
	initial.add_clause(i6);
	
	Clause i7(ctxt, old_variables.size());
	i7.set_variable(shape_next(nullid, tosid), shape_next.getid(nullid, tosid), CLAUSE_NEGATIVE);
	initial.add_clause(i7);
	
	Clause i8(ctxt, old_variables.size());
	i8.set_variable(shape_reach(tosid, nullid), shape_reach.getid(tosid, nullid), CLAUSE_NEGATIVE);
	initial.add_clause(i8);
	
	Clause i9(ctxt, old_variables.size());
	i9.set_variable(shape_reach(nullid, tosid), shape_reach.getid(nullid, tosid), CLAUSE_NEGATIVE);
	initial.add_clause(i9);
	
	Clause i3(ctxt, old_variables.size());
	i3.set_variable(ages_eq(tosid, nullid), ages_eq.getid(tosid, nullid), CLAUSE_POSITIVE);
	initial.add_clause(i3);
	
	Clause i4(ctxt, old_variables.size());
	i4.set_variable(ages_lt(tosid, nullid), ages_lt.getid(tosid, nullid), CLAUSE_NEGATIVE);
	initial.add_clause(i4);
	
	Clause i5(ctxt, old_variables.size());
	i5.set_variable(ages_lt(tosid, nullid), ages_lt.getid(tosid, nullid), CLAUSE_NEGATIVE);
	initial.add_clause(i5);
}










