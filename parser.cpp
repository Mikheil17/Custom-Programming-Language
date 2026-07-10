// ============================================================================
//  parser.cpp — Recursive-descent parser 
// ----------------------------------------------------------------------------
// MSU CSE 4714/6714 Capstone Project (Spring 2026)
// Author: Derek Willis
// ============================================================================

#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <set>
#include "lexer.h"
#include "ast.h"
#include "debug.h"
using namespace std;

// -----------------------------------------------------------------------------
// One-token lookahead
// -----------------------------------------------------------------------------
bool   havePeek = false;
Token  peekTok  = 0;
string peekLex;

inline const char* tname(Token t) { return tokName(t); }

Token peek() 
{
  if (!havePeek) {
    peekTok = yylex();
    if (peekTok == 0) { peekTok = TOK_EOF; peekLex.clear(); }
    else              { peekLex = yytext ? string(yytext) : string(); }
    dbg::line(string("peek: ") + tname(peekTok) + (peekLex.empty() ? "" : " ["+peekLex+"]")
              + " @ line " + to_string(yylineno));
    havePeek = true;
  }
  return peekTok;
}
Token nextTok() 
{
  Token t = peek();
  dbg::line(string("consume: ") + tname(t));
  havePeek = false;
  return t;
}
Token expect(Token want, const char* msg) 
{
  Token got = nextTok();
  if (got != want) {
    dbg::line(string("expect FAIL: wanted ") + tname(want) + ", got " + tname(got));
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected "
        << tname(want) << " — " << msg << ", got " << tname(got)
        << " [" << (yytext ? yytext : "") << "]";
    throw runtime_error(oss.str());
  }
  return got;
}

unique_ptr<Write> parseWrite();
unique_ptr<Read> parseRead();
unique_ptr<Assign> parseAssign();
unique_ptr<Compound> parseCompound();
unique_ptr<Statement> parseStatement();
unique_ptr<Block> parseBlock();
unique_ptr<Program> parseProgram();

unique_ptr<Primary> parsePrimary();
unique_ptr<Custom> parseCustom();
unique_ptr<Value> parseValue();
unique_ptr<Term> parseTerm();
unique_ptr<Factor> parseFactor();

unique_ptr<Expression> parseExpression();
unique_ptr<If> parseIf();
unique_ptr<While> parseWhile();
unique_ptr<RelOp> parseRelOp();

unique_ptr<RelOp> parseRelOp() {
  auto r = make_unique<RelOp>();
  r->type = peek();
  nextTok();

  return r;
}

unique_ptr<Expression> parseExpression() {
  auto e = make_unique<Expression>();

  e->left = parseValue();

  if (peek() == LESSTHAN || peek() == GREATERTHAN ||
      peek() == EQUALTO || peek() == NOTEQUALTO) {


    e->relOp = parseRelOp();
    e->right = parseValue();
  }

  return e;
}

unique_ptr<If> parseIf() {
  auto i = make_unique<If>();

  expect(IF, "IF");
  i->expression = parseExpression();
  expect(THEN, "THEN");

  i->thenStmt = parseStatement();

  if(peek() == ELSE) {
    expect(ELSE, "ELSE");
    i->elseStmt = parseStatement();
  }

  return i;
}

unique_ptr<While> parseWhile() {
  auto w = make_unique<While>();

  expect(WHILE, "WHILE");
  w->expression = parseExpression();
  w->stmt = parseStatement();

  return w;
}

unique_ptr<Custom> parseCustom() {
  auto c = make_unique<Custom>();
  expect(CUSTOM, "Expecting CUSTOM gone wrong.");
  return c;
}

//value → term { ( PLUS | MINUS | OR ) term }
unique_ptr<Value> parseValue() {
  auto v = make_unique<Value>();
  v->terms.push_back(parseTerm());

  while(peek() == PLUS || peek() == MINUS || peek() == TOK_OR) {
    if(peek() == PLUS) {
      v->types.push_back(PLUS);
      expect(PLUS, "Expecting PLUS went wrong");
      v->terms.push_back(parseTerm());
    }
    else if(peek() == MINUS) {
      v->types.push_back(MINUS);
      expect(MINUS, "Expecting MINUS went wrong");
      v->terms.push_back(parseTerm());
    }
    else if(peek() == TOK_OR) {
      v->types.push_back(TOK_OR);
      expect(TOK_OR, "Expecting OR went wrong");
      v->terms.push_back(parseTerm());
    }
  }
  return v;
}

//term → factor { ( MULTIPLY | DIVIDE | MOD | AND ) factor }
unique_ptr<Term> parseTerm() {
  auto t = make_unique<Term>();

  t->factors.push_back(parseFactor());

  while(peek() == MULTIPLY || peek() == DIVIDE || peek() == MOD || peek() == TOK_AND) {
    if(peek() == MULTIPLY) {
      t->types.push_back(MULTIPLY);
      expect(MULTIPLY, "Expecting MULTIPLY went wrong");
      t->factors.push_back(parseFactor());
    }
    else if(peek() == DIVIDE) {
      t->types.push_back(DIVIDE);
      expect(DIVIDE, "Expecting DIVIDE went wrong");
      t->factors.push_back(parseFactor());
    }
    else if(peek() == MOD) {
      t->types.push_back(MOD);
      expect(MOD, "Expecting MOD went wrong");
      t->factors.push_back(parseFactor());
    }
    else if(peek() == TOK_AND) {
      t->types.push_back(TOK_AND);
      expect(TOK_AND, "Expecting AND went wrong");
      t->factors.push_back(parseFactor());
    }
  }
  return t;
}

//factor → [ MINUS | NOT] primary
unique_ptr<Factor> parseFactor() {
  auto f = make_unique<Factor>();

  if(peek() == MINUS) {
    f->type = MINUS;
    expect(MINUS, "Expecting MINUS went wrong");
  } else if(peek() == TOK_NOT) {
    f->type = TOK_NOT;
    expect(TOK_NOT, "Expecting NOT went wrong");
  } 
  f->value = parsePrimary();
  return f;
}

//primary → FLOATLIT | INTLIT | IDENT | OPENPAREN value CLOSEPAREN
unique_ptr<Primary> parsePrimary() {
  auto p = make_unique<Primary>();

  Token t = peek();
  if( t == OPENPAREN) {
    p->type = OPENPAREN;
    expect(OPENPAREN, "Expecting OPENPAREN went wrong");
    p->parenthesizedValue = parseValue();
    expect(CLOSEPAREN, "Expecting CLOSEPAREN went wrong");

  } else if (t == INTLIT || t == FLOATLIT || t == IDENT) {
      if (t == IDENT && symbolTable.find(peekLex) == symbolTable.end()) {
          throw runtime_error("Undeclared var: " + peekLex);
      }
      p->value = peekLex;
      p->type = t;
      nextTok();
      
  } else {
    throw runtime_error("Not INTLIT, FLOATLIT, or IDENT");
  }
  return p;
}

//write → WRITE OPENPAREN ( IDENT | STRINGLIT ) CLOSEPAREN
unique_ptr<Write> parseWrite() {
  auto w = make_unique<Write>();
  expect(WRITE, "write");
  expect(OPENPAREN, "open paren");

  Token t = peek();

  if (t == STRINGLIT || t == IDENT) {
    if (t == IDENT && symbolTable.find(peekLex) == symbolTable.end())
      throw runtime_error("Undeclared var: " + peekLex);
    w->content = peekLex;
    w->type    = t;
    nextTok();
  } else {
    throw runtime_error("Not STRINGLIT or IDENT for Write");
  }

  expect(CLOSEPAREN, "close paren");
  return w;
}

//read → READ OPENPAREN IDENT CLOSEPAREN
unique_ptr<Read> parseRead() {
  auto r = make_unique<Read>();
  expect(READ, "read");
  expect(OPENPAREN, "open paren");

  peek();
  if (symbolTable.find(peekLex) == symbolTable.end())
    throw runtime_error("Undeclared var: " + peekLex);
  r->target = peekLex;
  expect(IDENT, "var name");

  expect(CLOSEPAREN, "close paren");
  return r;
}

//assign → IDENT ( ASSIGN | CUSTOM_OPERATORS ) value
unique_ptr<Assign> parseAssign() {
  auto a = make_unique<Assign>();

  peek();
  if (symbolTable.find(peekLex) == symbolTable.end())
    throw runtime_error("Undeclared var: " + peekLex);
  a->id = peekLex;
  expect(IDENT, "var name");
  
  Token t = peek();
  if (t == ASSIGN || t == SELF_PLUS || t == SELF_MINUS || 
      t == SELF_MULTIPLY || t == SELF_DIVIDE || t == SELF_MOD) {
    a->type = t;
    nextTok();
  } else {
    throw runtime_error("Not Assign or other operators.");
  }

  a->value = parseValue();
  return a;
}

unique_ptr<Compound> parseCompound(){
  auto bob = make_unique<Compound>();
  expect(TOK_BEGIN, "token begin");

  bob->statements.push_back(parseStatement());
  while(peek()==SEMICOLON) {
    expect(SEMICOLON, "semicolon");
    bob->statements.push_back(parseStatement());
  }
  expect(END, "end");
  return bob;
}

//Statement → assign | compound | read | write | if | while | CUSTOM
unique_ptr<Statement> parseStatement(){
  if(peek() == TOK_BEGIN) return parseCompound();
  else if (peek() == READ) return parseRead();
  else if (peek() == WRITE) return parseWrite();
  else if (peek() == IDENT) return parseAssign();
  else if (peek() == CUSTOM) return parseCustom();
  else if (peek() == IF) return parseIf();
  else if (peek() == WHILE) return parseWhile();
  else throw runtime_error("Statement error: not TOK_BEGIN READ WRITE OR IDENT");
}

unique_ptr<Block> parseBlock(){

  auto b = make_unique<Block>();

  if (peek() == VAR) {
    nextTok();

    while (peek() == IDENT) {
      string varName = peekLex;
      nextTok();

      if (symbolTable.find(varName) != symbolTable.end())
        throw runtime_error("Duplicate var declaration: " + varName);

      expect(COLON, "colon after var name");

      Token typeToken = peek();
      if (typeToken == INTEGER) {
        symbolTable[varName] = 0;
        nextTok();
      } else if (typeToken == REAL) {
        symbolTable[varName] = 0.0;
        nextTok();
      } else {
        throw runtime_error("Not INTEGER or REAL in declaration");
      }

      expect(SEMICOLON, "semicolon after declaration");
    }
  }

  b->compound = parseCompound();
  return b;
}

// -----------------------------------------------------------------------------
// Program → PROGRAM IDENT ';' Block EOF
// -----------------------------------------------------------------------------
unique_ptr<Program> parseProgram() {
  // Make a pointer to the node we need to build
  auto p = make_unique<Program>();
  // Step through the grammar, storing anything necessary as member variables
  expect(PROGRAM, "start of program");
  expect(IDENT, "program name");
  // Store the program name 
  p->name  = peekLex;
  expect(SEMICOLON, "after program name");
  // Store a pointer to the appropriate block
  p->block = parseBlock();
  expect(TOK_EOF, "at end of file (no trailing tokens after program)");
  // Nothing left in the grammar so we return our node pointer
  return p;
}

// -----------------------------------------------------------------------------
// Parser entry point (called by driver)
// -----------------------------------------------------------------------------
// *****************************************************
// To test piece-wise change the pointer type below
unique_ptr<Program> parse()
// *****************************************************
{
  // Reset lookahead state for a fresh parse
  havePeek = false;
  peekTok = 0;
  peekLex.clear();
  
  // *****************************************************
  // To test piece-wise change the parser function you set as your root
  auto root = parseProgram();
  // *****************************************************

  // Ensure no extra tokens remain
  if (peek() != TOK_EOF) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): extra tokens after <program>, got "
        << tname(peekTok) << " [" << (yytext ? yytext : "") << "]";
    throw runtime_error(oss.str());
  }

  return root;
}