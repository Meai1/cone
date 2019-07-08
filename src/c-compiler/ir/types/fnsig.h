/** Handling for function signature
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#ifndef fnsig_h
#define fnsig_h

typedef struct FnCallNode FnCallNode;

// Function signature is a type that defines the parameters and return type for a function.
// A function signature is never named (although a ptr/ref to a fnsig may be named).
// The parameter declaration list represents a namespace of local variables.
typedef struct FnSigNode {
    INodeHdr;
    Nodes *parms;            // Declared parameter nodes w/ defaults (VarDclTag)
    INode *rettype;        // void, a single type or a type tuple
} FnSigNode;

FnSigNode *newFnSigNode();
void fnSigPrint(FnSigNode *node);
// Name resolution of the function signature
void fnSigNameRes(NameResState *pstate, FnSigNode *sig);
void fnSigTypeCheck(TypeCheckState *pstate, FnSigNode *name);
int fnSigEqual(FnSigNode *node1, FnSigNode *node2);
int fnSigMatchesCall(FnSigNode *to, Nodes *args);
// Will the method call (caller) be able to call the 'to' function
// Return 0 if not. 1 if perfect match. 2+ for imperfect matches
int fnSigMatchMethCall(FnSigNode *to, Nodes *args);

#endif