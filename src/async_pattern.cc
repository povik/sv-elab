//
// Yosys slang frontend
//
// Copyright Martin Povišer <povik@cutebit.org>
// Distributed under the terms of the ISC license, see LICENSE
//
#include "kernel/log.h"
#include "slang/ast/Expression.h"
#include "slang/ast/SemanticFacts.h"
#include "slang/ast/Statement.h"
#include "slang/ast/TimingControl.h"
#include "slang/ast/expressions/AssignmentExpressions.h"
#include "slang/ast/expressions/ConversionExpression.h"
#include "slang/ast/expressions/MiscExpressions.h"
#include "slang/ast/expressions/Operator.h"
#include "slang/ast/expressions/OperatorExpressions.h"
#include "slang/ast/expressions/SelectExpressions.h"
#include "slang/ast/statements/ConditionalStatements.h"
#include "slang/ast/statements/MiscStatements.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/types/Type.h"
#include <cstddef>
#include <optional>

#include "async_pattern.h"
#include "diag.h"
#include "slang/syntax/AllSyntax.h"
#include "variables.h"

namespace slang_frontend {

void TimingPatternInterpretor::handle_always(const ast::ProceduralBlockSymbol &symbol)
{
	log_assert(symbol.procedureKind == ast::ProceduralBlockKind::Always ||
			   symbol.procedureKind == ast::ProceduralBlockKind::AlwaysFF);

	if (symbol.getBody().kind == ast::StatementKind::Block ||
			symbol.getBody().kind == ast::StatementKind::ConcurrentAssertion ||
			symbol.getBody().kind == ast::StatementKind::ImmediateAssertion) {
		// short-circuit for SVA; free-standing assertion
		handle_comb_like_process(symbol, symbol.getBody());
		return;
	} else if (symbol.getBody().kind != ast::StatementKind::Timed) {
		issuer.add_diag(diag::UnsynthesizableFeature, symbol.getBody().sourceRange.start());
		return;
	}

	const auto &timed = symbol.getBody().as<ast::TimedStatement>();
	const auto *top_ast_timing = &timed.timing;

	using TCKind = ast::TimingControlKind;
	std::span<const ast::TimingControl *const> events;
	const ast::TimingControl *const top_events[1] = {top_ast_timing};

	events = (top_ast_timing->kind == TCKind::EventList)
					 ? top_ast_timing->as<ast::EventListControl>().events
					 : std::span<const ast::TimingControl *const>(top_events);

	bool implicit = false;
	std::vector<const ast::SignalEventControl *> triggers;

	for (auto ev : events)
		switch (ev->kind) {
		case ast::TimingControlKind::SignalEvent: {
			const auto &sigev = ev->as<ast::SignalEventControl>();

			if (sigev.iffCondition && sigev.edge == ast::EdgeKind::None)
				issuer.add_diag(diag::IffUnsupported, sigev.iffCondition->sourceRange);

			switch (sigev.edge) {
			case ast::EdgeKind::None: {
				if (symbol.procedureKind == ast::ProceduralBlockKind::AlwaysFF) {
					issuer.add_diag(diag::AlwaysFFBadTiming, ev->sourceRange);
					break;
				}

				// Report on the top timing node as that makes for nicer reports in case there
				// are many signals in the sensitivity list
				issuer.add_diag(diag::SignalSensitivityAmbiguous, top_ast_timing->sourceRange);
				implicit = true;
			} break;

			case ast::EdgeKind::PosEdge:
			case ast::EdgeKind::NegEdge: triggers.push_back(&sigev); break;

			case ast::EdgeKind::BothEdges:
				if (!settings.allow_dual_edge_ff.value_or(false)) {
					issuer.add_diag(diag::BothEdgesUnsupported, sigev.sourceRange);
					break;
				}
				triggers.push_back(&sigev);
				break;
			}
		} break;

		case ast::TimingControlKind::ImplicitEvent: {
			if (symbol.procedureKind == ast::ProceduralBlockKind::AlwaysFF) {
				issuer.add_diag(diag::AlwaysFFBadTiming, ev->sourceRange);
				break;
			}

			implicit = true;
		} break;

		case ast::TimingControlKind::EventList: log_abort();

		case ast::TimingControlKind::Delay: {
			if (!settings.ignore_timing.value_or(false))
				issuer.add_diag(diag::GenericTimingUnsyn, ev->sourceRange);
			else
				implicit = true;
			break;
		}

		default: issuer.add_diag(diag::UnsynthesizableFeature, ev->sourceRange); break;
		}

	if (implicit && !triggers.empty())
		issuer.add_diag(diag::EdgeImplicitMixing, top_ast_timing->sourceRange);

	if (implicit) {
		handle_comb_like_process(symbol, timed.stmt);
	} else if (!triggers.empty()) {
		interpret_async_pattern(symbol, triggers, timed.stmt);
	}
}

namespace {

struct AloadConditionValue
{
	VariableBit signal;
	bool polarity;

	explicit AloadConditionValue(VariableBit signal, bool polarity = true)
		: signal(signal), polarity(polarity)
	{}

	AloadConditionValue operator!() const { return AloadConditionValue(signal, !polarity); }
};

} // namespace

static std::optional<AloadConditionValue> interpret_aload_condition(
		EvalContext &eval, const ast::Expression &expr)
{
	switch (expr.kind) {
	case ast::ExpressionKind::BinaryOp: {
		auto &biop = expr.as<ast::BinaryExpression>();
		auto const_ = biop.right().eval(eval.const_);

		if (biop.op == ast::BinaryOperator::Equality && const_.isInteger() &&
				!const_.hasUnknown()) {

			const slang::SVInt &int_const = const_.integer();

			// SVInt's == zero-extends both sides to the wider operand
			if (int_const == slang::SVInt::One) {
				// comparison target is constant one with optional zero extension,
				// pass through the left operand as is
				return interpret_aload_condition(eval, biop.left());
			}

			if (int_const == slang::SVInt::Zero) {
				// comparison target is fully zeroed,
				// pass through the left operand after negation
				auto left_interpreted = interpret_aload_condition(eval, biop.left());
				if (left_interpreted) {
					return !*left_interpreted;
				}
			}
		}

		// Inequality follows Equality handling with polarities swapped
		if (biop.op == ast::BinaryOperator::Inequality && const_.isInteger() &&
				!const_.hasUnknown()) {
			const slang::SVInt &int_const = const_.integer();

			// SVInt's == zero-extends both sides to the wider operand
			if (int_const == slang::SVInt::One) {
				auto left_interpreted = interpret_aload_condition(eval, biop.left());
				if (left_interpreted) {
					return !*left_interpreted;
				}
			}

			if (int_const == slang::SVInt::Zero) {
				return interpret_aload_condition(eval, biop.left());
			}
		}

		break;
	}

	case ast::ExpressionKind::UnaryOp: {
		auto &unop = expr.as<ast::UnaryExpression>();
		if (unop.op == ast::UnaryOperator::LogicalNot ||
				(unop.op == ast::UnaryOperator::BitwiseNot && unop.type->getBitWidth() == 1)) {
			auto op_interpreted = interpret_aload_condition(eval, unop.operand());
			if (op_interpreted) {
				return !*op_interpreted;
			}
		}

		break;
	}

	case ast::ExpressionKind::Conversion: {
		auto &conv = expr.as<ast::ConversionExpression>();
		if (conv.operand().type->isIntegral() && conv.type->isIntegral() &&
				!conv.operand().type->isSigned() && !conv.type->isSigned() &&
				/* maybe redundant: */ conv.type->getBitWidth() >= 1) {
			// conditions met for safe pass-through
			return interpret_aload_condition(eval, conv.operand());
		}

		break;
	}

	default: break;
	}

	// If none of the above cases matched, use the "lhs" evaluator
	// to describe the expression in terms of variable bits
	VariableBits expr_vbits = eval.lhs(expr, /* silent= */ true);
	if (expr_vbits.bitwidth() == 1 && !expr_vbits.has_dummy_bits()) {
		return AloadConditionValue(expr_vbits[0]);
	}

	return std::nullopt;
}

void TimingPatternInterpretor::interpret_async_pattern(const ast::ProceduralBlockSymbol &symbol,
		std::vector<const ast::SignalEventControl *> triggers, const ast::Statement &body)
{
	log_assert(symbol.getBody().kind == ast::StatementKind::Timed);
	const auto &timed = symbol.getBody().as<ast::TimedStatement>();
	const ast::Statement *stmt = &body;

	std::vector<AsyncBranch> found_async;
	const ast::StatementBlockSymbol *prologue_block = nullptr;
	std::vector<const ast::Statement *> prologue;
	bool did_something = true;
	while (did_something) {
		did_something = false;

		if (prologue_block == nullptr && stmt->kind == ast::StatementKind::Block &&
				stmt->as<ast::BlockStatement>().blockKind == ast::StatementBlockKind::Sequential) {
			prologue_block = stmt->as<ast::BlockStatement>().blockSymbol;
			stmt = &stmt->as<ast::BlockStatement>().body;
			did_something = true;
		}

		if (stmt->kind == ast::StatementKind::List && triggers.size() > 1) {
			auto &list = stmt->as<ast::StatementList>().list;
			std::vector<const ast::Statement *> pending_prologue;
			for (size_t i = 0; i < list.size(); i++) {
				auto &inner_stmt = list[i];
				if (inner_stmt->kind == ast::StatementKind::ExpressionStatement ||
						inner_stmt->kind == ast::StatementKind::VariableDeclaration) {
					pending_prologue.push_back(inner_stmt);
				} else if (i == list.size() - 1) {
					stmt = inner_stmt;
					prologue.insert(
							prologue.end(), pending_prologue.begin(), pending_prologue.end());
					pending_prologue.clear();
					did_something = true;
					break;
				} else {
					break;
				}
			}

			if (!pending_prologue.empty()) {
				break;
			}
		}
	}

	// Keep inferring asynchronous loads until we get to a single remaining edge trigger
	while (triggers.size() > 1) {
		if (stmt->kind != ast::StatementKind::Conditional ||
				stmt->as<ast::ConditionalStatement>().check != ast::UniquePriorityCheck::None ||
				stmt->as<ast::ConditionalStatement>().conditions.size() != 1 ||
				stmt->as<ast::ConditionalStatement>().conditions[0].pattern ||
				!stmt->as<ast::ConditionalStatement>().ifFalse) {
			auto &diag = issuer.add_diag(diag::ExpectingIfElseAload, stmt->sourceRange);
			diag.addNote(diag::NoteDuplicateEdgeSense, timed.timing.sourceRange);
			return;
		}

		auto &cond_stmt = stmt->as<ast::ConditionalStatement>();
		std::optional<AloadConditionValue> condition =
				interpret_aload_condition(eval, *cond_stmt.conditions[0].expr);

		if ((bool)condition) {
			auto found = std::find_if(
					triggers.begin(), triggers.end(), [&](const ast::SignalEventControl *trigger) {
						return eval.lhs(trigger->expr, /* silent= */ true) ==
							   VariableBits(condition->signal);
					});

			if (found != triggers.end()) {
				if ((*found)->iffCondition)
					issuer.add_diag(diag::IffUnsupported, (*found)->iffCondition->sourceRange);

				if ((*found)->edge !=
						(condition->polarity ? ast::EdgeKind::PosEdge : ast::EdgeKind::NegEdge)) {
					auto &diag = issuer.add_diag(
							diag::IfElseAloadPolarity, cond_stmt.conditions[0].expr->sourceRange);
					diag.addNote(diag::NoteSignalEvent, (*found)->sourceRange);
					diag.addNote(diag::NoteDuplicateEdgeSense, timed.timing.sourceRange);
					// We raised an error. Do infer the async load anyway
				}

				found_async.push_back({condition->signal, condition->polarity, cond_stmt.ifTrue});
				triggers.erase(found);
				stmt = cond_stmt.ifFalse;
				continue;
			}
		}

		auto &diag = issuer.add_diag(diag::IfElseAloadMismatch, stmt->sourceRange);
		diag.addNote(diag::NoteDuplicateEdgeSense, timed.timing.sourceRange);
		return;
	}

	handle_ff_process(symbol, *triggers[0], prologue_block, prologue, *stmt, found_async);
}

void TimingPatternInterpretor::interpret(const ast::ProceduralBlockSymbol &symbol)
{
	auto kind = symbol.procedureKind;
	switch (kind) {
	case ast::ProceduralBlockKind::Always:
	case ast::ProceduralBlockKind::AlwaysFF: handle_always(symbol); break;

	case ast::ProceduralBlockKind::AlwaysComb:
	case ast::ProceduralBlockKind::AlwaysLatch:
		handle_comb_like_process(symbol, symbol.getBody());
		break;

	case ast::ProceduralBlockKind::Initial: handle_initial_process(symbol, symbol.getBody()); break;

	case ast::ProceduralBlockKind::Final:
		// Final blocks are ignored by synthesis
		break;

	default: log_abort();
	}
}

}; // namespace slang_frontend
