#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <assert.h>
#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "FFIs/POSIX"

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
	if(argument != NULL && (dynamic_cast<Operation*>(argument) != NULL))
		return(argument); /* FIXME return true or false */
	else
		return(NULL);
}
static SmallInteger integers[256] = {
	SmallInteger(0),
	SmallInteger(1),
	SmallInteger(2),
	SmallInteger(3),
	SmallInteger(4),
	SmallInteger(5),
	SmallInteger(6),
	SmallInteger(7),
	SmallInteger(8),
	SmallInteger(9),
	SmallInteger(10),
	SmallInteger(11),
	SmallInteger(12),
	SmallInteger(13),
	SmallInteger(14),
	SmallInteger(15),
	SmallInteger(16),
	SmallInteger(17),
	SmallInteger(18),
	SmallInteger(19),
	SmallInteger(20),
	SmallInteger(21),
	SmallInteger(22),
	SmallInteger(23),
	SmallInteger(24),
	SmallInteger(25),
	SmallInteger(26),
	SmallInteger(27),
	SmallInteger(28),
	SmallInteger(29),
	SmallInteger(30),
	SmallInteger(31),
	SmallInteger(32),
	SmallInteger(33),
	SmallInteger(34),
	SmallInteger(35),
	SmallInteger(36),
	SmallInteger(37),
	SmallInteger(38),
	SmallInteger(39),
	SmallInteger(40),
	SmallInteger(41),
	SmallInteger(42),
	SmallInteger(43),
	SmallInteger(44),
	SmallInteger(45),
	SmallInteger(46),
	SmallInteger(47),
	SmallInteger(48),
	SmallInteger(49),
	SmallInteger(50),
	SmallInteger(51),
	SmallInteger(52),
	SmallInteger(53),
	SmallInteger(54),
	SmallInteger(55),
	SmallInteger(56),
	SmallInteger(57),
	SmallInteger(58),
	SmallInteger(59),
	SmallInteger(60),
	SmallInteger(61),
	SmallInteger(62),
	SmallInteger(63),
	SmallInteger(64),
	SmallInteger(65),
	SmallInteger(66),
	SmallInteger(67),
	SmallInteger(68),
	SmallInteger(69),
	SmallInteger(70),
	SmallInteger(71),
	SmallInteger(72),
	SmallInteger(73),
	SmallInteger(74),
	SmallInteger(75),
	SmallInteger(76),
	SmallInteger(77),
	SmallInteger(78),
	SmallInteger(79),
	SmallInteger(80),
	SmallInteger(81),
	SmallInteger(82),
	SmallInteger(83),
	SmallInteger(84),
	SmallInteger(85),
	SmallInteger(86),
	SmallInteger(87),
	SmallInteger(88),
	SmallInteger(89),
	SmallInteger(90),
	SmallInteger(91),
	SmallInteger(92),
	SmallInteger(93),
	SmallInteger(94),
	SmallInteger(95),
	SmallInteger(96),
	SmallInteger(97),
	SmallInteger(98),
	SmallInteger(99),
	SmallInteger(100),
	SmallInteger(101),
	SmallInteger(102),
	SmallInteger(103),
	SmallInteger(104),
	SmallInteger(105),
	SmallInteger(106),
	SmallInteger(107),
	SmallInteger(108),
	SmallInteger(109),
	SmallInteger(110),
	SmallInteger(111),
	SmallInteger(112),
	SmallInteger(113),
	SmallInteger(114),
	SmallInteger(115),
	SmallInteger(116),
	SmallInteger(117),
	SmallInteger(118),
	SmallInteger(119),
	SmallInteger(120),
	SmallInteger(121),
	SmallInteger(122),
	SmallInteger(123),
	SmallInteger(124),
	SmallInteger(125),
	SmallInteger(126),
	SmallInteger(127),
	SmallInteger(128),
	SmallInteger(129),
	SmallInteger(130),
	SmallInteger(131),
	SmallInteger(132),
	SmallInteger(133),
	SmallInteger(134),
	SmallInteger(135),
	SmallInteger(136),
	SmallInteger(137),
	SmallInteger(138),
	SmallInteger(139),
	SmallInteger(140),
	SmallInteger(141),
	SmallInteger(142),
	SmallInteger(143),
	SmallInteger(144),
	SmallInteger(145),
	SmallInteger(146),
	SmallInteger(147),
	SmallInteger(148),
	SmallInteger(149),
	SmallInteger(150),
	SmallInteger(151),
	SmallInteger(152),
	SmallInteger(153),
	SmallInteger(154),
	SmallInteger(155),
	SmallInteger(156),
	SmallInteger(157),
	SmallInteger(158),
	SmallInteger(159),
	SmallInteger(160),
	SmallInteger(161),
	SmallInteger(162),
	SmallInteger(163),
	SmallInteger(164),
	SmallInteger(165),
	SmallInteger(166),
	SmallInteger(167),
	SmallInteger(168),
	SmallInteger(169),
	SmallInteger(170),
	SmallInteger(171),
	SmallInteger(172),
	SmallInteger(173),
	SmallInteger(174),
	SmallInteger(175),
	SmallInteger(176),
	SmallInteger(177),
	SmallInteger(178),
	SmallInteger(179),
	SmallInteger(180),
	SmallInteger(181),
	SmallInteger(182),
	SmallInteger(183),
	SmallInteger(184),
	SmallInteger(185),
	SmallInteger(186),
	SmallInteger(187),
	SmallInteger(188),
	SmallInteger(189),
	SmallInteger(190),
	SmallInteger(191),
	SmallInteger(192),
	SmallInteger(193),
	SmallInteger(194),
	SmallInteger(195),
	SmallInteger(196),
	SmallInteger(197),
	SmallInteger(198),
	SmallInteger(199),
	SmallInteger(200),
	SmallInteger(201),
	SmallInteger(202),
	SmallInteger(203),
	SmallInteger(204),
	SmallInteger(205),
	SmallInteger(206),
	SmallInteger(207),
	SmallInteger(208),
	SmallInteger(209),
	SmallInteger(210),
	SmallInteger(211),
	SmallInteger(212),
	SmallInteger(213),
	SmallInteger(214),
	SmallInteger(215),
	SmallInteger(216),
	SmallInteger(217),
	SmallInteger(218),
	SmallInteger(219),
	SmallInteger(220),
	SmallInteger(221),
	SmallInteger(222),
	SmallInteger(223),
	SmallInteger(224),
	SmallInteger(225),
	SmallInteger(226),
	SmallInteger(227),
	SmallInteger(228),
	SmallInteger(229),
	SmallInteger(230),
	SmallInteger(231),
	SmallInteger(232),
	SmallInteger(233),
	SmallInteger(234),
	SmallInteger(235),
	SmallInteger(236),
	SmallInteger(237),
	SmallInteger(238),
	SmallInteger(239),
	SmallInteger(240),
	SmallInteger(241),
	SmallInteger(242),
	SmallInteger(243),
	SmallInteger(244),
	SmallInteger(245),
	SmallInteger(246),
	SmallInteger(247),
	SmallInteger(248),
	SmallInteger(249),
	SmallInteger(250),
	SmallInteger(251),
	SmallInteger(252),
	SmallInteger(253),
	SmallInteger(254),
	SmallInteger(255),
}; /* TODO ptr */
AST::Node* internNative(int value) {
	if(value < 256)
		return(&integers[value]);
	return(new SmallInteger(value));
}
AST::Node* internNative(NativeReal value) {
	return(new SmallReal(value)); /* TODO cache 0, 1. */
}
AST::Node* internNative(bool value) {
	/* FIXME */
	return(NULL);
}
Conser2::Conser2(AST::Node* head) {
	this->head = head;
}
AST::Node* Conser2::execute(AST::Node* argument) {
	/* FIXME error message if it doesn't work. */
	return(cons(head, dynamic_cast<AST::Cons*>(argument)));
}
AST::Node* Conser::execute(AST::Node* argument) {
	return(new Conser2(argument));
}
AST::Node* ConsP::execute(AST::Node* argument) {
	bool result = dynamic_cast<AST::Cons*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* SmallIntegerP::execute(AST::Node* argument) {
	bool result = dynamic_cast<SmallInteger*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* SmallRealP::execute(AST::Node* argument) {
	bool result = dynamic_cast<SmallReal*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* StringP::execute(AST::Node* argument) {
	using namespace AST;
	bool result = dynamic_cast<String*>(argument) != NULL;
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
static std::map<AST::Symbol*, AST::Node*> cachedDynamicBuiltins;
static AST::Node* get_dynamic_builtin(AST::Symbol* symbol) {
	const char* name;
	name = symbol->name;
	if((name[0] >= '0' && name[0] <= '9') || name[0] == '-') { /* hello, number */
		long int value;
		char* endptr = NULL;
		value = strtol(name, &endptr, 10);
		if(endptr || *endptr) { /* maybe a real */
			NativeReal value;
			std::istringstream sst(name);
			if(sst >> value)
				return(internNative(value));
			else
				return(NULL);
		}
		assert(sizeof(long int) == sizeof(int));
		return(internNative((int) value));
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
