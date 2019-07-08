/** Handling for value tuple nodes
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#ifndef vtuple_h
#define vtuple_h

// A value tuple is a comma-separated list of values, each of a different type
// It acts like an ad hoc struct, briefly binding together values often
// for a parallel assignment or multiple return values
typedef struct VTupleNode {
    ITypedNodeHdr;
    Nodes *values;
} VTupleNode;

// Create a new value tuple node
VTupleNode *newVTupleNode();

// Serialize a value tuple node
void vtuplePrint(VTupleNode *tuple);

// Name resolution for vtuple
void vtupleNameRes(NameResState *pstate, VTupleNode *tuple);

// Type check the value tuple node
// - Infer type tuple from types of vtuple's values
void vtupleTypeCheck(TypeCheckState *pstate, VTupleNode *node);

#endif