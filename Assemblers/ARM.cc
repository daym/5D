#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "AST/AST" /* mostly just for Symbol */
#include "AST/Symbols"
#include "Assemblers/ARM"
#include "Evaluators/FFI"

#define REX_W 0
namespace Assemblers {
namespace arch {
namespace ARM {
using namespace Symbols;
#include "Assemblers/ARM.inc"
}}};

