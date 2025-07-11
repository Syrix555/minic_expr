///
/// @file Antlr4CSTVisitor.cpp
/// @brief Antlr4的具体语法树的遍历产生AST
/// @author zenglj (zenglj@live.com)
/// @version 1.1
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// <tr><td>2024-11-23 <td>1.1     <td>zenglj  <td>表达式版增强
/// </table>
///

#include <any>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Antlr4CSTVisitor.h"
#include "AST.h"
#include "ArrayType.h"
#include "AttrType.h"
#include "MiniCParser.h"
#include "PointerType.h"
#include "Type.h"

#define Instanceof(res, type, var) auto res = dynamic_cast<type>(var)

/// @brief 用于从 visitVarDef 向 visitVarDecl 传递变量定义信息的辅助结构体
struct VarDefInfo {
	ast_node * id_node;                     // id节点，原visitVarDef返回值
    std::vector<ast_node*> dim_nodes;   	// 维度表达式节点列表
    ast_node* init_node = nullptr;      	// 初始化值节点
};

/// @brief 构造函数
MiniCCSTVisitor::MiniCCSTVisitor()
{}

/// @brief 析构函数
MiniCCSTVisitor::~MiniCCSTVisitor()
{}

/// @brief 遍历CST产生AST
/// @param root CST语法树的根结点
/// @return AST的根节点
ast_node * MiniCCSTVisitor::run(MiniCParser::CompileUnitContext * root)
{
    return std::any_cast<ast_node *>(visitCompileUnit(root));
}

/// @brief 非终结运算符compileUnit的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitCompileUnit(MiniCParser::CompileUnitContext * ctx)
{
    // compileUnit: (funcDef | varDecl)* EOF

    // 请注意这里必须先遍历全局变量后遍历函数。肯定可以确保全局变量先声明后使用的规则，但有些情况却不能检查出。
    // 事实上可能函数A后全局变量B后函数C，这时在函数A中是不能使用变量B的，需要报语义错误，但目前的处理不会。
    // 因此在进行语义检查时，可能追加检查行号和列号，如果函数的行号/列号在全局变量的行号/列号的前面则需要报语义错误
    // TODO 请追加实现。

    ast_node * temp_node;
    ast_node * compileUnitNode = create_contain_node(ast_operator_type::AST_OP_COMPILE_UNIT);

    // 可能多个变量，因此必须循环遍历
    for (auto varCtx: ctx->varDecl()) {

        // 变量函数定义
        temp_node = std::any_cast<ast_node *>(visitVarDecl(varCtx));
        (void) compileUnitNode->insert_son_node(temp_node);
    }

    // 可能有多个函数，因此必须循环遍历
    for (auto funcCtx: ctx->funcDef()) {

        // 变量函数定义
        temp_node = std::any_cast<ast_node *>(visitFuncDef(funcCtx));
        (void) compileUnitNode->insert_son_node(temp_node);
    }

    return compileUnitNode;
}

/// @brief 非终结运算符funcDef的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitFuncDef(MiniCParser::FuncDefContext * ctx)
{
    // 识别的文法产生式：funcDef: funcType T_ID T_L_PAREN funcFParams? T_R_PAREN block;

    // 函数返回类型，终结符
    type_attr funcReturnType = std::any_cast<type_attr>(visitFuncType(ctx->funcType()));

    // 创建函数名的标识符终结符节点，终结符
    char * id = strdup(ctx->T_ID()->getText().c_str());

    var_id_attr funcId{id, (int64_t) ctx->T_ID()->getSymbol()->getLine()};

    ast_node * formalParamsNode = nullptr;
    // 如果有形参，那么进行设置
    if (ctx->funcFParams()) {
        formalParamsNode = std::any_cast<ast_node *>(visitFuncFParams(ctx->funcFParams()));
	}

    // 遍历block结点创建函数体节点，非终结符
    auto blockNode = std::any_cast<ast_node *>(visitBlock(ctx->block()));

    // 创建函数定义的节点，孩子有类型，函数名，语句块和形参(实际上无)
    // create_func_def函数内会释放funcId中指向的标识符空间，切记，之后不要再释放，之前一定要是通过strdup函数或者malloc分配的空间
    return create_func_def(funcReturnType, funcId, blockNode, formalParamsNode);
}

/// @brief 非终结运算符funcType的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitFuncType(MiniCParser::FuncTypeContext * ctx)
{
	// 识别的文法产生式：funcType: T_INT | T_VOID;

    type_attr attr;
    if (ctx->T_INT()) {
        attr.type = BasicType::TYPE_INT;
        attr.lineno = (int64_t) ctx->T_INT()->getSymbol()->getLine();
    } else if (ctx->T_VOID()) {
        attr.type = BasicType::TYPE_VOID;
        attr.lineno = (int64_t) ctx->T_VOID()->getSymbol()->getLine();
    } else {
        attr.type = BasicType::TYPE_VOID;
        attr.lineno = -1;
    }

    return attr;
}

/// @brief 非终结运算符funcFParams的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitFuncFParams(MiniCParser::FuncFParamsContext * ctx)
{
    // 识别的文法产生式：funcFParams: funcFParam (T_COMMA funcFParam)*;

    ast_node * params_node = create_contain_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS);

    for (auto paramCtx: ctx->funcFParam()) {
		// 形参节点
        ast_node * param_node = std::any_cast<ast_node *>(visitFuncFParam(paramCtx));
		// 插入形参节点
        (void) params_node->insert_son_node(param_node);
    }

    return params_node;
}

/// @brief 非终结运算符funcFParam的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitFuncFParam(MiniCParser::FuncFParamContext * ctx)
{
	// 识别的文法产生式：funcFParam: basicType T_ID (T_L_BRACKET expr? T_R_BRACKET (T_L_BRACKET expr T_R_BRACKET)*)?;

    type_attr typeAttr = std::any_cast<type_attr>(visitBasicType(ctx->basicType()));
    auto varId = ctx->T_ID()->getText();
    int64_t lineNo = (int64_t) ctx->T_ID()->getSymbol()->getLine();

    ast_node * type_node = create_type_node(typeAttr);
    ast_node * id_node = ast_node::New(varId, lineNo);

    ast_node * param_node = ast_node::New(ast_operator_type::AST_OP_FUNC_FORMAL_PARAM, type_node, id_node, nullptr);

    const Type * baseType = type_node->type;
    const Type * completeType = baseType;

    // 是数组就进入
    if (ctx->T_L_BRACKET(0)) {
		// 第一维元素数默认为0
        ast_node * dim1_node = ast_node::New(digit_int_attr{0, lineNo});;
		ast_node * dim_node = create_contain_node(ast_operator_type::AST_OP_ARRAY_DIM, dim1_node);

		(void) param_node->insert_son_node(dim_node);

        std::vector<ast_node *> dim_nodes;
        for (auto dimCtx: ctx->dims) {
			ast_node * expr_node = std::any_cast<ast_node *>(visitExpr(dimCtx));
			ast_node * dim_node = create_contain_node(ast_operator_type::AST_OP_ARRAY_DIM, expr_node);

            dim_nodes.push_back(dim_node);
			(void) param_node->insert_son_node(dim_node);
		}

		for (auto it = dim_nodes.rbegin(); it != dim_nodes.rend(); ++it) {
			ast_node * dim_node = *it;
			std::optional<uint32_t> size_opt =
				calculate_const_dim_size(dim_node->sons.empty() ? nullptr : dim_node->sons[0]);

			uint32_t numElements = size_opt.value_or(0);
			completeType = ArrayType::get(const_cast<Type *>(completeType), numElements);
        }

		// 第一维退化处理
        completeType = ArrayType::get(const_cast<Type *>(completeType), 0);

		completeType = PointerType::get(const_cast<Type *>(completeType));
    }

    param_node->type = const_cast<Type *>(completeType);

    return param_node;
}

/// @brief 非终结运算符block的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitBlock(MiniCParser::BlockContext * ctx)
{
    // 识别的文法产生式：block : T_L_BRACE blockItemList? T_R_BRACE';
    if (!ctx->blockItemList()) {
        // 语句块没有语句

        // 为了方便创建一个空的Block节点
        return create_contain_node(ast_operator_type::AST_OP_BLOCK);
    }

    // 语句块含有语句

    // 内部创建Block节点，并把语句加入，这里不需要创建Block节点
    return visitBlockItemList(ctx->blockItemList());
}

/// @brief 非终结运算符blockItemList的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitBlockItemList(MiniCParser::BlockItemListContext * ctx)
{
    // 识别的文法产生式：blockItemList : blockItem +;
    // 正闭包 循环 至少一个blockItem
    auto block_node = create_contain_node(ast_operator_type::AST_OP_BLOCK);

    for (auto blockItemCtx: ctx->blockItem()) {

        // 非终结符，需遍历
        auto blockItem = std::any_cast<ast_node *>(visitBlockItem(blockItemCtx));

        // 插入到块节点中
        (void) block_node->insert_son_node(blockItem);
    }

    return block_node;
}

///
/// @brief 非终结运算符blockItem的遍历
/// @param ctx CST上下文
///
std::any MiniCCSTVisitor::visitBlockItem(MiniCParser::BlockItemContext * ctx)
{
    // 识别的文法产生式：blockItem : stmt | varDecl
    if (ctx->stmt()) {
        // 语句识别

        return visitStmt(ctx->stmt());
    } else if (ctx->varDecl()) {
        return visitVarDecl(ctx->varDecl());
    }

    return nullptr;
}

/// @brief 非终结运算符stmt中的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitStmt(MiniCParser::StmtContext * ctx)
{
    // 识别的文法产生式：stmt: T_ID T_ASSIGN expr T_SEMICOLON  # assignStatement
    // | T_RETURN expr T_SEMICOLON # returnStatement
    // | block  # blockStatement
    // | expr ? T_SEMICOLON #expressionStatement
    // | ifStmt	# ifStatement
	// | whileStmt                         # whileStatement
	// | breakStmt							# breakStatement
	// | continueStmt						# continueStatement;
    if (Instanceof(assignCtx, MiniCParser::AssignStatementContext *, ctx)) {
        return visitAssignStatement(assignCtx);
    } else if (Instanceof(returnCtx, MiniCParser::ReturnStatementContext *, ctx)) {
        return visitReturnStatement(returnCtx);
    } else if (Instanceof(blockCtx, MiniCParser::BlockStatementContext *, ctx)) {
        return visitBlockStatement(blockCtx);
    } else if (Instanceof(exprCtx, MiniCParser::ExpressionStatementContext *, ctx)) {
        return visitExpressionStatement(exprCtx);
    } else if (Instanceof(ifCtx, MiniCParser::IfStatementContext *, ctx)) {
        return visitIfStatement(ifCtx);
    } else if (Instanceof(whileCtx, MiniCParser::WhileStatementContext *, ctx)) {
        return visitWhileStatement(whileCtx);
	} else if (Instanceof(breakCtx, MiniCParser::BreakStatementContext *, ctx)) {
        return visitBreakStatement(breakCtx);
	} else if (Instanceof(continueCtx, MiniCParser::ContinueStatementContext *, ctx)) {
        return visitContinueStatement(continueCtx);
	}

    return nullptr;
}

///
/// @brief 非终结运算符stmt中的returnStatement的遍历
/// @param ctx CST上下文
///
std::any MiniCCSTVisitor::visitReturnStatement(MiniCParser::ReturnStatementContext * ctx)
{
    // 识别的文法产生式：returnStatement -> T_RETURN expr? T_SEMICOLON

    // 非终结符，表达式expr遍历
    ast_node * exprNode = nullptr;
    if (ctx->expr()) {
        exprNode = std::any_cast<ast_node *>(visitExpr(ctx->expr()));
	}

    // 创建返回节点，其孩子为Expr
    return create_contain_node(ast_operator_type::AST_OP_RETURN, exprNode);
}

/// @brief 内部产生的非终结符ifStatement的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitIfStatement(MiniCParser::IfStatementContext * ctx)
{
	// 识别产生式：stmt: ifStmt

    return visitIfStmt(ctx->ifStmt());
}

/// @brief 内部产生的非终结符whileStatement的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitWhileStatement(MiniCParser::WhileStatementContext * ctx)
{
    // 识别产生式：stmt: whileStmt
    
    return visitWhileStmt(ctx->whileStmt());
}

/// @brief 内部产生的非终结符breakStatement的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitBreakStatement(MiniCParser::BreakStatementContext * ctx)
{
    // 识别产生式：stmt: breakStmt
    
    return visitBreakStmt(ctx->breakStmt());
}

/// @brief 内部产生的非终结符continueStatement的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitContinueStatement(MiniCParser::ContinueStatementContext * ctx)
{
    // 识别产生式：stmt: continueStmt
    
    return visitContinueStmt(ctx->continueStmt());
}

/// @brief 非终结运算符expr的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitExpr(MiniCParser::ExprContext * ctx)
{
    // 识别产生式：expr: lOrExp

    return visitLOrExp(ctx->lOrExp());
}

/// @brief 非终结符cond的遍历
/// @param ctx
std::any MiniCCSTVisitor::visitCond(MiniCParser::CondContext * ctx)
{
    // 识别产生式：cond: lOrExp

    return visitLOrExp(ctx->lOrExp());
}

/// @brief 内部产生的非终结符assignStatement的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitAssignStatement(MiniCParser::AssignStatementContext * ctx)
{
    // 识别文法产生式：assignStatement: lVal T_ASSIGN expr T_SEMICOLON

    // 赋值左侧左值Lval遍历产生节点
    auto lvalNode = std::any_cast<ast_node *>(visitLVal(ctx->lVal()));

    // 赋值右侧expr遍历
    auto exprNode = std::any_cast<ast_node *>(visitExpr(ctx->expr()));

    // 创建一个AST_OP_ASSIGN类型的中间节点，孩子为Lval和Expr
    return ast_node::New(ast_operator_type::AST_OP_ASSIGN, lvalNode, exprNode, nullptr);
}

/// @brief 内部产生的非终结符blockStatement的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitBlockStatement(MiniCParser::BlockStatementContext * ctx)
{
    // 识别文法产生式 blockStatement: block

    return visitBlock(ctx->block());
}

/// @brief 非终结符mulExp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitMulExp(MiniCParser::MulExpContext * ctx)
{
    // 识别的文法产生式：mulExp : unaryExp (mulOp unaryExp)*;

    if (ctx->mulOp().empty()) {

        // 没有mulOp运算符，则说明闭包识别为0，只识别了第一个非终结符unaryExp
        return visitUnaryExp(ctx->unaryExp()[0]);
    }

    ast_node *left, *right;

    // 存在mulOp运算符，自
    auto opsCtxVec = ctx->mulOp();

    // 有操作符，肯定会进循环，使得right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {

        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitMulOp(opsCtxVec[k]));

        if (k == 0) {

            // 左操作数
            left = std::any_cast<ast_node *>(visitUnaryExp(ctx->unaryExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitUnaryExp(ctx->unaryExp()[k + 1]));

        // 新建结点作为下一个运算符的右操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结符mulOp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitMulOp(MiniCParser::MulOpContext * ctx)
{
    // 识别的文法产生式：mulOp: T_MUL | T_DIV | T_MOD
    if (ctx->T_MUL()) {
        return ast_operator_type::AST_OP_MUL;
    } else if (ctx->T_DIV()) {
        return ast_operator_type::AST_OP_DIV;
    } else {
        return ast_operator_type::AST_OP_MOD;
	}
}

/// @brief 非终结符AddExp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitAddExp(MiniCParser::AddExpContext * ctx)
{
    // 识别的文法产生式：addExp : mulExp (addOp mulExp)*;

    if (ctx->addOp().empty()) {

        // 没有addOp运算符，则说明闭包识别为0，只识别了第一个非终结符mulExp
        return visitMulExp(ctx->mulExp()[0]);
    }

    ast_node *left, *right;

    // 存在addOp运算符，自
    auto opsCtxVec = ctx->addOp();

    // 有操作符，肯定会进循环，使得right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {

        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitAddOp(opsCtxVec[k]));

        if (k == 0) {

            // 左操作数
            left = std::any_cast<ast_node *>(visitMulExp(ctx->mulExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitMulExp(ctx->mulExp()[k + 1]));

        // 新建结点作为下一个运算符的右操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;

}

/// @brief 非终结运算符addOp的遍历
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitAddOp(MiniCParser::AddOpContext * ctx)
{
    // 识别的文法产生式：addOp : T_ADD | T_SUB

    if (ctx->T_ADD()) {
        return ast_operator_type::AST_OP_ADD;
    } else {
        return ast_operator_type::AST_OP_SUB;
    }
}

/// @brief 非终结符relExp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitRelExp(MiniCParser::RelExpContext * ctx)
{
    // 识别的文法产生式：relExp: addExp (relOp addExp)*

    if (ctx->relOp().empty()) {
        // 没有relOp运算符，则说明闭包识别为0，只识别了第一个非终结符addExp
        return visitAddExp(ctx->addExp()[0]);
    }

	ast_node *left, *right;

    // 存在relOp的情况下，进行自动转化
    auto opsCtxVec = ctx->relOp();

    // 有操作符，必定会进入循环，为right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {

        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitRelOp(opsCtxVec[k]));

        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitAddExp(ctx->addExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitAddExp(ctx->addExp()[k + 1]));

        // 新建结点作为下一个运算符的右操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结符relOp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitRelOp(MiniCParser::RelOpContext * ctx)
{
    if (ctx->T_LT()) {
        return ast_operator_type::AST_OP_LT;
    } else if (ctx->T_GT()) {
        return ast_operator_type::AST_OP_GT;
    } else if (ctx->T_LE()) {
        return ast_operator_type::AST_OP_LE;
    } else {
        return ast_operator_type::AST_OP_GE;
	}
}

/// @brief 非终结符eqExp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitEqExp(MiniCParser::EqExpContext * ctx)
{
    // 识别的文法产生式：eqExp: relExp (eqOp relExp)*

    if (ctx->eqOp().empty()) {
        // 没有eqOp运算符，则说明闭包识别为0，只识别了第一个非终结符relExp
        return visitRelExp(ctx->relExp()[0]);
    }

	ast_node *left, *right;

    // 存在eqOp的情况下，进行自动转化
    auto opsCtxVec = ctx->eqOp();

    // 有操作符，必定会进入循环，为right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {

        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitEqOp(opsCtxVec[k]));

        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitRelExp(ctx->relExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitRelExp(ctx->relExp()[k + 1]));

        // 新建结点作为下一个运算符的右操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结符eqOp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitEqOp(MiniCParser::EqOpContext * ctx)
{
    if (ctx->T_EQ()) {
        return ast_operator_type::AST_OP_EQ;
    } else {
        return ast_operator_type::AST_OP_NE;
	}
}

/// @brief 非终结符lAndExp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitLAndExp(MiniCParser::LAndExpContext * ctx)
{
    // 识别的文法产生式：lAndExp: eqExp (lAndOp eqExp)*

    if (ctx->lAndOp().empty()) {
        // 没有lAndOp运算符，则说明闭包识别为0，只识别了第一个非终结符relExp
        return visitEqExp(ctx->eqExp()[0]);
    }

	ast_node *left, *right;

    // 存在lAndOp的情况下，进行自动转化
    auto opsCtxVec = ctx->lAndOp();

    // 有操作符，必定会进入循环，为right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {

        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitLAndOp(opsCtxVec[k]));

        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitEqExp(ctx->eqExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitEqExp(ctx->eqExp()[k + 1]));

        // 新建结点作为下一个运算符的右操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结符lAndOp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitLAndOp(MiniCParser::LAndOpContext * ctx)
{
    if (ctx->T_AND()) {
        return ast_operator_type::AST_OP_AND;
    } else {
        return std::any{nullptr};
	}
}

/// @brief 非终结符lOrExp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitLOrExp(MiniCParser::LOrExpContext * ctx)
{
    // 识别的文法产生式：lOrExp: lAndExp (lOrOp lAndExp)*

    if (ctx->lOrOp().empty()) {
        // 没有lOrOp运算符，则说明闭包识别为0，只识别了第一个非终结符lAndExp
        return visitLAndExp(ctx->lAndExp()[0]);
    }

	ast_node *left, *right;

    // 存在lOrOp的情况下，进行自动转化
    auto opsCtxVec = ctx->lOrOp();

    // 有操作符，必定会进入循环，为right设置正确的值
    for (int k = 0; k < (int) opsCtxVec.size(); k++) {

        // 获取运算符
        ast_operator_type op = std::any_cast<ast_operator_type>(visitLOrOp(opsCtxVec[k]));

        if (k == 0) {
            // 左操作数
            left = std::any_cast<ast_node *>(visitLAndExp(ctx->lAndExp()[k]));
        }

        // 右操作数
        right = std::any_cast<ast_node *>(visitLAndExp(ctx->lAndExp()[k + 1]));

        // 新建结点作为下一个运算符的右操作符
        left = ast_node::New(op, left, right, nullptr);
    }

    return left;
}

/// @brief 非终结符lOrOp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitLOrOp(MiniCParser::LOrOpContext * ctx)
{
    if (ctx->T_OR()) {
        return ast_operator_type::AST_OP_OR;
    } else {
        return std::any{nullptr};
	}
}

/// @brief 非终结符initVal的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitInitVal(MiniCParser::InitValContext * ctx)
{
    // 识别文法产生式：initVal: expr | T_L_BRACE initVal (T_COMMA initVal)* T_R_BRACE;

    if (ctx->expr()) {
        return visitExpr(ctx->expr());
    } else if (ctx->T_L_BRACE()) {

        ast_node * multiple_val_node = create_contain_node(ast_operator_type::AST_OP_ARRAY_INIT);

        for (auto valCtx: ctx->initVal()) {
            ast_node * val_node = std::any_cast<ast_node *>(visitInitVal(valCtx));
            (void) multiple_val_node->insert_son_node(val_node);
        }

        return multiple_val_node;
    }

    return nullptr;
}

/// @brief 非终结符ifStmt的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitIfStmt(MiniCParser::IfStmtContext * ctx)
{
    // 识别文法产生式：ifStmt: T_IF T_L_PAREN cond T_R_PAREN stmt? (T_ELSE stmt)?

    // 首先识别cond条件表达式
    ast_node * condNode = std::any_cast<ast_node *>(visitCond(ctx->cond()));

    // 然后识别条件为真的语句块
    // 现在可以正确接受nullptr了
    ast_node * thenNode = std::any_cast<ast_node *>(visitStmt(ctx->stmt(0)));

    // 最后识别可选的else语句块
    ast_node * elseNode = nullptr;
	// 检查是否有else
    if (ctx->T_ELSE() != nullptr) {
        // 再检查ctx是否有多个stmt
        if (ctx->stmt().size() > 1 && ctx->stmt(1)) {
            elseNode = std::any_cast<ast_node *>(visitStmt(ctx->stmt(1)));
		}
    }

    // 创建AST节点
    ast_node * ifNode = create_contain_node(ast_operator_type::AST_OP_IF,
                                            condNode,
                                            thenNode,
                                            elseNode);

    return ifNode;
}

/// @brief 非终结符whileStmt的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitWhileStmt(MiniCParser::WhileStmtContext * ctx)
{
    // 识别文法产生式：whileStmt: T_WHILE T_L_PAREN cond T_R_PAREN stmt

    // 首先识别cond条件表达式
    ast_node * condNode = std::any_cast<ast_node *>(visitCond(ctx->cond()));

    // 然后识别循环体
    ast_node * loopNode = std::any_cast<ast_node *>(visitStmt(ctx->stmt()));

    // 创建AST节点
    ast_node * whileNode = create_contain_node(ast_operator_type::AST_OP_WHILE,
                                               condNode,
                                               loopNode);

    return whileNode;
}

/// @brief 非终结符breakStmt的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitBreakStmt(MiniCParser::BreakStmtContext * ctx)
{
	// 识别文法产生式：breakStmt : T_BREAK T_SEMICOLON

    return create_contain_node(ast_operator_type::AST_OP_BREAK);
}

/// @brief 非终结符continueStmt的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitContinueStmt(MiniCParser::ContinueStmtContext * ctx)
{
    // 识别文法产生式：continueStmt: T_CONTINUE T_SEMICOLON

    return create_contain_node(ast_operator_type::AST_OP_CONTINUE);
}

/// @brief 非终结符unaryExp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitUnaryExp(MiniCParser::UnaryExpContext * ctx)
{
    // 识别文法产生式：unaryExp: primaryExp | T_ID T_L_PAREN realParamList? T_R_PAREN | unaryOp unaryExp;

    if (ctx->primaryExp()) {
        // 普通表达式
        return visitPrimaryExp(ctx->primaryExp());
    } else if (ctx->T_ID()) {

        // 创建函数调用名终结符节点
        ast_node * funcname_node = ast_node::New(ctx->T_ID()->getText(), (int64_t) ctx->T_ID()->getSymbol()->getLine());

        // 实参列表
        ast_node * paramListNode = nullptr;

        // 函数调用
        if (ctx->realParamList()) {
            // 有参数
            paramListNode = std::any_cast<ast_node *>(visitRealParamList(ctx->realParamList()));
        }

        // 创建函数调用节点，其孩子为被调用函数名和实参，
        return create_func_call(funcname_node, paramListNode);
    } else if (ctx->unaryOp()) {
		// 识别到求负或逻辑非运算
        auto operandNode = std::any_cast<ast_node *>(visitUnaryExp(ctx->unaryExp()));

        if (!operandNode) {
            return std::any{nullptr};
        }

        ast_operator_type op = std::any_cast<ast_operator_type>(visitUnaryOp(ctx->unaryOp()));

        ast_node * unaryNode = ast_node::New(op, operandNode, nullptr);

        return unaryNode;
	} else {
        return nullptr;
    }
}

/// @brief 非终结符unaryOp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitUnaryOp(MiniCParser::UnaryOpContext * ctx)
{
    if (ctx->T_NOT()) {
        return ast_operator_type::AST_OP_NOT;
	} else if (ctx->T_SUB()) {
		return ast_operator_type::AST_OP_SUB;
    } else {
        return std::any{nullptr};
	}
}

/// @brief 非终结符PrimaryExp的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitPrimaryExp(MiniCParser::PrimaryExpContext * ctx)
{
    // 识别文法产生式 primaryExp: T_L_PAREN expr T_R_PAREN | T_DIGIT | lVal;

    ast_node * node = nullptr;

    if (ctx->T_DIGIT()) {
        // 无符号整型字面量
        // 识别 primaryExp: T_DIGIT
        uint32_t val = (uint32_t) stoull(ctx->T_DIGIT()->getText(), nullptr, 0);					//将基数设置为0，以便stoull自动识别数据的进制
        int64_t lineNo = (int64_t) ctx->T_DIGIT()->getSymbol()->getLine();
        node = ast_node::New(digit_int_attr{val, lineNo});
    } else if (ctx->lVal()) {
        // 具有左值的表达式
        // 识别 primaryExp: lVal
        node = std::any_cast<ast_node *>(visitLVal(ctx->lVal()));
    } else if (ctx->expr()) {
        // 带有括号的表达式
        // primaryExp: T_L_PAREN expr T_R_PAREN
        node = std::any_cast<ast_node *>(visitExpr(ctx->expr()));
    }

    return node;
}

/// @brief 非终结符LVal的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitLVal(MiniCParser::LValContext * ctx)
{
    // 识别文法产生式：lVal: T_ID (T_L_BRACKET expr T_R_BRACKET)*;
    // 获取ID的名字
    auto varId = ctx->T_ID()->getText();

    // 获取行号
    int64_t lineNo = (int64_t) ctx->T_ID()->getSymbol()->getLine();

    ast_node * base_node = ast_node::New(varId, lineNo);
    ast_node * id_node = base_node;

    // 向上生长变量引用树
    for (auto indexCtx: ctx->expr()) {
        ast_node * expr_node = std::any_cast<ast_node *>(visitExpr(indexCtx));

        ast_node * new_index_node = nullptr;
        // 避免index嵌套
        //! 修改重点
        // TODO: 类型继承处理
        // if (base_node->node_type == ast_operator_type::AST_OP_ARRAY_INDEX) {
        // 	new_index_node = create_contain_node(ast_operator_type::AST_OP_ARRAY_INDEX, base_node, expr_node, nullptr);
        // } else if (base_node->node_type != ast_operator_type::AST_OP_ARRAY_INDEX &&
        //            expr_node->node_type != ast_operator_type::AST_OP_ARRAY_INDEX){
        //     new_index_node = create_contain_node(ast_operator_type::AST_OP_ARRAY_INDEX, id_node, expr_node, nullptr);
        // } else if (base_node->node_type != ast_operator_type::AST_OP_ARRAY_INDEX &&
        //            expr_node->node_type == ast_operator_type::AST_OP_ARRAY_INDEX){
        //     new_index_node = expr_node;
        // }
        if (base_node->node_type == ast_operator_type::AST_OP_ARRAY_INDEX) {
			new_index_node = create_contain_node(ast_operator_type::AST_OP_ARRAY_INDEX, base_node, expr_node, nullptr);
        } else{
            new_index_node = create_contain_node(ast_operator_type::AST_OP_ARRAY_INDEX, id_node, expr_node, nullptr);
        }

        base_node = new_index_node;
	}

    return base_node;
}

/// @brief 非终结符VarDecl的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitVarDecl(MiniCParser::VarDeclContext * ctx)
{
    // varDecl: basicType varDef (T_COMMA varDef)* T_SEMICOLON;

    // 声明语句节点
    ast_node * stmt_node = create_contain_node(ast_operator_type::AST_OP_DECL_STMT);

    // 类型节点
    type_attr typeAttr = std::any_cast<type_attr>(visitBasicType(ctx->basicType()));

    for (auto & varCtx: ctx->varDef()) {

        VarDefInfo info = std::any_cast<VarDefInfo>(visitVarDef(varCtx));

        // 变量名节点
        ast_node * id_node = info.id_node;
        // 创建类型节点   			//! 怎么能放循环外面，这就成共享节点了
		ast_node * type_node = create_type_node(typeAttr);

        Type * baseType = type_node->type;
        const Type * completeType = baseType;

        if (!info.dim_nodes.empty()) {
            for (auto it = info.dim_nodes.rbegin(); it != info.dim_nodes.rend(); ++it) {
                ast_node * dim_node = *it;
                ast_node * expr_node = dim_node->sons[0];
                std::optional<uint32_t> size_opt = calculate_const_dim_size(expr_node);

                if (size_opt) {
                    uint32_t numElements = *size_opt;
                    if (numElements) {
                        completeType = ArrayType::get(const_cast<Type *>(completeType), numElements);
                    }
					//? 优化：常量折叠
                    ast_node * literal_node = ast_node::New(digit_int_attr{numElements, id_node->line_no});
                    dim_node->sons[0] = literal_node;
                    ast_node::Delete(expr_node);
				}
			}
        }

        // 创建变量定义节点
        ast_node * decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL);
        decl_node->type = const_cast<Type *>(completeType);
        (void) decl_node->insert_son_node(type_node);
        (void) decl_node->insert_son_node(id_node);

        // 插入数组维度
        for (auto dim_node: info.dim_nodes) {
            (void) decl_node->insert_son_node(dim_node);
        }

        // 插入初值
        if (info.init_node != nullptr) {
            (void) decl_node->insert_son_node(info.init_node);
		}

        // 插入到变量声明语句
        (void) stmt_node->insert_son_node(decl_node);
    }

    return stmt_node;
}

/// @brief 非终结符VarDecl的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitVarDef(MiniCParser::VarDefContext * ctx)
{
    // varDef: T_ID (T_L_BRACKET expr T_R_BRACKET)* (T_ASSIGN initVal)?;
    //! 这里修改方法，返回结构体，保存相关信息

    VarDefInfo info;

    auto varId = ctx->T_ID()->getText();

    // 获取行号
    int64_t lineNo = (int64_t) ctx->T_ID()->getSymbol()->getLine();

    info.id_node = ast_node::New(varId, lineNo);

    for (auto indexCtx: ctx->expr()) {
        ast_node * index_node = create_contain_node(ast_operator_type::AST_OP_ARRAY_DIM);
        ast_node * expr_node = std::any_cast<ast_node *>(visitExpr(indexCtx));

        (void) index_node->insert_son_node(expr_node);
        info.dim_nodes.push_back(index_node);
	}

    if (ctx->T_ASSIGN()) {
        info.init_node = std::any_cast<ast_node *>(visitInitVal(ctx->initVal()));
	}
    return info;
}

/// @brief 非终结符BasicType的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitBasicType(MiniCParser::BasicTypeContext * ctx)
{
    // basicType: T_INT;
    type_attr attr{BasicType::TYPE_VOID, -1};
    if (ctx->T_INT()) {
        attr.type = BasicType::TYPE_INT;
        attr.lineno = (int64_t) ctx->T_INT()->getSymbol()->getLine();
    }

    return attr;
}

/// @brief 非终结符RealParamList的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitRealParamList(MiniCParser::RealParamListContext * ctx)
{
    // 识别的文法产生式：realParamList : expr (T_COMMA expr)*;

    auto paramListNode = create_contain_node(ast_operator_type::AST_OP_FUNC_REAL_PARAMS);

    for (auto paramCtx: ctx->expr()) {

        auto paramNode = std::any_cast<ast_node *>(visitExpr(paramCtx));

        paramListNode->insert_son_node(paramNode);
    }

    return paramListNode;
}

/// @brief 非终结符ExpressionStatement的分析
/// @param ctx CST上下文
std::any MiniCCSTVisitor::visitExpressionStatement(MiniCParser::ExpressionStatementContext * ctx)
{
    // 识别文法产生式  expr ? T_SEMICOLON #expressionStatement;
    if (ctx->expr()) {
        // 表达式语句

        // 遍历expr非终结符，创建表达式节点后返回
        return visitExpr(ctx->expr());
    } else {
        // 空语句

        // 直接返回空指针，需要再把语句加入到语句块时要注意判断，空语句不要加入
        //! 用ast_node包裹nullptr，避免出现bad_cast
        return static_cast<ast_node *>(nullptr);
    }
}

std::optional<uint32_t> MiniCCSTVisitor::calculate_const_dim_size(ast_node * node)
{
    // 基本情况：如果节点为空，则无法计算
    if (!node) {
        return std::nullopt;
    }

    // 根据节点类型进行分情况处理
    switch (node->node_type) {
        // 递归的成功基石：节点本身就是一个无符号整数字面量
        case ast_operator_type::AST_OP_LEAF_LITERAL_UINT: {
            return node->integer_val;
        }

        // 递归步骤：对于二元运算符，先递归计算左右子树，再合并结果
        case ast_operator_type::AST_OP_ADD:
        case ast_operator_type::AST_OP_SUB:
        case ast_operator_type::AST_OP_MUL:
        case ast_operator_type::AST_OP_DIV:
        case ast_operator_type::AST_OP_MOD: {
            // 确保有两个子节点
            if (node->sons.size() != 2) {
                return std::nullopt;
            }

            // 递归计算左、右子节点
            auto left_opt = calculate_const_dim_size(node->sons[0]);
            auto right_opt = calculate_const_dim_size(node->sons[1]);

            // 只有当左右子节点都成功计算出常量值时，才继续
            if (left_opt && right_opt) {
                uint32_t left_val = *left_opt;  // 从optional中取出值
                uint32_t right_val = *right_opt;

                // 根据具体运算符进行计算
                switch (node->node_type) {
                    case ast_operator_type::AST_OP_ADD: return left_val + right_val;
                    case ast_operator_type::AST_OP_SUB: return left_val - right_val;
                    case ast_operator_type::AST_OP_MUL: return left_val * right_val;
                    case ast_operator_type::AST_OP_DIV:
                        if (right_val == 0) return std::nullopt; // 错误：除以0
                        return left_val / right_val;
                    case ast_operator_type::AST_OP_MOD:
                         if (right_val == 0) return std::nullopt; // 错误：对0取模
                        return left_val % right_val;
                    default: return std::nullopt; // 不应到达这里
                }
            } else {
                // 如果任一子节点不是常量，则整个表达式都不是常量
                return std::nullopt;
            }
        }

        // 失败情况：如果节点是变量、函数调用等其他任何类型，都认为它不是编译时常量
        // case ast_operator_type::AST_OP_LEAF_VAR_ID:
        // case ast_operator_type::AST_OP_FUNC_CALL:
        default: {
            return std::nullopt;
        }
    }
}