#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "AST/AST" /* mostly just for Symbol */
#include "AST/Symbols"

#define REX_W 0
namespace Assemblers {
namespace arch {
namespace X86 {
using namespace Symbols;
#include "X86.inc"
}}};

