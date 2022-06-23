#include "PrismHighlighter.h"

#include <QStringLiteral>
#include <QFont>
#include <QColor>

namespace stamina {
namespace gui {
namespace addons {
namespace highlighter {

PrismHighlighter(QTextDocument * parent)
	: Highlighter(parent)
{
	setupKeyWordPatterns();
}

void
PrismHighlighter::setupKeyWordPatterns() {
	ColorScheme & cs = ColorSchemes::darkMode;
	HighlightingRule rule;

	keywordFormat.setForeground(cs.keyword);
	keywordFormat.setFontWeight(QFont::Bold);
	const QString keywordPatterns[] = {
		QStringLiteral("\\bA\\b")
		, QStringLiteral("\\bbool\\b")
		, QStringLiteral("\\bclock\\b")
		, QStringLiteral("\\bconst\\b")
		, QStringLiteral("\\bctmc\\b")
		, QStringLiteral("\\bC\\b")
		, QStringLiteral("\\bdouble\\b")
		, QStringLiteral("\\bdtmc\\b")
		, QStringLiteral("\\bE\\b")
		, QStringLiteral("\\bendinit\\b")
		, QStringLiteral("\\bendinvariant\\b")
		, QStringLiteral("\\bendmodule\\b")
		, QStringLiteral("\\bendobservables\\b")
		, QStringLiteral("\\bendrewards\\b")
		, QStringLiteral("\\bendsystem\\b")
		, QStringLiteral("\\bfalse\\b")
		, QStringLiteral("\\bformula\\b")
		, QStringLiteral("\\bfilter\\b")
		, QStringLiteral("\\bfunc\\b")
		, QStringLiteral("\\bF\\b")
		, QStringLiteral("\\bglobal\\b")
		, QStringLiteral("\\bG\\b")
		, QStringLiteral("\\binit\\b")
		, QStringLiteral("\\binvariant\\b")
		, QStringLiteral("\\bI\\b")
		, QStringLiteral("\\bint\\b")
		, QStringLiteral("\\blabel\\b")
		, QStringLiteral("\\bmax\\b")
		, QStringLiteral("\\bmdp\\b")
		, QStringLiteral("\\bmin\\b")
		, QStringLiteral("\\bmodule\\b")
		, QStringLiteral("\\bX\\b")
		, QStringLiteral("\\bnondeterministic\\b")
		, QStringLiteral("\\bobservable\\b")
		, QStringLiteral("\\bobservables\\b")
		, QStringLiteral("\\bof\\b")
		, QStringLiteral("\\bPmax\\b")
		, QStringLiteral("\\bPmin\\b")
		, QStringLiteral("\\bP\\b")
		, QStringLiteral("\\bpomdp\\b")
		, QStringLiteral("\\bpopta\\b")
		, QStringLiteral("\\bprobabilistic\\b")
		, QStringLiteral("\\bprob\\b")
		, QStringLiteral("\\bpta\\b")
		, QStringLiteral("\\brate\\b")
		, QStringLiteral("\\brewards\\b")
		, QStringLiteral("\\bRmax\\b")
		, QStringLiteral("\\bRmin\\b")
		, QStringLiteral("\\bR\\b")
		, QStringLiteral("\\bS\\b")
		, QStringLiteral("\\bstochastic\\b")
		, QStringLiteral("\\bsystem\\b")
		, QStringLiteral("\\btrue\\b")
		, QStringLiteral("\\bU\\b")
		, QStringLiteral("\\bW\\b")
	};

	for (const QString &pattern : keywordPatterns) {
		rule.pattern = QRegularExpression(pattern);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}

	// String expressions
	classFormat.setFontWeight(QFont::Bold);
	classFormat.setForeground(cs.string);
	rule.pattern = QRegularExpression(QStringLiteral("\\bQ[A-Za-z]+\\b"));
	rule.format = classFormat;
	highlightingRules.append(rule);

	quotationFormat.setForeground(cs.function);
	rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
	rule.format = quotationFormat;
	highlightingRules.append(rule);

	functionFormat.setFontItalic(true);
	functionFormat.setForeground(cs.number);
	rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
	rule.format = functionFormat;
	highlightingRules.append(rule);

	singleLineCommentFormat.setForeground(cs.comment);
	rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);

	multiLineCommentFormat.setForeground(cs.comment);

	commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
	commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}


} // namespace highlighter
} // namespace addons
} // namespace gui
} // namespace stamina