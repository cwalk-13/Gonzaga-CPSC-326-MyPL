//----------------------------------------------------------------------
// NAME: Charles Walker
// FILE: lexer.h
// DATE: 2/1/2021
// DESC: Lexer analysis for MyPL
//----------------------------------------------------------------------

#ifndef LEXER_H
#define LEXER_H

#include <istream>
#include <string>
#include "token.h"
#include "mypl_exception.h"


class Lexer
{
public:

  // construct a new lexer from the input stream
  Lexer(std::istream& input_stream);

  // return the next available token in the input stream (including
  // EOS if at the end of the stream)
  Token next_token();
  
private:

  // input stream, current line, and current column
  std::istream& input_stream;
  int line;
  int column;

  // return a single character from the input stream and advance
  char read();

  // return a single character from the input stream without advancing
  char peek();

  // create and throw a mypl_exception (exits the lexer)
  void error(const std::string& msg, int line, int column) const;
};


Lexer::Lexer(std::istream& input_stream)
  : input_stream(input_stream), line(1), column(1)
{
}


char Lexer::read()
{
  return input_stream.get();
}


char Lexer::peek()
{
  return input_stream.peek();
}


void Lexer::error(const std::string& msg, int line, int column) const
{
  throw MyPLException(LEXER, msg, line, column);
}


Token Lexer::next_token()
{
  // TODO
  std::string lexeme = "";
  char ch = read();
  column++;
  //checks if white space
  while(std::isspace(ch)) {
    if (ch == '\n') {
      ch = read();
      line++;
      column = 1;
    }
    if (ch == '\r') {
      ch = read();
      line++;
      column = 1;
    }
    if (ch == ' ') {
      ch = read();
      column++;
    }
    if (ch == '\t') {
      ch = read();
      column+= 2; 
    }
  }

  //check for comments
  if (ch == '#')
  {
    bool multiline = true;
    while(multiline) {

        while(ch != '\n') {
          ch = read();
        }
        line++;
        column = 1;
        ch = read();
      
      while (std::isspace(ch))
      {
        if (ch == ' ')
        {
          ch = read();
          column++;
        }

        if (ch == '\n')
        {
          ch = read();
          line++;
          column = 1;
        }
      }

      if (ch != '#')
      {
        multiline = false;
      }
    }
  }
  if (ch == EOF) {
    return Token(EOS, "", line, column);
  }

  //checks for simple symbols
  if (ch == '(') {
    return Token(LPAREN, "(", line, column);
  }
  if (ch == ')') {

    return Token(RPAREN, ")", line, column);
  }
  if (ch == '.') {
    return Token(DOT, ".", line, column);
  }
  if (ch == ',') {
    return Token(COMMA, ",", line, column);
  }
  if (ch == ':') {
    return Token(COLON, ":", line, column);
  }
  if (ch == '+') {
    return Token(PLUS, "+", line, column);
  }
    if (ch == '-') {
    return Token(MINUS, "-", line, column);
  }
    if (ch == '*') {
    return Token(MULTIPLY, "*", line, column);
  }
    if (ch == '/') {
    return Token(DIVIDE, "/", line, column);
  }
  if (ch == '%') {
    return Token(MODULO, "%", line, column);
  }
  // check for more involved symbols
  if (ch == '=') {
    char next = peek();
    if (next == '=') {
      ch = read();
      int start_col = column;
      column++;  
      return Token(EQUAL, "==", line, start_col);
    }
    return Token(ASSIGN, "=", line, column);

  }

  if (ch == '<') {
    char next = peek();
    if (next == '=') {
      ch = read();
      int start_col = column;
      column++;
      return Token(LESS_EQUAL, "<=", line, start_col);
    }
    return Token(LESS, "<", line, column);
  }

  if (ch == '>') {
    char next = peek();
    if (next == '=') {
      ch = read();
      int start_col = column;
      column++;
      return Token(GREATER_EQUAL, ">=", line, start_col);
    }
    return Token(GREATER, ">", line, column);
  }

  if (ch == '!') {
    char next = peek();
    if (next == '=') {
      ch = read();
      int start_col = column;
      column++;
      return Token(NOT_EQUAL, "!=", line, start_col);
    }
    error("invalid symbol", line, column);
  }

  //check for char values
  if (ch == '\'') {
    ch = read();
    int start_col = column;
    std::string lexeme = "";
    char next = peek();
    if(next == '\'') {
      lexeme += ch;
      ch = read();
      column++;
      return Token(CHAR_VAL, lexeme, line, start_col);
    }
    error("invalid symbol", line, column);
  }

  //check for string values
  if (ch == '"') {
    std::string lexeme = "";
    ch = read();
    int start_col = column;
    column++;
    if(ch == '"') {
      return Token(STRING_VAL, "", line, start_col);
    }
    while(peek() != '"') {
      if (ch == EOS) {
        error("missing \"", line, column);
      }
      if (isspace(ch))
      {
        if (ch == '\n' && isspace(peek()))
          error("Strings need to be one continuous string of characters", line, start_col);
      }
      lexeme += ch;
      column++;
      ch = read();
    }
    lexeme += ch;
    ch = read();
    return Token(STRING_VAL, lexeme, line, start_col);
  }

  // check for numeric values
  if (std::isdigit(ch)) {
    int start_col = column;
    bool is_double = false;
    lexeme += ch;

    while (std::isdigit(peek()) || peek() == '.')
    {
      //  If the char is a dot, flag the lexeme as a double
      if (ch == '.')
        is_double = true;

      lexeme += ch;
      ch = read();
      column++;
    }

    // if (std::isalpha(peek()))
    //   error ("Int followed by id without space", line, start_col);

    if (is_double)
      return Token(DOUBLE_VAL, lexeme, line, start_col);

    return Token(INT_VAL, lexeme, line, start_col);
  }


  //check for reserved words

  if (std::isalpha(ch)) {
    int start_col = column;
    std::string lexeme = "";
    lexeme += ch;
    while(!(std::isspace(peek())) && peek() != ',' && peek() != '(' && peek() != ')' 
      && peek() != ':' && peek() != '='&& peek() != '.' && peek() != '+' && peek() != '-' 
      && peek() != '/' && peek() != '*' && peek() != '%' && peek() != '#'
      && peek() != '<' && peek() != '>') 
    {
      ch = read();
      lexeme += ch;
      column++;
    }

    if (lexeme == "neg") {
      return Token(NEG, lexeme, line, start_col);
    }
    if (lexeme == "and") {
      return Token(AND, lexeme, line, start_col);
    }
    if (lexeme == "or") {
      return Token(OR, lexeme, line, start_col);
    }
    if (lexeme == "not") {
      return Token(NOT, lexeme, line, start_col);
    }
    if (lexeme == "type") {
      return Token(TYPE, lexeme, line, start_col);
    }
    if (lexeme == "while") {
      return Token(WHILE, lexeme, line, start_col);
    }
    if (lexeme == "for") {
      return Token(FOR, lexeme, line, start_col);
    }   
    if (lexeme == "to") {
      return Token(TO, lexeme, line, start_col);
    }
    if (lexeme == "do") {
      return Token(DO, lexeme, line, start_col);
    }
    if (lexeme == "if") {
      return Token(IF, lexeme, line, start_col);
    }
    if (lexeme == "then") {
      return Token(THEN, lexeme, line, start_col);
    }
    if (lexeme == "elseif") {
      return Token(ELSEIF, lexeme, line, start_col);
    }
    if (lexeme == "else") {
      return Token(ELSE, lexeme, line, start_col);
    }
    if (lexeme == "end") {
      return Token(END, lexeme, line, start_col);
    }
    if (lexeme == "fun") {
      return Token(FUN, lexeme, line, start_col);
    }
    if (lexeme == "var") {
      return Token(VAR, lexeme, line, start_col);
    }
    if (lexeme == "return") {
      return Token(RETURN, lexeme, line, start_col);
    }
    if (lexeme == "new") {
      return Token(NEW, lexeme, line, start_col);
    }
    if (lexeme == "bool") {
      return Token(BOOL_TYPE, lexeme, line, start_col);
    }
    if (lexeme == "int") {
      return Token(INT_TYPE, lexeme, line, start_col);
    }
    if (lexeme == "double") {
      return Token(DOUBLE_TYPE, lexeme, line, start_col);
    }
    if (lexeme == "char") {
      return Token(CHAR_TYPE, lexeme, line, start_col);
    }
    if (lexeme == "string") {
      return Token(STRING_TYPE, lexeme, line, start_col);
    }
    if (lexeme == "nil") {
      return Token(NIL, lexeme, line, start_col);
    }
    if (lexeme == "true" | lexeme == "false") {
      return Token(BOOL_VAL, lexeme, line, start_col);
    } else {
      return Token(ID, lexeme, line, start_col);
    }

    return Token(EOS, "", line, start_col);
  }
  
}


#endif
