#ifndef GENCODE_H
#define GENCODE_H

#include "symtable.h"

struct cst * craft_operation(
	const struct cst *,
	const struct cst *,
	const char *,
	const char *);

struct cst * craft_boolean_operation(
	const struct cst *,
	const struct cst *,
	const char *);

int craft_store(struct var *, const struct cst *);

#endif
