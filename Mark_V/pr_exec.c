/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2009-2014 Baker and others

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

typedef struct
{
	int				s;
	dfunction_t		*f;
} prstack_t;

#define	MAX_STACK_DEPTH		32
static prstack_t	pr_stack[MAX_STACK_DEPTH];
static int			pr_depth;

#define	LOCALSTACK_SIZE		2048
static int			localstack[LOCALSTACK_SIZE];
static int			localstack_used;

cbool	pr_trace;
dfunction_t	*pr_xfunction;
int			pr_xstatement;
int		pr_argc;

static const char *pr_opnames[] =
{
"DONE",

"MUL_F",
"MUL_V",
"MUL_FV",
"MUL_VF",

"DIV",

"ADD_F",
"ADD_V",

"SUB_F",
"SUB_V",

"EQ_F",
"EQ_V",
"EQ_S",
"EQ_E",
"EQ_FNC",

"NE_F",
"NE_V",
"NE_S",
"NE_E",
"NE_FNC",

"LE",
"GE",
"LT",
"GT",

"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",

"ADDRESS",

"STORE_F",
"STORE_V",
"STORE_S",
"STORE_ENT",
"STORE_FLD",
"STORE_FNC",

"STOREP_F",
"STOREP_V",
"STOREP_S",
"STOREP_ENT",
"STOREP_FLD",
"STOREP_FNC",

"RETURN",

"NOT_F",
"NOT_V",
"NOT_S",
"NOT_ENT",
"NOT_FNC",

"IF",
"IFNOT",

"CALL0",
"CALL1",
"CALL2",
"CALL3",
"CALL4",
"CALL5",
"CALL6",
"CALL7",
"CALL8",

"STATE",

"GOTO",

"AND",
"OR",

"BITAND",
"BITOR"
};

const char *PR_GlobalString (int ofs);
const char *PR_GlobalStringNoContents (int ofs);


//=============================================================================

/*
=================
PR_PrintStatement
=================
*/
static void PR_PrintStatement (dstatement_t *s)
{
	int		i;

	if ( (unsigned int)s->op < sizeof(pr_opnames)/sizeof(pr_opnames[0]))
	{
		Con_PrintContf ("%s ",  pr_opnames[s->op]);
		i = strlen(pr_opnames[s->op]);
		for ( ; i<10 ; i++)
			Con_PrintContf (" ");
	}

	if (s->op == OP_IF || s->op == OP_IFNOT)
		Con_PrintContf ("%sbranch %d", PR_GlobalString(s->a),s->b);
	else if (s->op == OP_GOTO)
	{
		Con_PrintContf ("branch %d", s->a);
	}
	else if ( (unsigned int)(s->op - OP_STORE_F) < 6)
	{
		Con_PrintContf ("%s", PR_GlobalString(s->a));
		Con_PrintContf ("%s", PR_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
			Con_PrintContf ("%s", PR_GlobalString(s->a));
		if (s->b)
			Con_PrintContf ("%s", PR_GlobalString(s->b));
		if (s->c)
			Con_PrintContf ("%s", PR_GlobalStringNoContents(s->c));
	}
	Con_PrintLine ();
}

/*
============
PR_StackTrace
============
*/
static void PR_StackTrace (void)
{
	int		i;
	dfunction_t	*f;

	if (pr_depth == 0)
	{
		Con_PrintLinef ("<NO STACK>");
		return;
	}

	pr_stack[pr_depth].f = pr_xfunction;
	for (i=pr_depth ; i>=0 ; i--)
	{
		f = pr_stack[i].f;
		if (!f)
		{
			Con_PrintLinef ("<NO FUNCTION>");
		}
		else
		{
			Con_PrintLinef ("%12s : %s", PR_GetString( f->s_file), PR_GetString( f->s_name));
		}
	}
}


/*
============
PR_Profile_f

============
*/
void PR_Profile_f (lparse_t *line)
{
	int		i, num;
	int		pmax;
	dfunction_t	*f, *best;

	// Baker: the fix for the profile command.  If you aren't running a server, crash ...
	if (!sv.active)
	{
		Con_SafePrintLinef ("%s : Can't profile .. no active server.", line->args[0]);
		return;
	}

	num = 0;
	do
	{
		pmax = 0;
		best = NULL;
		for (i=0 ; i<progs->numfunctions ; i++)
		{
			f = &pr_functions[i];
			if (f->profile > pmax)
			{
				pmax = f->profile;
				best = f;
			}
		}
		if (best)
		{
			if (num < 10)
				Con_PrintLinef ("%7i %s", best->profile, PR_GetString(best->s_name));
			num++;
			best->profile = 0;
		}
	} while (best);
}


/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError (const char *error, ...)
{
	VA_EXPAND (text, SYSTEM_STRING_SIZE_1024, error);

	PR_PrintStatement (pr_statements + pr_xstatement);
	PR_StackTrace ();

	Con_PrintLinef ("%s", text);

	pr_depth = 0;		// dump the stack so host_error can shutdown functions

	Host_Error ("Program error");
}

/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
#ifdef SUPPORTS_COOP_ENHANCEMENTS
void PR_In_Killed_Check (dfunction_t *f, int depth);
void PR_In_Killed_Finish (void);
#endif // SUPPORTS_COOP_ENHANCEMENTS

static int PR_EnterFunction (dfunction_t *f)
{
	int		i, j, c, o;


	if (f == sv.pr_putclientinserver /* when coop protection doesn't apply pr_putclientinserver will be null and cannot match any non-null pointer */) {
		if (sv.level_coop_enhanced) {
			// host_client is already set when entering this function.  It could be a bot in a player slot, makes no difference to us.
			edict_t *self_edict = PROG_TO_EDICT(pr_global_struct->self);
			int playernum = NUM_FOR_EDICT(self_edict); // Is self the edict number?  or something else?
			client_t *svr_player;
			if (!in_range(1, playernum, svs.maxclients_internal)) // Because the cap can change at any time now.
				Host_Error ("Edict num %d out of player bounds (1 to %d)", playernum, svs.maxclients_internal); // Because the cap can change at any time now.
			svr_player = &svs.clients[playernum - 1];
			svr_player->coop_protect_end_time = sv.time + COOP_PROTECT_INTERVAL_5_0;
			//Con_DPrintLinef ("Coop protect for " QUOTED_S " start now %g until %g", svr_player->name, sv.time, svr_player->coop_protect_end_time);
		}

	}

	pr_stack[pr_depth].s = pr_xstatement;
	pr_stack[pr_depth].f = pr_xfunction;
	pr_depth++;
	if (pr_depth >= MAX_STACK_DEPTH)
		PR_RunError ("stack overflow");

// save off any locals that the new function steps on
	c = f->locals;
	if (localstack_used + c > LOCALSTACK_SIZE)
		PR_RunError ("PR_ExecuteProgram: locals stack overflow"  NEWLINE);

	for (i=0 ; i < c ; i++)
		localstack[localstack_used+i] = ((int *)pr_globals)[f->parm_start + i];
	localstack_used += c;

// copy parameters
	o = f->parm_start;
	for (i=0 ; i<f->numparms ; i++)
	{
		for (j=0 ; j<f->parm_size[i] ; j++)
		{
			((int *)pr_globals)[o] = ((int *)pr_globals)[OFS_PARM0+i*3+j];
			o++;
		}
	}

#ifdef SUPPORTS_COOP_ENHANCEMENTS
	if (sv.pr_handles_killed && f == sv.pr_handles_killed && !sv.pr_in_killed)
		PR_In_Killed_Check (f, pr_depth);
#endif // SUPPORTS_COOP_ENHANCEMENTS


	if (sv.pr_marcher_key_touch && f == sv.pr_marcher_key_touch && !sv.pr_in_marcher_key_touch) {
		edict_t *toucher = PROG_TO_EDICT(pr_global_struct->other);
		//const char *toucher_classname = PR_GetString( toucher->v.classname);
		if (toucher->v.solid == SOLID_SLIDEBOX) {
			int playernum = NUM_FOR_EDICT(toucher);
			if (in_range (1, playernum, svs.maxclients_internal)) {  // Because the cap can change at any time now.
				// It is a player.
				edict_t *key_ent = PROG_TO_EDICT(pr_global_struct->self);
				int key_entnum = NUM_FOR_EDICT(key_ent);
				
				sv.pr_in_marcher_key_touch_edict = key_ent;
				sv.pr_in_marcher_key_touch_solid = key_ent->v.solid;
				sv.pr_in_marcher_key_touch_modelstringint = key_ent->v.model;
				sv.pr_in_marcher_key_touch_modelindex = key_ent->v.modelindex;
				sv.pr_in_marcher_key_touch = pr_depth; // Activate
			}
		}
	}

	pr_xfunction = f;
	return f->first_statement - 1;	// offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
static int PR_LeaveFunction (void)
{
	int		i, c;

	// Baker: Future reduce CPU tax by combining into one watcher
//	if (pr_in_marcher_keytouch && pr_depth == pr_in_marcher_keytouch)
//		PR_In_Marcher_Finish;

#ifdef SUPPORTS_COOP_ENHANCEMENTS
	if (sv.pr_in_killed && pr_depth == sv.pr_in_killed)
		PR_In_Killed_Finish ();
#endif // SUPPORTS_COOP_ENHANCEMENTS

	if (pr_depth <= 0)
		Host_Error("prog stack underflow");

// Restore locals from the stack
	c = pr_xfunction->locals;
	localstack_used -= c;
	if (localstack_used < 0)
		PR_RunError ("PR_ExecuteProgram: locals stack underflow");

	for (i=0 ; i < c ; i++)
		((int *)pr_globals)[pr_xfunction->parm_start + i] = localstack[localstack_used+i];

// up stack
	pr_depth--;
	pr_xfunction = pr_stack[pr_depth].f;
	return pr_stack[pr_depth].s;
}


/*
====================
PR_ExecuteProgram

The interpretation main loop
====================
*/

#if 1
#define OPA ((eval_t *)&pr_globals[(unsigned short)st->a])
#define OPB ((eval_t *)&pr_globals[(unsigned short)st->b])
#define OPC ((eval_t *)&pr_globals[(unsigned short)st->c])

void PR_ExecuteProgram (func_t fnum)
{
	eval_t		*ptr;
	dstatement_t	*st;
	dfunction_t	*f, *newf;
	int profile, startprofile;
	edict_t		*ed;
	int		exitdepth;

	if (!fnum || fnum >= progs->numfunctions)
	{
		if (pr_global_struct->self)
			ED_Print (PROG_TO_EDICT(pr_global_struct->self));
		Host_Error ("PR_ExecuteProgram: NULL function");
	}

	f = &pr_functions[fnum];

	pr_trace = false;

// make a stack frame
	exitdepth = pr_depth;

	st = &pr_statements[PR_EnterFunction(f)];
	startprofile = profile = 0;

    while (1)
    {
	st++;	/* next statement */

	if (++profile > 10000000)	//spike -- was 100000
	{
		pr_xstatement = st - pr_statements;
		PR_RunError("runaway loop error");
	}

	if (pr_trace)
		PR_PrintStatement(st);

	switch (st->op)
	{
	case OP_ADD_F:
		OPC->_float = OPA->_float + OPB->_float;
		break;
	case OP_ADD_V:
		OPC->vector[0] = OPA->vector[0] + OPB->vector[0];
		OPC->vector[1] = OPA->vector[1] + OPB->vector[1];
		OPC->vector[2] = OPA->vector[2] + OPB->vector[2];
		break;

	case OP_SUB_F:
		OPC->_float = OPA->_float - OPB->_float;
		break;
	case OP_SUB_V:
		OPC->vector[0] = OPA->vector[0] - OPB->vector[0];
		OPC->vector[1] = OPA->vector[1] - OPB->vector[1];
		OPC->vector[2] = OPA->vector[2] - OPB->vector[2];
		break;

	case OP_MUL_F:
		OPC->_float = OPA->_float * OPB->_float;
		break;
	case OP_MUL_V:
		OPC->_float = OPA->vector[0] * OPB->vector[0] +
			      OPA->vector[1] * OPB->vector[1] +
			      OPA->vector[2] * OPB->vector[2];
		break;
	case OP_MUL_FV:
		OPC->vector[0] = OPA->_float * OPB->vector[0];
		OPC->vector[1] = OPA->_float * OPB->vector[1];
		OPC->vector[2] = OPA->_float * OPB->vector[2];
		break;
	case OP_MUL_VF:
		OPC->vector[0] = OPB->_float * OPA->vector[0];
		OPC->vector[1] = OPB->_float * OPA->vector[1];
		OPC->vector[2] = OPB->_float * OPA->vector[2];
		break;

	case OP_DIV_F:
		OPC->_float = OPA->_float / OPB->_float;
		break;

	case OP_BITAND:
		OPC->_float = (int)OPA->_float & (int)OPB->_float;
		break;

	case OP_BITOR:
		OPC->_float = (int)OPA->_float | (int)OPB->_float;
		break;

	case OP_GE:
		OPC->_float = OPA->_float >= OPB->_float;
		break;
	case OP_LE:
		OPC->_float = OPA->_float <= OPB->_float;
		break;
	case OP_GT:
		OPC->_float = OPA->_float > OPB->_float;
		break;
	case OP_LT:
		OPC->_float = OPA->_float < OPB->_float;
		break;
	case OP_AND:
		OPC->_float = OPA->_float && OPB->_float;
		break;
	case OP_OR:
		OPC->_float = OPA->_float || OPB->_float;
		break;

	case OP_NOT_F:
		OPC->_float = !OPA->_float;
		break;
	case OP_NOT_V:
		OPC->_float = !OPA->vector[0] && !OPA->vector[1] && !OPA->vector[2];
		break;
	case OP_NOT_S:
		OPC->_float = !OPA->string || !*PR_GetString(OPA->string);
		break;
	case OP_NOT_FNC:
		OPC->_float = !OPA->function;
		break;
	case OP_NOT_ENT:
		OPC->_float = (PROG_TO_EDICT(OPA->edict) == sv.edicts);
		break;

	case OP_EQ_F:
		OPC->_float = OPA->_float == OPB->_float;
		break;
	case OP_EQ_V:
		OPC->_float = (OPA->vector[0] == OPB->vector[0]) &&
			      (OPA->vector[1] == OPB->vector[1]) &&
			      (OPA->vector[2] == OPB->vector[2]);
		break;
	case OP_EQ_S:
		OPC->_float = !strcmp(PR_GetString(OPA->string), PR_GetString(OPB->string));
		break;
	case OP_EQ_E:
		OPC->_float = OPA->_int == OPB->_int;
		break;
	case OP_EQ_FNC:
		OPC->_float = OPA->function == OPB->function;
		break;

	case OP_NE_F:
		OPC->_float = OPA->_float != OPB->_float;
		break;
	case OP_NE_V:
		OPC->_float = (OPA->vector[0] != OPB->vector[0]) ||
			      (OPA->vector[1] != OPB->vector[1]) ||
			      (OPA->vector[2] != OPB->vector[2]);
		break;
	case OP_NE_S:
		OPC->_float = strcmp(PR_GetString(OPA->string), PR_GetString(OPB->string));
		break;
	case OP_NE_E:
		OPC->_float = OPA->_int != OPB->_int;
		break;
	case OP_NE_FNC:
		OPC->_float = OPA->function != OPB->function;
		break;

	case OP_STORE_F:
	case OP_STORE_ENT:
	case OP_STORE_FLD:	// integers
	case OP_STORE_S:
	case OP_STORE_FNC:	// pointers
		OPB->_int = OPA->_int;
		break;
	case OP_STORE_V:
		OPB->vector[0] = OPA->vector[0];
		OPB->vector[1] = OPA->vector[1];
		OPB->vector[2] = OPA->vector[2];
		break;

	case OP_STOREP_F:
	case OP_STOREP_ENT:
	case OP_STOREP_FLD:	// integers
	case OP_STOREP_S:
	case OP_STOREP_FNC:	// pointers
		ptr = (eval_t *)((byte *)sv.edicts + OPB->_int);
		ptr->_int = OPA->_int;
		break;
	case OP_STOREP_V:
		ptr = (eval_t *)((byte *)sv.edicts + OPB->_int);
		ptr->vector[0] = OPA->vector[0];
		ptr->vector[1] = OPA->vector[1];
		ptr->vector[2] = OPA->vector[2];
		break;

	case OP_ADDRESS:
		ed = PROG_TO_EDICT(OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
		if (ed == (edict_t *)sv.edicts && sv.state == ss_active)
		{
			pr_xstatement = st - pr_statements;
			PR_RunError("assignment to world entity");
		}
		OPC->_int = (byte *)((int *)&ed->v + OPB->_int) - (byte *)sv.edicts;
		break;

	case OP_LOAD_F:
	case OP_LOAD_FLD:
	case OP_LOAD_ENT:
	case OP_LOAD_S:
	case OP_LOAD_FNC:
		ed = PROG_TO_EDICT(OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
		OPC->_int = ((eval_t *)((int *)&ed->v + OPB->_int))->_int;
		break;

	case OP_LOAD_V:
		ed = PROG_TO_EDICT(OPA->edict);
#ifdef PARANOID
		NUM_FOR_EDICT(ed);	// Make sure it's in range
#endif
		ptr = (eval_t *)((int *)&ed->v + OPB->_int);
		OPC->vector[0] = ptr->vector[0];
		OPC->vector[1] = ptr->vector[1];
		OPC->vector[2] = ptr->vector[2];
		break;

	case OP_IFNOT:
		if (!OPA->_int)
			st += st->b - 1;	/* -1 to offset the st++ */
		break;

	case OP_IF:
		if (OPA->_int)
			st += st->b - 1;	/* -1 to offset the st++ */
		break;

	case OP_GOTO:
		st += st->a - 1;		/* -1 to offset the st++ */
		break;

	case OP_CALL0:
	case OP_CALL1:
	case OP_CALL2:
	case OP_CALL3:
	case OP_CALL4:
	case OP_CALL5:
	case OP_CALL6:
	case OP_CALL7:
	case OP_CALL8:
		pr_xfunction->profile += profile - startprofile;
		startprofile = profile;
		pr_xstatement = st - pr_statements;
		pr_argc = st->op - OP_CALL0;
		if (!OPA->function)
			PR_RunError("NULL function");
		newf = &pr_functions[OPA->function];
		if (newf->first_statement < 0)
		{ // Built-in function
			int i = -newf->first_statement;
			if (i >= pr_numbuiltins)
				PR_RunError("Bad builtin call number %d", i);
			pr_builtins[i]();
			break;
		}
		// Normal function
		st = &pr_statements[PR_EnterFunction(newf)];
		break;

	case OP_DONE:
	case OP_RETURN:
		pr_xfunction->profile += profile - startprofile;
		startprofile = profile;
		pr_xstatement = st - pr_statements;
		pr_globals[OFS_RETURN] = pr_globals[(unsigned short)st->a];
		pr_globals[OFS_RETURN + 1] = pr_globals[(unsigned short)st->a + 1];
		pr_globals[OFS_RETURN + 2] = pr_globals[(unsigned short)st->a + 2];
		st = &pr_statements[PR_LeaveFunction()];
		if (pr_depth == exitdepth)
		{ // Done
			return;
		}
		break;

	case OP_STATE:
		ed = PROG_TO_EDICT(pr_global_struct->self);
		ed->v.nextthink = pr_global_struct->time + 0.1;
		ed->v.frame = OPA->_float;
		ed->v.think = OPB->function;
		break;

	default:
		pr_xstatement = st - pr_statements;
		PR_RunError("Bad opcode %d", st->op);
	}
    }	/* end of while(1) loop */
}
#undef OPA
#undef OPB
#undef OPC

#else
void PR_ExecuteProgram (func_t fnum)
{
	eval_t	*ptr;
	eval_t	*a, *b, *c;
	int			s;
	dstatement_t	*st;
	dfunction_t	*f, *newf;
	int		runaway;
	int		i;
	edict_t	*ed;
	int		exitdepth;

	if (!fnum || fnum >= progs->numfunctions)
	{
		if (pr_global_struct->self)
			ED_Print (PROG_TO_EDICT(pr_global_struct->self));
		Host_Error ("PR_ExecuteProgram: NULL function");
	}

	f = &pr_functions[fnum];

	runaway = 100000;
	pr_trace = false;

// make a stack frame
	exitdepth = pr_depth;

	s = PR_EnterFunction (f);

	while (1)
	{
		s++;	// next statement

		st = &pr_statements[s];
		a = (eval_t *)&pr_globals[st->a];
		b = (eval_t *)&pr_globals[st->b];
		c = (eval_t *)&pr_globals[st->c];

		if (!--runaway)
			PR_RunError ("runaway loop error");

		pr_xfunction->profile++;
		pr_xstatement = s;

		if (pr_trace)
			PR_PrintStatement (st);

		switch (st->op)
		{
		case OP_ADD_F:
			c->_float = a->_float + b->_float;
			break;
		case OP_ADD_V:
			c->vector[0] = a->vector[0] + b->vector[0];
			c->vector[1] = a->vector[1] + b->vector[1];
			c->vector[2] = a->vector[2] + b->vector[2];
			break;

		case OP_SUB_F:
			c->_float = a->_float - b->_float;
			break;
		case OP_SUB_V:
			c->vector[0] = a->vector[0] - b->vector[0];
			c->vector[1] = a->vector[1] - b->vector[1];
			c->vector[2] = a->vector[2] - b->vector[2];
			break;

		case OP_MUL_F:
			c->_float = a->_float * b->_float;
			break;
		case OP_MUL_V:
			c->_float = a->vector[0]*b->vector[0]
					+ a->vector[1]*b->vector[1]
					+ a->vector[2]*b->vector[2];
			break;
		case OP_MUL_FV:
			c->vector[0] = a->_float * b->vector[0];
			c->vector[1] = a->_float * b->vector[1];
			c->vector[2] = a->_float * b->vector[2];
			break;
		case OP_MUL_VF:
			c->vector[0] = b->_float * a->vector[0];
			c->vector[1] = b->_float * a->vector[1];
			c->vector[2] = b->_float * a->vector[2];
			break;

		case OP_DIV_F:
			c->_float = a->_float / b->_float;
			break;

		case OP_BITAND:
			c->_float = (int)a->_float & (int)b->_float;
			break;

		case OP_BITOR:
			c->_float = (int)a->_float | (int)b->_float;
			break;


		case OP_GE:
			c->_float = a->_float >= b->_float;
			break;
		case OP_LE:
			c->_float = a->_float <= b->_float;
			break;
		case OP_GT:
			c->_float = a->_float > b->_float;
			break;
		case OP_LT:
			c->_float = a->_float < b->_float;
			break;
		case OP_AND:
			c->_float = a->_float && b->_float;
			break;
		case OP_OR:
			c->_float = a->_float || b->_float;
			break;

		case OP_NOT_F:
			c->_float = !a->_float;
			break;
		case OP_NOT_V:
			c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];
			break;
		case OP_NOT_S:
			c->_float = !a->string || !pr_strings[a->string];
			break;
		case OP_NOT_FNC:
			c->_float = !a->function;
			break;
		case OP_NOT_ENT:
			c->_float = (PROG_TO_EDICT(a->edict) == sv.edicts);
			break;

		case OP_EQ_F:
			c->_float = a->_float == b->_float;
			break;
		case OP_EQ_V:
			c->_float = (a->vector[0] == b->vector[0]) &&
						(a->vector[1] == b->vector[1]) &&
						(a->vector[2] == b->vector[2]);
			break;
		case OP_EQ_S:
			c->_float = !strcmp(PR_GetString(a->string),PR_GetString(b->string));
			break;
		case OP_EQ_E:
			c->_float = a->_int == b->_int;
			break;
		case OP_EQ_FNC:
			c->_float = a->function == b->function;
			break;


		case OP_NE_F:
			c->_float = a->_float != b->_float;
			break;
		case OP_NE_V:
			c->_float = (a->vector[0] != b->vector[0]) ||
						(a->vector[1] != b->vector[1]) ||
						(a->vector[2] != b->vector[2]);
			break;
		case OP_NE_S:
			c->_float = strcmp(PR_GetString(a->string),PR_GetString(b->string));
			break;
		case OP_NE_E:
			c->_float = a->_int != b->_int;
			break;
		case OP_NE_FNC:
			c->_float = a->function != b->function;
			break;

		case OP_STORE_F:
		case OP_STORE_ENT:
		case OP_STORE_FLD:		// integers
		case OP_STORE_S:
		case OP_STORE_FNC:		// pointers
			b->_int = a->_int;
			break;
		case OP_STORE_V:
			b->vector[0] = a->vector[0];
			b->vector[1] = a->vector[1];
			b->vector[2] = a->vector[2];
			break;

		case OP_STOREP_F:
		case OP_STOREP_ENT:
		case OP_STOREP_FLD:		// integers
		case OP_STOREP_S:
		case OP_STOREP_FNC:		// pointers
			ptr = (eval_t *)((byte *)sv.edicts + b->_int);
			ptr->_int = a->_int;
			break;
		case OP_STOREP_V:
			ptr = (eval_t *)((byte *)sv.edicts + b->_int);
			ptr->vector[0] = a->vector[0];
			ptr->vector[1] = a->vector[1];
			ptr->vector[2] = a->vector[2];
			break;

		case OP_ADDRESS:
			ed = PROG_TO_EDICT(a->edict);
	#ifdef PARANOID
			NUM_FOR_EDICT(ed);		// Make sure it's in range
	#endif
			if (ed == (edict_t *)sv.edicts && sv.state == ss_active)
			{
				PR_RunError ("assignment to world entity");
			}
			c->_int = (byte *)((int *)&ed->v + b->_int) - (byte *)sv.edicts;
			break;

		case OP_LOAD_F:
		case OP_LOAD_FLD:
		case OP_LOAD_ENT:
		case OP_LOAD_S:
		case OP_LOAD_FNC:
			ed = PROG_TO_EDICT(a->edict);
	#ifdef PARANOID
			NUM_FOR_EDICT(ed);		// Make sure it's in range
	#endif
			a = (eval_t *)((int *)&ed->v + b->_int);
			c->_int = a->_int;
			break;

		case OP_LOAD_V:
			ed = PROG_TO_EDICT(a->edict);
	#ifdef PARANOID
			NUM_FOR_EDICT(ed);		// Make sure it's in range
	#endif
			a = (eval_t *)((int *)&ed->v + b->_int);
			c->vector[0] = a->vector[0];
			c->vector[1] = a->vector[1];
			c->vector[2] = a->vector[2];
			break;

		case OP_IFNOT:
			if (!a->_int)
				s += st->b - 1;	// offset the s++
			break;

		case OP_IF:
			if (a->_int)
				s += st->b - 1;	// offset the s++
			break;

		case OP_GOTO:
			s += st->a - 1;	// offset the s++
			break;

		case OP_CALL0:
		case OP_CALL1:
		case OP_CALL2:
		case OP_CALL3:
		case OP_CALL4:
		case OP_CALL5:
		case OP_CALL6:
		case OP_CALL7:
		case OP_CALL8:
			pr_argc = st->op - OP_CALL0;
			if (!a->function)
				PR_RunError ("NULL function");

			newf = &pr_functions[a->function];

			if (newf->first_statement < 0)
			{	// negative statements are built in functions
				i = -newf->first_statement;
				if (i >= pr_numbuiltins)
					PR_RunError ("Bad builtin call number");
				pr_builtins[i] ();
				break;
			}
			// Normal function
			s = PR_EnterFunction (newf);
			break;

		case OP_DONE:
		case OP_RETURN:
			pr_globals[OFS_RETURN] = pr_globals[st->a];
			pr_globals[OFS_RETURN+1] = pr_globals[st->a+1];
			pr_globals[OFS_RETURN+2] = pr_globals[st->a+2];

			s = PR_LeaveFunction ();
			if (pr_depth == exitdepth)
			{ // Done
				return;
			}
			break;

		case OP_STATE:
			ed = PROG_TO_EDICT(pr_global_struct->self);
			ed->v.nextthink = pr_global_struct->time + 0.1;
			if (a->_float != ed->v.frame)
			{
				ed->v.frame = a->_float;
			}
			ed->v.think = b->function;
			break;

		default:
			PR_RunError ("Bad opcode %d", st->op);
		}
	}	/* end of while(1) loop */

}

#endif