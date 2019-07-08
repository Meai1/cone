/** Handling for record-based types with properties
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#ifndef struct_h
#define struct_h

// This is a reusable structure for named types having properties
// (e.g., struct, trait, interface, alloc, etc.)
// It holds:
// - properties. An ordered list of nodes for variable-like elements 
//     that hold typed data. All/most nodes are named and findable.
// - methods. A name-mapped node hash for methods.
//     A method may be a FnTuple, a list of methods with same name.
//     Note: the namespace is comprised of properties + methods
//       with no duplicate names between them
// - subtypes. An ordered list of nodes for its traits/interfaces
typedef struct StructNode {
    IMethodNodeHdr;
} StructNode;

#define FlagStructOpaque   0x8000  // Has no fields
#define FlagStructNoCopy   0x4000  // Only supports move semantics
#define FlagStructPrivate  0x2000  // Has private fields

StructNode *newStructNode(Name *namesym);
void structPrint(StructNode *node);

// Name resolution of a struct type
void structNameRes(NameResState *pstate, StructNode *node);

// Type check a struct type
void structTypeCheck(TypeCheckState *pstate, StructNode *name);

int structEqual(StructNode *node1, StructNode *node2);
int structCoerces(StructNode *to, StructNode *from);

#endif