#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
import symbols
from symbols import intern
def parse(tokenizer, env):
	"""
	This works like the following:
		#operators is a stack of operators. It is always kept in order of ascending precedence. 
		(non-operator) values are just emitted.
		An opening parenthesis would just be put on the operator stack (without popping anything off), EVEN THOUGH it is recorded with the lowest precedence ever.
		An operator is handled like this: if we try to push a new operator with lower precedence on top, it will keep popping (and emitting) existing operators off the operator stack until the invariant is satisfied. Then the operator is pushed on the operator stack.
		A closing parenthesis is an "operator" which is not being put on the operator stack (it would do nothing anyway), otherwise handled like any other operator.
	"""
	OPL = env
	error = env(intern("error"))
	errorP = env(intern("error?"))
	operatorP = OPL(intern("operator?"))
	operatorArgcount = OPL(intern("operatorArgcount"))
	operatorPrefixNeutral = OPL(intern("operatorPrefixNeutral"))
	operators = [] # deferred operators
	operatorLE = OPL(intern("operator<="))
	openingParenP = OPL(intern("openingParen?"))
	closingParenP = OPL(intern("closingParen?"))
	openingParenOf = OPL(intern("openingParenOf"))
	argcount = 0
	# TODO check argcount and scream at the right moment if necessary
	# TODO support two-argument prefix operators ("if", "let", "import", "\\")
	# TODO in principle, there would also be non-associative operators like relational operators. We handle them as left-associative.
	prevInput = intern("(")
	for input in tokenizer:
		# TODO right-associative operators, suffix operators, prefix operators
		if openingParenP(input):
			operators.append(input)
		elif operatorP(input):
			def unaryPrefixP(input):
				argcount = operatorArgcount(input)
				if argcount > 1:
					neutral = operatorPrefixNeutral(input)
					return not errorP(neutral)
				else:
					return argcount == 1
			bUnary = False
			if operatorP(prevInput): # special-case prefix operators
				bUnary = True
				if operatorArgcount(input) > 1:
					neutral = operatorPrefixNeutral(input)
					yield neutral
					if errorP(neutral):
						return
			# else binary, postfix
			bClosingParen = closingParenP(input)
			insideP = (lambda o, bUnary=bUnary: not openingParenP(o) and (not bUnary or unaryPrefixP(o))) if bClosingParen else (lambda o, bUnary=bUnary: not bUnary or unaryPrefixP(o))
			while len(operators) > 0 and insideP(operators[-1]):
				pendingOperator = operators[-1]
				if operatorLE(input, pendingOperator):
					yield operators.pop()
				else:
					break
			if bClosingParen: # TODO (5*)
				if len(operators) > 0 and (operators[-1] is openingParenOf(input)):
					operators.pop()
				else: # unbalanced paren, wrong paren
					yield error(openingParenOf(input), operators[-1].text if len(operators) > 0 else "<value>")
					return
			else:
				operators.append(input)
			argcount = operatorArgcount(input)
		else:
			yield input
			if not operatorP(prevInput):
				yield intern(" ")
		prevInput = input if not closingParenP(input) else None
	while len(operators) > 0:
		yield operators.pop()
	# TODO check that in the end there is exactly one item on the result stack
if __name__ == "__main__":
	import scanner
	import StringIO as io
	def str1(val):
		if isinstance(val, str):
			return "%r" % val
		else:
			return str(val)
	def env(name):
		level = {
			intern("("): -1,
			intern("{"): -1,
			intern("["): -1,
			intern("."): 32,
			intern("_"): 31,
			intern("^"): 31,
			intern("**"): 30,
			intern("*"): 29,
			intern("⋅"): 29,
			intern("/"): 29,
			intern("⨯"): 28,
			intern(":"): 27,
			intern("'"): 26,
			intern(" "): 24,
			intern("++"): 23,
			intern("+"): 22,
			intern("‒"): 22,
			intern("-"): 22,
			intern("%"): 21,
			intern("∩"): 16,
			intern("∪"): 15,
			intern("∈"): 14,
			intern("⊂"): 14,
			intern("⊃"): 14,
			intern("⊆"): 14,
			intern("⊇"): 14,
			intern("="): 10,
			intern("≟"): 10,
			intern("/="): 10,
			intern("<"): 9,
			intern("<="): 9,
			intern(">"): 9,
			intern(">="): 9,
			intern("≤"): 9,
			intern("≥"): 9,
			intern("&&"): 8,
			intern("∧"): 8,
			intern("||"): 7,
			intern("∨"): 7,
			intern(","): 6,
			intern("$"): 5,
			#intern("if"): 4,
			intern("elif"): 4,
			intern("else"): 4,
			intern("|"): 3,
			intern("=>"): 2,
			intern(";"): 2,
			intern("?;"): 2,
			intern("\\"): 1,
			intern("let"): 0,
			intern("let!"): 0,
			intern("import"): 0,
			intern(")"): -1,
			intern("}"): -1,
			intern("]"): -1,
		}
		def errorP(node):
			return isinstance(node, tuple) and len(node) > 0 and node[0] is intern("error")
		def operatorPrefixNeutral(node):
			return intern("0") if node is intern("-") else \
			       env(intern("error"))("<prefix-operator>", str1(node))
		def operatorP(node):
			return node in level.keys() # is intern("+") or node is intern("-") or node is intern("*") or node is intern("/")
		def operatorArgcount(node): # positive, > 1: left assoc, negative: right assoc. = 1: prefix, = -1: suffix
			R = -2
			N = 2 # FIXME
			P = 1
			S = -1
			return R if node is intern("_") else \
			       R if node is intern("^") else \
			       R if node is intern("**") else \
			       R if node is intern("⨯") else \
			       R if node is intern(":") else \
			       P if node is intern("'") else \
			       N if node is intern("∈") else \
			       N if node is intern("⊂") else \
			       N if node is intern("⊃") else \
			       N if node is intern("⊆") else \
			       N if node is intern("⊇") else \
			       N if node is intern("=") else \
			       N if node is intern("≟") else \
			       N if node is intern("/=") else \
			       N if node is intern("<") else \
			       N if node is intern("<=") else \
			       N if node is intern(">") else \
			       N if node is intern(">=") else \
			       N if node is intern("≤") else \
			       N if node is intern("≥") else \
			       R if node is intern(",") else \
			       R if node is intern("$") else \
			       R if node is intern("elif") else \
			       R if node is intern("else") else \
			       R if node is intern(";") else \
			       P if node is intern("\\") else \
			       P if node is intern("let") else \
			       P if node is intern("let!") else \
			       P if node is intern("import") else \
			       2
		def operatorLE(a,b):
			return level[a] < level[b] or (level[a] == level[b] and operatorArgcount(b) > 0) # latter: leave right-associative operators on stack if in doubt.
		def openingParenP(node):
			return node is intern("(") or node is intern("{") or node is intern("[")
		def closingParenP(node):
			return node is intern(")") or node is intern("}") or node is intern("]")
		def openingParenOf(node):
			return intern("(") if node is intern(")") else intern("{") if node is intern("}") else intern("[") if node is intern("]") else intern("<none>")
		#def closingParenOfP(node):
		def shebang(v):
			return None
		def stringFromSymbol(v):
			return v.text
		if name == intern("operator?"):
			return operatorP
		elif name == intern("operatorArgcount"):
			return operatorArgcount
		elif name == intern("operator<="):
			return operatorLE
		elif name == intern("openingParen?"):
			return openingParenP
		elif name == intern("closingParen?"):
			return closingParenP
		elif name == intern("openingParenOf"):
			return openingParenOf
		elif name == intern("operatorPrefixNeutral"):
			return operatorPrefixNeutral
		elif name == intern("#!"):
			return shebang
		elif name == intern("stringFromSymbol"):
			return stringFromSymbol
		elif name == intern("#"):
			def readHash(inputFile, c):
				if c == 'e': # exports, probably.
					if inputFile.read(len("xports")) == "xports":
						#if inputFile.read(1) == '[':
						print "FIXME run the entire parser"
						return None, inputFile.read(1)
				elif c == 't':
					return intern("#t"), inputFile.read(1)
				elif c == 'f':
					return intern("#f"), inputFile.read(1)
				return (env(intern("error"))("<value>"), "")
			return readHash
		elif name is intern("error"):
			return lambda *args: (intern("error"), "expected \"%s\", got \"%s\"" % (args[0], args[1]))
		elif name is intern("error?"):
			return errorP
		else:
			#return env(intern("error"))("<envitem>", name)
			return lambda *args: (env(intern("error"))("<envitem>", name), "")
	print "---"
	errorP = env(intern("error?"))
	def test1(text, expected):
		got = [x for x in parse(scanner.tokenize(io.StringIO(text), env), env)]
		print text, got
		assert got == map(intern, expected)
	def test1Error(text, expected):
		f = io.StringIO(text)
		got = [x for x in parse(scanner.tokenize(f, env), env) if errorP(x)]
		assert(len(got) > 0)
		print "positionx", f.tell(), got[-1], errorP(got[-1])
	test1("2+3", ["2", "3", "+"])
	test1("2+3*5", ["2", "3", "5", "*", "+"])
	test1("2*3+5", ["2", "3", "*", "5", "+"])
	test1("2*(3+5)", ["2", "3", "5", "+", "*"])
	test1("2*(3+(5+4)+2)", ["2", "3", "5", "4", "+", "+", "2", "+", "*"])
	test1("2*3*4+5", ["2", "3", "*", "4", "*", "5", "+"])
	test1("2*3*4+5-10/3", ["2", "3", "*", "4", "*", "5", "+", "10", "3", "/", "-"])
	test1("-3", ["0", "3", "-"])
	test1("2** -3", ["2", "0", "3", "-", "**"])
	test1("let x = 5", ["x", "let"])
	test1Error(")", [])
	test1Error("3*", [])
	#inputFile = io.StringIO("2+2")
	co = 31
	inputFile = open(sys.argv[1], "r")
	for val in scanner.tokenize(inputFile, env):
		sys.stdout.write("\033[%dm" % co)
		co = 32 if co == 31 else 31
		sys.stdout.write(str1(val) + " ")
		if(str(val) == "<LF>"):
			sys.stdout.write("\n")
	sys.stdout.write("\033[m")
	inputFile.seek(0)
	for val in parse(scanner.tokenize(inputFile, env), env):
		print val,

