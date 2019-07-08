/** Handling for variable declaration nodes
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#ifndef vardcl_h
#define vardcl_h

// Name declaration node (e.g., variable, fn implementation, or named type)
typedef struct VarDclNode {
    INamedNodeHdr;                // 'vtype': type of this name's value
    INode *value;                // Starting value/declaration (NULL if not initialized)
    LLVMValueRef llvmvar;        // LLVM's handle for a declared variable (for generation)
    INode *perm;            // Permission type (often mut or imm)
    uint16_t scope;                // 0=global
    uint16_t index;                // index within this scope (e.g., parameter number)
    uint16_t flowflags;         // Data flow pass permanent flags
    uint16_t flowtempflags;     // Data flow pass temporary flags
} VarDclNode;

enum VarFlowTemp {
    VarInitialized = 0x0001     // Variable has been initialized
};

VarDclNode *newVarDclNode(Name *namesym, uint16_t tag, INode *perm);
VarDclNode *newVarDclFull(Name *namesym, uint16_t tag, INode *sig, INode *perm, INode *val);
void varDclPrint(VarDclNode *fn);

// Name resolution of vardcl
void varDclNameRes(NameResState *pstate, VarDclNode *node);

// Type check vardcl
void varDclTypeCheck(TypeCheckState *pstate, VarDclNode *node);

// Perform data flow analysis
void varDclFlow(FlowState *fstate, VarDclNode **vardclnode);

#endif