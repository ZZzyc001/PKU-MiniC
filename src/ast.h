#pragma once

#include <iostream>

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string              ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << " }";
    }
};

// FuncType
class FuncTypeAST : public BaseAST {
public:
    void Dump() const override {
        std::cout << "FuncTypeAST { ";
        std::cout << "int";
        std::cout << " }";
    }
};

// Block
class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }
};

// Stmt
class StmtAST : public BaseAST {
public:
    int number;

    void Dump() const override {
        std::cout << "StmtAST { ";
        std::cout << number;
        std::cout << " }";
    }
};

// // Number
// class NumberAST : public BaseAST {
//     public:
//     int int_const;

//     void Dump() const override {
//         std::cout << int_const;
//     }
// }

// CompUnitAST { FuncDefAST { FuncTypeAST { int }, main, BlockAST { StmtAST { 0 } } } }

/*CompUnit {
    func_def: FuncDef {
        func_type: Int,
        ident: "main",
        block: Block {
            stmt: Stmt {
                num: 0,
            },
        },
    },
}*/