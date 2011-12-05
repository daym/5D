
static AST::Node* wrapMessageBox(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
	std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
	AST::Str* text = dynamic_cast<AST::Box*>(iter->second);
	AST::Node* messageType = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("@type:"));
	MessageBoxW(
	++iter;
	AST::Node* world = iter->second;
	char text[2049];
	AST::Node* result = NULL;
	if(fgets(text, 2048, (FILE*) f->native)) {
		result = Evaluators::internNative(text);
	} else
		result = NULL;
	return(Evaluators::makeIOMonad(result, world));
}
