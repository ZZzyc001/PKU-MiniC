%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "ast.h"

std::vector<std::vector<std::unique_ptr<BaseAST>>> env_stk;

int branch_id = 0;

void add_inst(BaseAST * ast)
{
  env_stk.rbegin()->push_back(std::unique_ptr<BaseAST>(ast));
}

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
%union {
    std::string * str_val;
    int int_val;
    BaseAST * ast_val;
    ValueBaseAST * value_ast_val;
    LValueBaseAST * lvalue_ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT UNARYOP MULOP ADDOP RELOP EQOP LANDOP LOROP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block BlockItem Stmt Decl OpenStmt ClosStmt SimpStmt
%type <value_ast_val> Exp PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <lvalue_ast_val> LVal
%type <int_val> Number 

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    $$ = ast;
  }
  ;

Block
  : '{' {
    env_stk.push_back({});
  }
  BlockItems '}' {
    auto ast = new BlockAST();
    for (auto it = env_stk[env_stk.size() - 1].begin(); it < env_stk[env_stk.size() - 1].end(); ++it)
      if (it->get())
        ast->insts.push_back(std::move(*it));
    $$ = ast;
    env_stk.pop_back();
  }
  | '{' '}' {
    auto ast = new BlockAST();
    $$ = ast;
  }
  ;

BlockItems : BlockItem {
    add_inst($1);
  }
  | BlockItems BlockItem {
    add_inst($2);
  }
  ;

BlockItem : Decl | Stmt;

Stmt: OpenStmt | ClosStmt;

OpenStmt: IF '(' Exp ')' Stmt {
    auto ast = new BranchStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->lval = unique_ptr<BaseAST>($5);
    ast->branch_id = branch_id++;
    
    $$ = ast;
  }
  | IF '(' Exp ')' ClosStmt ELSE OpenStmt {
    auto ast = new BranchStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->lval = unique_ptr<BaseAST>($5);
    ast->rval = unique_ptr<BaseAST>($7);
    ast->branch_id = branch_id++;
    
    $$ = ast;
  }
  | WHILE '(' Exp ')' OpenStmt {
    auto ast = new WhileStmtAST();
    ast -> exp = unique_ptr<BaseAST>($3);
    ast -> stmt = unique_ptr<BaseAST>($5);
    ast -> while_id = branch_id++;

    $$ = ast;
  }
  ;

ClosStmt : SimpStmt
  | IF '(' Exp ')' ClosStmt ELSE ClosStmt {
    auto ast = new BranchStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->lval = unique_ptr<BaseAST>($5);
    ast->rval = unique_ptr<BaseAST>($7);
    ast->branch_id = branch_id++;
    
    $$ = ast;
  }
  | WHILE '(' Exp ')' ClosStmt {
    auto ast = new WhileStmtAST();
    ast -> exp = unique_ptr<BaseAST>($3);
    ast -> stmt = unique_ptr<BaseAST>($5);
    ast -> while_id = branch_id++;

    $$ = ast;
  }
  ;

SimpStmt
  : LVal '=' Exp ';' {
    auto ast = new AssignStmtAST();
    ast -> lval = unique_ptr<LValueBaseAST>($1);
    ast -> exp = unique_ptr<BaseAST>($3);

    $$ = ast;
  }
  | ';' {
    $$ = nullptr;
  }
  | Exp ';' {
    $$ = $1;
  }
  | Block {
    $$ = $1;
  }
  | BREAK ';' {
    auto ast = new BreakAST();
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new ContinueAST();
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new ReturnStmtAST();
    ast -> exp = unique_ptr<BaseAST>($2);

    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')'{
    auto ast = new PrimaryExpAST();
    ast -> type = PrimaryExpAST::PrimaryExpType::Exp;
    ast -> exp = unique_ptr<ValueBaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new ExpAST();
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast -> type = PrimaryExpAST::PrimaryExpType::Number;
    ast -> number = $1;
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast -> type = UnaryExpAST::UnaryExpType::PrimaryExp;
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  | UNARYOP UnaryExp {
    auto ast = new UnaryExpAST();
    ast -> type = UnaryExpAST::UnaryExpType::UnaryExp;
    ast -> op = *unique_ptr<string>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($2);
    $$ = ast;
  }
  | ADDOP UnaryExp {
    auto ast = new UnaryExpAST();
    ast -> type = UnaryExpAST::UnaryExpType::UnaryExp;
    ast -> op = *unique_ptr<string>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($2);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast -> type = MulExpAST::MulExpType::Unary;
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  | MulExp MULOP UnaryExp {
    auto ast = new MulExpAST();
    ast -> type = MulExpAST::MulExpType::Binary;
    ast -> op = *unique_ptr<string>($2);
    ast -> left_exp = unique_ptr<ValueBaseAST>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($3);
    $$ = ast;
  }

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast -> type = AddExpAST::AddExpType::Unary;
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  | AddExp ADDOP MulExp {
    auto ast = new AddExpAST();
    ast -> type = AddExpAST::AddExpType::Binary;
    ast -> op = *unique_ptr<string>($2);
    ast -> left_exp = unique_ptr<ValueBaseAST>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($3);
    $$ = ast;
  };

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast -> type = RelExpAST::RelExpType::Unary;
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    ast -> type = RelExpAST::RelExpType::Binary;
    ast -> op = *unique_ptr<string>($2);
    ast -> left_exp = unique_ptr<ValueBaseAST>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($3);
    $$ = ast;
  };

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast -> type = EqExpAST::EqExpType::Unary;
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast -> type = EqExpAST::EqExpType::Binary;
    ast -> op = *unique_ptr<string>($2);
    ast -> left_exp = unique_ptr<ValueBaseAST>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($3);
    $$ = ast;
  };

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast -> type = LAndExpAST::LAndExpType::Unary;
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  | LAndExp LANDOP EqExp {
    auto ast = new LAndExpAST();
    ast -> type = LAndExpAST::LAndExpType::Binary;
    ast -> left_exp = unique_ptr<ValueBaseAST>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($3);
    ast->branch_id = branch_id++;
    $$ = ast;
  };

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast -> type = LOrExpAST::LOrExpType::Unary;
    ast -> exp = unique_ptr<ValueBaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOROP LAndExp {
    auto ast = new LOrExpAST();
    ast -> type = LOrExpAST::LOrExpType::Binary;
    ast -> left_exp = unique_ptr<ValueBaseAST>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($3);
    ast->branch_id = branch_id++;
    $$ = ast;
  };

Decl : ConstDecl {
    $$ = nullptr;
  }
  | VarDecl {
    $$ = nullptr;
  }

ConstDecl : CONST INT ConstDefs ';';
ConstDefs : ConstDef | ConstDefs ',' ConstDef;
ConstDef
  : IDENT '=' Exp {
    auto ast = new ConstDefAST();
    ast -> name = *unique_ptr<string>($1);
    ast -> exp = unique_ptr<ValueBaseAST>($3);
    add_inst(ast);
  };

VarDecl : INT VarDefs ';';
VarDefs : VarDef | VarDefs ',' VarDef;
VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast -> name = *unique_ptr<string>($1);
    add_inst(ast);
  }
  | IDENT '=' Exp {
    auto ast = new VarDefAST();
    ast -> name = *unique_ptr<string>($1);
    ast -> exp = unique_ptr<BaseAST>($3);
    add_inst(ast);
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast -> name = *unique_ptr<string>($1);
    $$ = ast;
  };

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}