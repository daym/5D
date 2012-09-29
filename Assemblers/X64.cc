#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "AST/AST"
#include "AST/Symbols"
#include "Assemblers/X64"
#include "Evaluators/FFI"

#define REX_W 0x48
namespace Assemblers {
namespace arch {
namespace X64 {
using namespace Symbols;
#include "Assemblers/X86.inc"
}}};
