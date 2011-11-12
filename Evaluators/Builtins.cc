#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <assert.h>
#include <limits.h>
#include "AST/AST"
#include "AST/Symbol"
#include "AST/Keyword"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Scanners/MathParser"
#include "Scanners/OperatorPrecedenceList"
#include "FFIs/FFIs"

namespace Evaluators {

bool Quoter::eager_P(void) const {
	return(false);
}
AST::Node* Quoter::execute(AST::Node* argument) {
	/* there's a special case in the annotator, so this cannot happen:
	AST::SymbolReference* ref = dynamic_cast<AST::SymbolReference*>(argument);
	return(ref ? ref->symbol : argument);
	*/
	return(argument);
}
AST::Node* ProcedureP::execute(AST::Node* argument) {
	return(internNative(argument != NULL && (dynamic_cast<Operation*>(argument) != NULL)));
}
static Int integers[256] = {
	Int(0),
	Int(1),
	Int(2),
	Int(3),
	Int(4),
	Int(5),
	Int(6),
	Int(7),
	Int(8),
	Int(9),
	Int(10),
	Int(11),
	Int(12),
	Int(13),
	Int(14),
	Int(15),
	Int(16),
	Int(17),
	Int(18),
	Int(19),
	Int(20),
	Int(21),
	Int(22),
	Int(23),
	Int(24),
	Int(25),
	Int(26),
	Int(27),
	Int(28),
	Int(29),
	Int(30),
	Int(31),
	Int(32),
	Int(33),
	Int(34),
	Int(35),
	Int(36),
	Int(37),
	Int(38),
	Int(39),
	Int(40),
	Int(41),
	Int(42),
	Int(43),
	Int(44),
	Int(45),
	Int(46),
	Int(47),
	Int(48),
	Int(49),
	Int(50),
	Int(51),
	Int(52),
	Int(53),
	Int(54),
	Int(55),
	Int(56),
	Int(57),
	Int(58),
	Int(59),
	Int(60),
	Int(61),
	Int(62),
	Int(63),
	Int(64),
	Int(65),
	Int(66),
	Int(67),
	Int(68),
	Int(69),
	Int(70),
	Int(71),
	Int(72),
	Int(73),
	Int(74),
	Int(75),
	Int(76),
	Int(77),
	Int(78),
	Int(79),
	Int(80),
	Int(81),
	Int(82),
	Int(83),
	Int(84),
	Int(85),
	Int(86),
	Int(87),
	Int(88),
	Int(89),
	Int(90),
	Int(91),
	Int(92),
	Int(93),
	Int(94),
	Int(95),
	Int(96),
	Int(97),
	Int(98),
	Int(99),
	Int(100),
	Int(101),
	Int(102),
	Int(103),
	Int(104),
	Int(105),
	Int(106),
	Int(107),
	Int(108),
	Int(109),
	Int(110),
	Int(111),
	Int(112),
	Int(113),
	Int(114),
	Int(115),
	Int(116),
	Int(117),
	Int(118),
	Int(119),
	Int(120),
	Int(121),
	Int(122),
	Int(123),
	Int(124),
	Int(125),
	Int(126),
	Int(127),
	Int(128),
	Int(129),
	Int(130),
	Int(131),
	Int(132),
	Int(133),
	Int(134),
	Int(135),
	Int(136),
	Int(137),
	Int(138),
	Int(139),
	Int(140),
	Int(141),
	Int(142),
	Int(143),
	Int(144),
	Int(145),
	Int(146),
	Int(147),
	Int(148),
	Int(149),
	Int(150),
	Int(151),
	Int(152),
	Int(153),
	Int(154),
	Int(155),
	Int(156),
	Int(157),
	Int(158),
	Int(159),
	Int(160),
	Int(161),
	Int(162),
	Int(163),
	Int(164),
	Int(165),
	Int(166),
	Int(167),
	Int(168),
	Int(169),
	Int(170),
	Int(171),
	Int(172),
	Int(173),
	Int(174),
	Int(175),
	Int(176),
	Int(177),
	Int(178),
	Int(179),
	Int(180),
	Int(181),
	Int(182),
	Int(183),
	Int(184),
	Int(185),
	Int(186),
	Int(187),
	Int(188),
	Int(189),
	Int(190),
	Int(191),
	Int(192),
	Int(193),
	Int(194),
	Int(195),
	Int(196),
	Int(197),
	Int(198),
	Int(199),
	Int(200),
	Int(201),
	Int(202),
	Int(203),
	Int(204),
	Int(205),
	Int(206),
	Int(207),
	Int(208),
	Int(209),
	Int(210),
	Int(211),
	Int(212),
	Int(213),
	Int(214),
	Int(215),
	Int(216),
	Int(217),
	Int(218),
	Int(219),
	Int(220),
	Int(221),
	Int(222),
	Int(223),
	Int(224),
	Int(225),
	Int(226),
	Int(227),
	Int(228),
	Int(229),
	Int(230),
	Int(231),
	Int(232),
	Int(233),
	Int(234),
	Int(235),
	Int(236),
	Int(237),
	Int(238),
	Int(239),
	Int(240),
	Int(241),
	Int(242),
	Int(243),
	Int(244),
	Int(245),
	Int(246),
	Int(247),
	Int(248),
	Int(249),
	Int(250),
	Int(251),
	Int(252),
	Int(253),
	Int(254),
	Int(255),
}; /* TODO ptr */
AST::Node* internNative(NativeInt value) {
	if(value >= 0 && value < 256)
		return(&integers[value]);
	return(new Int(value));
}
AST::Node* internNative(NativeReal value) {
	return(new SmallReal(value)); /* TODO cache 0, 1. */
}
using namespace AST;
AST::Node* churchTrue = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f t))", NULL));
AST::Node* churchFalse = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f f))", NULL));
AST::Node* internNative(bool value) {
	return(value ? churchTrue : churchFalse);
}
Conser2::Conser2(AST::Node* head) {
	this->head = head;
}
AST::Node* Conser2::execute(AST::Node* argument) {
	/* FIXME error message if it doesn't work. */
	return(cons(head, dynamic_cast<AST::Cons*>(argument)));
}
std::string Conser2::str(void) const {
	return("(cons " + (head ? head->str() : "()") + ")");
}
AST::Node* Conser::execute(AST::Node* argument) {
	return(new Conser2(argument));
}
AST::Node* ConsP::execute(AST::Node* argument) {
	bool result = dynamic_cast<AST::Cons*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* IntP::execute(AST::Node* argument) {
	bool result = dynamic_cast<Int*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* SmallRealP::execute(AST::Node* argument) {
	bool result = dynamic_cast<SmallReal*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* SymbolP::execute(AST::Node* argument) {
	bool result = dynamic_cast<Symbol*>(argument) != NULL || dynamic_cast<SymbolReference*>(argument) != NULL;
	return(internNative(result)); /* TODO SymbolReference? */
}
AST::Node* KeywordP::execute(AST::Node* argument) {
	bool result = dynamic_cast<Keyword*>(argument) != NULL;
	return(internNative(result));
}
#if 0
AST::Node* Int0::execute(AST::Node* argument) {
	return(internNative(0)); /* i.e. integers[0] */
}
#endif
AST::Node* IntSucc::execute(AST::Node* argument) {
	Int* int1 = dynamic_cast<Int*>(argument);
	if(int1) {
		NativeInt value = int1->value;
		if(value + 1 < value) /* overflow */
			return(NULL); /* FIXME bigger numbers */
		return(internNative(value + 1));
	} else
		return(NULL);
}
AST::Node* StrP::execute(AST::Node* argument) {
	using namespace AST;
	bool result = dynamic_cast<Str*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* HeadGetter::execute(AST::Node* argument) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(argument);
	if(consNode)
		return(consNode->head);
	else
		return(NULL); // FIXME proper error message!
}
AST::Node* TailGetter::execute(AST::Node* argument) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(argument);
	if(consNode)
		return(consNode->tail);
	else
		return(NULL); // FIXME proper error message!
}
AST::Node* Interner::execute(AST::Node* argument) {
	AST::Str* stringNode = dynamic_cast<AST::Str*>(argument);
	if(stringNode)
		return(AST::intern(stringNode->text.c_str()));
	else
		return(NULL);
}
AST::Node* KeywordFromStringGetter::execute(AST::Node* argument) {
	AST::Str* stringNode = dynamic_cast<AST::Str*>(argument);
	if(stringNode)
		return(AST::keywordFromString(stringNode->text.c_str()));
	else
		return(NULL);
}
AST::Node* KeywordStr::execute(AST::Node* argument) {
	AST::Keyword* keywordNode = dynamic_cast<AST::Keyword*>(argument);
	if(keywordNode)
		return(str_literal(keywordNode->name)); // TODO stop converting it back and forth and back and forth
	else
		return(NULL);
}
static std::map<AST::Symbol*, AST::Node*> cachedDynamicBuiltins;
static AST::Node* get_dynamic_builtin(AST::Symbol* symbol) {
	const char* name;
	name = symbol->name;
	if((name[0] >= '0' && name[0] <= '9') || name[0] == '-') { /* hello, number */
		long int value;
		char* endptr = NULL;
		value = strtol(name, &endptr, 10);
		if(endptr && *endptr) { /* maybe a real */
			NativeReal value;
			std::istringstream sst(name);
			if(sst >> value)
				return(internNative(value));
			else
				return(NULL);
		} else { /* maybe too big anyway */
			if(value == LONG_MIN || value == LONG_MAX) {
				return(symbol); // TODO allow to hook into this.
			}
		}
		return(internNative((NativeInt) value));
	} else
		return(NULL);
}
AST::Node* provide_dynamic_builtins_impl(AST::Node* body, std::set<AST::Symbol*>::const_iterator end_iter, std::set<AST::Symbol*>::const_iterator iter) {
	if(iter == end_iter)
		return(body);
	else {
		AST::Symbol* name = *iter;
		AST::Node* value = get_dynamic_builtin(name);
		++iter;
		return(value ? Evaluators::close(name, value, provide_dynamic_builtins_impl(body, end_iter, iter)) : provide_dynamic_builtins_impl(body, end_iter, iter));
	}
}
AST::Node* provide_dynamic_builtins(AST::Node* body) {
	std::set<AST::Symbol*> freeNames;
	std::set<AST::Symbol*>::const_iterator end_iter;
	get_free_variables(body, freeNames);
	end_iter = freeNames.end();
	return(provide_dynamic_builtins_impl(body, end_iter, freeNames.begin()));
}
}; /* end namespace Evaluators */
