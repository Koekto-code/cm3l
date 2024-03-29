#include <cm3l/Lexer.h>
#include <cm3l/Parser.h>
#include <stdio.h>

void indent(unsigned num)
{
	for (unsigned i = 0; i != num; ++i)
		putchar(' ');
}

static inline const char *operTypeToStr(cm3l_TokenData tdt)
{
	switch (tdt)
	{
		case Tdt_Oper_Assign: return "=";
		case Tdt_Oper_AddA: return "+=";
		case Tdt_Oper_SubA: return "-=";
		case Tdt_Oper_MulA: return "*=";
		case Tdt_Oper_DivA: return "/=";
		case Tdt_Oper_RemA: return "%=";
		case Tdt_Oper_ShlA: return "<<=";
		case Tdt_Oper_ShrA: return ">>=";
		case Tdt_Oper_XorA: return "^=";
		default:
			return "<unknown>";
	}
}

void printStat(cm3l_ParserData const *comp, cm3l_Statement const *st, unsigned r)
{
	if (r > 20) return;
	if (st->type == Stt_AssignOper)
	{
		cm3l_StatementBinaryOper *binopst = cm3l_VectorAtM(&comp->binOpers, st->index, cm3l_StatementBinaryOper);
		cm3l_Statement *first = cm3l_VectorAtM(&comp->statements, binopst->first, cm3l_Statement);
		cm3l_Statement *second = cm3l_VectorAtM(&comp->statements, binopst->second, cm3l_Statement);

		printf("operator %s\n", operTypeToStr(binopst->type));
		indent(r * 2); printf("|>== ");
		printStat(comp, first, r + 1);
		indent(r * 2); printf("|>== ");
		printStat(comp, second, r + 1);
	}
	else if (st->type == Stt_Reference)
	{
		cm3l_StatementReference *refst = cm3l_VectorAtM(&comp->references, st->index, cm3l_StatementReference);

		if (refst->id == CM3L_NONE)
			printf("<ref: none"); // shouldn't appear
		else if (refst->id == CM3L_RESERVED)
			printf("<ref: reserved");
		else printf("<ref: %zu", refst->id);

		if (refst->flags & cm3l_SttRefIsLiteral)
			printf(" | literal");
		if (refst->flags & cm3l_SttRefIsNested)
			printf(" | nested");
		if (refst->flags & cm3l_SttRefIsAbsolute)
			printf(" | absolute");

		printf(">\n");
	}
	else if (st->type == Stt_VarDecl)
	{
		cm3l_StatementVarDecl *vdst = cm3l_VectorAtM(&comp->varDecls, st->index, cm3l_StatementVarDecl);
		printf("<variable decl>\n");

		cm3l_Statement refst = {
			.container = (void *)&comp->references,
			.index = vdst->ref,
			.type = Stt_Reference
		};
		cm3l_Statement *typest = cm3l_VectorAtM(&comp->statements, vdst->type, cm3l_Statement);
		indent(r * 2); printf("|>== ");
		printStat(comp, typest, r + 1);
		indent(r * 2); printf("|>== ");
		printStat(comp, &refst, r + 1);
	}
	else if (st->type == Stt_Undefined)
	{
		printf("<undefined>\n");
	}
	else if (st->type == Stt_BasicOper)
	{
		cm3l_StatementBasicOper *opst = cm3l_VectorAtM(&comp->basicOpers, st->index, cm3l_StatementBasicOper);
		const char *desc =
			opst->type == Tdt_Control_Return ? "return" :
			opst->type == Tdt_Control_Break ? "break" :
			"<unknown>";
		printf("operator %s\n", desc);
	}
	else if (st->type == Stt_Grouping)
	{
		cm3l_StatementGrouping *grp = cm3l_VectorAtM(&comp->groups, st->index, cm3l_StatementGrouping);
		printf("<grouping>\n");

		for (size_t i = 0; i != grp->members.length; ++i)
		{
			size_t memb_index = *cm3l_VectorAtM(&grp->members, i, size_t);
			cm3l_Statement *memb = cm3l_VectorAtM(&comp->statements, memb_index, cm3l_Statement);

			indent(r * 2); printf("|>== ");
			printStat(comp, memb, r + 1);
		}
	}
	else if (st->type == Stt_InlineFunction)
	{
		cm3l_StatementInlineFunction *fn = cm3l_VectorAtM(&comp->inlineFns, st->index, cm3l_StatementInlineFunction);

		cm3l_StatementGrouping *args = cm3l_VectorAtM(&comp->groups, fn->args, cm3l_StatementGrouping);
		cm3l_Statement *body = cm3l_VectorAtM(&comp->statements, fn->body, cm3l_Statement);

		cm3l_Statement argsWrap = { .type = Stt_Grouping, .container = (void*)&comp->groups, .index = fn->args };

		printf("<function>\n");
		indent(r * 2); printf("|>== ");
		printStat(comp, &argsWrap, r + 1);
		indent(r * 2); printf("|>== ");
		printStat(comp, body, r + 1);
	}
	else if (st->type == Stt_Sequence)
	{
		cm3l_StatementSequence *seq = cm3l_VectorAtM(&comp->sequences, st->index, cm3l_StatementSequence);
		printf("<sequence> len: %d\n", (int)seq->statements.length);

		for (size_t i = 0; i != seq->statements.length; ++i)
		{
			size_t st_index = *cm3l_VectorAtM(&seq->statements, i, size_t);
			cm3l_Statement *innerStat = cm3l_VectorAtM(&comp->statements, st_index, cm3l_Statement);

			indent(r * 2); printf("|>== ");
			printStat(comp, innerStat, r + 1);
		}
	}
	else if (st->type == Stt_UnaryOper)
	{
		cm3l_StatementUnaryOper *uopst = cm3l_VectorAtM(&comp->unaryOpers, st->index, cm3l_StatementUnaryOper);
		const char *subtype = (uopst->type == Tdt_Control_Return) ?
			"return" :
			"unknown";
		printf("<unary oper %s>\n", subtype);

		indent(r * 2); printf("|>== ");
		cm3l_Statement *innerStat = cm3l_VectorAtM(&comp->statements, uopst->st, cm3l_Statement);
		printStat(comp, innerStat, r + 1);
	}
	else {
		printf("unknown type %d\n", (int)st->type);
	}
}

int main(int argc, char **argv)
{
	const char *filename = argc > 1 ? argv[1] : "input.cm3l";

	cm3l_LexerData lex = cm3l_LexerDataCreate();
	int proc = cm3l_LexerProcess(&lex, "input.cm3l");
	if (proc != 0)
	{
		printf (
			"failed to process %s\n"
			"syntax errors: %d\n"
			"tokens: %u\n",
			filename, lex.syntaxErrors, (unsigned)lex.tokens.length
		);
		return proc;
	}

	cm3l_ParserData comp = cm3l_ParserDataCreate();
	unsigned comp_errcount = cm3l_ParserProcess(&lex, &comp);

	if (comp_errcount)
	{
		printf("Parsing error count: %u\n", comp_errcount);
		return 1;
	}

	printf("Abstract Syntax Tree ======================\n");
	for (unsigned i = 0; i != comp.toplevel.length; ++i)
	{
		size_t index = *cm3l_VectorAtM(&comp.toplevel, i, size_t);
		cm3l_Statement *st = cm3l_VectorAtM(&comp.statements, index, cm3l_Statement);
		printStat(&comp, st, 0);
	}
	cm3l_LexerDataDestroy(&lex);
	cm3l_ParserDataDestroy(&comp);
}
