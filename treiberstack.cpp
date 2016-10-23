#include "variable.hpp"
#include "ast.hpp"
#include "program.hpp"
#include "observer.hpp"
#include "stackobserver.hpp"
#include "view.hpp"


/*
global NULL, ToS;
local top, node;

function init(){
	ToS = NULL;
}

function push(){
	node = malloc();
	node.data = _in_;
	do{
		top = ToS;
		node.next = top;
	}while(!CAS(ToS, top, node));  *** IF (ToS === top) in(node.data) ***
}

function pop(){
	do{
		top = ToS  *** IF (TOS == NULL) out(/) ***
		if(top == NULL){
			return;
		}
		node = top.next;
	}while(!CAS(ToS, top, node));  *** IF (ToS === top) out(top.data) ***
	_out_ = top.data;
	free(top);
}
*/




Program *treiberstack(){
	Variable *null = new SingleVariable("NULL", true, false);
	Variable *tos = new SingleVariable("ToS", true, false);
	Variable *node = new SingleVariable("node", false, false);
	Variable *top = new SingleVariable("top", false, false);
	
	
	Function *init = new Function(
		"init",
		false,
		new Sequence(std::vector<Block*>({
			new PointerAssignment(tos, null)
		}))
	);
	
	
	Function *push = new Function(
		"push",
		true,
		new Sequence(std::vector<Block*>({
			new Malloc(node),
			new InputAssignment(node),
			new Footloop(
				new CAS(tos, top, node, true, new LinearisationEvent(IN_PACMP, node, tos, top)),
				new Sequence(std::vector<Block*>({
					new PointerAssignment(top, tos),
					new NextPointerAssignment(node, top)
				}))
				
			)
		}))
	);
	
	
	Function *pop = new Function(
		"pop",
		false,
		new Sequence(std::vector<Block*>({
			new Footloop(
				new CAS(tos, top, node, true, new LinearisationEvent(OUT_PACMP, top, tos, top)),
				new Sequence(std::vector<Block*>({
					new PointerAssignment(top, tos, new LinearisationEvent(OUT_PCMP, NULL, tos, null)),
					new IfThen(
						new PointerComparison(top, null, false),
						new Sequence(std::vector<Block*>({
							new Return()
						}))
					),
					new PointerNextAssignment(node, top)
				}))
			),
			new OutputAssignment(top),
			new Free(top)
		}))
	);
	
	
	
	Program *stack = new Program(
		"Treiber's Stack",
		std::vector<Variable*>({null, tos, node, top}),
		init,
		std::vector<Function*>({push, pop})
	);
	
	return stack;
}
	

Program *testprogram(){
	return treiberstack();
}

Observer *testobserver(){
	return stackobserver();
}

void OneThreadView::build_initial(){
	int tosid = 5, nullid = 4;
	
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







