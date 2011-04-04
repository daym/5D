#include <stdlib.h>
#include "UTFStateMachine"

namespace Formatters {

#include "UTF-8_to_LATEX_result.h"

UTFStateMachine::UTFStateMachine(void) {
	state = 0;
}
unsigned int get_transition(unsigned int state, int input) {
	struct State* result;
	result = &state_table[state];
	for(unsigned int i = 0; i < result->choice_count; ++i)
		if(result->inputs[i] == input && input != 0)
			return(result->new_states[i]);
	for(unsigned int i = result->choice_count - 1; i >= 0; --i)
		if(result->inputs[i] == 0)
			return(result->new_states[i]);
	return(0);
}
/* input is what the next input would have been */
const char* UTFStateMachine::get_final_result(int input) { /* can and will return NULL if there is no final result (yet?) */
	if(get_transition(state, input) == 0) // the end
		return(state_table[state].result);
	return(NULL);
}
unsigned int UTFStateMachine::transition(int input) {
	state = get_transition(state, input);
	return(state);
}
void UTFStateMachine::reset(void) {
	state = 0;
}
}; // end namespace
