#include "lab.h"
#include "lab_designs.h"
#include "kit/kit.h"
#define LAB_EXTERN(sym) extern const LabDesign sym;
LAB_DESIGNS(LAB_EXTERN)
#define LAB_POINTER(sym) &sym,
static const LabDesign *const DESIGNS[] = { LAB_DESIGNS(LAB_POINTER) };
int main(void) { kit_shell_run(DESIGNS[0]); }
