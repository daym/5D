#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <5D/FFIs>
#include "Values/Values"
#include "Values/Symbols"
#include "Assemblers/X86"
#include "Evaluators/FFI"

#define REX_W 0
namespace Assemblers {
namespace arch {
namespace X86 {
using namespace Symbols;
#include "Assemblers/X86.inc"
}}};

