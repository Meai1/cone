/** Handling for logic nodes
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "../ir.h"

// Create a new logic operator node
LogicNode *newLogicNode(int16_t typ) {
    LogicNode *node;
    newNode(node, LogicNode, typ);
    node->vtype = (INode*)boolType;
    return node;
}

// Serialize logic node
void logicPrint(LogicNode *node) {
    if (node->tag == NotLogicTag) {
        inodeFprint("!");
        inodePrintNode(node->lexp);
    }
    else {
        inodeFprint(node->tag == AndLogicTag? "(&&, " : "(||, ");
        inodePrintNode(node->lexp);
        inodeFprint(", ");
        inodePrintNode(node->rexp);
        inodeFprint(")");
    }
}

// Name resolution of not logic node
void logicNotNameRes(NameResState *pstate, LogicNode *node) {
    inodeNameRes(pstate, &node->lexp);
}

// Type check not logic node
void logicNotTypeCheck(TypeCheckState *pstate, LogicNode *node) {
    inodeTypeCheck(pstate, &node->lexp);
    if (0 == iexpCoerces((INode*)boolType, &node->lexp))
        errorMsgNode(node->lexp, ErrorInvType, "Conditional expression must be coercible to boolean value.");
}

// Name resolution of logic node
void logicNameRes(NameResState *pstate, LogicNode *node) {
    inodeNameRes(pstate, &node->lexp);
    inodeNameRes(pstate, &node->rexp);
}

// Analyze logic node
void logicTypeCheck(TypeCheckState *pstate, LogicNode *node) {
    inodeTypeCheck(pstate, &node->lexp);
    inodeTypeCheck(pstate, &node->rexp);

    if (0 == iexpCoerces((INode*)boolType, &node->lexp))
        errorMsgNode(node->lexp, ErrorInvType, "Conditional expression must be coercible to boolean value.");
    if (0 == iexpCoerces((INode*)boolType, &node->rexp))
        errorMsgNode(node->rexp, ErrorInvType, "Conditional expression must be coercible to boolean value.");
}
