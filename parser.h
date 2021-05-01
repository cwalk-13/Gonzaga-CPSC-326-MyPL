//----------------------------------------------------------------------
// NAME: Charles Walker
// FILE: parser.h
// DATE: 2/14/2021
// DESC: implimentation of the parser for myPl
//----------------------------------------------------------------------

#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "mypl_exception.h"
#include "ast.h"

class Parser
{
public:

  // create a new recursive descent parser
  Parser(const Lexer& program_lexer);

  // run the parser
  void parse(Program& node);
  void parse(Repl& node);
private:
  Lexer lexer;
  Token curr_token;
  bool re_found = false;
  // helper functions
  void advance();
  void eat(TokenType t, std::string err_msg);
  void error(std::string err_msg);
  bool is_operator(TokenType t);
  void dtype();
  // top-level
  void tdecl(TypeDecl& node);
  void fdecl(FunDecl& node);
  //expressions
  void expr(Expr& node);
  void simple_term(SimpleTerm& node);
  void simple_rvalue(SimpleRValue& node);
  void new_rvalue(NewRValue& node);
  void neg_rvalue(NegatedRValue& node);
  //statements
  void repl_endpoint(ReplEndpoint& node);
  void stmt(std::list<Stmt*>& stmts, bool in_repl = false);
  void vdecl_stmt(VarDeclStmt& node);
  void assign_stmt(AssignStmt& node);
  void return_stmt(ReturnStmt& node);
  void while_stmt(WhileStmt& node);
  void for_stmt(ForStmt& node);
  void if_stmt(IfStmt& node);
};


// constructor
Parser::Parser(const Lexer& program_lexer) : lexer(program_lexer)
{
}


// Helper functions

void Parser::advance()
{
  curr_token = lexer.next_token();
}


void Parser::eat(TokenType t, std::string err_msg)
{
  if (curr_token.type() == t)
    advance();
  else
    error(err_msg);
}


void Parser::error(std::string err_msg)
{
  std::string s = err_msg + "found '" + curr_token.lexeme() + "'";
  int line = curr_token.line();
  int col = curr_token.column();
  throw MyPLException(SYNTAX, s, line, col);
}


bool Parser::is_operator(TokenType t)
{
  return t == PLUS or t == MINUS or t == DIVIDE or t == MULTIPLY or
    t == MODULO or t == AND or t == OR or t == EQUAL or t == LESS or
    t == GREATER or t == LESS_EQUAL or t == GREATER_EQUAL or t == NOT_EQUAL;
}


//----------------------------------------------------------------------
// Function, Variable, and Type Declarations
//----------------------------------------------------------------------
//Repl session
void Parser::parse(Repl& node)
{
  advance();
  // std::cout << "in Repl \n";
  while (re_found == false)
  {
    stmt(node.stmts, true);
  }
  re_found = false;

  if(curr_token.type() == EOS) {
    eat(EOS, "Expecting end-of-file ");
  }
}

void Parser::parse(Program& node)
{
  advance();
  while (curr_token.type() != EOS)
  {
    if (curr_token.type() == TYPE)
    {
      //Type declaration
      TypeDecl* t = new TypeDecl();
      tdecl(*t);
      node.decls.push_back(t);
    }

    else if (curr_token.type() == FUN)
    {
      //Function declaration
      FunDecl* f = new FunDecl();
      fdecl(*f);
      node.decls.push_back(f);
    }

    else
    {
      error("Expecting Decl");
    }
  }
  eat(EOS, "Expecting end-of-file ");
}

//Function Declaration
void Parser::fdecl(FunDecl& node)
{
  eat(FUN, "expecting FUN ");
  node.return_type = curr_token;
  if (curr_token.type() == NIL)
    eat(NIL, "expecting nil ");
  else 
    dtype();
  
  node.id = curr_token;
  eat(ID, "expecting ID ");
  eat(LPAREN, "expecting lparen ");

  while(curr_token.type() != RPAREN)
  {
    FunDecl::FunParam f;
    f.id = curr_token;
    advance();
    eat(COLON, "expecting colon ");
    f.type = curr_token;
    dtype();
    node.params.push_back(f);

    if (curr_token.type() == COMMA) {
      eat(COMMA, "expecting comma ");
    }
  }
  eat(RPAREN, "expecting rparen");

  while (curr_token.type() != END)
  {
    stmt(node.stmts);
  }
  eat(END, "expecting end");
}

//Type declaration
void Parser::tdecl(TypeDecl& node)
{
  eat(TYPE, "expecting type ");
  node.id = curr_token;
  eat(ID, "expecting id ");

  while (curr_token.type() != END)
  {
    VarDeclStmt* v = new VarDeclStmt();
    vdecl_stmt(*v);
    node.vdecls.push_back(v);
  }
  eat(END, "expecteing end ");
}

//----------------------------------------------------------------------
// Statement nodes
//----------------------------------------------------------------------

//helper function for stmts 
void Parser::stmt(std::list<Stmt*>& stmts, bool in_repl)
{
  // check the first terminal of every non-terminal options
  //assign and call_expr
  
  if (curr_token.type() == VAR) 
  {
    VarDeclStmt* v = new VarDeclStmt();
    vdecl_stmt(*v);
    stmts.push_back(v);
  }

  else if (curr_token.type() == ID)
  {
    // call expression case
    Token id = curr_token;
    eat(ID,"expecting ID ");
    if (curr_token.type() == LPAREN)
    {
      eat(LPAREN, "Expected LPAREN ");
      CallExpr* c = new CallExpr();
      c->function_id = id;

      //check for idrval

      while (curr_token.type() != RPAREN)
      {
        Expr* e = new Expr();
        expr(*e);
        c->arg_list.push_back(e);
        if (curr_token.type() == COMMA) 
          eat(COMMA, "expecting comma ");
      }
      eat(RPAREN, "Expected RPAREN ");
      stmts.push_back(c);
    }
    else 
    {
      // variable assignment case
      AssignStmt* a = new AssignStmt();
      a->lvalue_list.push_back(id);

      while (curr_token.type() != ASSIGN)
      {
        eat(DOT, "Expected DOT ");
        a->lvalue_list.push_back(curr_token);
        eat(ID, "Expected ID ");
      }

      eat(ASSIGN, "Expected ASSIGN ");
      Expr* e = new Expr();
      expr(*e);
      a->expr = e;
      stmts.push_back(a);
    }
  }
  else if (curr_token.type() == IF) {
    IfStmt* i = new IfStmt();
    if_stmt(*i);
    stmts.push_back(i);
  }
  else if (curr_token.type() == WHILE) {
    WhileStmt* w = new WhileStmt();
    while_stmt(*w);
    stmts.push_back(w);
  }
  else if (curr_token.type() == FOR) {
    ForStmt* f = new ForStmt();
    for_stmt(*f);
    stmts.push_back(f);
  }
  else if (curr_token.type() == RETURN) {
    ReturnStmt* r = new ReturnStmt();
    return_stmt(*r);
    stmts.push_back(r);
  }
  else if (in_repl) {
    ReplEndpoint* re = new ReplEndpoint();
    repl_endpoint(*re);
    stmts.push_back(re);
  }
  else 
    error("unexpected token ");
}


//repl endpoint
void Parser::repl_endpoint(ReplEndpoint& node)
{
  std::cout << "endpoint found \n";
  Expr* e = new Expr();
  expr(*e);
  node.expr = e;
  re_found = true;
}



//vardecl stmt node
void Parser::vdecl_stmt(VarDeclStmt& node)
{
  eat(VAR, "expecting var ");

  node.id = curr_token;

  eat(ID, "expecting ID ");

  if (curr_token.type() == COLON) 
  {
    eat(COLON, "expecting colon ");
    Token* id = new Token();
    *id = curr_token;
    node.type = id;
    advance();
  }

  eat(ASSIGN, "expecting assign ");
  Expr* e = new Expr();
  expr(*e);
  node.expr = e;
}

//assignmentstmt node
void Parser::assign_stmt(AssignStmt& node)
{
  node.lvalue_list.push_back(curr_token);
  eat(ID, "expecting id ");

  while (curr_token.type() != ASSIGN)
  {
    eat(DOT, "expecting dot ");
    node.lvalue_list.push_back(curr_token);
    eat(ID, "expecting id ");
  }

  eat(ASSIGN, "expecting assign ");
  Expr* e = new Expr();
  expr(*e);
  node.expr = e;
}

//returnstmt node
void Parser::return_stmt(ReturnStmt& node)
{
  eat(RETURN, "expecting return ");
  Expr* e = new Expr();
  expr(*e);
  node.expr = e;
}

//ifstmt node
void Parser::if_stmt(IfStmt& node)
{
  BasicIf* b = new BasicIf();

  std::list<Stmt*> b_stmt_list;
  std::list<BasicIf*> elif_stmt_list;
  std::list<Stmt*> else_stmt_list;

  eat(IF, "expecting if ");
  Expr* e = new Expr();
  expr(*e);
  b->expr = e;
  eat(THEN, "expcting then ");
  // add stmts to basicif
  while (curr_token.type() != END && curr_token.type() != ELSEIF && curr_token.type() != ELSE)
  {
    stmt(b->stmts);
  }

  if (curr_token.type() == ELSEIF)
  {
    //  Read through all ELSEIF stmts
    while (curr_token.type() != ELSE && curr_token.type() != END)
    {
      // else if case
      BasicIf* b2 = new BasicIf();
      //stmt list for new Basic If stmts
      std::list<Stmt*> b2_stmt_list;
      eat(ELSEIF, "expecting elseif ");
      Expr* eb2 = new Expr();
      expr(*eb2); 
      b2->expr = eb2;
      eat(THEN, "expecting then ");
      while (curr_token.type() != END && curr_token.type() != ELSE && curr_token.type() != ELSEIF)
      {
        stmt(b2_stmt_list);
      }
      b2->stmts = b2_stmt_list;
      elif_stmt_list.push_back(b2);
    }
  }

  if (curr_token.type() == ELSE)
  {
    eat(ELSE, "expecting else ");

    while (curr_token.type() != END)
      stmt(else_stmt_list);
  }

  eat(END, "expecting end ");

  node.if_part = b;
  node.else_ifs = elif_stmt_list;
  node.body_stmts = else_stmt_list;
}

//whilestmt node
void Parser::while_stmt(WhileStmt& node)
{
  eat(WHILE, "expecting while ");
  Expr* e = new Expr();
  expr(*e);
  node.expr = e;
  eat(DO, "expecting do ");
  //new list for stmts made in the while loop
  std::list<Stmt*> stmt_list;

  while (curr_token.type() != END) 
  {
    stmt(stmt_list);
  }
  node.stmts = stmt_list;
  eat(END, "expecting end ");
}

//forstmt node
void Parser::for_stmt(ForStmt& node)
{
  eat(FOR, "expecting for");
  node.var_id = curr_token;
  eat(ID, "expecting id");
  eat(ASSIGN, "expecting assign");
  Expr* start_e = new Expr();
  expr(*start_e);
  node.start = start_e;
  eat(TO, "expecting to");
  Expr* e = new Expr();
  expr(*e);
  node.end = e;
  eat(DO, "expecting do");

  std::list<Stmt*> stmt_list;
  //add stmts to for loop
  while (curr_token.type() != END)
  {
    stmt(stmt_list);
  }
  eat(END, "expecting end");

}

//----------------------------------------------------------------------
// Expression and Expression Terms
//----------------------------------------------------------------------

//expression node
void Parser::expr(Expr& node)
{
  //negated case
  if (curr_token.type() == NEG)
  {
    eat(NEG, "expecting neg ");
    node.negated = true;
  }

  //complex term
  if (curr_token.type() == NOT)
  {
    ComplexTerm* c = new ComplexTerm();
    eat(NOT, "expecting not ");
    node.negated = true;
    Expr* e = new Expr();
    expr(*e);
    c->expr = e;
    node.first = c;
  }
  else if (curr_token.type() == LPAREN)
  {
    //  LPAREN implies a complex stmt
    
    eat(LPAREN, "expecting lparen ");
    ComplexTerm* c = new ComplexTerm();
    Expr* e = new Expr();
    expr(*e);
    c->expr = e;
    node.first = c;
    if (curr_token.type() == RPAREN)
      eat(RPAREN, "expecting rparen ");
  }
  else
  {
    //simple statement
    SimpleTerm* s = new SimpleTerm();
    simple_term(*s);
    node.first = s;
  }

  //optional operator 
  if (is_operator(curr_token.type()))
  {
    Token* new_op = new Token;
    *new_op = curr_token;
    node.op = new_op;
    advance();
    Expr* e = new Expr();
    expr(*e);
    node.rest = e;

    //keep here just in case
    // if (curr_token.type() == RPAREN) {
    //   eat(RPAREN, "Expected RPAREN ");
    // }
  }
}

//simple term node
void Parser::simple_term(SimpleTerm& node)
{
  if (curr_token.type() == INT_VAL || curr_token.type() == DOUBLE_VAL
      || curr_token.type() == BOOL_VAL || curr_token.type() == CHAR_VAL
      || curr_token.type() == STRING_VAL || curr_token.type() == NIL)
  {
    //  Simple RValue case
    SimpleRValue* sr = new SimpleRValue();
    // simple_rvalue(*sr);
    sr->value = curr_token;
    advance();
    node.rvalue = sr;
  }

  else if (curr_token.type() == NEW)
  {
    //  NewRValue Case
    NewRValue* n = new NewRValue();
    eat(NEW, "Expected NEW ");
    // n->type_id = curr_token;
    new_rvalue(*n);
    node.rvalue = n;
  }
  
  else if (curr_token.type() == NEG)
  {
    //  NegatedRValue Case
    NegatedRValue* n = new NegatedRValue();
    Expr* ne = new Expr();
    expr(*ne);
    n->expr = ne;
    node.rvalue = n;
  }

  else if (curr_token.type() == ID)
  {
    //  Determine if this should create an IDR val or CallExpr
    
    Token new_id = curr_token;
    eat(ID, "Expected ID ");

    if (curr_token.type() == LPAREN)
    {
      //  CallExpr case
      eat(LPAREN, "Expected LPAREN ");
      CallExpr* c = new CallExpr();
      c->function_id = new_id;
      while (curr_token.type() != RPAREN)
      {
        Expr* e = new Expr();
        expr(*e);
        c->arg_list.push_back(e);
        if (curr_token.type() == COMMA) 
          eat(COMMA, "expecting comma ");
      }

      eat(RPAREN, "Expected RPAREN ");
      node.rvalue = c;
    }
    else
    {
      //  IDRValue case
      IDRValue* v = new IDRValue();
      v->path.push_back(new_id);

      while(curr_token.type() == DOT)
      {
        eat(DOT, "Expected DOT ");
        v->path.push_back(curr_token);
        eat(ID, "Expected ID ");
      }
      node.rvalue = v;
    }
  }
  
  else
    error("Something went wrong ");
    
}


//----------------------------------------------------------------------
// Statement nodes
//----------------------------------------------------------------------

//simplervalue node
void Parser::simple_rvalue(SimpleRValue& node)
{
  node.value = curr_token;
}

//newrvalue node
void Parser::new_rvalue(NewRValue& node)
{
  node.type_id = curr_token;
  eat(ID, "expected id ");
}


//dtype helper funtion
void Parser::dtype()
{
  if (curr_token.type() == INT_TYPE) {
    eat(INT_TYPE, "expecting int_type");
  
  }
  else if (curr_token.type() == DOUBLE_TYPE) {
    eat(DOUBLE_TYPE, "expecting double_type");
  
  }
  else if (curr_token.type() == BOOL_TYPE) {
    eat(BOOL_TYPE, "expecting bool_type");
  
  }
  else if (curr_token.type() == CHAR_TYPE) {
    eat(CHAR_TYPE, "expecting char_type");
  
  }
  else if (curr_token.type() == STRING_TYPE) {
    eat(STRING_TYPE, "expecting string_type");
  
  }
  else if (curr_token.type() == ID) {
    eat(ID, "expecting id");
  
  }
  else {
    error("unexpected token ");
  }

}

#endif
