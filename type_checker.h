//----------------------------------------------------------------------
// NAME: Charles Walker
// FILE: type_checker.h
// DATE: 
// DESC: 
//----------------------------------------------------------------------


#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include <iostream>
#include "ast.h"
#include "symbol_table.h"


class TypeChecker : public Visitor
{
public:

  // top-level
  void visit(Program& node);
  void visit(Repl& node);
  void visit(FunDecl& node);
  void visit(TypeDecl& node);
  // statements
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

private:

  // the symbol table 
  SymbolTable sym_table;

  // the previously inferred type
  std::string curr_type;

  // helper to add built in functions
  void initialize_built_in_types();

  // error message
  void error(const std::string& msg, const Token& token);
  void error(const std::string& msg); 

};


void TypeChecker::error(const std::string& msg, const Token& token)
{
  throw MyPLException(SEMANTIC, msg, token.line(), token.column());
}


void TypeChecker::error(const std::string& msg)
{
  throw MyPLException(SEMANTIC, msg);
}


void TypeChecker::initialize_built_in_types()
{
  // print function
  sym_table.add_name("print");
  sym_table.set_vec_info("print", StringVec {"string", "nil"});
  // stoi function
  sym_table.add_name("stoi");
  sym_table.set_vec_info("stoi", StringVec {"string", "int"});  

  // TODO: finish the rest of the built-in functions: stod, itos,
  // dtos, get, length, and read
  // stod function 
  sym_table.add_name("stod");
  sym_table.set_vec_info("stod", StringVec {"string", "double"});
  // itos
  sym_table.add_name("itos");
  sym_table.set_vec_info("itos", StringVec {"int", "string"});
  // dtos
  sym_table.add_name("dtos");
  sym_table.set_vec_info("dtos", StringVec {"double", "string"});
  // get
  sym_table.add_name("get");
  sym_table.set_vec_info("get", StringVec {"string", "char"});
  // length
  sym_table.add_name("length");
  sym_table.set_vec_info("length", StringVec {"string", "int"});
  //read
  sym_table.add_name("read");
  sym_table.set_vec_info("read", StringVec {"string", "nil"});

}

//----------------------------------------------------------------------
// Function, Variable, and Type Declarations
//----------------------------------------------------------------------

void TypeChecker::visit(Program& node)
{
  // push the global environment

  sym_table.push_environment();
  // add built-in functions
  initialize_built_in_types();
  // push 
  for (Decl* d : node.decls)
    d->accept(*this);
  // check for a main function
  if (sym_table.name_exists("main") and sym_table.has_vec_info("main")) {
    // TODO: finish checking that the main function is defined with
    // the correct signature
    StringVec main_info;
    sym_table.get_vec_info("main", main_info);

    //  Ensure that main function has no parameters
    if (main_info.size() > 0)
      error("Main function should have no parameters");
  }
  else {
    // NOTE: the only time the 1-argument version of error should be
    // called!
    error("undefined 'main' function");
  }
   // pop the global environment
  sym_table.pop_environment();
}

void TypeChecker::visit(Repl& node)
{
}

// TODO: Implement the remaining visitor functions
void TypeChecker::visit(FunDecl& node)
{
  bool return_nil = false;
  std::string return_stmt_type;
  //  Check that function isnt already declared
  if (sym_table.name_exists_in_curr_env(node.id.lexeme()))
    error("Redeclaration of function ", node.id);

  if (node.return_type.lexeme() == "nil")
    return_nil = true;

  //  Check return type
  if ( node.return_type.lexeme() != "int" && node.return_type.lexeme() != "double"
     && node.return_type.lexeme() != "char" && node.return_type.lexeme() != "string"
     && node.return_type.lexeme() != "bool" && node.return_type.lexeme() != "nil")
  {
    //  Check if name is a UDT
    if ( !(sym_table.name_exists_in_curr_env(node.return_type.lexeme())) )
      error("Invalid return type: ", node.return_type);
  }

  //return type nil has no return stmt
  if (return_nil)
  {
    if (sym_table.name_exists_in_curr_env("return") && sym_table.has_str_info("return"))
      error("Cannot return a value when return type is nil");
  }
  else
  {
    if (sym_table.name_exists_in_curr_env("return") && sym_table.has_str_info("return"))
    {
      sym_table.get_str_info("return", return_stmt_type);
      // check matching return types
      if (node.return_type.lexeme() != return_stmt_type && return_stmt_type != "nil")
        error("Return type and returned value do not match: "+node.return_type.lexeme()+" and "+return_stmt_type, node.return_type);
    }
  }
  StringVec the_type;
  
  //to get parameters
  for(auto vars = node.params.begin(); vars != node.params.end(); ++vars)
  {
  	sym_table.add_name(vars->id.lexeme());
  	sym_table.set_str_info(vars->id.lexeme(), vars->type.lexeme());//add type to var name
    the_type.push_back(vars->type.lexeme());
  }
  
  the_type.push_back(node.return_type.lexeme());//add return type
  sym_table.add_name(node.id.lexeme());//add function name
  sym_table.set_vec_info(node.id.lexeme(), the_type);//add type and params
  
  sym_table.push_environment();//push environment
  
  //FUNCTION BODY
  sym_table.add_name("return");//add new return value for later checking
  sym_table.set_str_info("return", node.return_type.lexeme());
  
  //Continue to statements
  for(Stmt* s : node.stmts)
    s->accept(*this);
  
  sym_table.pop_environment();//pop it back
}


void TypeChecker::visit(TypeDecl& node)
{
  //Check for redeclaration
  if (sym_table.name_exists_in_curr_env(node.id.lexeme()))
    error("User Defined Type is already in scope ", node.id);

  StringMap type_info;
  sym_table.add_name(node.id.lexeme());
  sym_table.push_environment();
  sym_table.set_map_info(node.id.lexeme(), type_info);
  //add all declarations to map
  for(VarDeclStmt* v : node.vdecls)
  {
    v->accept(*this);
    type_info[v->id.lexeme()] = curr_type;
  }
  
  sym_table.pop_environment();
  sym_table.set_map_info(node.id.lexeme(), type_info);
}

//----------------------------------------------------------------------
// Statement nodes
//----------------------------------------------------------------------

void TypeChecker::visit(VarDeclStmt& node)
{
	//check that type is UDT first
  if(node.type != nullptr && node.type->lexeme() != "nil" && node.type->lexeme()  != "int" && node.type->lexeme()  
    != "double" && node.type->lexeme()  != "bool" && node.type->lexeme()  != "string" && node.type->lexeme()  != "char") {
    if(sym_table.name_exists(node.type->lexeme()) == false)
      error("UDT " + node.type->lexeme() + " does not exist", node.id);
  }
  node.expr->accept(*this);

  //then check if explicitly defined and value is nil
  if(node.type != nullptr && curr_type == "nil")
    curr_type = node.type->lexeme();

  //check if type matches expression for explicit functions
  if(node.type != nullptr)
  {
    if(node.type->lexeme() != curr_type && curr_type != "nil")
      error("Types do not match: "+node.type->lexeme() +" and "+curr_type, node.id);
  }

  std::string rhs_type = curr_type;
	//var already exists
  if(sym_table.name_exists_in_curr_env(node.id.lexeme()))
    error("Redeclaration of var ", node.id);

  sym_table.add_name(node.id.lexeme());
  sym_table.set_str_info(node.id.lexeme(), rhs_type);
}

void TypeChecker :: visit (AssignStmt & node)
{
  std::string prev_type;
  int i = 1;
  for(Token t : node.lvalue_list)//loop through lvalue list
  {	
  		if(i == 1) 
      {
  		  if(sym_table.name_exists(t.lexeme()))
  				sym_table.get_str_info(t.lexeme(), curr_type);
  			else
  				error("var " + node.lvalue_list.front().lexeme() + " used before def", node.lvalue_list.front());
  		}
      //go into path
  		else {
  		  if(sym_table.has_map_info(prev_type) == false)
  				error("UDT var " + t.lexeme() + " does not exist", node.lvalue_list.front());
  				
				StringMap map;
        //info of the previous type
				sym_table.get_map_info(prev_type, map);
				if(map.count(t.lexeme()) > 0)
					curr_type = map[t.lexeme()];
				else//type not found
					error("Path value does not exist");
  		}
  	prev_type = curr_type;
    //continue to traverser through path
		++i;
  }
  //lhs must match rhs
  //infer lhs type
  std::string lhs_type = curr_type;
	Expr* e = node.expr;
  e->accept(*this);
  std::string rhs_type = curr_type;
  //strings and chars can be used together
  if (rhs_type == "char" && lhs_type == "string")
    rhs_type = "string";
  if (rhs_type != "nil" && lhs_type!= "nil" && lhs_type != rhs_type ) {
    error("Types do not match: "+lhs_type+" and " +rhs_type, node.lvalue_list.front());
  }
}

void TypeChecker::visit(ReturnStmt& node)
{
  node.expr->accept(*this);
  sym_table.add_name("return");
  sym_table.set_str_info("return", curr_type);

}

void TypeChecker::visit(IfStmt& node)
{
  // check that if condition is bool
  node.if_part->expr->accept(*this);
  if (curr_type != "bool")
    error("If stmt must be boolean not "+curr_type, node.if_part->expr->first_token());

  //typecheck body
  sym_table.push_environment();
  for (Stmt* s : node.if_part->stmts)
    s->accept(*this);
  sym_table.pop_environment();

  // else if stmts
  if (!(node.else_ifs.empty()))
  {
    for (BasicIf* s : node.else_ifs)
    {
      // check if boolean
      s->expr->accept(*this);
      if (curr_type != "bool")
        error("If stmt must be boolean not "+curr_type, s->expr->first_token());

      //typecheck body
      sym_table.push_environment();
      for (Stmt* stmt : s->stmts)
        stmt->accept(*this);
      sym_table.pop_environment();
    }
  }
  //else stmt
  if (!(node.body_stmts.empty()))
  {
    //typecheck body
    sym_table.push_environment();
    for (Stmt* s : node.body_stmts)
      s->accept(*this);
    sym_table.pop_environment();
  }
}

void TypeChecker::visit(WhileStmt& node)
{
  node.expr->accept(*this);
  //check that while condition is bool
  if(curr_type != "bool")
    error("While stmt condition must be bool not "+curr_type, node.expr->first_token());

  //typecheck body
  sym_table.push_environment();
  for (Stmt* s : node.stmts)
    s->accept(*this);
  sym_table.pop_environment();
}

void TypeChecker::visit(ForStmt& node)
{
  //for loop expression must use whole numbers
  node.start->accept(*this);
  if(curr_type != "int")
    error("For loop start and end expressions must be int, got ", node.end->first_token());

  node.end->accept(*this);
  if(curr_type != "int")
    error("For loop start and end expressions must be int, got ", node.end->first_token());

  //typecheck body
  sym_table.push_environment();
  for (Stmt* s : node.stmts)
    s->accept(*this);
  sym_table.pop_environment();
}

//----------------------------------------------------------------------
// Expressions and Expression Terms
//----------------------------------------------------------------------

void TypeChecker::visit(Expr& node)
{
  //check
  if (node.op == nullptr)
  {
    //  Get type of expr
    node.first->accept(*this);

    //  If the expr is negated ensure that the expression is a boolean
    node.first->accept(*this);
    if (node.negated && curr_type != "bool")
      error("Not should be used , got "+curr_type, node.first_token());
  }
  else 
  {
    //typecheck lhs of expr
    node.first->accept(*this);
    std::string lhs_type;
    lhs_type = curr_type;

    //if rest exists, typecheck rhs and compare 
    //and check if op is compatible
    if(node.rest != nullptr)
    {
      node.rest->accept(*this); // new curr_type
      
      //check for op errors
      //equivalence ops
      if(node.op->type() == EQUAL || node.op->type() == NOT_EQUAL)
      {
        //types must be equivalent
        if(lhs_type != curr_type)
        {
          //check if rhs is nil
          if(lhs_type == "nil" || curr_type == "nil" )
            curr_type = "bool";
          else
            error("Cannot compare types "+lhs_type+" and "+curr_type, node.first_token());
        }
        curr_type = "bool";
      }
      //comparison ops
      else if(node.op->type() == LESS || node.op->type() == LESS_EQUAL 
        || node.op->type() == GREATER || node.op->type() == GREATER_EQUAL)
      {
        //types must be int, double, char, string
        if((lhs_type == "int" || lhs_type == "double" || lhs_type == "char" || lhs_type == "string") &&
        (curr_type == "int" || curr_type == "double" || curr_type == "char" || curr_type == "string"))
        {
          //types must be equivalent
          if(lhs_type != curr_type)
            error("Cannot compare types "+lhs_type+" and "+curr_type, node.first_token());
        }
        else
          error("Types must be int, double, char, or str to compare not "+lhs_type + " and "+curr_type, node.first_token());
        curr_type = "bool";
      }
      //plus op
      else if(node.op->type() == PLUS)
      {
        //plus works for int, double, char, str
        if (lhs_type == "int" && curr_type == "int")
          curr_type = "int";
        else if (lhs_type == "double" && curr_type == "double")
          curr_type = "double";
        else if (lhs_type == "char" || lhs_type == "string")
        {
            if (curr_type == "char" || curr_type == "string")
              curr_type == "string";

            else
              error("Can only add strings and chars not "+lhs_type+" and "+curr_type, node.first_token());
        }
        else
          error("Cannot add "+lhs_type+ " with " +curr_type, node.first_token());
      }
      // other arithmetic ops
      else if(node.op->type() == MINUS || node.op->type() == MULTIPLY || node.op->type() == DIVIDE || node.op->type() == MODULO)
      {
        //mod can only be ints
        if (node.op->type() == MODULO && !(lhs_type == "int" && curr_type == "int"))
          error("Can only use % with ints not " +lhs_type + " and " +curr_type, node.first_token());
        //can only be ints or doubles
        if(lhs_type == "int" && curr_type == "int")
          curr_type = "int";
        else if(lhs_type == "double" && curr_type == "double")
          curr_type = "double";
        else
          error("Cannot operate between types "+lhs_type+" and " +curr_type, node.first_token());
      }
    }
  }
}

void TypeChecker::visit(SimpleTerm& node)
{
  node.rvalue->accept(*this);
}

void TypeChecker::visit(ComplexTerm& node)
{
  node.expr->accept(*this);
}
//----------------------------------------------------------------------
// RValue nodes
//----------------------------------------------------------------------

void TypeChecker :: visit ( SimpleRValue & node )
{
  // infer type based on token type
  if (node.value.type() == CHAR_VAL)
    curr_type = "char";
  else if (node.value.type() == STRING_VAL)
    curr_type = "string";
  else if (node.value.type() == INT_VAL)
    curr_type = "int";
  else if (node.value.type() == DOUBLE_VAL)
    curr_type = "double";
  else if (node.value.type() == BOOL_VAL)
    curr_type = "bool";
  else if (node.value.type() == NIL)
    curr_type = "nil";
  else
    error("Unexpected value ");
}

void TypeChecker::visit(NewRValue& node)
{
  if (sym_table.name_exists(node.type_id.lexeme()))
  {
    //type must have data mapped
    if ( !(sym_table.has_map_info(node.type_id.lexeme())) )
      error("This type has no associated data  ", node.type_id);

    curr_type = node.type_id.lexeme();
  }
  else
    error("This type does not exist: "+curr_type, node.type_id);
}

void TypeChecker::visit(CallExpr& node)
{
  //function must be in scope
  if(!sym_table.name_exists(node.function_id.lexeme()))
    error("Function does not exist: ", node.function_id);

  StringVec fun_type;
  sym_table.get_vec_info(node.function_id.lexeme(), fun_type);
  
  //arg list must be same size as declaration
  if(fun_type.size()-1 != node.arg_list.size())
  	error("Fun Call requires " + std::to_string(fun_type.size()-1) + " arguments, got " + std::to_string(node.arg_list.size()), node.function_id);
  //args types must match declaration types
  int i = 0;
  for(Expr* x : node.arg_list)
  {
  	x->accept(*this);
  	if(fun_type[i] != curr_type && curr_type != "nil")
  		error("Expected "+ fun_type[i] + ", got "+ curr_type, node.function_id);
  	++i;
  }
  curr_type = fun_type[fun_type.size()-1];
}

void TypeChecker::visit(IDRValue& node)
{
  std::string prev_type;
  int i = 1;
  //Go through path
  for(Token t : node.path)
  {	
  		if(i == 1)
  		{
        //var must exist in scope
        if (!sym_table.name_exists(node.first_token().lexeme()))
          error("Variable does not exist in scope ", node.first_token());
        else
          sym_table.get_str_info(t.lexeme(), curr_type);
  		}
      //go into path
  		else
  		{
  		  if(sym_table.has_map_info(prev_type) == false)//if it is the second value
  				error("Variable does not exist in scope ", node.path.front());
				StringMap map;
				sym_table.get_map_info(prev_type, map);
			
				if(map.count(t.lexeme()) > 0)
					curr_type = map[t.lexeme()];
				else
					error("Path value does not exist");
  		}
  	//continue to traverse
  	prev_type = curr_type;
		++i;
  }
}

void TypeChecker::visit(NegatedRValue& node)
{
  node.expr->accept(*this);
  // expr must be type int or double
  if (curr_type != "int" || curr_type != "double")
    error("Expecting int or double for negation, not "+curr_type, node.expr->first_token());

  node.expr->accept(*this);
}




#endif
