// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode sub-types representing expressions
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2022 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
// This files contains all 'AstNode' sub-types that representing expressions,
// i.e.: those constructs that evaluate to [a possible void/unit] value.  The
// root of the hierarchy is 'AstNodeMath', which could also be called
// 'AstNodeExpr'.
//
// Warning: Although the above is what we are aiming for, currently there are
// some 'AstNode' sub-types defined elsewhere, that represent expressions but
// are not part of the `AstNodeMath` hierarchy (e.g.: 'AstNodeCall' and its
// sub-types). These should eventually be fixed and moved under 'AstNodeMath'.
//
//*************************************************************************

#ifndef VERILATOR_V3ASTNODEMATH_H_
#define VERILATOR_V3ASTNODEMATH_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#include "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class AstNodeMath VL_NOT_FINAL : public AstNode {
    // Math -- anything that's part of an expression tree
protected:
    AstNodeMath(VNType t, FileLine* fl)
        : AstNode{t, fl} {}

public:
    ASTGEN_MEMBERS_NodeMath;
    // METHODS
    void dump(std::ostream& str) const override;
    bool hasDType() const override { return true; }
    virtual string emitVerilog() = 0;  /// Format string for verilog writing; see V3EmitV
    // For documentation on emitC format see EmitCFunc::emitOpName
    virtual string emitC() = 0;
    virtual string emitSimpleOperator() { return ""; }  // "" means not ok to use
    virtual bool emitCheckMaxWords() { return false; }  // Check VL_MULS_MAX_WORDS
    virtual bool cleanOut() const = 0;  // True if output has extra upper bits zero
    // Someday we will generically support data types on every math node
    // Until then isOpaque indicates we shouldn't constant optimize this node type
    bool isOpaque() const { return VN_IS(this, CvtPackString); }
};
class AstNodeBiop VL_NOT_FINAL : public AstNodeMath {
    // Binary expression
    // @astgen op1 := lhsp : AstNode
    // @astgen op2 := rhsp : AstNode
protected:
    AstNodeBiop(VNType t, FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : AstNodeMath{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }

public:
    ASTGEN_MEMBERS_NodeBiop;
    // Clone single node, just get same type back.
    virtual AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) = 0;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    virtual void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) = 0;
    virtual bool cleanLhs() const = 0;  // True if LHS must have extra upper bits zero
    virtual bool cleanRhs() const = 0;  // True if RHS must have extra upper bits zero
    virtual bool sizeMattersLhs() const = 0;  // True if output result depends on lhs size
    virtual bool sizeMattersRhs() const = 0;  // True if output result depends on rhs size
    virtual bool doubleFlavor() const { return false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    virtual bool signedFlavor() const { return false; }
    virtual bool stringFlavor() const { return false; }  // N flavor of nodes with both flavors?
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode*) const override { return true; }
};
class AstNodeBiCom VL_NOT_FINAL : public AstNodeBiop {
    // Binary math with commutative properties
protected:
    AstNodeBiCom(VNType t, FileLine* fl, AstNode* lhs, AstNode* rhs)
        : AstNodeBiop{t, fl, lhs, rhs} {}

public:
    ASTGEN_MEMBERS_NodeBiCom;
};
class AstNodeBiComAsv VL_NOT_FINAL : public AstNodeBiCom {
    // Binary math with commutative & associative properties
protected:
    AstNodeBiComAsv(VNType t, FileLine* fl, AstNode* lhs, AstNode* rhs)
        : AstNodeBiCom{t, fl, lhs, rhs} {}

public:
    ASTGEN_MEMBERS_NodeBiComAsv;
};
class AstNodeSel VL_NOT_FINAL : public AstNodeBiop {
    // Single bit range extraction, perhaps with non-constant selection or array selection
    // @astgen alias op1 := fromp // Expression we are indexing into
    // @astgen alias op2 := bitp // The index // TOOD: rename to idxp
protected:
    AstNodeSel(VNType t, FileLine* fl, AstNode* fromp, AstNode* bitp)
        : AstNodeBiop{t, fl, fromp, bitp} {}

public:
    ASTGEN_MEMBERS_NodeSel;
    int bitConst() const;
    bool hasDType() const override { return true; }
};
class AstNodeStream VL_NOT_FINAL : public AstNodeBiop {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
protected:
    AstNodeStream(VNType t, FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : AstNodeBiop{t, fl, lhsp, rhsp} {
        if (lhsp->dtypep()) dtypeSetLogicSized(lhsp->dtypep()->width(), VSigning::UNSIGNED);
    }

public:
    ASTGEN_MEMBERS_NodeStream;
};
class AstNodeSystemBiop VL_NOT_FINAL : public AstNodeBiop {
public:
    AstNodeSystemBiop(VNType t, FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : AstNodeBiop(t, fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_NodeSystemBiop;
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL_TRIG; }
    bool doubleFlavor() const override { return true; }
};
class AstNodeQuadop VL_NOT_FINAL : public AstNodeMath {
    // 4-ary expression
    // @astgen op1 := lhsp : AstNode
    // @astgen op2 := rhsp : AstNode
    // @astgen op3 := thsp : AstNode
    // @astgen op4 := fhsp : AstNode
protected:
    AstNodeQuadop(VNType t, FileLine* fl, AstNode* lhsp, AstNode* rhsp, AstNode* thsp,
                  AstNode* fhsp)
        : AstNodeMath{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
        this->fhsp(fhsp);
    }

public:
    ASTGEN_MEMBERS_NodeQuadop;
    // METHODS
    // Set out to evaluation of a AstConst'ed
    virtual void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                               const V3Number& ths, const V3Number& fhs)
        = 0;
    virtual bool cleanLhs() const = 0;  // True if LHS must have extra upper bits zero
    virtual bool cleanRhs() const = 0;  // True if RHS must have extra upper bits zero
    virtual bool cleanThs() const = 0;  // True if THS must have extra upper bits zero
    virtual bool cleanFhs() const = 0;  // True if THS must have extra upper bits zero
    virtual bool sizeMattersLhs() const = 0;  // True if output result depends on lhs size
    virtual bool sizeMattersRhs() const = 0;  // True if output result depends on rhs size
    virtual bool sizeMattersThs() const = 0;  // True if output result depends on ths size
    virtual bool sizeMattersFhs() const = 0;  // True if output result depends on ths size
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode*) const override { return true; }
};
class AstNodeTermop VL_NOT_FINAL : public AstNodeMath {
    // Terminal operator -- a operator with no "inputs"
protected:
    AstNodeTermop(VNType t, FileLine* fl)
        : AstNodeMath{t, fl} {}

public:
    ASTGEN_MEMBERS_NodeTermop;
    // Know no children, and hot function, so skip iterator for speed
    // cppcheck-suppress functionConst
    void iterateChildren(VNVisitor& v) {}
    void dump(std::ostream& str) const override;
};
class AstNodeTriop VL_NOT_FINAL : public AstNodeMath {
    // Ternary expression
    // @astgen op1 := lhsp : AstNode
    // @astgen op2 := rhsp : AstNode
    // @astgen op3 := thsp : AstNode
protected:
    AstNodeTriop(VNType t, FileLine* fl, AstNode* lhsp, AstNode* rhsp, AstNode* thsp)
        : AstNodeMath{t, fl} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
        this->thsp(thsp);
    }

public:
    ASTGEN_MEMBERS_NodeTriop;
    // METHODS
    void dump(std::ostream& str) const override;
    // Set out to evaluation of a AstConst'ed
    virtual void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                               const V3Number& ths)
        = 0;
    virtual bool cleanLhs() const = 0;  // True if LHS must have extra upper bits zero
    virtual bool cleanRhs() const = 0;  // True if RHS must have extra upper bits zero
    virtual bool cleanThs() const = 0;  // True if THS must have extra upper bits zero
    virtual bool sizeMattersLhs() const = 0;  // True if output result depends on lhs size
    virtual bool sizeMattersRhs() const = 0;  // True if output result depends on rhs size
    virtual bool sizeMattersThs() const = 0;  // True if output result depends on ths size
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode*) const override { return true; }
};
class AstNodeCond VL_NOT_FINAL : public AstNodeTriop {
    // @astgen alias op1 := condp
    // @astgen alias op2 := thenp
    // @astgen alias op3 := elsep
protected:
    AstNodeCond(VNType t, FileLine* fl, AstNode* condp, AstNode* thenp, AstNode* elsep)
        : AstNodeTriop{t, fl, condp, thenp, elsep} {
        if (thenp) {
            dtypeFrom(thenp);
        } else if (elsep) {
            dtypeFrom(elsep);
        }
    }

public:
    ASTGEN_MEMBERS_NodeCond;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                       const V3Number& ths) override;
    string emitVerilog() override { return "%k(%l %f? %r %k: %t)"; }
    string emitC() override { return "VL_COND_%nq%lq%rq%tq(%nw, %P, %li, %ri, %ti)"; }
    bool cleanOut() const override { return false; }  // clean if e1 & e2 clean
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return false; }
    bool cleanThs() const override { return false; }  // Propagates up
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool sizeMattersThs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_BRANCH; }
    virtual AstNode* cloneType(AstNode* condp, AstNode* thenp, AstNode* elsep) = 0;
};
class AstNodeUniop VL_NOT_FINAL : public AstNodeMath {
    // Unary expression
    // @astgen op1 := lhsp : AstNode
protected:
    AstNodeUniop(VNType t, FileLine* fl, AstNode* lhsp)
        : AstNodeMath{t, fl} {
        dtypeFrom(lhsp);
        this->lhsp(lhsp);
    }

public:
    ASTGEN_MEMBERS_NodeUniop;
    // METHODS
    void dump(std::ostream& str) const override;
    // Set out to evaluation of a AstConst'ed lhs
    virtual void numberOperate(V3Number& out, const V3Number& lhs) = 0;
    virtual bool cleanLhs() const = 0;
    virtual bool sizeMattersLhs() const = 0;  // True if output result depends on lhs size
    virtual bool doubleFlavor() const { return false; }  // D flavor of nodes with both flavors?
    // Signed flavor of nodes with both flavors?
    virtual bool signedFlavor() const { return false; }
    virtual bool stringFlavor() const { return false; }  // N flavor of nodes with both flavors?
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode*) const override { return true; }
};
class AstNodeSystemUniop VL_NOT_FINAL : public AstNodeUniop {
public:
    AstNodeSystemUniop(VNType t, FileLine* fl, AstNode* lhsp)
        : AstNodeUniop(t, fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_NodeSystemUniop;
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL_TRIG; }
    bool doubleFlavor() const override { return true; }
};
class AstNodeVarRef VL_NOT_FINAL : public AstNodeMath {
    // An AstVarRef or AstVarXRef
private:
    VAccess m_access;  // Left hand side assignment
    AstVar* m_varp;  // [AfterLink] Pointer to variable itself
    AstVarScope* m_varScopep = nullptr;  // Varscope for hierarchy
    AstNodeModule* m_classOrPackagep = nullptr;  // Package hierarchy
    string m_name;  // Name of variable
    string m_selfPointer;  // Output code object pointer (e.g.: 'this')

protected:
    AstNodeVarRef(VNType t, FileLine* fl, const string& name, const VAccess& access)
        : AstNodeMath{t, fl}
        , m_access{access}
        , m_name{name} {
        varp(nullptr);
    }
    AstNodeVarRef(VNType t, FileLine* fl, const string& name, AstVar* varp, const VAccess& access)
        : AstNodeMath{t, fl}
        , m_access{access}
        , m_name{name} {
        // May have varp==nullptr
        this->varp(varp);
    }

public:
    ASTGEN_MEMBERS_NodeVarRef;
    void dump(std::ostream& str) const override;
    bool hasDType() const override { return true; }
    const char* broken() const override;
    int instrCount() const override { return widthInstrs(); }
    void cloneRelink() override;
    string name() const override { return m_name; }  // * = Var name
    void name(const string& name) override { m_name = name; }
    VAccess access() const { return m_access; }
    void access(const VAccess& flag) { m_access = flag; }  // Avoid using this; Set in constructor
    AstVar* varp() const { return m_varp; }  // [After Link] Pointer to variable
    void varp(AstVar* varp) {
        m_varp = varp;
        dtypeFrom((AstNode*)varp);
    }
    AstVarScope* varScopep() const { return m_varScopep; }
    void varScopep(AstVarScope* varscp) { m_varScopep = varscp; }
    string selfPointer() const { return m_selfPointer; }
    void selfPointer(const string& value) { m_selfPointer = value; }
    string selfPointerProtect(bool useSelfForThis) const;
    AstNodeModule* classOrPackagep() const { return m_classOrPackagep; }
    void classOrPackagep(AstNodeModule* nodep) { m_classOrPackagep = nodep; }
    // Know no children, and hot function, so skip iterator for speed
    // cppcheck-suppress functionConst
    void iterateChildren(VNVisitor& v) {}
};

// === Concrete node types =====================================================

// === AstNodeMath ===
class AstAddrOfCFunc final : public AstNodeMath {
    // Get address of CFunc
private:
    AstCFunc* m_funcp;  // Pointer to function itself

public:
    AstAddrOfCFunc(FileLine* fl, AstCFunc* funcp)
        : ASTGEN_SUPER_AddrOfCFunc(fl)
        , m_funcp{funcp} {
        dtypep(findCHandleDType());
    }

public:
    ASTGEN_MEMBERS_AddrOfCFunc;
    void cloneRelink() override;
    const char* broken() const override;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    AstCFunc* funcp() const { return m_funcp; }
};
class AstCMath final : public AstNodeMath {
    // @astgen op1 := exprsp : List[AstNode] // Expressions to print
private:
    const bool m_cleanOut;
    bool m_pure;  // Pure optimizable
public:
    // Emit C textual math function (like AstUCFunc)
    AstCMath(FileLine* fl, AstNode* exprsp)
        : ASTGEN_SUPER_CMath(fl)
        , m_cleanOut{true}
        , m_pure{false} {
        addExprsp(exprsp);
        dtypeFrom(exprsp);
    }
    inline AstCMath(FileLine* fl, const string& textStmt, int setwidth, bool cleanOut = true);
    ASTGEN_MEMBERS_CMath;
    bool isGateOptimizable() const override { return m_pure; }
    bool isPredictOptimizable() const override { return m_pure; }
    bool cleanOut() const override { return m_cleanOut; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool same(const AstNode* /*samep*/) const override { return true; }
    bool pure() const { return m_pure; }
    void pure(bool flag) { m_pure = flag; }
};
class AstConsAssoc final : public AstNodeMath {
    // Construct an assoc array and return object, '{}
    // @astgen op1 := defaultp : Optional[AstNode]
public:
    AstConsAssoc(FileLine* fl, AstNode* defaultp)
        : ASTGEN_SUPER_ConsAssoc(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_ConsAssoc;
    string emitVerilog() override { return "'{}"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstConsDynArray final : public AstNodeMath {
    // Construct a queue and return object, '{}. '{lhs}, '{lhs. rhs}
    // @astgen op1 := lhsp : Optional[AstNode]
    // @astgen op2 := rhsp : Optional[AstNode]
public:
    explicit AstConsDynArray(FileLine* fl, AstNode* lhsp = nullptr, AstNode* rhsp = nullptr)
        : ASTGEN_SUPER_ConsDynArray(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_ConsDynArray;
    string emitVerilog() override { return "'{%l, %r}"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstConsQueue final : public AstNodeMath {
    // Construct a queue and return object, '{}. '{lhs}, '{lhs. rhs}
    // @astgen op1 := lhsp : Optional[AstNode]
    // @astgen op2 := rhsp : Optional[AstNode]
public:
    explicit AstConsQueue(FileLine* fl, AstNode* lhsp = nullptr, AstNode* rhsp = nullptr)
        : ASTGEN_SUPER_ConsQueue(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_ConsQueue;
    string emitVerilog() override { return "'{%l, %r}"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstConsWildcard final : public AstNodeMath {
    // Construct a wildcard assoc array and return object, '{}
    // @astgen op1 := defaultp : Optional[AstNode]
public:
    AstConsWildcard(FileLine* fl, AstNode* defaultp)
        : ASTGEN_SUPER_ConsWildcard(fl) {
        this->defaultp(defaultp);
    }
    ASTGEN_MEMBERS_ConsWildcard;
    string emitVerilog() override { return "'{}"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstConst final : public AstNodeMath {
    // A constant
private:
    V3Number m_num;  // Constant value
    void initWithNumber() {
        if (m_num.isDouble()) {
            dtypeSetDouble();
        } else if (m_num.isString()) {
            dtypeSetString();
        } else {
            dtypeSetLogicUnsized(m_num.width(), (m_num.sized() ? 0 : m_num.widthMin()),
                                 VSigning::fromBool(m_num.isSigned()));
        }
        m_num.nodep(this);
    }

public:
    AstConst(FileLine* fl, const V3Number& num)
        : ASTGEN_SUPER_Const(fl)
        , m_num(num) {
        initWithNumber();
    }
    class WidthedValue {};  // for creator type-overload selection
    AstConst(FileLine* fl, WidthedValue, int width, uint32_t value)
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, width, value) {
        initWithNumber();
    }
    class DTyped {};  // for creator type-overload selection
    // Zero/empty constant with a type matching nodetypep
    AstConst(FileLine* fl, DTyped, const AstNodeDType* nodedtypep)
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, nodedtypep) {
        initWithNumber();
    }
    class StringToParse {};  // for creator type-overload selection
    AstConst(FileLine* fl, StringToParse, const char* sourcep)
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, sourcep) {
        initWithNumber();
    }
    class VerilogStringLiteral {};  // for creator type-overload selection
    AstConst(FileLine* fl, VerilogStringLiteral, const string& str)
        : ASTGEN_SUPER_Const(fl)
        , m_num(V3Number::VerilogStringLiteral(), this, str) {
        initWithNumber();
    }
    AstConst(FileLine* fl, uint32_t num)
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, 32, num) {
        dtypeSetLogicUnsized(m_num.width(), 0, VSigning::UNSIGNED);
    }
    class Unsized32 {};  // for creator type-overload selection
    AstConst(FileLine* fl, Unsized32, uint32_t num)  // Unsized 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, 32, num) {
        m_num.width(32, false);
        dtypeSetLogicUnsized(32, m_num.widthMin(), VSigning::UNSIGNED);
    }
    class Signed32 {};  // for creator type-overload selection
    AstConst(FileLine* fl, Signed32, int32_t num)  // Signed 32-bit integer of specified value
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, 32, num) {
        m_num.width(32, true);
        dtypeSetLogicUnsized(32, m_num.widthMin(), VSigning::SIGNED);
    }
    class Unsized64 {};  // for creator type-overload selection
    AstConst(FileLine* fl, Unsized64, uint64_t num)
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, 64, 0) {
        m_num.setQuad(num);
        dtypeSetLogicSized(64, VSigning::UNSIGNED);
    }
    class SizedEData {};  // for creator type-overload selection
    AstConst(FileLine* fl, SizedEData, uint64_t num)
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, VL_EDATASIZE, 0) {
        m_num.setQuad(num);
        dtypeSetLogicSized(VL_EDATASIZE, VSigning::UNSIGNED);
    }
    class RealDouble {};  // for creator type-overload selection
    AstConst(FileLine* fl, RealDouble, double num)
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, 64) {
        m_num.setDouble(num);
        dtypeSetDouble();
    }
    class String {};  // for creator type-overload selection
    AstConst(FileLine* fl, String, const string& num)
        : ASTGEN_SUPER_Const(fl)
        , m_num(V3Number::String(), this, num) {
        dtypeSetString();
    }
    class BitFalse {};
    AstConst(FileLine* fl, BitFalse)  // Shorthand const 0, dtype should be a logic of size 1
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, 1, 0) {
        dtypeSetBit();
    }
    // Shorthand const 1 (or with argument 0/1), dtype should be a logic of size 1
    class BitTrue {};
    AstConst(FileLine* fl, BitTrue, bool on = true)
        : ASTGEN_SUPER_Const(fl)
        , m_num(this, 1, on) {
        dtypeSetBit();
    }
    class Null {};
    AstConst(FileLine* fl, Null)
        : ASTGEN_SUPER_Const(fl)
        , m_num(V3Number::Null{}, this) {
        dtypeSetBit();  // Events 1 bit, objects 64 bits, so autoExtend=1 and use bit here
        initWithNumber();
    }
    ASTGEN_MEMBERS_Const;
    string name() const override { return num().ascii(); }  // * = Value
    const V3Number& num() const { return m_num; }  // * = Value
    V3Number& num() { return m_num; }  // * = Value
    uint32_t toUInt() const { return num().toUInt(); }
    int32_t toSInt() const { return num().toSInt(); }
    uint64_t toUQuad() const { return num().toUQuad(); }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    bool same(const AstNode* samep) const override {
        const AstConst* const sp = static_cast<const AstConst*>(samep);
        return num().isCaseEq(sp->num());
    }
    int instrCount() const override { return widthInstrs(); }
    bool isEqAllOnes() const { return num().isEqAllOnes(width()); }
    bool isEqAllOnesV() const { return num().isEqAllOnes(widthMinV()); }
    // Parse string and create appropriate type of AstConst.
    // May return nullptr on parse failure.
    static AstConst* parseParamLiteral(FileLine* fl, const string& literal);
};
class AstEmptyQueue final : public AstNodeMath {
public:
    explicit AstEmptyQueue(FileLine* fl)
        : ASTGEN_SUPER_EmptyQueue(fl) {}
    ASTGEN_MEMBERS_EmptyQueue;
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitVerilog() override { return "{}"; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    bool cleanOut() const override { return true; }
};
class AstEnumItemRef final : public AstNodeMath {
private:
    AstEnumItem* m_itemp;  // [AfterLink] Pointer to item
    AstNodeModule* m_classOrPackagep;  // Package hierarchy
public:
    AstEnumItemRef(FileLine* fl, AstEnumItem* itemp, AstNodeModule* classOrPackagep)
        : ASTGEN_SUPER_EnumItemRef(fl)
        , m_itemp{itemp}
        , m_classOrPackagep{classOrPackagep} {
        dtypeFrom(m_itemp);
    }
    ASTGEN_MEMBERS_EnumItemRef;
    void dump(std::ostream& str) const override;
    string name() const override { return itemp()->name(); }
    int instrCount() const override { return 0; }
    const char* broken() const override;
    void cloneRelink() override {
        if (m_itemp->clonep()) m_itemp = m_itemp->clonep();
    }
    bool same(const AstNode* samep) const override {
        const AstEnumItemRef* const sp = static_cast<const AstEnumItemRef*>(samep);
        return itemp() == sp->itemp();
    }
    AstEnumItem* itemp() const { return m_itemp; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    AstNodeModule* classOrPackagep() const { return m_classOrPackagep; }
    void classOrPackagep(AstNodeModule* nodep) { m_classOrPackagep = nodep; }
};
class AstExprStmt final : public AstNodeMath {
    // Perform a statement, often assignment inside an expression/math node,
    // the parent gets passed the 'resultp()'.
    // resultp is evaluated AFTER the statement(s).
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := resultp : AstNode
public:
    AstExprStmt(FileLine* fl, AstNode* stmtsp, AstNode* resultp)
        : ASTGEN_SUPER_ExprStmt(fl) {
        addStmtsp(stmtsp);
        this->resultp(resultp);
        dtypeFrom(resultp);
    }
    ASTGEN_MEMBERS_ExprStmt;
    // METHODS
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool same(const AstNode*) const override { return true; }
};
class AstFError final : public AstNodeMath {
    // @astgen op1 := filep : AstNode
    // @astgen op2 := strp : AstNode
public:
    AstFError(FileLine* fl, AstNode* filep, AstNode* strp)
        : ASTGEN_SUPER_FError(fl) {
        this->filep(filep);
        this->strp(strp);
    }
    ASTGEN_MEMBERS_FError;
    string emitVerilog() override { return "%f$ferror(%l, %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    virtual bool cleanLhs() const { return true; }
    virtual bool sizeMattersLhs() const { return false; }
    int instrCount() const override { return widthInstrs() * 64; }
    bool isPure() const override { return false; }  // SPECIAL: $display has 'visual' ordering
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstFRead final : public AstNodeMath {
    // @astgen op1 := memp : AstNode // VarRef for result
    // @astgen op2 := filep : AstNode // file (must be a VarRef)
    // @astgen op3 := startp : Optional[AstNode] // Offset
    // @astgen op4 := countp : Optional[AstNode] // Size
public:
    AstFRead(FileLine* fl, AstNode* memp, AstNode* filep, AstNode* startp, AstNode* countp)
        : ASTGEN_SUPER_FRead(fl) {
        this->memp(memp);
        this->filep(filep);
        this->startp(startp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_FRead;
    string verilogKwd() const override { return "$fread"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool isPure() const override { return false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() const override { return true; }  // SPECIAL: makes output
    bool cleanOut() const override { return false; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstFRewind final : public AstNodeMath {
    // Parents: stmtlist
    // @astgen op1 := filep : Optional[AstNode]
public:
    AstFRewind(FileLine* fl, AstNode* filep)
        : ASTGEN_SUPER_FRewind(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_FRewind;
    string verilogKwd() const override { return "$frewind"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool isPure() const override { return false; }
    bool isOutputter() const override { return true; }
    bool isUnlikely() const override { return true; }
    bool cleanOut() const override { return false; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstFScanF final : public AstNodeMath {
    // @astgen op1 := exprsp : List[AstNode] // VarRefs for results
    // @astgen op2 := filep : Optional[AstNode] // file (must be a VarRef)
private:
    string m_text;

public:
    AstFScanF(FileLine* fl, const string& text, AstNode* filep, AstNode* exprsp)
        : ASTGEN_SUPER_FScanF(fl)
        , m_text{text} {
        addExprsp(exprsp);
        this->filep(filep);
    }
    ASTGEN_MEMBERS_FScanF;
    string name() const override { return m_text; }
    string verilogKwd() const override { return "$fscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool isPure() const override { return false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() const override { return true; }  // SPECIAL: makes output
    bool cleanOut() const override { return false; }
    bool same(const AstNode* samep) const override {
        return text() == static_cast<const AstFScanF*>(samep)->text();
    }
    string text() const { return m_text; }  // * = Text to display
    void text(const string& text) { m_text = text; }
};
class AstFSeek final : public AstNodeMath {
    // @astgen op1 := filep : AstNode // file (must be a VarRef)
    // @astgen op2 := offset : Optional[AstNode]
    // @astgen op3 := operation : Optional[AstNode]
public:
    AstFSeek(FileLine* fl, AstNode* filep, AstNode* offset, AstNode* operation)
        : ASTGEN_SUPER_FSeek(fl) {
        this->filep(filep);
        this->offset(offset);
        this->operation(operation);
    }
    ASTGEN_MEMBERS_FSeek;
    string verilogKwd() const override { return "$fseek"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool isPure() const override { return false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() const override { return true; }  // SPECIAL: makes output
    bool cleanOut() const override { return false; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstFTell final : public AstNodeMath {
    // Parents: stmtlist
    // @astgen op1 := filep : AstNode // file (must be a VarRef)
public:
    AstFTell(FileLine* fl, AstNode* filep)
        : ASTGEN_SUPER_FTell(fl) {
        this->filep(filep);
    }
    ASTGEN_MEMBERS_FTell;
    string verilogKwd() const override { return "$ftell"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool isPure() const override { return false; }
    bool isOutputter() const override { return true; }
    bool isUnlikely() const override { return true; }
    bool cleanOut() const override { return false; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstFell final : public AstNodeMath {
    // Verilog $fell
    // @astgen op1 := exprp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
public:
    AstFell(FileLine* fl, AstNode* exprp)
        : ASTGEN_SUPER_Fell(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Fell;
    string emitVerilog() override { return "$fell(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstGatePin final : public AstNodeMath {
    // Possibly expand a gate primitive input pin value to match the range of the gate primitive
    // @astgen op1 := exprp : AstNode // Pin expression
    // @astgen op2 := rangep : AstRange // Range of pin
public:
    AstGatePin(FileLine* fl, AstNode* exprp, AstRange* rangep)
        : ASTGEN_SUPER_GatePin(fl) {
        this->exprp(exprp);
        this->rangep(rangep);
    }
    ASTGEN_MEMBERS_GatePin;
    string emitVerilog() override { return "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
};
class AstImplication final : public AstNodeMath {
    // Verilog |-> |=>
    // @astgen op1 := lhsp : AstNode
    // @astgen op2 := rhsp : AstNode
    // @astgen op3 := sentreep : Optional[AstSenTree]
public:
    AstImplication(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Implication(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_Implication;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstInside final : public AstNodeMath {
    // @astgen op1 := exprp : AstNode
    // @astgen op2 := itemsp : List[AstNode]
public:
    AstInside(FileLine* fl, AstNode* exprp, AstNode* itemsp)
        : ASTGEN_SUPER_Inside(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_Inside;
    string emitVerilog() override { return "%l inside { %r }"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }  // NA
};
class AstInsideRange final : public AstNodeMath {
    // @astgen op1 := lhsp : AstNode
    // @astgen op2 := rhsp : AstNode
public:
    AstInsideRange(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_InsideRange(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_InsideRange;
    string emitVerilog() override { return "[%l:%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }  // NA
    // Create AstAnd(AstGte(...), AstLte(...))
    AstNode* newAndFromInside(AstNode* exprp, AstNode* lhsp, AstNode* rhsp);
};
class AstLambdaArgRef final : public AstNodeMath {
    // Lambda argument usage
    // These are not AstVarRefs because we need to be able to delete/clone lambdas during
    // optimizations and AstVar's are painful to remove.
private:
    string m_name;  // Name of variable
    bool m_index;  // Index, not value

public:
    AstLambdaArgRef(FileLine* fl, const string& name, bool index)
        : ASTGEN_SUPER_LambdaArgRef(fl)
        , m_name{name}
        , m_index(index) {}
    ASTGEN_MEMBERS_LambdaArgRef;
    bool same(const AstNode* /*samep*/) const override { return true; }
    string emitVerilog() override { return name(); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    bool hasDType() const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    string name() const override { return m_name; }  // * = Var name
    void name(const string& name) override { m_name = name; }
    bool index() const { return m_index; }
};
class AstMemberSel final : public AstNodeMath {
    // Parents: math|stmt
    // @astgen op1 := fromp : AstNode
private:
    // Don't need the class we are extracting from, as the "fromp()"'s datatype can get us to it
    string m_name;
    AstVar* m_varp = nullptr;  // Post link, variable within class that is target of selection
public:
    AstMemberSel(FileLine* fl, AstNode* fromp, VFlagChildDType, const string& name)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{name} {
        this->fromp(fromp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstMemberSel(FileLine* fl, AstNode* fromp, AstNodeDType* dtp)
        : ASTGEN_SUPER_MemberSel(fl)
        , m_name{dtp->name()} {
        this->fromp(fromp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_MemberSel;
    void cloneRelink() override;
    const char* broken() const override;
    void dump(std::ostream& str) const override;
    string name() const override { return m_name; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool same(const AstNode* samep) const override { return true; }  // dtype comparison does it
    int instrCount() const override { return widthInstrs(); }
    AstVar* varp() const { return m_varp; }
    void varp(AstVar* nodep) { m_varp = nodep; }
};
class AstNewCopy final : public AstNodeMath {
    // New as shallow copy
    // Parents: math|stmt
    // @astgen op1 := rhsp : AstNode
public:
    AstNewCopy(FileLine* fl, AstNode* rhsp)
        : ASTGEN_SUPER_NewCopy(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_NewCopy;
    string emitVerilog() override { return "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    int instrCount() const override { return widthInstrs(); }
};
class AstNewDynamic final : public AstNodeMath {
    // New for dynamic array
    // Parents: math|stmt
    // @astgen op1 := sizep : AstNode
    // @astgen op2 := rhsp : Optional[AstNode]
public:
    AstNewDynamic(FileLine* fl, AstNode* sizep, AstNode* rhsp)
        : ASTGEN_SUPER_NewDynamic(fl) {
        dtypeFrom(rhsp);  // otherwise V3Width will resolve
        this->sizep(sizep);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_NewDynamic;
    string emitVerilog() override { return "new"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    int instrCount() const override { return widthInstrs(); }
};
class AstPast final : public AstNodeMath {
    // Verilog $past
    // @astgen op1 := exprp : AstNode
    // @astgen op2 := ticksp : Optional[AstNode]
    // @astgen op3 := sentreep : Optional[AstSenTree]
public:
    AstPast(FileLine* fl, AstNode* exprp, AstNode* ticksp)
        : ASTGEN_SUPER_Past(fl) {
        this->exprp(exprp);
        this->ticksp(ticksp);
    }
    ASTGEN_MEMBERS_Past;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstPatMember final : public AstNodeMath {
    // Verilog '{a} or '{a{b}}
    // Parents: AstPattern
    // Children: expression, AstPattern, replication count
    // Expression to assign or another AstPattern (list if replicated)
    // @astgen op1 := lhssp : List[AstNode]
    // @astgen op2 := keyp : Optional[AstNode]
    // @astgen op3 := repp : Optional[AstNode]  // replication count, or nullptr for count 1
private:
    bool m_default = false;

public:
    AstPatMember(FileLine* fl, AstNode* lhssp, AstNode* keyp, AstNode* repp)
        : ASTGEN_SUPER_PatMember(fl) {
        this->addLhssp(lhssp);
        this->keyp(keyp);
        this->repp(repp);
    }
    ASTGEN_MEMBERS_PatMember;
    string emitVerilog() override { return lhssp() ? "%f{%r{%k%l}}" : "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }
    int instrCount() const override { return widthInstrs() * 2; }
    void dump(std::ostream& str = std::cout) const override;
    bool isDefault() const { return m_default; }
    void isDefault(bool flag) { m_default = flag; }
};
class AstPattern final : public AstNodeMath {
    // Verilog '{a,b,c,d...}
    // Parents: AstNodeAssign, AstPattern, ...
    // Children: expression, AstPattern, AstPatReplicate
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstNode] // AstPatReplicate, AstPatMember, etc
public:
    AstPattern(FileLine* fl, AstNode* itemsp)
        : ASTGEN_SUPER_Pattern(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_Pattern;
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }
    int instrCount() const override { return widthInstrs(); }
    AstNodeDType* getChildDTypep() const override { return childDTypep(); }
    virtual AstNodeDType* subDTypep() const { return dtypep() ? dtypep() : childDTypep(); }
};
class AstRand final : public AstNodeMath {
    // $random/$random(seed) or $urandom/$urandom(seed)
    // Return a random number, based upon width()
    // @astgen op1 := seedp : Optional[AstNode]
private:
    const bool m_urandom = false;  // $urandom vs $random
    const bool m_reset = false;  // Random reset, versus always random
public:
    class Reset {};
    AstRand(FileLine* fl, Reset, AstNodeDType* dtp, bool reset)
        : ASTGEN_SUPER_Rand(fl)
        , m_reset{reset} {
        dtypep(dtp);
    }
    AstRand(FileLine* fl, AstNode* seedp, bool urandom)
        : ASTGEN_SUPER_Rand(fl)
        , m_urandom{urandom} {
        this->seedp(seedp);
    }
    ASTGEN_MEMBERS_Rand;
    string emitVerilog() override {
        return seedp() ? (m_urandom ? "%f$urandom(%l)" : "%f$random(%l)")
                       : (m_urandom ? "%f$urandom()" : "%f$random()");
    }
    string emitC() override {
        return m_reset ? "VL_RAND_RESET_%nq(%nw, %P)"
               : seedp()
                   ? (urandom() ? "VL_URANDOM_SEEDED_%nq%lq(%li)" : "VL_RANDOM_SEEDED_%nq%lq(%li)")
               : isWide() ? "VL_RANDOM_%nq(%nw, %P)"  //
                          : "VL_RANDOM_%nq()";
    }
    bool cleanOut() const override { return false; }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_PLI; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    bool combinable(const AstRand* samep) const {
        return !seedp() && !samep->seedp() && reset() == samep->reset()
               && urandom() == samep->urandom();
    }
    bool reset() const { return m_reset; }
    bool urandom() const { return m_urandom; }
};
class AstRose final : public AstNodeMath {
    // Verilog $rose
    // @astgen op1 := exprp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
public:
    AstRose(FileLine* fl, AstNode* exprp)
        : ASTGEN_SUPER_Rose(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Rose;
    string emitVerilog() override { return "$rose(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstSScanF final : public AstNodeMath {
    // @astgen op1 := exprsp : List[AstNode] // VarRefs for results
    // @astgen op2 := fromp : AstNode
private:
    string m_text;

public:
    AstSScanF(FileLine* fl, const string& text, AstNode* fromp, AstNode* exprsp)
        : ASTGEN_SUPER_SScanF(fl)
        , m_text{text} {
        this->addExprsp(exprsp);
        this->fromp(fromp);
    }
    ASTGEN_MEMBERS_SScanF;
    string name() const override { return m_text; }
    string verilogKwd() const override { return "$sscanf"; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool isPure() const override { return false; }  // SPECIAL: has 'visual' ordering
    bool isOutputter() const override { return true; }  // SPECIAL: makes output
    bool cleanOut() const override { return false; }
    bool same(const AstNode* samep) const override {
        return text() == static_cast<const AstSScanF*>(samep)->text();
    }
    string text() const { return m_text; }  // * = Text to display
    void text(const string& text) { m_text = text; }
};
class AstSampled final : public AstNodeMath {
    // Verilog $sampled
    // @astgen op1 := exprp : AstNode
public:
    AstSampled(FileLine* fl, AstNode* exprp)
        : ASTGEN_SUPER_Sampled(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Sampled;
    string emitVerilog() override { return "$sampled(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }
    int instrCount() const override { return 0; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstScopeName final : public AstNodeMath {
    // For display %m and DPI context imports
    // Parents:  DISPLAY
    // @astgen op1 := scopeAttrp : List[AstText]
    // @astgen op2 := scopeEntrp : List[AstText]
private:
    bool m_dpiExport = false;  // Is for dpiExport
    const bool m_forFormat = false;  // Is for a format %m
    string scopeNameFormatter(AstText* scopeTextp) const;
    string scopePrettyNameFormatter(AstText* scopeTextp) const;

public:
    class ForFormat {};
    AstScopeName(FileLine* fl, bool forFormat)
        : ASTGEN_SUPER_ScopeName(fl)
        , m_forFormat{forFormat} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_ScopeName;
    bool same(const AstNode* samep) const override {
        return (m_dpiExport == static_cast<const AstScopeName*>(samep)->m_dpiExport
                && m_forFormat == static_cast<const AstScopeName*>(samep)->m_forFormat);
    }
    string emitVerilog() override { return ""; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    void dump(std::ostream& str = std::cout) const override;
    string scopeSymName() const {  // Name for __Vscope variable including children
        return scopeNameFormatter(scopeAttrp());
    }
    string scopeDpiName() const {  // Name for DPI import scope
        return scopeNameFormatter(scopeEntrp());
    }
    string scopePrettySymName() const {  // Name for __Vscope variable including children
        return scopePrettyNameFormatter(scopeAttrp());
    }
    string scopePrettyDpiName() const {  // Name for __Vscope variable including children
        return scopePrettyNameFormatter(scopeEntrp());
    }
    bool dpiExport() const { return m_dpiExport; }
    void dpiExport(bool flag) { m_dpiExport = flag; }
    bool forFormat() const { return m_forFormat; }
};
class AstSetAssoc final : public AstNodeMath {
    // Set an assoc array element and return object, '{}
    // Parents: math
    // @astgen op1 := lhsp : AstNode
    // @astgen op2 := keyp : Optional[AstNode]
    // @astgen op3 := valuep : AstNode
public:
    AstSetAssoc(FileLine* fl, AstNode* lhsp, AstNode* keyp, AstNode* valuep)
        : ASTGEN_SUPER_SetAssoc(fl) {
        this->lhsp(lhsp);
        this->keyp(keyp);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_SetAssoc;
    string emitVerilog() override { return "'{}"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstSetWildcard final : public AstNodeMath {
    // Set a wildcard assoc array element and return object, '{}
    // Parents: math
    // @astgen op1 := lhsp : AstNode
    // @astgen op2 := keyp : Optional[AstNode]
    // @astgen op3 := valuep : AstNode
public:
    AstSetWildcard(FileLine* fl, AstNode* lhsp, AstNode* keyp, AstNode* valuep)
        : ASTGEN_SUPER_SetWildcard(fl) {
        this->lhsp(lhsp);
        this->keyp(keyp);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_SetWildcard;
    string emitVerilog() override { return "'{}"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstStable final : public AstNodeMath {
    // Verilog $stable
    // @astgen op1 := exprp : AstNode
    // @astgen op2 := sentreep : Optional[AstSenTree]
public:
    AstStable(FileLine* fl, AstNode* exprp)
        : ASTGEN_SUPER_Stable(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_Stable;
    string emitVerilog() override { return "$stable(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstSystemF final : public AstNodeMath {
    // $system used as function
    // @astgen op1 := lhsp : AstNode
public:
    AstSystemF(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_SystemF(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_SystemF;
    string verilogKwd() const override { return "$system"; }
    string emitVerilog() override { return verilogKwd(); }
    string emitC() override { return "VL_SYSTEM_%nq(%lw, %P)"; }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool isPure() const override { return false; }
    bool isOutputter() const override { return true; }
    bool isUnlikely() const override { return true; }
    bool cleanOut() const override { return true; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstTestPlusArgs final : public AstNodeMath {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNode]
public:
    AstTestPlusArgs(FileLine* fl, AstNode* searchp)
        : ASTGEN_SUPER_TestPlusArgs(fl) {
        this->searchp(searchp);
    }
    ASTGEN_MEMBERS_TestPlusArgs;
    string verilogKwd() const override { return "$test$plusargs"; }
    string emitVerilog() override { return verilogKwd(); }
    string emitC() override { return "VL_VALUEPLUSARGS_%nq(%lw, %P, nullptr)"; }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool cleanOut() const override { return true; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstUCFunc final : public AstNodeMath {
    // User's $c function
    // Perhaps this should be an AstNodeListop; but there's only one list math right now
    // @astgen op1 := exprsp : List[AstNode] // Expressions to print
public:
    AstUCFunc(FileLine* fl, AstNode* exprsp)
        : ASTGEN_SUPER_UCFunc(fl) {
        this->addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_UCFunc;
    bool cleanOut() const override { return false; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isPure() const override { return false; }  // SPECIAL: User may order w/other sigs
    bool isOutputter() const override { return true; }
    bool isGateOptimizable() const override { return false; }
    bool isSubstOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_PLI; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstUnbounded final : public AstNodeMath {
    // A $ in the parser, used for unbounded and queues
    // Due to where is used, treated as Signed32
public:
    explicit AstUnbounded(FileLine* fl)
        : ASTGEN_SUPER_Unbounded(fl) {
        dtypeSetSigned32();
    }
    ASTGEN_MEMBERS_Unbounded;
    string emitVerilog() override { return "$"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
};
class AstValuePlusArgs final : public AstNodeMath {
    // Search expression. If nullptr then this is a $test$plusargs instead of $value$plusargs.
    // @astgen op1 := searchp : Optional[AstNode]
    // @astgen op2 := outp : AstNode // VarRef for result
public:
    AstValuePlusArgs(FileLine* fl, AstNode* searchp, AstNode* outp)
        : ASTGEN_SUPER_ValuePlusArgs(fl) {
        this->searchp(searchp);
        this->outp(outp);
    }
    ASTGEN_MEMBERS_ValuePlusArgs;
    string verilogKwd() const override { return "$value$plusargs"; }
    string emitVerilog() override { return "%f$value$plusargs(%l, %k%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    bool isPure() const override { return !outp(); }
    bool cleanOut() const override { return true; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};

// === AstNodeBiop ===
class AstBufIf1 final : public AstNodeBiop {
    // lhs is enable, rhs is data to drive
    // Note unlike the Verilog bufif1() UDP, this allows any width; each lhsp
    // bit enables respective rhsp bit
public:
    AstBufIf1(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_BufIf1(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_BufIf1;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstBufIf1(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opBufIf1(lhs, rhs);
    }
    string emitVerilog() override { return "bufif(%r,%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanOut() const override { V3ERROR_NA_RETURN(""); }  // Lclean || Rclean
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstCastDynamic final : public AstNodeBiop {
    // Verilog $cast used as a function
    // Task usage of $cast is converted during parse to assert($cast(...))
    // Parents: MATH
    // Children: MATH
    // lhsp() is value (we are converting FROM) to match AstCCast etc, this
    // is opposite of $cast's order, because the first access is to the
    // value reading from.  Suggest use fromp()/top() instead of lhsp/rhsp().
public:
    AstCastDynamic(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_CastDynamic(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_CastDynamic;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        V3ERROR_NA;
    }
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstCastDynamic(this->fileline(), lhsp, rhsp);
    }
    string emitVerilog() override { return "%f$cast(%r, %l)"; }
    string emitC() override { return "VL_DYNAMIC_CAST(%r, %l)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 20; }
    bool isPure() const override { return true; }
    AstNode* fromp() const { return lhsp(); }
    AstNode* top() const { return rhsp(); }
};
class AstCompareNN final : public AstNodeBiop {
    // Verilog str.compare() and str.icompare()
private:
    const bool m_ignoreCase;  // True for str.icompare()
public:
    AstCompareNN(FileLine* fl, AstNode* lhsp, AstNode* rhsp, bool ignoreCase)
        : ASTGEN_SUPER_CompareNN(fl, lhsp, rhsp)
        , m_ignoreCase{ignoreCase} {
        dtypeSetUInt32();
    }
    ASTGEN_MEMBERS_CompareNN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstCompareNN(this->fileline(), lhsp, rhsp, m_ignoreCase);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opCompareNN(lhs, rhs, m_ignoreCase);
    }
    string name() const override { return m_ignoreCase ? "icompare" : "compare"; }
    string emitVerilog() override {
        return m_ignoreCase ? "%k(%l.icompare(%r))" : "%k(%l.compare(%r))";
    }
    string emitC() override {
        return m_ignoreCase ? "VL_CMP_NN(%li,%ri,true)" : "VL_CMP_NN(%li,%ri,false)";
    }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstConcat final : public AstNodeBiop {
    // If you're looking for {#{}}, see AstReplicate
public:
    AstConcat(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Concat(fl, lhsp, rhsp) {
        if (lhsp->dtypep() && rhsp->dtypep()) {
            dtypeSetLogicSized(lhsp->dtypep()->width() + rhsp->dtypep()->width(),
                               VSigning::UNSIGNED);
        }
    }
    ASTGEN_MEMBERS_Concat;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstConcat(this->fileline(), lhsp, rhsp);
    }
    string emitVerilog() override { return "%f{%l, %k%r}"; }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opConcat(lhs, rhs);
    }
    string emitC() override { return "VL_CONCAT_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 2; }
};
class AstConcatN final : public AstNodeBiop {
    // String concatenate
public:
    AstConcatN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_ConcatN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_ConcatN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstConcatN(this->fileline(), lhsp, rhsp);
    }
    string emitVerilog() override { return "%f{%l, %k%r}"; }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opConcatN(lhs, rhs);
    }
    string emitC() override { return "VL_CONCATN_NNN(%li, %ri)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_STR; }
    bool stringFlavor() const override { return true; }
};
class AstDiv final : public AstNodeBiop {
public:
    AstDiv(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Div(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Div;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstDiv(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opDiv(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f/ %r)"; }
    string emitC() override { return "VL_DIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_DIV; }
};
class AstDivD final : public AstNodeBiop {
public:
    AstDivD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_DivD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_DivD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstDivD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opDivD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f/ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "/"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL_DIV; }
    bool doubleFlavor() const override { return true; }
};
class AstDivS final : public AstNodeBiop {
public:
    AstDivS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_DivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_DivS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstDivS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opDivS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f/ %r)"; }
    string emitC() override { return "VL_DIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_DIV; }
    bool signedFlavor() const override { return true; }
};
class AstEqWild final : public AstNodeBiop {
    // Note wildcard operator rhs differs from lhs
public:
    AstEqWild(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_EqWild(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_EqWild;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstEqWild(this->fileline(), lhsp, rhsp);
    }
    static AstNodeBiop* newTyped(FileLine* fl, AstNode* lhsp,
                                 AstNode* rhsp);  // Return AstEqWild/AstEqD
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opWildEq(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f==? %r)"; }
    string emitC() override { return "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "=="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstFGetS final : public AstNodeBiop {
public:
    AstFGetS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_FGetS(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_FGetS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstFGetS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { return "%f$fgets(%l,%r)"; }
    string emitC() override {
        return strgp()->dtypep()->basicp()->isString() ? "VL_FGETS_NI(%li, %ri)"
                                                       : "VL_FGETS_%nqX%rq(%lw, %P, &(%li), %ri)";
    }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 64; }
    AstNode* strgp() const { return lhsp(); }
    AstNode* filep() const { return rhsp(); }
};
class AstFUngetC final : public AstNodeBiop {
public:
    AstFUngetC(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_FUngetC(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_FUngetC;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        V3ERROR_NA;
    }
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstFUngetC(this->fileline(), lhsp, rhsp);
    }
    string emitVerilog() override { return "%f$ungetc(%r, %l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override {
        return "(%li ? (ungetc(%ri, VL_CVT_I_FP(%li)) >= 0 ? 0 : -1) : -1)";
    }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 64; }
    bool isPure() const override { return false; }  // SPECIAL: $display has 'visual' ordering
    AstNode* filep() const { return lhsp(); }
    AstNode* charp() const { return rhsp(); }
};
class AstGetcN final : public AstNodeBiop {
    // Verilog string.getc()
public:
    AstGetcN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_GetcN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_GetcN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGetcN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGetcN(lhs, rhs);
    }
    string name() const override { return "getc"; }
    string emitVerilog() override { return "%k(%l.getc(%r))"; }
    string emitC() override { return "VL_GETC_N(%li,%ri)"; }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstGetcRefN final : public AstNodeBiop {
    // Verilog string[#] on the left-hand-side of assignment
    // Spec says is of type byte (not string of single character)
public:
    AstGetcRefN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_GetcRefN(fl, lhsp, rhsp) {
        dtypeSetBitSized(8, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_GetcRefN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGetcRefN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { return "%k%l[%r]"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstGt final : public AstNodeBiop {
public:
    AstGt(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Gt(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_Gt;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGt(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGt(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f> %r)"; }
    string emitC() override { return "VL_GT_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return ">"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstGtD final : public AstNodeBiop {
public:
    AstGtD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_GtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_GtD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGtD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGtD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return ">"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstGtN final : public AstNodeBiop {
public:
    AstGtN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_GtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_GtN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGtN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGtN(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f> %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return ">"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_STR; }
    bool stringFlavor() const override { return true; }
};
class AstGtS final : public AstNodeBiop {
public:
    AstGtS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_GtS(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_GtS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGtS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGtS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f> %r)"; }
    string emitC() override { return "VL_GTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool signedFlavor() const override { return true; }
};
class AstGte final : public AstNodeBiop {
public:
    AstGte(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Gte(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_Gte;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGte(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGte(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f>= %r)"; }
    string emitC() override { return "VL_GTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return ">="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstGteD final : public AstNodeBiop {
public:
    AstGteD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_GteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_GteD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGteD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGteD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return ">="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstGteN final : public AstNodeBiop {
public:
    AstGteN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_GteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_GteN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGteN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGteN(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f>= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return ">="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_STR; }
    bool stringFlavor() const override { return true; }
};
class AstGteS final : public AstNodeBiop {
public:
    AstGteS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_GteS(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_GteS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstGteS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opGteS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f>= %r)"; }
    string emitC() override { return "VL_GTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool signedFlavor() const override { return true; }
};
class AstLogAnd final : public AstNodeBiop {
public:
    AstLogAnd(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LogAnd(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LogAnd;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLogAnd(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLogAnd(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f&& %r)"; }
    string emitC() override { return "VL_LOGAND_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "&&"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() + INSTR_COUNT_BRANCH; }
};
class AstLogIf final : public AstNodeBiop {
public:
    AstLogIf(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LogIf(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LogIf;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLogIf(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLogIf(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f-> %r)"; }
    string emitC() override { return "VL_LOGIF_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "->"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() + INSTR_COUNT_BRANCH; }
};
class AstLogOr final : public AstNodeBiop {
    // LOGOR with optional side effects
    // Side effects currently used in some V3Width code
    // TBD if this concept is generally adopted for side-effect tracking
    // versus V3Const tracking it itself
    bool m_sideEffect = false;  // Has side effect, relies on short-circuiting
public:
    AstLogOr(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LogOr(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LogOr;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLogOr(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLogOr(lhs, rhs);
    }
    bool same(const AstNode* samep) const override {
        const AstLogOr* const sp = static_cast<const AstLogOr*>(samep);
        return m_sideEffect == sp->m_sideEffect;
    }
    void dump(std::ostream& str = std::cout) const override;
    string emitVerilog() override { return "%k(%l %f|| %r)"; }
    string emitC() override { return "VL_LOGOR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "||"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() + INSTR_COUNT_BRANCH; }
    bool isPure() const override { return !m_sideEffect; }
    void sideEffect(bool flag) { m_sideEffect = flag; }
    bool sideEffect() const { return m_sideEffect; }
};
class AstLt final : public AstNodeBiop {
public:
    AstLt(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Lt(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_Lt;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLt(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLt(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f< %r)"; }
    string emitC() override { return "VL_LT_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "<"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstLtD final : public AstNodeBiop {
public:
    AstLtD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LtD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LtD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLtD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLtD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "<"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstLtN final : public AstNodeBiop {
public:
    AstLtN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LtN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LtN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLtN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLtN(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f< %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "<"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_STR; }
    bool stringFlavor() const override { return true; }
};
class AstLtS final : public AstNodeBiop {
public:
    AstLtS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LtS(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LtS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLtS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLtS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f< %r)"; }
    string emitC() override { return "VL_LTS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool signedFlavor() const override { return true; }
};
class AstLte final : public AstNodeBiop {
public:
    AstLte(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Lte(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_Lte;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLte(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLte(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f<= %r)"; }
    string emitC() override { return "VL_LTE_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "<="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstLteD final : public AstNodeBiop {
public:
    AstLteD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LteD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LteD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLteD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLteD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "<="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstLteN final : public AstNodeBiop {
public:
    AstLteN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LteN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LteN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLteN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLteN(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f<= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "<="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_STR; }
    bool stringFlavor() const override { return true; }
};
class AstLteS final : public AstNodeBiop {
public:
    AstLteS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LteS(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LteS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLteS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLteS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f<= %r)"; }
    string emitC() override { return "VL_LTES_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool signedFlavor() const override { return true; }
};
class AstModDiv final : public AstNodeBiop {
public:
    AstModDiv(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_ModDiv(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_ModDiv;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstModDiv(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opModDiv(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f%% %r)"; }
    string emitC() override { return "VL_MODDIV_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_DIV; }
};
class AstModDivS final : public AstNodeBiop {
public:
    AstModDivS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_ModDivS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_ModDivS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstModDivS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opModDivS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f%% %r)"; }
    string emitC() override { return "VL_MODDIVS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_DIV; }
    bool signedFlavor() const override { return true; }
};
class AstNeqWild final : public AstNodeBiop {
public:
    AstNeqWild(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_NeqWild(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_NeqWild;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstNeqWild(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opWildNeq(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f!=? %r)"; }
    string emitC() override { return "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "!="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstPow final : public AstNodeBiop {
public:
    AstPow(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Pow(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Pow;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstPow(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opPow(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f** %r)"; }
    string emitC() override { return "VL_POW_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    bool emitCheckMaxWords() override { return true; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_MUL * 10; }
};
class AstPowD final : public AstNodeBiop {
public:
    AstPowD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_PowD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_PowD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstPowD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opPowD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f** %r)"; }
    string emitC() override { return "pow(%li,%ri)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL_DIV * 5; }
    bool doubleFlavor() const override { return true; }
};
class AstPowSS final : public AstNodeBiop {
public:
    AstPowSS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_PowSS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_PowSS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstPowSS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opPowSS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f** %r)"; }
    string emitC() override { return "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,1)"; }
    bool emitCheckMaxWords() override { return true; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_MUL * 10; }
    bool signedFlavor() const override { return true; }
};
class AstPowSU final : public AstNodeBiop {
public:
    AstPowSU(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_PowSU(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_PowSU;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstPowSU(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opPowSU(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f** %r)"; }
    string emitC() override { return "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 1,0)"; }
    bool emitCheckMaxWords() override { return true; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_MUL * 10; }
    bool signedFlavor() const override { return true; }
};
class AstPowUS final : public AstNodeBiop {
public:
    AstPowUS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_PowUS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_PowUS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstPowUS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opPowUS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f** %r)"; }
    string emitC() override { return "VL_POWSS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri, 0,1)"; }
    bool emitCheckMaxWords() override { return true; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_MUL * 10; }
    bool signedFlavor() const override { return true; }
};
class AstReplicate final : public AstNodeBiop {
    // Also used as a "Uniop" flavor of Concat, e.g. "{a}"
    // Verilog {rhs{lhs}} - Note rhsp() is the replicate value, not the lhsp()
    // @astgen alias op1 := srcp
    // @astgen alias op2 := countp
public:
    AstReplicate(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Replicate(fl, lhsp, rhsp) {
        if (lhsp) {
            if (const AstConst* const constp = VN_CAST(rhsp, Const)) {
                dtypeSetLogicSized(lhsp->width() * constp->toUInt(), VSigning::UNSIGNED);
            }
        }
    }
    AstReplicate(FileLine* fl, AstNode* lhsp, uint32_t repCount)
        : AstReplicate(fl, lhsp, new AstConst(fl, repCount)) {}
    ASTGEN_MEMBERS_Replicate;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstReplicate(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opRepl(lhs, rhs);
    }
    string emitVerilog() override { return "%f{%r{%k%l}}"; }
    string emitC() override { return "VL_REPLICATE_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 2; }
};
class AstReplicateN final : public AstNodeBiop {
    // String replicate
public:
    AstReplicateN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_ReplicateN(fl, lhsp, rhsp) {
        dtypeSetString();
    }
    AstReplicateN(FileLine* fl, AstNode* lhsp, uint32_t repCount)
        : AstReplicateN(fl, lhsp, new AstConst(fl, repCount)) {}
    ASTGEN_MEMBERS_ReplicateN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstReplicateN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opReplN(lhs, rhs);
    }
    string emitVerilog() override { return "%f{%r{%k%l}}"; }
    string emitC() override { return "VL_REPLICATEN_NN%rq(%li, %ri)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 2; }
    bool stringFlavor() const override { return true; }
};
class AstShiftL final : public AstNodeBiop {
public:
    AstShiftL(FileLine* fl, AstNode* lhsp, AstNode* rhsp, int setwidth = 0)
        : ASTGEN_SUPER_ShiftL(fl, lhsp, rhsp) {
        if (setwidth) dtypeSetLogicSized(setwidth, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_ShiftL;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstShiftL(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opShiftL(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f<< %r)"; }
    string emitC() override { return "VL_SHIFTL_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override {
        return (rhsp()->isWide() || rhsp()->isQuad()) ? "" : "<<";
    }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return false; }
};
class AstShiftR final : public AstNodeBiop {
public:
    AstShiftR(FileLine* fl, AstNode* lhsp, AstNode* rhsp, int setwidth = 0)
        : ASTGEN_SUPER_ShiftR(fl, lhsp, rhsp) {
        if (setwidth) dtypeSetLogicSized(setwidth, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_ShiftR;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstShiftR(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opShiftR(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f>> %r)"; }
    string emitC() override { return "VL_SHIFTR_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override {
        return (rhsp()->isWide() || rhsp()->isQuad()) ? "" : ">>";
    }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    // LHS size might be > output size, so don't want to force size
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstShiftRS final : public AstNodeBiop {
    // Shift right with sign extension, >>> operator
    // Output data type's width determines which bit is used for sign extension
public:
    AstShiftRS(FileLine* fl, AstNode* lhsp, AstNode* rhsp, int setwidth = 0)
        : ASTGEN_SUPER_ShiftRS(fl, lhsp, rhsp) {
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        if (setwidth) dtypeSetLogicSized(setwidth, VSigning::SIGNED);
    }
    ASTGEN_MEMBERS_ShiftRS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstShiftRS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opShiftRS(lhs, rhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { return "%k(%l %f>>> %r)"; }
    string emitC() override { return "VL_SHIFTRS_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool signedFlavor() const override { return true; }
};
class AstSub final : public AstNodeBiop {
public:
    AstSub(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Sub(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Sub;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstSub(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opSub(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f- %r)"; }
    string emitC() override { return "VL_SUB_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "-"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
};
class AstSubD final : public AstNodeBiop {
public:
    AstSubD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_SubD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_SubD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstSubD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opSubD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f- %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "-"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstURandomRange final : public AstNodeBiop {
    // $urandom_range
public:
    explicit AstURandomRange(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_URandomRange(fl, lhsp, rhsp) {
        dtypeSetUInt32();  // Says IEEE
    }
    ASTGEN_MEMBERS_URandomRange;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstURandomRange(fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { return "%f$urandom_range(%l, %r)"; }
    string emitC() override { return "VL_URANDOM_RANGE_%nq(%li, %ri)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_PLI; }
};

// === AstNodeBiCom ===
class AstEq final : public AstNodeBiCom {
public:
    AstEq(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Eq(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_Eq;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstEq(this->fileline(), lhsp, rhsp);
    }
    static AstNodeBiop* newTyped(FileLine* fl, AstNode* lhsp,
                                 AstNode* rhsp);  // Return AstEq/AstEqD
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opEq(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f== %r)"; }
    string emitC() override { return "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "=="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstEqCase final : public AstNodeBiCom {
public:
    AstEqCase(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_EqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_EqCase;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstEqCase(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opCaseEq(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f=== %r)"; }
    string emitC() override { return "VL_EQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "=="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstEqD final : public AstNodeBiCom {
public:
    AstEqD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_EqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_EqD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstEqD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opEqD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "=="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstEqN final : public AstNodeBiCom {
public:
    AstEqN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_EqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_EqN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstEqN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opEqN(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f== %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "=="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_STR; }
    bool stringFlavor() const override { return true; }
};
class AstLogEq final : public AstNodeBiCom {
public:
    AstLogEq(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_LogEq(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LogEq;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstLogEq(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opLogEq(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f<-> %r)"; }
    string emitC() override { return "VL_LOGEQ_%nq%lq%rq(%nw,%lw,%rw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "<->"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() + INSTR_COUNT_BRANCH; }
};
class AstNeq final : public AstNodeBiCom {
public:
    AstNeq(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Neq(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_Neq;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstNeq(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opNeq(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f!= %r)"; }
    string emitC() override { return "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "!="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstNeqCase final : public AstNodeBiCom {
public:
    AstNeqCase(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_NeqCase(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_NeqCase;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstNeqCase(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opCaseNeq(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f!== %r)"; }
    string emitC() override { return "VL_NEQ_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "!="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstNeqD final : public AstNodeBiCom {
public:
    AstNeqD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_NeqD(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_NeqD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstNeqD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opNeqD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "!="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstNeqN final : public AstNodeBiCom {
public:
    AstNeqN(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_NeqN(fl, lhsp, rhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_NeqN;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstNeqN(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opNeqN(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f!= %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "!="; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_STR; }
    bool stringFlavor() const override { return true; }
};

// === AstNodeBiComAsv ===
class AstAdd final : public AstNodeBiComAsv {
public:
    AstAdd(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Add(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Add;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstAdd(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opAdd(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f+ %r)"; }
    string emitC() override { return "VL_ADD_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "+"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
};
class AstAddD final : public AstNodeBiComAsv {
public:
    AstAddD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_AddD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_AddD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstAddD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opAddD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f+ %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "+"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstAnd final : public AstNodeBiComAsv {
public:
    AstAnd(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_And(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_And;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstAnd(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opAnd(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f& %r)"; }
    string emitC() override { return "VL_AND_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "&"; }
    bool cleanOut() const override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstMul final : public AstNodeBiComAsv {
public:
    AstMul(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Mul(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Mul;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstMul(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opMul(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f* %r)"; }
    string emitC() override { return "VL_MUL_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "*"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_MUL; }
};
class AstMulD final : public AstNodeBiComAsv {
public:
    AstMulD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_MulD(fl, lhsp, rhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_MulD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstMulD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opMulD(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f* %r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "*"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstMulS final : public AstNodeBiComAsv {
public:
    AstMulS(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_MulS(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_MulS;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstMulS(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opMulS(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f* %r)"; }
    string emitC() override { return "VL_MULS_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return ""; }
    bool emitCheckMaxWords() override { return true; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    int instrCount() const override { return widthInstrs() * INSTR_COUNT_INT_MUL; }
    bool signedFlavor() const override { return true; }
};
class AstOr final : public AstNodeBiComAsv {
public:
    AstOr(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Or(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Or;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstOr(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opOr(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f| %r)"; }
    string emitC() override { return "VL_OR_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "|"; }
    bool cleanOut() const override { V3ERROR_NA_RETURN(false); }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};
class AstXor final : public AstNodeBiComAsv {
public:
    AstXor(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Xor(fl, lhsp, rhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Xor;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstXor(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opXor(lhs, rhs);
    }
    string emitVerilog() override { return "%k(%l %f^ %r)"; }
    string emitC() override { return "VL_XOR_%lq(%lW, %P, %li, %ri)"; }
    string emitSimpleOperator() override { return "^"; }
    bool cleanOut() const override { return false; }  // Lclean && Rclean
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
};

// === AstNodeSel ===
class AstArraySel final : public AstNodeSel {
    // Parents: math|stmt
    // Children: varref|arraysel, math
private:
    void init(AstNode* fromp) {
        if (fromp && VN_IS(fromp->dtypep()->skipRefp(), NodeArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), NodeArrayDType)->subDTypep());
        }
    }

public:
    AstArraySel(FileLine* fl, AstNode* fromp, AstNode* bitp)
        : ASTGEN_SUPER_ArraySel(fl, fromp, bitp) {
        init(fromp);
    }
    AstArraySel(FileLine* fl, AstNode* fromp, int bit)
        : ASTGEN_SUPER_ArraySel(fl, fromp, new AstConst(fl, bit)) {
        init(fromp);
    }
    ASTGEN_MEMBERS_ArraySel;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstArraySel(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        V3ERROR_NA; /* How can from be a const? */
    }
    string emitVerilog() override { return "%k(%l%f[%r])"; }
    string emitC() override { return "%li%k[%ri]"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool isGateOptimizable() const override { return true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() const override { return true; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    // Special operators
    // Return base var (or const) nodep dereferences
    static AstNode* baseFromp(AstNode* nodep, bool overMembers);
};
class AstAssocSel final : public AstNodeSel {
    // Parents: math|stmt
    // Children: varref|arraysel, math
private:
    void init(AstNode* fromp) {
        if (fromp && VN_IS(fromp->dtypep()->skipRefp(), AssocArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), AssocArrayDType)->subDTypep());
        }
    }

public:
    AstAssocSel(FileLine* fl, AstNode* fromp, AstNode* bitp)
        : ASTGEN_SUPER_AssocSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_AssocSel;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstAssocSel(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { return "%k(%l%f[%r])"; }
    string emitC() override { return "%li%k[%ri]"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool isGateOptimizable() const override { return true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() const override { return false; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    int instrCount() const override { return widthInstrs(); }
};
class AstWildcardSel final : public AstNodeSel {
    // Parents: math|stmt
    // Children: varref|arraysel, math
private:
    void init(AstNode* fromp) {
        if (fromp && VN_IS(fromp->dtypep()->skipRefp(), WildcardArrayDType)) {
            // Strip off array to find what array references
            dtypeFrom(VN_AS(fromp->dtypep()->skipRefp(), WildcardArrayDType)->subDTypep());
        }
    }

public:
    AstWildcardSel(FileLine* fl, AstNode* fromp, AstNode* bitp)
        : ASTGEN_SUPER_WildcardSel(fl, fromp, bitp) {
        init(fromp);
    }
    ASTGEN_MEMBERS_WildcardSel;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstWildcardSel{this->fileline(), lhsp, rhsp};
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { return "%k(%l%f[%r])"; }
    string emitC() override { return "%li%k[%ri]"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool isGateOptimizable() const override { return true; }  // esp for V3Const::ifSameAssign
    bool isPredictOptimizable() const override { return false; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    int instrCount() const override { return widthInstrs(); }
};
class AstWordSel final : public AstNodeSel {
    // Select a single word from a multi-word wide value
public:
    AstWordSel(FileLine* fl, AstNode* fromp, AstNode* bitp)
        : ASTGEN_SUPER_WordSel(fl, fromp, bitp) {
        dtypeSetUInt32();  // Always used on WData arrays so returns edata size
    }
    ASTGEN_MEMBERS_WordSel;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstWordSel(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& from, const V3Number& bit) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { return "%k(%l%f[%r])"; }
    string emitC() override {
        return "%li[%ri]";
    }  // Not %k, as usually it's a small constant rhsp
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};

// === AstNodeStream ===
class AstStreamL final : public AstNodeStream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
public:
    AstStreamL(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_StreamL(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_StreamL;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstStreamL(this->fileline(), lhsp, rhsp);
    }
    string emitVerilog() override { return "%f{ << %r %k{%l} }"; }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opStreamL(lhs, rhs);
    }
    string emitC() override { return "VL_STREAML_%nq%lq%rq(%lw, %P, %li, %ri)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 2; }
};
class AstStreamR final : public AstNodeStream {
    // Verilog {rhs{lhs}} - Note rhsp() is the slice size, not the lhsp()
public:
    AstStreamR(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_StreamR(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_StreamR;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstStreamR(this->fileline(), lhsp, rhsp);
    }
    string emitVerilog() override { return "%f{ >> %r %k{%l} }"; }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.opAssign(lhs);
    }
    string emitC() override { return isWide() ? "VL_ASSIGN_W(%nw, %P, %li)" : "%li"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 2; }
};

// === AstNodeSystemBiop ===
class AstAtan2D final : public AstNodeSystemBiop {
public:
    AstAtan2D(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_Atan2D(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_Atan2D;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstAtan2D(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.setDouble(std::atan2(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { return "%f$atan2(%l,%r)"; }
    string emitC() override { return "atan2(%li,%ri)"; }
};
class AstHypotD final : public AstNodeSystemBiop {
public:
    AstHypotD(FileLine* fl, AstNode* lhsp, AstNode* rhsp)
        : ASTGEN_SUPER_HypotD(fl, lhsp, rhsp) {}
    ASTGEN_MEMBERS_HypotD;
    AstNode* cloneType(AstNode* lhsp, AstNode* rhsp) override {
        return new AstHypotD(this->fileline(), lhsp, rhsp);
    }
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs) override {
        out.setDouble(std::hypot(lhs.toDouble(), rhs.toDouble()));
    }
    string emitVerilog() override { return "%f$hypot(%l,%r)"; }
    string emitC() override { return "hypot(%li,%ri)"; }
};

// === AstNodeQuadop ===
class AstCountBits final : public AstNodeQuadop {
    // Number of bits set in vector
public:
    AstCountBits(FileLine* fl, AstNode* exprp, AstNode* ctrl1p)
        : ASTGEN_SUPER_CountBits(fl, exprp, ctrl1p, ctrl1p->cloneTree(false),
                                 ctrl1p->cloneTree(false)) {}
    AstCountBits(FileLine* fl, AstNode* exprp, AstNode* ctrl1p, AstNode* ctrl2p)
        : ASTGEN_SUPER_CountBits(fl, exprp, ctrl1p, ctrl2p, ctrl2p->cloneTree(false)) {}
    AstCountBits(FileLine* fl, AstNode* exprp, AstNode* ctrl1p, AstNode* ctrl2p, AstNode* ctrl3p)
        : ASTGEN_SUPER_CountBits(fl, exprp, ctrl1p, ctrl2p, ctrl3p) {}
    ASTGEN_MEMBERS_CountBits;
    void numberOperate(V3Number& out, const V3Number& expr, const V3Number& ctrl1,
                       const V3Number& ctrl2, const V3Number& ctrl3) override {
        out.opCountBits(expr, ctrl1, ctrl2, ctrl3);
    }
    string emitVerilog() override { return "%f$countbits(%l, %r, %f, %o)"; }
    string emitC() override { return ""; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool cleanThs() const override { return true; }
    bool cleanFhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool sizeMattersThs() const override { return false; }
    bool sizeMattersFhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 16; }
};

// === AstNodeTermop ===
class AstTime final : public AstNodeTermop {
    VTimescale m_timeunit;  // Parent module time unit
public:
    AstTime(FileLine* fl, const VTimescale& timeunit)
        : ASTGEN_SUPER_Time(fl)
        , m_timeunit{timeunit} {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_Time;
    string emitVerilog() override { return "%f$time"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_TIME; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    void dump(std::ostream& str = std::cout) const override;
    void timeunit(const VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() const { return m_timeunit; }
};
class AstTimeD final : public AstNodeTermop {
    VTimescale m_timeunit;  // Parent module time unit
public:
    AstTimeD(FileLine* fl, const VTimescale& timeunit)
        : ASTGEN_SUPER_TimeD(fl)
        , m_timeunit{timeunit} {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_TimeD;
    string emitVerilog() override { return "%f$realtime"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    bool isGateOptimizable() const override { return false; }
    bool isPredictOptimizable() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_TIME; }
    bool same(const AstNode* /*samep*/) const override { return true; }
    void dump(std::ostream& str = std::cout) const override;
    void timeunit(const VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() const { return m_timeunit; }
};

// === AstNodeTriop ===
class AstPostAdd final : public AstNodeTriop {
    // Post-increment/add
    // Parents:  MATH
    // Children: lhsp: AstConst (1) as currently support only ++ not +=
    // Children: rhsp: tree with AstVarRef that is value to read before operation
    // Children: thsp: tree with AstVarRef LValue that is stored after operation
public:
    AstPostAdd(FileLine* fl, AstNode* lhsp, AstNode* rhsp, AstNode* thsp)
        : ASTGEN_SUPER_PostAdd(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_PostAdd;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                       const V3Number& ths) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { return "%k(%r++)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool cleanThs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    bool sizeMattersThs() const override { return true; }
};
class AstPostSub final : public AstNodeTriop {
    // Post-decrement/subtract
    // Parents:  MATH
    // Children: lhsp: AstConst (1) as currently support only -- not -=
    // Children: rhsp: tree with AstVarRef that is value to read before operation
    // Children: thsp: tree with AstVarRef LValue that is stored after operation
public:
    AstPostSub(FileLine* fl, AstNode* lhsp, AstNode* rhsp, AstNode* thsp)
        : ASTGEN_SUPER_PostSub(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_PostSub;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                       const V3Number& ths) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { return "%k(%r--)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool cleanThs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    bool sizeMattersThs() const override { return true; }
};
class AstPreAdd final : public AstNodeTriop {
    // Pre-increment/add
    // Parents:  MATH
    // Children: lhsp: AstConst (1) as currently support only ++ not +=
    // Children: rhsp: tree with AstVarRef that is value to read before operation
    // Children: thsp: tree with AstVarRef LValue that is stored after operation
public:
    AstPreAdd(FileLine* fl, AstNode* lhsp, AstNode* rhsp, AstNode* thsp)
        : ASTGEN_SUPER_PreAdd(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_PreAdd;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                       const V3Number& ths) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { return "%k(++%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool cleanThs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    bool sizeMattersThs() const override { return true; }
};
class AstPreSub final : public AstNodeTriop {
    // Pre-decrement/subtract
    // Parents:  MATH
    // Children: lhsp: AstConst (1) as currently support only -- not -=
    // Children: rhsp: tree with AstVarRef that is value to read before operation
    // Children: thsp: tree with AstVarRef LValue that is stored after operation
public:
    AstPreSub(FileLine* fl, AstNode* lhsp, AstNode* rhsp, AstNode* thsp)
        : ASTGEN_SUPER_PreSub(fl, lhsp, rhsp, thsp) {}
    ASTGEN_MEMBERS_PreSub;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                       const V3Number& ths) override {
        V3ERROR_NA;  // Need to modify lhs
    }
    string emitVerilog() override { return "%k(--%r)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return false; }
    bool cleanThs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
    bool sizeMattersRhs() const override { return true; }
    bool sizeMattersThs() const override { return true; }
};
class AstPutcN final : public AstNodeTriop {
    // Verilog string.putc()
public:
    AstPutcN(FileLine* fl, AstNode* lhsp, AstNode* rhsp, AstNode* ths)
        : ASTGEN_SUPER_PutcN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_PutcN;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                       const V3Number& ths) override {
        out.opPutcN(lhs, rhs, ths);
    }
    string name() const override { return "putc"; }
    string emitVerilog() override { return "%k(%l.putc(%r,%t))"; }
    string emitC() override { return "VL_PUTC_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool cleanThs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool sizeMattersThs() const override { return false; }
};
class AstSel final : public AstNodeTriop {
    // Multiple bit range extraction
    // Parents: math|stmt
    // Children: varref|arraysel, math, constant math
    // @astgen alias op1 := fromp
    // @astgen alias op2 := lsbp
    // @astgen alias op3 := widthp
private:
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
    int m_declElWidth;  // If a packed array, the number of bits per element
public:
    AstSel(FileLine* fl, AstNode* fromp, AstNode* lsbp, AstNode* widthp)
        : ASTGEN_SUPER_Sel(fl, fromp, lsbp, widthp)
        , m_declElWidth{1} {
        if (VN_IS(widthp, Const)) {
            dtypeSetLogicSized(VN_AS(widthp, Const)->toUInt(), VSigning::UNSIGNED);
        }
    }
    AstSel(FileLine* fl, AstNode* fromp, int lsb, int bitwidth)
        : ASTGEN_SUPER_Sel(fl, fromp, new AstConst(fl, lsb), new AstConst(fl, bitwidth))
        , m_declElWidth{1} {
        dtypeSetLogicSized(bitwidth, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_Sel;
    void dump(std::ostream& str) const override;
    void numberOperate(V3Number& out, const V3Number& from, const V3Number& bit,
                       const V3Number& width) override {
        out.opSel(from, bit.toUInt() + width.toUInt() - 1, bit.toUInt());
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override {
        return widthp()->isOne() ? "VL_BITSEL_%nq%lq%rq%tq(%lw, %P, %li, %ri)"
               : isWide()        ? "VL_SEL_%nq%lq%rq%tq(%nw,%lw, %P, %li, %ri, %ti)"
                                 : "VL_SEL_%nq%lq%rq%tq(%lw, %P, %li, %ri, %ti)";
    }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool cleanThs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool sizeMattersThs() const override { return false; }
    bool same(const AstNode*) const override { return true; }
    int instrCount() const override { return widthInstrs() * (VN_CAST(lsbp(), Const) ? 3 : 10); }
    int widthConst() const { return VN_AS(widthp(), Const)->toSInt(); }
    int lsbConst() const { return VN_AS(lsbp(), Const)->toSInt(); }
    int msbConst() const { return lsbConst() + widthConst() - 1; }
    VNumRange& declRange() { return m_declRange; }
    const VNumRange& declRange() const { return m_declRange; }
    void declRange(const VNumRange& flag) { m_declRange = flag; }
    int declElWidth() const { return m_declElWidth; }
    void declElWidth(int flag) { m_declElWidth = flag; }
};
class AstSliceSel final : public AstNodeTriop {
    // Multiple array element extraction
    // Parents: math|stmt
    // @astgen alias op1 := fromp
private:
    VNumRange m_declRange;  // Range of the 'from' array if isRanged() is set, else invalid
public:
    AstSliceSel(FileLine* fl, AstNode* fromp, const VNumRange& declRange)
        : ASTGEN_SUPER_SliceSel(fl, fromp, new AstConst(fl, declRange.lo()),
                                new AstConst(fl, declRange.elements()))
        , m_declRange{declRange} {}
    ASTGEN_MEMBERS_SliceSel;
    void dump(std::ostream& str) const override;
    void numberOperate(V3Number& out, const V3Number& from, const V3Number& lo,
                       const V3Number& width) override {
        V3ERROR_NA;
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }  // Removed before EmitC
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool cleanRhs() const override { return true; }
    bool cleanThs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool sizeMattersThs() const override { return false; }
    bool same(const AstNode*) const override { return true; }
    int instrCount() const override { return 10; }  // Removed before matters
    // For widthConst()/loConst etc, see declRange().elements() and other VNumRange methods
    VNumRange& declRange() { return m_declRange; }
    const VNumRange& declRange() const { return m_declRange; }
    void declRange(const VNumRange& flag) { m_declRange = flag; }
};
class AstSubstrN final : public AstNodeTriop {
    // Verilog string.substr()
public:
    AstSubstrN(FileLine* fl, AstNode* lhsp, AstNode* rhsp, AstNode* ths)
        : ASTGEN_SUPER_SubstrN(fl, lhsp, rhsp, ths) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_SubstrN;
    void numberOperate(V3Number& out, const V3Number& lhs, const V3Number& rhs,
                       const V3Number& ths) override {
        out.opSubstrN(lhs, rhs, ths);
    }
    string name() const override { return "substr"; }
    string emitVerilog() override { return "%k(%l.substr(%r,%t))"; }
    string emitC() override { return "VL_SUBSTR_N(%li,%ri,%ti)"; }
    string emitSimpleOperator() override { return ""; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool cleanRhs() const override { return true; }
    bool cleanThs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool sizeMattersRhs() const override { return false; }
    bool sizeMattersThs() const override { return false; }
};

// === AstNodeCond ===
class AstCond final : public AstNodeCond {
    // Conditional ?: statement
    // Parents:  MATH
    // Children: MATH
public:
    AstCond(FileLine* fl, AstNode* condp, AstNode* thenp, AstNode* elsep)
        : ASTGEN_SUPER_Cond(fl, condp, thenp, elsep) {}
    ASTGEN_MEMBERS_Cond;
    AstNode* cloneType(AstNode* condp, AstNode* thenp, AstNode* elsep) override {
        return new AstCond(this->fileline(), condp, thenp, elsep);
    }
};
class AstCondBound final : public AstNodeCond {
    // Conditional ?: statement, specially made for safety checking of array bounds
    // Parents:  MATH
    // Children: MATH
public:
    AstCondBound(FileLine* fl, AstNode* condp, AstNode* thenp, AstNode* elsep)
        : ASTGEN_SUPER_CondBound(fl, condp, thenp, elsep) {}
    ASTGEN_MEMBERS_CondBound;
    AstNode* cloneType(AstNode* condp, AstNode* thenp, AstNode* elsep) override {
        return new AstCondBound(this->fileline(), condp, thenp, elsep);
    }
};

// === AstNodeUniop ===
class AstAtoN final : public AstNodeUniop {
    // string.atoi(), atobin(), atohex(), atooct(), atoireal()
public:
    enum FmtType { ATOI = 10, ATOHEX = 16, ATOOCT = 8, ATOBIN = 2, ATOREAL = -1 };

private:
    const FmtType m_fmt;  // Operation type
public:
    AstAtoN(FileLine* fl, AstNode* lhsp, FmtType fmt)
        : ASTGEN_SUPER_AtoN(fl, lhsp)
        , m_fmt{fmt} {
        fmt == ATOREAL ? dtypeSetDouble() : dtypeSetSigned32();
    }
    ASTGEN_MEMBERS_AtoN;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opAtoN(lhs, m_fmt); }
    string name() const override {
        switch (m_fmt) {
        case ATOI: return "atoi";
        case ATOHEX: return "atohex";
        case ATOOCT: return "atooct";
        case ATOBIN: return "atobin";
        case ATOREAL: return "atoreal";
        default: V3ERROR_NA;
        }
    }
    string emitVerilog() override { return "%l." + name() + "()"; }
    string emitC() override {
        switch (m_fmt) {
        case ATOI: return "VL_ATOI_N(%li, 10)";
        case ATOHEX: return "VL_ATOI_N(%li, 16)";
        case ATOOCT: return "VL_ATOI_N(%li, 8)";
        case ATOBIN: return "VL_ATOI_N(%li, 2)";
        case ATOREAL: return "std::atof(%li.c_str())";
        default: V3ERROR_NA;
        }
    }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    FmtType format() const { return m_fmt; }
};
class AstBitsToRealD final : public AstNodeUniop {
public:
    AstBitsToRealD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_BitsToRealD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_BitsToRealD;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opBitsToRealD(lhs); }
    string emitVerilog() override { return "%f$bitstoreal(%l)"; }
    string emitC() override { return "VL_CVT_D_Q(%li)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }  // Eliminated before matters
    bool sizeMattersLhs() const override { return false; }  // Eliminated before matters
    int instrCount() const override { return INSTR_COUNT_DBL; }
};
class AstCCast final : public AstNodeUniop {
    // Cast to C-based data type
private:
    int m_size;

public:
    AstCCast(FileLine* fl, AstNode* lhsp, int setwidth, int minwidth = -1)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        m_size = setwidth;
        if (setwidth) {
            if (minwidth == -1) minwidth = setwidth;
            dtypeSetLogicUnsized(setwidth, minwidth, VSigning::UNSIGNED);
        }
    }
    AstCCast(FileLine* fl, AstNode* lhsp, AstNode* typeFromp)
        : ASTGEN_SUPER_CCast(fl, lhsp) {
        dtypeFrom(typeFromp);
        m_size = width();
    }
    ASTGEN_MEMBERS_CCast;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { return "%f$_CAST(%l)"; }
    string emitC() override { return "VL_CAST_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }  // Special cased in V3Cast
    bool same(const AstNode* samep) const override {
        return size() == static_cast<const AstCCast*>(samep)->size();
    }
    void dump(std::ostream& str = std::cout) const override;
    //
    int size() const { return m_size; }
};
class AstCLog2 final : public AstNodeUniop {
public:
    AstCLog2(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_CLog2(fl, lhsp) {
        dtypeSetSigned32();
    }
    ASTGEN_MEMBERS_CLog2;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opCLog2(lhs); }
    string emitVerilog() override { return "%f$clog2(%l)"; }
    string emitC() override { return "VL_CLOG2_%lq(%lW, %P, %li)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 16; }
};
class AstCountOnes final : public AstNodeUniop {
    // Number of bits set in vector
public:
    AstCountOnes(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_CountOnes(fl, lhsp) {}
    ASTGEN_MEMBERS_CountOnes;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opCountOnes(lhs); }
    string emitVerilog() override { return "%f$countones(%l)"; }
    string emitC() override { return "VL_COUNTONES_%lq(%lW, %P, %li)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 16; }
};
class AstCvtPackString final : public AstNodeUniop {
    // Convert to Verilator Packed String (aka verilog "string")
public:
    AstCvtPackString(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_CvtPackString(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_CvtPackString;
    void numberOperate(V3Number& out, const V3Number& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { return "%f$_CAST(%l)"; }
    string emitC() override { return "VL_CVT_PACK_STR_N%lq(%lW, %li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool same(const AstNode* /*samep*/) const override { return true; }
};
class AstExtend final : public AstNodeUniop {
    // Expand a value into a wider entity by 0 extension.  Width is implied from nodep->width()
public:
    AstExtend(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_Extend(fl, lhsp) {}
    AstExtend(FileLine* fl, AstNode* lhsp, int width)
        : ASTGEN_SUPER_Extend(fl, lhsp) {
        dtypeSetLogicSized(width, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_Extend;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opAssign(lhs); }
    string emitVerilog() override { return "%l"; }
    string emitC() override { return "VL_EXTEND_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override {
        return false;  // Because the EXTEND operator self-casts
    }
    int instrCount() const override { return 0; }
};
class AstExtendS final : public AstNodeUniop {
    // Expand a value into a wider entity by sign extension.  Width is implied from nodep->width()
public:
    AstExtendS(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {}
    AstExtendS(FileLine* fl, AstNode* lhsp, int width)
        // Important that widthMin be correct, as opExtend requires it after V3Expand
        : ASTGEN_SUPER_ExtendS(fl, lhsp) {
        dtypeSetLogicSized(width, VSigning::UNSIGNED);
    }
    ASTGEN_MEMBERS_ExtendS;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.opExtendS(lhs, lhsp()->widthMinV());
    }
    string emitVerilog() override { return "%l"; }
    string emitC() override { return "VL_EXTENDS_%nq%lq(%nw,%lw, %P, %li)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override {
        return false;  // Because the EXTEND operator self-casts
    }
    int instrCount() const override { return 0; }
    bool signedFlavor() const override { return true; }
};
class AstFEof final : public AstNodeUniop {
public:
    AstFEof(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_FEof(fl, lhsp) {}
    ASTGEN_MEMBERS_FEof;
    void numberOperate(V3Number& out, const V3Number& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { return "%f$feof(%l)"; }
    string emitC() override { return "(%li ? feof(VL_CVT_I_FP(%li)) : true)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 16; }
    bool isPure() const override { return false; }  // SPECIAL: $display has 'visual' ordering
    AstNode* filep() const { return lhsp(); }
};
class AstFGetC final : public AstNodeUniop {
public:
    AstFGetC(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_FGetC(fl, lhsp) {}
    ASTGEN_MEMBERS_FGetC;
    void numberOperate(V3Number& out, const V3Number& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { return "%f$fgetc(%l)"; }
    // Non-existent filehandle returns EOF
    string emitC() override { return "(%li ? fgetc(VL_CVT_I_FP(%li)) : -1)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 64; }
    bool isPure() const override { return false; }  // SPECIAL: $display has 'visual' ordering
    AstNode* filep() const { return lhsp(); }
};
class AstISToRD final : public AstNodeUniop {
    // $itor where lhs is signed
public:
    AstISToRD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_ISToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_ISToRD;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opISToRD(lhs); }
    string emitVerilog() override { return "%f$itor($signed(%l))"; }
    string emitC() override { return "VL_ISTOR_D_%lq(%lw, %li)"; }
    bool emitCheckMaxWords() override { return true; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
};
class AstIToRD final : public AstNodeUniop {
    // $itor where lhs is unsigned
public:
    AstIToRD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_IToRD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_IToRD;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opIToRD(lhs); }
    string emitVerilog() override { return "%f$itor(%l)"; }
    string emitC() override { return "VL_ITOR_D_%lq(%lw, %li)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
};
class AstIsUnbounded final : public AstNodeUniop {
    // True if is unmbounded ($)
public:
    AstIsUnbounded(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_IsUnbounded(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_IsUnbounded;
    void numberOperate(V3Number& out, const V3Number&) override {
        // Any constant isn't unbounded
        out.setZero();
    }
    string emitVerilog() override { return "%f$isunbounded(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
};
class AstIsUnknown final : public AstNodeUniop {
    // True if any unknown bits
public:
    AstIsUnknown(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_IsUnknown(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_IsUnknown;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opIsUnknown(lhs); }
    string emitVerilog() override { return "%f$isunknown(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
};
class AstLenN final : public AstNodeUniop {
    // Length of a string
public:
    AstLenN(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_LenN(fl, lhsp) {
        dtypeSetSigned32();
    }
    ASTGEN_MEMBERS_LenN;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opLenN(lhs); }
    string emitVerilog() override { return "%f(%l)"; }
    string emitC() override { return "VL_LEN_IN(%li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
};
class AstLogNot final : public AstNodeUniop {
public:
    AstLogNot(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_LogNot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_LogNot;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opLogNot(lhs); }
    string emitVerilog() override { return "%f(! %l)"; }
    string emitC() override { return "VL_LOGNOT_%nq%lq(%nw,%lw, %P, %li)"; }
    string emitSimpleOperator() override { return "!"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
};
class AstNegate final : public AstNodeUniop {
public:
    AstNegate(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_Negate(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Negate;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opNegate(lhs); }
    string emitVerilog() override { return "%f(- %l)"; }
    string emitC() override { return "VL_NEGATE_%lq(%lW, %P, %li)"; }
    string emitSimpleOperator() override { return "-"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
};
class AstNegateD final : public AstNodeUniop {
public:
    AstNegateD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_NegateD(fl, lhsp) {
        dtypeSetDouble();
    }
    ASTGEN_MEMBERS_NegateD;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opNegateD(lhs); }
    string emitVerilog() override { return "%f(- %l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { return "-"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
    bool doubleFlavor() const override { return true; }
};
class AstNot final : public AstNodeUniop {
public:
    AstNot(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_Not(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_Not;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opNot(lhs); }
    string emitVerilog() override { return "%f(~ %l)"; }
    string emitC() override { return "VL_NOT_%lq(%lW, %P, %li)"; }
    string emitSimpleOperator() override { return "~"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool sizeMattersLhs() const override { return true; }
};
class AstNullCheck final : public AstNodeUniop {
    // Return LHS after checking that LHS is non-null
    // Children: VarRef or something returning pointer
public:
    AstNullCheck(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_NullCheck(fl, lhsp) {
        dtypeFrom(lhsp);
    }
    ASTGEN_MEMBERS_NullCheck;
    void numberOperate(V3Number& out, const V3Number& lhs) override { V3ERROR_NA; }
    int instrCount() const override { return 1; }  // Rarely executes
    string emitVerilog() override { return "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    string emitSimpleOperator() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    bool same(const AstNode* samep) const override { return fileline() == samep->fileline(); }
};
class AstOneHot final : public AstNodeUniop {
    // True if only single bit set in vector
public:
    AstOneHot(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_OneHot(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_OneHot;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opOneHot(lhs); }
    string emitVerilog() override { return "%f$onehot(%l)"; }
    string emitC() override { return "VL_ONEHOT_%lq(%lW, %P, %li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 4; }
};
class AstOneHot0 final : public AstNodeUniop {
    // True if only single bit, or no bits set in vector
public:
    AstOneHot0(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_OneHot0(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_OneHot0;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opOneHot0(lhs); }
    string emitVerilog() override { return "%f$onehot0(%l)"; }
    string emitC() override { return "VL_ONEHOT0_%lq(%lW, %P, %li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return widthInstrs() * 3; }
};
class AstRToIRoundS final : public AstNodeUniop {
    // Convert real to integer, with arbitrary sized output (not just "integer" format)
public:
    AstRToIRoundS(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_RToIRoundS(fl, lhsp) {
        dtypeSetSigned32();
    }
    ASTGEN_MEMBERS_RToIRoundS;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opRToIRoundS(lhs); }
    string emitVerilog() override { return "%f$rtoi_rounded(%l)"; }
    string emitC() override {
        return isWide() ? "VL_RTOIROUND_%nq_D(%nw, %P, %li)" : "VL_RTOIROUND_%nq_D(%li)";
    }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return INSTR_COUNT_DBL; }
};
class AstRToIS final : public AstNodeUniop {
    // $rtoi(lhs)
public:
    AstRToIS(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_RToIS(fl, lhsp) {
        dtypeSetSigned32();
    }
    ASTGEN_MEMBERS_RToIS;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opRToIS(lhs); }
    string emitVerilog() override { return "%f$rtoi(%l)"; }
    string emitC() override { return "VL_RTOI_I_D(%li)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }  // Eliminated before matters
    bool sizeMattersLhs() const override { return false; }  // Eliminated before matters
    int instrCount() const override { return INSTR_COUNT_DBL; }
};
class AstRealToBits final : public AstNodeUniop {
public:
    AstRealToBits(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_RealToBits(fl, lhsp) {
        dtypeSetUInt64();
    }
    ASTGEN_MEMBERS_RealToBits;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opRealToBits(lhs); }
    string emitVerilog() override { return "%f$realtobits(%l)"; }
    string emitC() override { return "VL_CVT_Q_D(%li)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }  // Eliminated before matters
    bool sizeMattersLhs() const override { return false; }  // Eliminated before matters
    int instrCount() const override { return INSTR_COUNT_DBL; }
};
class AstRedAnd final : public AstNodeUniop {
public:
    AstRedAnd(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_RedAnd(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_RedAnd;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opRedAnd(lhs); }
    string emitVerilog() override { return "%f(& %l)"; }
    string emitC() override { return "VL_REDAND_%nq%lq(%lw, %P, %li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
};
class AstRedOr final : public AstNodeUniop {
public:
    AstRedOr(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_RedOr(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_RedOr;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opRedOr(lhs); }
    string emitVerilog() override { return "%f(| %l)"; }
    string emitC() override { return "VL_REDOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
};
class AstRedXor final : public AstNodeUniop {
public:
    AstRedXor(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_RedXor(fl, lhsp) {
        dtypeSetBit();
    }
    ASTGEN_MEMBERS_RedXor;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opRedXor(lhs); }
    string emitVerilog() override { return "%f(^ %l)"; }
    string emitC() override { return "VL_REDXOR_%lq(%lW, %P, %li)"; }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override {
        const int w = lhsp()->width();
        return (w != 1 && w != 2 && w != 4 && w != 8 && w != 16);
    }
    bool sizeMattersLhs() const override { return false; }
    int instrCount() const override { return 1 + V3Number::log2b(width()); }
};
class AstSigned final : public AstNodeUniop {
    // $signed(lhs)
public:
    AstSigned(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_Signed(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_Signed;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { return "%f$signed(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }  // Eliminated before matters
    bool sizeMattersLhs() const override { return true; }  // Eliminated before matters
    int instrCount() const override { return 0; }
};
class AstTimeImport final : public AstNodeUniop {
    // Take a constant that represents a time and needs conversion based on time units
    VTimescale m_timeunit;  // Parent module time unit
public:
    AstTimeImport(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_TimeImport(fl, lhsp) {}
    ASTGEN_MEMBERS_TimeImport;
    void numberOperate(V3Number& out, const V3Number& lhs) override { V3ERROR_NA; }
    string emitVerilog() override { return "%l"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }
    bool sizeMattersLhs() const override { return false; }
    void dump(std::ostream& str = std::cout) const override;
    void timeunit(const VTimescale& flag) { m_timeunit = flag; }
    VTimescale timeunit() const { return m_timeunit; }
};
class AstToLowerN final : public AstNodeUniop {
    // string.tolower()
public:
    AstToLowerN(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_ToLowerN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_ToLowerN;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opToLowerN(lhs); }
    string emitVerilog() override { return "%l.tolower()"; }
    string emitC() override { return "VL_TOLOWER_NN(%li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
};
class AstToUpperN final : public AstNodeUniop {
    // string.toupper()
public:
    AstToUpperN(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_ToUpperN(fl, lhsp) {
        dtypeSetString();
    }
    ASTGEN_MEMBERS_ToUpperN;
    void numberOperate(V3Number& out, const V3Number& lhs) override { out.opToUpperN(lhs); }
    string emitVerilog() override { return "%l.toupper()"; }
    string emitC() override { return "VL_TOUPPER_NN(%li)"; }
    bool cleanOut() const override { return true; }
    bool cleanLhs() const override { return true; }
    bool sizeMattersLhs() const override { return false; }
};
class AstUnsigned final : public AstNodeUniop {
    // $unsigned(lhs)
public:
    AstUnsigned(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_Unsigned(fl, lhsp) {
        UASSERT_OBJ(!v3Global.assertDTypesResolved(), this,
                    "not coded to create after dtypes resolved");
    }
    ASTGEN_MEMBERS_Unsigned;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.opAssign(lhs);
        out.isSigned(false);
    }
    string emitVerilog() override { return "%f$unsigned(%l)"; }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return false; }
    bool cleanLhs() const override { return false; }  // Eliminated before matters
    bool sizeMattersLhs() const override { return true; }  // Eliminated before matters
    int instrCount() const override { return 0; }
};

// === AstNodeSystemUniop ===
class AstAcosD final : public AstNodeSystemUniop {
public:
    AstAcosD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_AcosD(fl, lhsp) {}
    ASTGEN_MEMBERS_AcosD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::acos(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$acos(%l)"; }
    string emitC() override { return "acos(%li)"; }
};
class AstAcoshD final : public AstNodeSystemUniop {
public:
    AstAcoshD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_AcoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_AcoshD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::acosh(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$acosh(%l)"; }
    string emitC() override { return "acosh(%li)"; }
};
class AstAsinD final : public AstNodeSystemUniop {
public:
    AstAsinD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_AsinD(fl, lhsp) {}
    ASTGEN_MEMBERS_AsinD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::asin(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$asin(%l)"; }
    string emitC() override { return "asin(%li)"; }
};
class AstAsinhD final : public AstNodeSystemUniop {
public:
    AstAsinhD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_AsinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AsinhD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::asinh(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$asinh(%l)"; }
    string emitC() override { return "asinh(%li)"; }
};
class AstAtanD final : public AstNodeSystemUniop {
public:
    AstAtanD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_AtanD(fl, lhsp) {}
    ASTGEN_MEMBERS_AtanD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::atan(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$atan(%l)"; }
    string emitC() override { return "atan(%li)"; }
};
class AstAtanhD final : public AstNodeSystemUniop {
public:
    AstAtanhD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_AtanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_AtanhD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::atanh(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$atanh(%l)"; }
    string emitC() override { return "atanh(%li)"; }
};
class AstCeilD final : public AstNodeSystemUniop {
public:
    AstCeilD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_CeilD(fl, lhsp) {}
    ASTGEN_MEMBERS_CeilD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::ceil(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$ceil(%l)"; }
    string emitC() override { return "ceil(%li)"; }
};
class AstCosD final : public AstNodeSystemUniop {
public:
    AstCosD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_CosD(fl, lhsp) {}
    ASTGEN_MEMBERS_CosD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::cos(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$cos(%l)"; }
    string emitC() override { return "cos(%li)"; }
};
class AstCoshD final : public AstNodeSystemUniop {
public:
    AstCoshD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_CoshD(fl, lhsp) {}
    ASTGEN_MEMBERS_CoshD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::cosh(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$cosh(%l)"; }
    string emitC() override { return "cosh(%li)"; }
};
class AstExpD final : public AstNodeSystemUniop {
public:
    AstExpD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_ExpD(fl, lhsp) {}
    ASTGEN_MEMBERS_ExpD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::exp(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$exp(%l)"; }
    string emitC() override { return "exp(%li)"; }
};
class AstFloorD final : public AstNodeSystemUniop {
public:
    AstFloorD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_FloorD(fl, lhsp) {}
    ASTGEN_MEMBERS_FloorD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::floor(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$floor(%l)"; }
    string emitC() override { return "floor(%li)"; }
};
class AstLog10D final : public AstNodeSystemUniop {
public:
    AstLog10D(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_Log10D(fl, lhsp) {}
    ASTGEN_MEMBERS_Log10D;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::log10(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$log10(%l)"; }
    string emitC() override { return "log10(%li)"; }
};
class AstLogD final : public AstNodeSystemUniop {
public:
    AstLogD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_LogD(fl, lhsp) {}
    ASTGEN_MEMBERS_LogD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::log(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$ln(%l)"; }
    string emitC() override { return "log(%li)"; }
};
class AstSinD final : public AstNodeSystemUniop {
public:
    AstSinD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_SinD(fl, lhsp) {}
    ASTGEN_MEMBERS_SinD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::sin(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$sin(%l)"; }
    string emitC() override { return "sin(%li)"; }
};
class AstSinhD final : public AstNodeSystemUniop {
public:
    AstSinhD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_SinhD(fl, lhsp) {}
    ASTGEN_MEMBERS_SinhD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::sinh(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$sinh(%l)"; }
    string emitC() override { return "sinh(%li)"; }
};
class AstSqrtD final : public AstNodeSystemUniop {
public:
    AstSqrtD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_SqrtD(fl, lhsp) {}
    ASTGEN_MEMBERS_SqrtD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::sqrt(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$sqrt(%l)"; }
    string emitC() override { return "sqrt(%li)"; }
};
class AstTanD final : public AstNodeSystemUniop {
public:
    AstTanD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_TanD(fl, lhsp) {}
    ASTGEN_MEMBERS_TanD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::tan(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$tan(%l)"; }
    string emitC() override { return "tan(%li)"; }
};
class AstTanhD final : public AstNodeSystemUniop {
public:
    AstTanhD(FileLine* fl, AstNode* lhsp)
        : ASTGEN_SUPER_TanhD(fl, lhsp) {}
    ASTGEN_MEMBERS_TanhD;
    void numberOperate(V3Number& out, const V3Number& lhs) override {
        out.setDouble(std::tanh(lhs.toDouble()));
    }
    string emitVerilog() override { return "%f$tanh(%l)"; }
    string emitC() override { return "tanh(%li)"; }
};

// === AstNodeVarRef ===
class AstVarRef final : public AstNodeVarRef {
    // A reference to a variable (lvalue or rvalue)
public:
    AstVarRef(FileLine* fl, const string& name, const VAccess& access)
        : ASTGEN_SUPER_VarRef(fl, name, nullptr, access) {}
    // This form only allowed post-link because output/wire compression may
    // lead to deletion of AstVar's
    inline AstVarRef(FileLine* fl, AstVar* varp, const VAccess& access);
    // This form only allowed post-link (see above)
    inline AstVarRef(FileLine* fl, AstVarScope* varscp, const VAccess& access);
    ASTGEN_MEMBERS_VarRef;
    void dump(std::ostream& str) const override;
    bool same(const AstNode* samep) const override;
    inline bool same(const AstVarRef* samep) const;
    inline bool sameNoLvalue(AstVarRef* samep) const;
    int instrCount() const override {
        return widthInstrs() * (access().isReadOrRW() ? INSTR_COUNT_LD : 1);
    }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
};
class AstVarXRef final : public AstNodeVarRef {
    // A VarRef to something in another module before AstScope.
    // Includes pin on a cell, as part of a ASSIGN statement to connect I/Os until AstScope
private:
    string m_dotted;  // Dotted part of scope the name()'ed reference is under or ""
    string m_inlinedDots;  // Dotted hierarchy flattened out
public:
    AstVarXRef(FileLine* fl, const string& name, const string& dotted, const VAccess& access)
        : ASTGEN_SUPER_VarXRef(fl, name, nullptr, access)
        , m_dotted{dotted} {}
    inline AstVarXRef(FileLine* fl, AstVar* varp, const string& dotted, const VAccess& access);
    ASTGEN_MEMBERS_VarXRef;
    void dump(std::ostream& str) const override;
    string dotted() const { return m_dotted; }
    void dotted(const string& dotted) { m_dotted = dotted; }
    string inlinedDots() const { return m_inlinedDots; }
    void inlinedDots(const string& flag) { m_inlinedDots = flag; }
    string emitVerilog() override { V3ERROR_NA_RETURN(""); }
    string emitC() override { V3ERROR_NA_RETURN(""); }
    bool cleanOut() const override { return true; }
    int instrCount() const override { return widthInstrs(); }
    bool same(const AstNode* samep) const override {
        const AstVarXRef* asamep = static_cast<const AstVarXRef*>(samep);
        return (selfPointer() == asamep->selfPointer() && varp() == asamep->varp()
                && name() == asamep->name() && dotted() == asamep->dotted());
    }
};

#endif  // Guard
