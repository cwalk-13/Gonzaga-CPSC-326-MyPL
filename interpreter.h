//----------------------------------------------------------------------
// NAME: Charles Walker
// FILE: interpreter.h
// DATE: 4/15/2021
// DESC: interpreter for MYPL 
//----------------------------------------------------------------------


#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <iostream>
#include <unordered_map>
#include <regex>
#include "ast.h"
#include "symbol_table.h"
#include "data_object.h"
#include "heap.h"


class Interpreter : public Visitor
{
public:

  // top-level
  void visit(Program& node);
  void visit(FunDecl& node);
  void visit(Repl& node);
  void visit(TypeDecl& node);
  // statements
  void visit(ReplEndpoint& node);
  void visit(VarDeclStmt& node);
  void visit(AssignStmt& node);
  void visit(ReturnStmt& node);
  void visit(IfStmt& node);
  void visit(WhileStmt& node);
  void visit(ForStmt& node);
  // expressions
  void visit(Expr& node);
  void visit(SimpleTerm& node);
  void visit(ComplexTerm& node);
  // rvalues
  void visit(SimpleRValue& node);
  void visit(NewRValue& node);
  void visit(CallExpr& node);
  void visit(IDRValue& node);
  void visit(NegatedRValue& node);

  // return code from calling main
  int return_code() const;

  
private:

  // return exception
  class MyPLReturnException : public std::exception {};
  
  // the symbol table 
  SymbolTable sym_table;

  // holds the previously computed value
  DataObject curr_val;

  // the heap
  Heap heap;

  // the next oid
  size_t next_oid = 0;
  
  // the functions (all within the global environment)
  std::unordered_map<std::string,FunDecl*> functions;
  
  // the user-defined types (all within the global environment)
  std::unordered_map<std::string,TypeDecl*> types;

  // the global environment id
  int global_env_id = 0;
  
  // the program return code
  int ret_code = 0;

  // error message
  void error(const std::string& msg, const Token& token);
  void error(const std::string& msg); 
};



int Interpreter::return_code() const
{
  return ret_code;
}

void Interpreter::error(const std::string& msg, const Token& token)
{
  throw MyPLException(RUNTIME, msg, token.line(), token.column());
}


void Interpreter::error(const std::string& msg)
{
  throw MyPLException(RUNTIME, msg);
}


//----------------------------------------------------------------------
// Function, Variable, and Type Declarations
//----------------------------------------------------------------------
void Interpreter::visit(Repl& node)
{
  sym_table.push_environment();
  global_env_id = sym_table.get_environment_id();
  std::list<Stmt*> stmts = node.stmts;
  while (stmts.size() != 0)
  {
    stmts.front() -> accept(*this);
    stmts.pop_front();
  }
  sym_table.pop_environment();
}

void Interpreter::visit(Program& node)
{
  sym_table.push_environment();
  global_env_id = sym_table.get_environment_id();
  for (Decl * d: node.decls)
    d -> accept(*this);
  CallExpr expr;
  expr.function_id = functions["main"] -> id;
  expr.accept(*this);
  sym_table.pop_environment();
}

void Interpreter::visit(FunDecl& node)
{
  FunDecl * temp = new FunDecl;
  *temp = node;
  functions[node.id.lexeme()] = temp;
  temp = nullptr;
}

void Interpreter::visit(TypeDecl& node)
{
  TypeDecl * temp;
  *temp = node;
  types[node.id.lexeme()] = temp;
  temp = nullptr;
}

void Interpreter::visit(ReplEndpoint& node)
{
  // Repl* temp = new Repl;
  // *temp = node;
  // functions[node.id.lexeme()] = temp;
  // temp = nullptr;
}

void Interpreter::visit(VarDeclStmt& node)
{
  node.expr -> accept(*this);
  //int case
  if (curr_val.is_integer())
  {
    int v;
    curr_val.value(v);
    DataObject obj(v);

    sym_table.add_name(node.id.lexeme());
    sym_table.set_val_info(node.id.lexeme(), obj);
  }
  //double case
  else if (curr_val.is_double())
  {
    double v;
    curr_val.value(v);
    DataObject obj(v);
    sym_table.add_name(node.id.lexeme());
    sym_table.set_val_info(node.id.lexeme(), obj);
  }
  //string case
  else if (curr_val.is_string())
  {
    std::string v;
    curr_val.value(v);
    DataObject obj(v);
    sym_table.add_name(node.id.lexeme());
    sym_table.set_val_info(node.id.lexeme(), obj);
  }
  //char case
  else if (curr_val.is_char())
  {
    char v;
    curr_val.value(v);
    DataObject obj(v);
    sym_table.add_name(node.id.lexeme());
    sym_table.set_val_info(node.id.lexeme(), obj);
  }
  //bool case 
  else if (curr_val.is_bool())
  {
    bool v;
    curr_val.value(v);
    DataObject obj(v);
    sym_table.add_name(node.id.lexeme());
    sym_table.set_val_info(node.id.lexeme(), obj);
  }
}

void Interpreter::visit(AssignStmt& node)
{
  node.expr -> accept(*this);
  if (node.lvalue_list.size() > 1) // UDT attribute
  {
    std::list<Token> tokens = node.lvalue_list;
    DataObject info;
    if (sym_table.has_val_info(tokens.front().lexeme())) {
      // info isan oid
      sym_table.get_val_info(tokens.front().lexeme(), info);
    }
    tokens.pop_front();
    while (tokens.size() > 1)
    {
      HeapObject h_obj;
      size_t oid;
      info.value(oid) ;
      heap.get_obj(oid, h_obj);
      if (h_obj.has_att(tokens.front() .lexeme())) {
        h_obj.get_val(tokens.front().lexeme(), info);
      }
      tokens.pop_front();
    }

    HeapObject h_obj;
    size_t oid;
    info.value(oid);
    heap.get_obj(oid, h_obj);
    if (h_obj.has_att(tokens.front().lexeme()))
      h_obj.set_att(tokens.front().lexeme(), curr_val);
  }
  else
    sym_table.set_val_info(node.lvalue_list.front().lexeme(), curr_val);
}

void Interpreter::visit(ReturnStmt& node)
{
    node.expr -> accept(*this);
    std::string str = curr_val.to_string();
    str = std::regex_replace(str, std::regex("\\\\n"), "\n");
    str = std::regex_replace(str, std::regex("\\\\t"), "\t");
    std::cout <<">>>" << str << "\n";
    // throw new MyPLReturnException;
}

void Interpreter::visit(IfStmt& node)
{
  node.if_part -> expr -> accept(*this) ;
  bool entered = false;
  bool v;
  curr_val.value(v);
  if (v == true)
  {
    
    sym_table.push_environment();
    std::list<Stmt*> stmts = node.if_part -> stmts;
    while (stmts.size() != 0)
    {
      stmts.front() -> accept(*this);
      stmts.pop_front();
    }
    entered = true;
    sym_table.pop_environment();
  }
  // else ifs
  else if (node.else_ifs.size() > 0)
  {
    std::list<BasicIf*> elseif_list = node.else_ifs;
    while (entered == false & elseif_list.size() != 0) 
    { 
      BasicIf* if_stmt = elseif_list.front();
      if_stmt -> expr -> accept(*this);
      curr_val.value(v);
      if (v == true)
      {
        sym_table.push_environment();
        std::list<Stmt*> stmts = if_stmt -> stmts;
        while (stmts.size() != 0)
        {
          stmts.front() -> accept(*this);
          stmts.pop_front();
        }
        entered = true;
        sym_table.pop_environment();
      }
    }
  }
  // else part
  if (entered == false && node.body_stmts.size() != 0)
  {
    sym_table.push_environment();
    std::list<Stmt*> stmts = node.body_stmts;
    while (stmts.size() != 9)
    {
      stmts.front() -> accept(*this);
      stmts.pop_front();
    }
    sym_table.pop_environment();
  }
}

void Interpreter::visit(WhileStmt& node)
{
  node.expr -> accept(*this);
  bool v;
  curr_val.value(v);
  sym_table.push_environment();
  while (v == true)
  {
    std::list<Stmt*> stmts = node.stmts;
    while (stmts.size() != 0)
    {
      stmts.front() -> accept(*this);
      stmts.pop_front();
    }
    node.expr -> accept(*this);
    curr_val.value(v);
  }
  sym_table.pop_environment();
}

void Interpreter::visit(ForStmt& node)
{
  sym_table.push_environment();
  node.start -> accept(*this);
  int num;
  curr_val.value(num);
  int start_val = num;
  DataObject obj(curr_val);

  sym_table.add_name(node.var_id.lexeme());
  sym_table.set_val_info(node.var_id.lexeme(), obj);
  node.end -> accept(*this);

  curr_val.value(num);
  int end_val = num;
  // go through loop
  sym_table.push_environment();
  for (int i = start_val; i <= end_val; ++i)
  {
    std::list<Stmt*> stmts = node.stmts;
    while (stmts.size() != 0)
    {
      stmts.front() -> accept(*this);
      stmts.pop_front();
    }
  }
  sym_table.pop_environment();
  sym_table.pop_environment();
}

void Interpreter::visit(Expr& node)
{
  if (node.negated)
  {
    node.first -> accept(*this);
    bool val;
    curr_val.value(val);
    curr_val.set(!val);
  }
  else
  {
    node.first -> accept(*this);
    if (node.op != nullptr)
    {
      DataObject lhs_val = curr_val;
      node.rest -> accept(*this);
      DataObject rhs_val = curr_val;
      //  Cases for operand
      switch (node.op->type())
      {
        //case for == !=
        case EQUAL: case NOT_EQUAL: case LESS_EQUAL: case GREATER_EQUAL: case LESS: case GREATER:
        {
          if (lhs_val.is_integer())
          {
            int lval;
            int rval;
            lhs_val.value(lval);
            rhs_val.value(rval);
            if(EQUAL) {
              DataObject obj(lval == rval);
              curr_val = obj;
            }
            else if (NOT_EQUAL){
              DataObject obj(lval != rval);
              curr_val = obj;
            }
            else if (LESS_EQUAL){
              DataObject obj(lval <= rval);
              curr_val = obj;
            }
            else if (GREATER_EQUAL){
              DataObject obj(lval >= rval);
              curr_val = obj;
            }
            else if (LESS){
              DataObject obj(lval < rval);
              curr_val = obj;
            }
            else if (GREATER){
              DataObject obj(lval > rval);
              curr_val = obj;
            }
          }
          else if (lhs_val.is_double())
          {
            double lval;
            double rval;
            lhs_val.value(lval);
            rhs_val.value(rval);
            if(EQUAL) {
              DataObject obj(lval == rval);
              curr_val = obj;
            }
            else if (NOT_EQUAL){
              DataObject obj(lval != rval);
              curr_val = obj;
            }
            else if (LESS_EQUAL){
              DataObject obj(lval <= rval);
              curr_val = obj;
            }
            else if (GREATER_EQUAL){
              DataObject obj(lval >= rval);
              curr_val = obj;
            }
            else if (LESS){
              DataObject obj(lval < rval);
              curr_val = obj;
            }
            else if (GREATER){
              DataObject obj(lval > rval);
              curr_val = obj;
            }
          }
          else if (lhs_val.is_bool())
          {
            bool lval;
            bool rval;
            lhs_val.value(lval);
            rhs_val.value(rval);
            if(EQUAL) {
              DataObject obj(lval == rval);
              curr_val = obj;
            }
            else if (NOT_EQUAL){
              DataObject obj(lval != rval);
              curr_val = obj;
            }
            else if (LESS_EQUAL){
              DataObject obj(lval <= rval);
              curr_val = obj;
            }
            else if (GREATER_EQUAL){
              DataObject obj(lval >= rval);
              curr_val = obj;
            }
            else if (LESS){
              DataObject obj(lval < rval);
              curr_val = obj;
            }
            else if (GREATER){
              DataObject obj(lval > rval);
              curr_val = obj;
            }
          }
          else if (lhs_val.is_string())
          {
            std::string lval;
            std::string rval;
            lhs_val.value(lval);
            rhs_val.value(rval);
            if(EQUAL) {
              DataObject obj(lval == rval);
              curr_val = obj;
            }
            else if (NOT_EQUAL){
              DataObject obj(lval != rval);
              curr_val = obj;
            }
            else if (LESS_EQUAL){
              DataObject obj(lval <= rval);
              curr_val = obj;
            }
            else if (GREATER_EQUAL){
              DataObject obj(lval >= rval);
              curr_val = obj;
            }
            else if (LESS){
              DataObject obj(lval < rval);
              curr_val = obj;
            }
            else if (GREATER){
              DataObject obj(lval > rval);
              curr_val = obj;
            }
          }
          else if (lhs_val.is_char())
          {
            char lval;
            char rval;
            lhs_val.value(lval);
            rhs_val.value(rval);
            if(EQUAL) {
              DataObject obj(lval == rval);
              curr_val = obj;
            }
            else if (NOT_EQUAL){
              DataObject obj(lval != rval);
              curr_val = obj;
            }
            else if (LESS_EQUAL){
              DataObject obj(lval <= rval);
              curr_val = obj;
            }
            else if (GREATER_EQUAL){
              DataObject obj(lval >= rval);
              curr_val = obj;
            }
            else if (LESS){
              DataObject obj(lval < rval);
              curr_val = obj;
            }
            else if (GREATER){
              DataObject obj(lval > rval);
              curr_val = obj;
            }
          }
        }
        //mathematical operators
        case PLUS: case MINUS: case MULTIPLY: case DIVIDE: case MODULO:
        {
          if (lhs_val.is_integer())
          {
            int lval;
            int rval;
            lhs_val.value(lval);
            rhs_val.value(rval);
            if(PLUS)
              curr_val.set(lval + rval);
            else if(MINUS)
              curr_val.set(lval - rval);
            else if(MULTIPLY)
              curr_val.set(lval * rval);
            else if(DIVIDE)
              curr_val.set(lval / rval);
            else if(MODULO)
              curr_val.set(lval % rval);
          }
          else if (lhs_val.is_double())
          {
            double lval;
            double rval;
            lhs_val.value(lval);
            rhs_val.value(rval);
            if(PLUS)
              curr_val.set(lval + rval);
            else if(MINUS)
              curr_val.set(lval - rval);
            else if(MULTIPLY)
              curr_val.set(lval * rval);
            else if(DIVIDE)
              curr_val.set(lval / rval);
          }
          
          //addition with strings and chars
          else 
          {
            //char and string -> string
            if (lhs_val.is_char() & rhs_val.is_string())
            {
              char lval;
              std::string rval;
              lhs_val.value(lval);
              rhs_val.value(rval);
              DataObject obj(lval + rval);
              curr_val = obj;
            }
            //string and char -> string
            else if (lhs_val.is_string() && rhs_val.is_char())
            {
              char rval;
              std::string lval;
              lhs_val.value(lval);
              rhs_val.value(rval);
              DataObject obj(lval + rval);
              curr_val = obj;
            }
            //chars -> string
            else if (lhs_val.is_char() && rhs_val.is_char())
            {
              char rval;
              char lval;
              lhs_val.value(lval);
              rhs_val.value(rval);
              DataObject obj(lval + rval);
              curr_val = obj;
            }
            //strings
            else 
            {
              std::string rval;
              std::string lval;
              lhs_val.value(lval);
              rhs_val.value(rval);
              DataObject obj(lval + rval);
              curr_val = obj;
            }
          }
        }
        //case for and or 
        case AND: case OR:
        {
          if(node.op->type() == AND or node.op->type() == OR) {
            bool lval;
            bool rval;
            lhs_val.value(lval);
            rhs_val.value(rval);
            if (AND)
              curr_val.set(lval and rval);
            else if(OR)
              curr_val.set(lval or rval);
          }
        }
      }
    }
  }
  // std::cout << curr_val.to_string() << "\n";
}

void Interpreter::visit(SimpleTerm& node)
{
  node.rvalue -> accept(*this);
}

void Interpreter::visit(ComplexTerm& node)
{
  node.expr -> accept(*this);
}

void Interpreter::visit(SimpleRValue& node)
{
  if (node.value.type() == BOOL_VAL)
  {
    if (node.value.lexeme() == "true")
      curr_val.set(true);
    else if (node.value.lexeme() == "false")
      curr_val.set(false);
  }
  else if (node.value.type() == INT_VAL)
  {
    int val = std::stoi(node.value.lexeme());
    curr_val.set(val);
  }
  else if (node.value.type() == DOUBLE_VAL)
  {
    double val = std::stod(node.value.lexeme());
    curr_val.set(val);
  }
  else if (node.value.type() == CHAR_VAL)
    curr_val.set(node.value.lexeme().at(3));
  else if (node.value.type() == STRING_VAL)
    curr_val.set(node.value.lexeme());
  else
    curr_val.set_nil();
}

void Interpreter::visit (NewRValue& node)
{
  DataObject obj;
  obj.set(next_oid);
  ++next_oid;
  curr_val = obj;
}

void Interpreter::visit(CallExpr& node)
{
  std::string fun_name = node. function_id.lexeme();
  // built-in print function
  if (fun_name == "print")
  {
    node.arg_list.front() -> accept(*this);
    std::string str = curr_val.to_string();
    str = std::regex_replace(str, std::regex("\\\\n"), "\n");
    str = std::regex_replace(str, std::regex("\\\\t"), "\t");
    std::cout << str;
  }
  //built in string to int
  else if (fun_name == "stoi")
  {
    node.arg_list.front() -> accept(*this);
    std::string str;
    curr_val.value(str);
    int val = std::stoi(str);
    DataObject obj(val);
    curr_val = obj;
  }
  //built in string to double
  else if (fun_name == "stod")
  {
    node.arg_list.front() -> accept(*this);
    std::string str;
    curr_val.value(str);
    double val = std::stod(str);
    DataObject obj(val);
    curr_val = obj;
  }
  //built in int to string and double to string
  else if (fun_name == "itos" || fun_name == "dtos")
  {
    node.arg_list.front() -> accept(*this);
    std::string str = curr_val.to_string();
    DataObject obj(str);
    curr_val = obj;
  }
  //built in get
  else if (fun_name == "get")
  {
    std::list<Expr*> expr_list = node.arg_list;
    expr_list.front() -> accept(*this); // first arg is an int
    int index;
    curr_val.value(index);
    expr_list.pop_front();
    expr_list.front() -> accept(*this); // next arg is a string
    std::string str;
    curr_val.value(str);
    DataObject obj(str.at(index)); // char object
    curr_val = obj;
  }
  //built in length
  else if (fun_name == "length")
  {
    node.arg_list.front() -> accept(*this);
    std::string str;
    curr_val.value(str);
    DataObject obj(str.length()); // int object
    curr_val = obj;
  }
  //built in read
  else if (fun_name == "read")
  {
    // no args
    std::string str;
    std::cin >> str;
    DataObject obj(str); // string object
    curr_val = obj;
  }
  //user defined function
  else 
  {
    std::list<Expr*> expr_list = node.arg_list;
    std::list<DataObject> args;
    while (expr_list.size() != 0)
    {
      Expr* e = expr_list.front();
      DataObject obj = curr_val;
      args.push_back(obj);
      expr_list.pop_front();
    }

    int curr_env_id = sym_table.get_environment_id();
    sym_table.set_environment_id(global_env_id);
    sym_table.push_environment();
    FunDecl* fun_node = functions[fun_name];
    for (FunDecl::FunParam param : fun_node -> params)
    {
      sym_table.add_name(param.id.lexeme());
      sym_table.set_val_info(param.id.lexeme(), args.front());
      args.pop_front();
    }
    try {
      for (Stmt* stmt : fun_node -> stmts)
        stmt -> accept(*this);
    }
    catch (MyPLReturnException* e) {
      // return stmt found
    }
    sym_table.pop_environment();
    sym_table.set_environment_id(curr_env_id);
  }
}

void Interpreter::visit(IDRValue& node)
{
  if (node.path.size() > 1)
  {
  std::list<Token> path_list = node.path;
  DataObject info;
  if (sym_table.has_val_info(path_list.front().lexeme())){
    sym_table.get_val_info(path_list.front().lexeme(), info);
  }

  path_list.pop_front();
  while (path_list.size() > 1)
  {
    HeapObject h_obj;
    size_t oid;
    info.value(oid);
    heap.get_obj(oid, h_obj);
    if (h_obj.has_att(path_list.front().lexeme())) {
      //info is oid
      h_obj.get_val(path_list.front().lexeme(), info);
    }
    path_list.pop_front();
  }
    HeapObject h_obj;
    size_t oid;
    info.value(oid);
    heap.get_obj(oid, h_obj);
    if (h_obj.has_att(path_list.front().lexeme()))
    {
      DataObject value_info;
      h_obj.get_val(path_list.front().lexeme(), value_info);
      curr_val = value_info;
    }
  }
  else
  {
    if (sym_table.has_val_info(node.path.front().lexeme()))
    {
      DataObject info;
      sym_table.get_val_info(node.path.front().lexeme(), info);
      curr_val = info;
    }
  }
}

void Interpreter::visit(NegatedRValue& node)
{
  node.expr -> accept(*this);
  if (curr_val.is_double())
  {
    double val;
    curr_val.value(val);
    curr_val.set(-1.0 * val);
  }
  else
  {
    int val;
    curr_val.value(val);
    curr_val.set(-1 * val);
  }
}

#endif
