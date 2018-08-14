/** AST handling for references and pointers
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir.h"
#include "../../shared/memory.h"
#include "../../parser/lexer.h"
#include "../nametbl.h"

// Create a new pointer type whose info will be filled in afterwards
PtrAstNode *newPtrTypeNode() {
	PtrAstNode *ptrnode;
	newNode(ptrnode, PtrAstNode, RefTag);
	return ptrnode;
}

// Serialize a pointer type
void ptrTypePrint(PtrAstNode *node) {
	inodeFprint(node->asttype == RefTag? "&(" : "*(");
	inodePrintNode(node->alloc);
	inodeFprint(", ");
	inodePrintNode((INode*)node->perm);
	inodeFprint(", ");
	inodePrintNode(node->pvtype);
	inodeFprint(")");
}

// Semantically analyze a pointer type
void ptrTypePass(PassState *pstate, PtrAstNode *node) {
	inodeWalk(pstate, &node->alloc);
	inodeWalk(pstate, (INode**)&node->perm);
	inodeWalk(pstate, &node->pvtype);
}

// Compare two pointer signatures to see if they are equivalent
int ptrTypeEqual(PtrAstNode *node1, PtrAstNode *node2) {
	return typeIsSame(node1->pvtype,node2->pvtype) 
		&& permIsSame(node1->perm, node2->perm)
		&& node1->alloc == node2->alloc;
}

// Will from pointer coerce to a to pointer (we know they are not the same)
int ptrTypeMatches(PtrAstNode *to, PtrAstNode *from) {
	if (0 == permMatches(to->perm, from->perm)
		|| (to->alloc != from->alloc && to->alloc != voidType))
		return 0;
	return typeMatches(to->pvtype, from->pvtype) == 1 ? 1 : 2;
}