/** Allocator generation via LLVM
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir/ir.h"
#include "../parser/lexer.h"
#include "../shared/error.h"
#include "../coneopts.h"
#include "../ir/nametbl.h"
#include "../shared/fileio.h"
#include "genllvm.h"

#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Transforms/Scalar.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

// Function declarations for malloc() and free()
LLVMValueRef genlmallocval = NULL;
LLVMValueRef genlfreeval = NULL;

// Call malloc() (and generate declaration if needed)
LLVMValueRef genlmalloc(GenState *gen, long long size) {
    // Declare malloc() external function
    if (genlmallocval == NULL) {
        LLVMTypeRef parmtype = (LLVMPointerSize(gen->datalayout) == 4) ? LLVMInt32TypeInContext(gen->context) : LLVMInt64TypeInContext(gen->context);
        LLVMTypeRef rettype = LLVMPointerType(LLVMInt8TypeInContext(gen->context), 0);
        LLVMTypeRef fnsig = LLVMFunctionType(rettype, &parmtype, 1, 0);
        genlmallocval = LLVMAddFunction(gen->module, "malloc", fnsig);
    }
    // Call malloc
    LLVMValueRef sizeval = LLVMConstInt(genlType(gen, (INode*)usizeType), size, 0);
    return LLVMBuildCall(gen->builder, genlmallocval, &sizeval, 1, "");
}

// Call free() (and generate declaration if needed)
LLVMValueRef genlFree(GenState *gen, LLVMValueRef ref) {
    LLVMTypeRef parmtype = LLVMPointerType(LLVMInt8TypeInContext(gen->context), 0);
    // Declare free() external function
    if (genlfreeval == NULL) {
        LLVMTypeRef rettype = LLVMVoidTypeInContext(gen->context);
        LLVMTypeRef fnsig = LLVMFunctionType(rettype, &parmtype, 1, 0);
        genlfreeval = LLVMAddFunction(gen->module, "free", fnsig);
    }
    // Cast ref to *u8 and then call free()
    LLVMValueRef refcast = LLVMBuildBitCast(gen->builder, ref, parmtype, "");
    return LLVMBuildCall(gen->builder, genlfreeval, &refcast, 1, "");
}

// Generate code that creates an allocated ref by allocating and initializing
LLVMValueRef genlallocref(GenState *gen, AddrNode *addrnode) {
    RefNode *reftype = (RefNode*)addrnode->vtype;
    long long valsize = LLVMABISizeOfType(gen->datalayout, genlType(gen, reftype->pvtype));
    long long allocsize = 0;
    if (reftype->alloc == (INode*)rcAlloc)
        allocsize = LLVMABISizeOfType(gen->datalayout, genlType(gen, (INode*)usizeType));
    LLVMValueRef malloc = genlmalloc(gen, allocsize + valsize);
    if (reftype->alloc == (INode*)rcAlloc) {
        LLVMValueRef constone = LLVMConstInt(genlType(gen, (INode*)usizeType), 1, 0);
        LLVMTypeRef ptrusize = LLVMPointerType(genlType(gen, (INode*)usizeType), 0);
        LLVMValueRef counterptr = LLVMBuildBitCast(gen->builder, malloc, ptrusize, "");
        LLVMBuildStore(gen->builder, constone, counterptr); // Store 1 into refcounter
        malloc = LLVMBuildGEP(gen->builder, malloc, &constone, 1, ""); // Point to value
    }
    LLVMValueRef valcast = LLVMBuildBitCast(gen->builder, malloc, genlType(gen, addrnode->vtype), "");
    LLVMBuildStore(gen->builder, genlExpr(gen, addrnode->exp), valcast);
    return valcast;
}

// Dealias a lex allocated reference
void genlDealiasLex(GenState *gen, LLVMValueRef ref) {
    genlFree(gen, ref);
}

// Dealias an rc allocated reference
void genlDealiasRc(GenState *gen, LLVMValueRef ref) {
    // Point backwards to ref counter
    LLVMTypeRef usize = genlType(gen, (INode*)usizeType);
    LLVMValueRef intptr = LLVMBuildPtrToInt(gen->builder, ref, usize, "");
    LLVMValueRef cntintptr = LLVMBuildSub(gen->builder, intptr, genlSizeof(gen, (INode*)usizeType), "");
    LLVMTypeRef ptrusize = LLVMPointerType(genlType(gen, (INode*)usizeType), 0);
    LLVMValueRef cntptr = LLVMBuildIntToPtr(gen->builder, cntintptr, ptrusize, "refcount");

    // Decrement ref counter
    LLVMValueRef cnt = LLVMBuildLoad(gen->builder, cntptr, "");
    LLVMValueRef newcnt = LLVMBuildSub(gen->builder, cnt, LLVMConstInt(usize, 1, 0), "");
    LLVMBuildStore(gen->builder, newcnt, cntptr);

    // Free if zero. Otherwise, don't
    LLVMBasicBlockRef nofree = genlInsertBlock(gen, "nofree");
    LLVMBasicBlockRef dofree = genlInsertBlock(gen, "free");
    LLVMValueRef test = LLVMBuildICmp(gen->builder, LLVMIntEQ, newcnt, LLVMConstInt(usize, 0, 0), "iszero");
    LLVMBuildCondBr(gen->builder, test, dofree, nofree);
    LLVMPositionBuilderAtEnd(gen->builder, dofree);
    genlFree(gen, cntptr);
    LLVMBuildBr(gen->builder, nofree);
    LLVMPositionBuilderAtEnd(gen->builder, nofree);
}

// Is value's type an Rc allocated ref we might copy (and increment refcount)?
int genlDoAliasRc(INode *rval) {
    RefNode *reftype = (RefNode *)iexpGetTypeDcl(rval);
    if (reftype->tag != RefTag || reftype->alloc != (INode*)rcAlloc)
        return 0;

    switch (rval->tag) {
    // Assignment/fncall copies increment counter on refs from these nodes
    case AssignTag:
    case DerefTag:
    case VarNameUseTag:
    case StrFieldTag:
    case ArrIndexTag:
        return 1;

    // Decrement counter on these nodes when block throws out the value
    // (Copies don't increment, because counter is already +1)
    case AddrTag:
    case FnCallTag:
    case BlockTag:
    case IfTag:
        return -1;

    // Don't allow vtuple to get here!
    default:
        assert(0 && "Invalid tag");
    }
    return 0;
}

// Alias an rc allocated reference
void genlAliasRc(GenState *gen, LLVMValueRef ref) {
    // Point backwards to ref counter
    LLVMTypeRef usize = genlType(gen, (INode*)usizeType);
    LLVMValueRef intptr = LLVMBuildPtrToInt(gen->builder, ref, usize, "");
    LLVMValueRef cntintptr = LLVMBuildSub(gen->builder, intptr, genlSizeof(gen, (INode*)usizeType), "");
    LLVMTypeRef ptrusize = LLVMPointerType(genlType(gen, (INode*)usizeType), 0);
    LLVMValueRef cntptr = LLVMBuildIntToPtr(gen->builder, cntintptr, ptrusize, "refcount");

    // Increment ref counter
    LLVMValueRef cnt = LLVMBuildLoad(gen->builder, cntptr, "");
    LLVMValueRef newcnt = LLVMBuildAdd(gen->builder, cnt, LLVMConstInt(usize, 1, 0), "");
    LLVMBuildStore(gen->builder, newcnt, cntptr);
}

// Progressively dealias or drop all declared variables in nodes list
void genlDealiasNodes(GenState *gen, Nodes *nodes) {
    if (nodes == NULL)
        return;
    INode **nodesp;
    uint32_t cnt;
    for (nodesFor(nodes, cnt, nodesp)) {
        VarDclNode *var = (VarDclNode *)*nodesp;
        RefNode *reftype = (RefNode *)var->vtype;
        if (reftype->tag == RefTag) {
            LLVMValueRef ref = LLVMBuildLoad(gen->builder, var->llvmvar, "allocref");
            if (reftype->alloc == (INode*)lexAlloc) {
                genlDealiasLex(gen, ref);
            }
            else if (reftype->alloc == (INode*)rcAlloc) {
                genlDealiasRc(gen, ref);
            }
        }
    }
}
