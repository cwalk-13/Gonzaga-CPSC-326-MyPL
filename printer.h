//----------------------------------------------------------------------
// NAME: Charles Walker
// FILE: printer.h
// DATE: 3/1/21
// DESC: 
//----------------------------------------------------------------------


#ifndef PRINTER_H
#define PRINTER_H

#include <iostream>
#include "ast.h"
#include <string>




class Printer : public Visitor
{
public:
  // constructor
  Printer(std::ostream& output_stream) : out(output_stream) {}

  // top-level
  void visit(Program& node);
  void visit(FunDecl& node);
  void visit(TypeDecl& node);
  void visit(Repl& node);
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
  std::ostream& out;
  int indent = 0;

  void inc_indent() {indent += 3;}
  void dec_indent() {indent -= 3;}
  std::string get_indent() {return std::string(indent, ' ');}

};


// TODO: Implement the visitor functions 
  // top-level
  void Printer::visit(Program& node)
  {
    // out << "prog'\n'";
    //for each
    for(Decl* d : node.decls)
    {
      d->accept(*this);
    }
    
  }
  void Printer::visit(Repl& node)
  {
  }
  void Printer::visit(FunDecl& node)
  {
    // out << "fundecl'\n'";
    std::string params_str = "(";
    bool found = false;
    for(FunDecl::FunParam p : node.params) 
    {
      found = true;
      std::string p_str = "";
      p_str += p.id.lexeme();
      p_str += ": ";
      p_str += p.type.lexeme();
      p_str += ", ";
      params_str += p_str;
    }
    if (found == true) {
      params_str.pop_back();
      params_str.pop_back();
    }
  
    params_str += ")";
    out << "fun " + node.return_type.lexeme() + " " + node.id.lexeme() + params_str + '\n';
    inc_indent();
    for(Stmt* s : node.stmts) 
    {
      s->accept(*this);
    }
    dec_indent();
    out << "end\n";
  }

  void Printer::visit(TypeDecl& node)
  {
    // out << "typedecl'\n'";
    std::string first_line = "type " + node.id.lexeme()+ '\n';
    out << first_line;
    inc_indent();

    for(VarDeclStmt* v : node.vdecls) 
    {
      v->accept(*this);
    }
    dec_indent();
    out << "end\n";
  }

  // statements
  void Printer::visit(VarDeclStmt& node)
  {
    //explicit type case
    if(node.type != nullptr) {
      out << get_indent() + "var " + node.id.lexeme() + ": " + node.type->lexeme() + " " + "= ";
    } else {
      // non-explicity type case
      out << get_indent() + "var " + node.id.lexeme() + " " + "= ";
    }
    node.expr->accept(*this);
    out << "\n";
  }

  void Printer::visit(AssignStmt& node)
  {
    std::string ids_str = "";
    for(Token lval : node.lvalue_list)
    {
      ids_str += lval.lexeme();
      ids_str += " ";
    }
    out << get_indent() + ids_str + "= ";
    node.expr->accept(*this);
    out << "\n";
  }

  void Printer::visit(ReturnStmt& node)
  {
    out << get_indent() + "return ";
    node.expr->accept(*this);
    out << "\n";
  }
  void Printer::visit(IfStmt& node)
  {
    //if part
    out << get_indent() + "if ";
    node.if_part->expr->accept(*this);
    out << "then\n";
    inc_indent();
    //print all stmts
    for(Stmt* s : node.if_part->stmts) 
      s->accept(*this);
    
    dec_indent();
    //loop through possible else ifs
    for(BasicIf* elif : node.else_ifs)
    {
      out << get_indent() + "else if ";
      elif->expr->accept(*this);
      out << "then\n";
      inc_indent();
      //print all stmts
      for(Stmt* s : elif->stmts) 
        s->accept(*this);
      
      dec_indent();
    }
    //if there is an else 
    if (node.body_stmts.size() > 0)
    {
      out << get_indent() + "else\n";
      inc_indent();
      for(Stmt* s : node.body_stmts) 
        s->accept(*this);
    }
    // dec_indent();
    out << get_indent() + "end\n";

  }
  void Printer::visit(WhileStmt& node)
  {
    // out << "while stmt";
    out << get_indent() + "while ";
    node.expr->accept(*this);
    out << "do\n";
    inc_indent();
    //print all stmts
    for(Stmt* s : node.stmts) 
    {
      s->accept(*this);
    }
    dec_indent();
    out << get_indent() + "end\n";
    
  }
  void Printer::visit(ForStmt& node)
  {
    // out << "for stmt";
    out << get_indent() + "for " + node.var_id.lexeme() + " " + "= ";
    node.start->accept(*this); 
    out << "to ";
    node.end->accept(*this);
    out << "do\n";
    inc_indent();
    //print all stmts
    for(Stmt* s : node.stmts) 
    {
      s->accept(*this);
    }
    dec_indent();
    out << "end\n";
  }
  // expressions
  void Printer::visit(Expr& node)
  {
    // out << "exprdecl'\n'";
    if (node.negated == true)
      out << "not ";
    //first term
    node.first->accept(*this);
    //if there is an operator
    if(node.op != nullptr) {
      out << node.op->lexeme() + " ";
    }
    //if there is an expression after operator
    if(node.rest != nullptr) {
      node.rest->accept(*this);
    }
  }

  void Printer::visit(SimpleTerm& node)
  { 
    // out << "simpterm'\n'";
    node.rvalue->accept(*this);
  }
  void Printer::visit(ComplexTerm& node)
  {
    // out << "compterm'\n'";
    out << "( ";
    node.expr->accept(*this);
    out << ") ";
  }
  // rvalues
  void Printer::visit(SimpleRValue& node)
  {
    // out << "simprval'\n'";
    out << node.value.lexeme() + " "; 
  }
  void Printer::visit(NewRValue& node)
  {
    // out << "newrval'\n'";
    out << node.type_id.lexeme(); 
  }
  void Printer::visit(CallExpr& node)
  {
    // out << "callexpr'\n'";
    out << get_indent() + node.function_id.lexeme() + "("; 
    //loop through arg_list
    for(Expr* expr : node.arg_list) 
    {
      expr->accept(*this);
    }
    out << ")\n";
  }
  void Printer::visit(IDRValue& node)
  {
    // out << "idr'\n'";
    std::string ids_str = "";
    for(Token p : node.path)
    {
      ids_str += p.lexeme();
      ids_str += ".";
    }
    if (ids_str.size() > 1)
      ids_str.pop_back();
    out << ids_str + " ";
  }
  void Printer::visit(NegatedRValue& node)
  {
    // out << "negated'\n'";
    // out << node.value.lexeme() + '\n'; 
  }


#endif
