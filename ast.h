// =============================================================================
//   ast.h 
// =============================================================================
// MSU CSE 4714/6714 Capstone Project (Spring 2026)
// Author: Derek Willis
// =============================================================================
#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <variant>
#include <cmath>
#include "lexer.h"
using namespace std;

inline map<string, variant<int,double>> symbolTable;

// -----------------------------------------------------------------------------
// Pretty printer
// -----------------------------------------------------------------------------
inline void ast_line(ostream& os, string prefix, bool last, string label) {
  os << prefix << (last ? "└── " : "├── ") << label << "\n";
}

inline double as_double(const variant<int, double>& v) {
  return holds_alternative<int>(v) ? static_cast<double>(get<int>(v)) : get<double>(v);
}

const double EPSILON = 0.000001;
inline bool isTrue(const variant<int,double>& v) {
  return fabs(as_double(v)) > EPSILON;
}


struct Statement2
{
  // Member Function to Print
  virtual void print_tree(ostream& os,string prefix,bool last) = 0;

  // Member Function to Interpret
  virtual variant<int,double> interpret() = 0;
};

//Statement → assign | compound | read | write | if | while CUSTOM
//Statement → assign | compound | read | write | CUSTOM
//statement → assign | compound | read | write
struct Statement
{
  // Member Function to Print
  virtual void print_tree(ostream& os,string prefix,bool last) = 0;

  // Member Function to Interpret
  virtual void interpret(ostream& out) = 0;
};

//relOp → LESSTHAN | GREATERTHAN | EQUALTO | NOTEQUALTO
struct RelOp {
  Token type;

  void print_tree(ostream& os, string prefix, bool last) {
    ast_line(os, prefix, last, "RelOp: " + type);
  }
};

//primary → FLOATLIT | INTLIT | IDENT | OPENPAREN value CLOSEPAREN
//primary → FLOATLIT | INTLIT | IDENT | OPENPAREN value CLOSEPAREN
//primary → FLOATLIT | INTLIT | IDENT
struct Primary : Statement2 
{
  Token type;
  string value;
  unique_ptr<Statement2> parenthesizedValue;

  void print_tree(ostream& os, string prefix, bool last) override{
    ast_line(os, prefix, last, "Primary");
    string child_prefix = prefix + (last ? "    " : "│   ");
    if(type == OPENPAREN) {
      ast_line(os, child_prefix, true, "(" + value + ")");
    } else {
      ast_line(os, child_prefix, true, type + ": " + value);
    }
  
  }

  variant<int, double> interpret()override{
    if(type == OPENPAREN) {
      return parenthesizedValue->interpret();
    } else if(type  == FLOATLIT) {
      return stod(value);
    } else if (type == INTLIT) {
      return stoi(value);
    } else if (type == IDENT) {
      return symbolTable[value];
    } else {
      throw runtime_error("Not FLOATLIT, INTLIT, or IDENT");
    }
  };
};

//factor → [ MINUS | NOT ] primary
//factor → [ MINUS ] primary
struct Factor : Statement2 
{
  Token type;
  unique_ptr<Primary> value;
  
  void print_tree(ostream& os, string prefix, bool last) override{
    ast_line(os, prefix, last, "Factor:");
    string child_prefix = prefix + (last ? "    " : "│   ");
    ast_line(os, child_prefix, true, (type == MINUS || type == TOK_NOT) ? "-" : "" + value->value);
  }

  variant<int, double> interpret()override{
    auto val = value->interpret();

    if(type == MINUS) {
      visit([&](auto&& value) { value = -value;}, val);
      return val;
    } else if (type == TOK_NOT) {
      return isTrue(val) ? 0 : 1;
    } else {
        return val;
      }
  };
};

//term → factor { ( MULTIPLY | DIVIDE | MOD | AND ) factor}
//term → factor { ( MULTIPLY | DIVIDE | MOD ) factor }
struct Term : Statement2 
{
  vector<unique_ptr<Factor>> factors;
  vector<Token> types;

  void print_tree(ostream& os, string prefix, bool last) override{
      ast_line(os, prefix, last, "Term");
      string child_prefix = prefix + (last ? "    " : "│   ");

      for(auto i =  0; i < (int)factors.size(); i++) {
        if(i > 0) {
          ast_line(os, child_prefix, false, "Operator: " + types[i-1]);
        }
        bool isLast = (i == (int)factors.size() - 1);
        factors[i]->print_tree(os, child_prefix, isLast);
      }

  }

  variant<int, double> interpret()override{
    if(factors.empty()) {
        throw runtime_error("No factors in term");
    }

    auto result = factors[0]->interpret();

    for(auto i = 0; i < (int)types.size(); i++) {
      auto next = factors[i+1]->interpret();
      if(types[i] == MULTIPLY) {
        if(holds_alternative<double>(result) || holds_alternative<double>(next)) {
          result = as_double(result) * as_double(next);
        } else {
          result = get<int>(result) * get<int>(next);
        }
        
      } else if(types[i] == DIVIDE) {
        if(holds_alternative<double>(result) || holds_alternative<double>(next)) {
          result = as_double(result) / as_double(next);
        } else {
          result = get<int>(result) / get<int>(next);
        }
        
      } else if(types[i] == MOD) {
        if(holds_alternative<double>(result) || holds_alternative<double>(next)) {
          throw runtime_error("MOD: At least one is double.");
        } else {
          result = get<int>(result) % get<int>(next);
        }
        
      } else if(types[i] == TOK_AND) {
        result = (isTrue(result) && isTrue(next)) ? 1 : 0;
      }
      
    }
    return result;
  
  }  

};

//value → term { ( PLUS | MINUS | OR ) term }
//value → term { ( PLUS | MINUS ) term }
struct Value : Statement2 
{
  vector<unique_ptr<Term>> terms;
  vector<Token> types;

    void print_tree(ostream& os, string prefix, bool last) override{
        ast_line(os, prefix, last, "Value");
        string child_prefix = prefix + (last ? "    " : "│   ");
        
        for(auto i =  0; i < (int)terms.size(); i++) {
          if(i > 0) {
            ast_line(os, child_prefix, false, "Operator: " + types[i-1]);
          }
          bool isLast = (i == (int)terms.size() - 1);
          terms[i]->print_tree(os, child_prefix, isLast);
        }
    }

  variant<int, double> interpret()override{
    if(terms.empty()) {
        throw runtime_error("No terms in Value.");
    }

    auto result = terms[0]->interpret();

    for(auto i = 0; i < (int)types.size(); i++) {
      auto next = terms[i+1]->interpret();

      if(types[i] == PLUS) {
        if(holds_alternative<double>(result) || holds_alternative<double>(next)) {
          result = as_double(result) + as_double(next);
        } else {
          result = get<int>(result) + get<int>(next);
        }
        
      } else if(types[i] == MINUS) {
        if(holds_alternative<double>(result) || holds_alternative<double>(next)) {
          result = as_double(result) - as_double(next);
        } else {
          result = get<int>(result) - get<int>(next);
        }
      } else if(types[i] == TOK_OR) {
        result = (isTrue(result) || isTrue(next)) ? 1 : 0;
      }
    }
    return result;
  }
};

//expression → value [ relOp value ]
struct Expression : Statement2 {
  unique_ptr<Value> left;
  unique_ptr<RelOp> relOp;        
  unique_ptr<Value> right;     

  void print_tree(ostream& os, string prefix, bool last) override {
    ast_line(os, prefix, last, "Expression");
    string child_prefix = prefix + (last ? "    " : "│   ");

    if (right) {
      left->print_tree(os, child_prefix, false);
      relOp->print_tree(os, child_prefix, false);
      right->print_tree(os, child_prefix, true);
    } else {
        left->print_tree(os, child_prefix, true);
    }
  }

  variant<int,double> interpret() override {
    auto l = left->interpret();

    if (!right) return l;

    auto r = right->interpret();

    Token op = relOp->type;

    if (op == LESSTHAN) {
      return as_double(l) < as_double(r) ? 1 : 0;
    }
    if (op == GREATERTHAN) {
      return as_double(l) > as_double(r) ? 1 : 0;
    } 
    if (op == EQUALTO)  {
      return fabs(as_double(l) - as_double(r)) < EPSILON ? 1 : 0;
    }
    if (op == NOTEQUALTO) {
      return fabs(as_double(l) - as_double(r)) >= EPSILON ? 1 : 0;
    }
    throw runtime_error("Not <=, >=, ==, or <>");
  }
};

//if → IF expression THEN statement [ ELSE statement ]
struct If : Statement {
  unique_ptr<Expression> expression;
  unique_ptr<Statement> thenStmt;
  unique_ptr<Statement> elseStmt;

  void print_tree(ostream& os,string prefix,bool last) override {
    ast_line(os,prefix,last,"If");
  }

  void interpret(ostream& out) override {
    if(isTrue(expression->interpret())) {
      thenStmt->interpret(out);
    } else if(elseStmt) {
      elseStmt->interpret(out);
    }
  }

};

//while → WHILE expression statement
struct While : Statement {
  unique_ptr<Expression> expression;
  unique_ptr<Statement> stmt;

  void print_tree(ostream& os,string prefix,bool last) override {
    ast_line(os,prefix,last,"While");
  }

  void interpret(ostream& out) override {
    while(isTrue(expression->interpret())) {
      stmt->interpret(out);
    }
  }
};

struct Custom : Statement 
{
  void print_tree(ostream& os, string prefix, bool last) override{
    ast_line(os, prefix, last, "Custom");
    string child_prefix = prefix + (last ? "    " : "│   ");
  }   

  void interpret(ostream& out)override{ 
    (void)out;
    throw runtime_error("Why would you do that?");
    return;
  }  

};

//assign → IDENT ( ASSIGN | CUSTOM_OPER ) value
//assign → IDENT ( ASSIGN | CUSTOM_OPERATORS ) value
//assign → IDENT ASSIGN value
struct Assign : Statement
{
  Token type;
  string id;
  unique_ptr<Value> value;

  // Member Function to Print
  void print_tree(ostream& os, string prefix, bool last) override{
    ast_line(os, prefix, last, "Assign:");
    string child_prefix = prefix + (last ? "    " : "│   ");
    ast_line(os, child_prefix, false, "Target: " + id);
    ast_line(os, child_prefix, false, "Operator: " + type);
    value->print_tree(os, child_prefix, true);
  }

  // Member Function to Interpret
  void interpret(ostream& out)override{

    (void)out;
    auto& lhs = symbolTable[id];
    auto rhs = value->interpret();

    if(type == ASSIGN) {
      visit([&](auto& slot) {
        using T = decay_t<decltype(slot)>;
        visit([&](auto r){ slot = static_cast<T>(r); }, rhs);

      }, lhs);
    } else {

      visit([&](auto& slot) {
        using T = decay_t<decltype(slot)>;
        visit([&](auto r) {
          T rhsVal = static_cast<T>(r);
          if (type == SELF_PLUS) {
            slot += rhsVal;
          } else if (type == SELF_MINUS) {
            slot -= rhsVal;
          } else if (type == SELF_MULTIPLY) {
            slot *= rhsVal;
          } else if (type == SELF_DIVIDE) {
            slot /= rhsVal;
          } else if (type == SELF_MOD) {
            // MOD only works with integers
            if constexpr (is_same_v<T, int>) {
              slot %= rhsVal;
            } else {
              throw runtime_error("Not int for MOD");
            }
          }
        }, rhs);
      }, lhs);
    }

  };
};

//read → READ OPENPAREN IDENT CLOSEPAREN
struct Read : Statement
{
  string target;

  // Member Function to Print
  void print_tree(ostream& os, string prefix, bool last) override{
    ast_line(os, prefix, last, "Read Statement");
    string child_prefix = prefix + (last ? "    " : "│   ");
    ast_line(os, child_prefix, true, "Target: " + target);
  }

  // Member Function to Interpret
  void interpret(ostream& out) override{
    (void)out;
    auto it = symbolTable.find(target);
    visit([&](auto& value) { cin >> value; }, it->second);
  };
};

//write → WRITE OPENPAREN ( IDENT | STRINGLIT ) CLOSEPAREN
struct Write : Statement
{
  string content;
  Token type;

  // Member Function to Print
  void print_tree(ostream& os, string prefix, bool last) override{
    ast_line(os, prefix, last, "Write Statement");
    string child_prefix = prefix + (last ? "    " : "│   ");
    ast_line(os, child_prefix, true, "Output: " + content);
  }

  // Member Function to Interpret
  void interpret(ostream& out)override{
    if(type == IDENT) {
      auto it = symbolTable.find(content);
      visit([&out](auto&& value){ out << value << endl; }, it->second);
    } else{
      out << content << endl;
    }
  };
};

//compound → TOK_BEGIN statement { SEMICOLON statement } END
struct Compound : Statement
{
  // TODO: Declare Any Member Variables
  vector<unique_ptr<Statement>> statements;

  // Member Function to Print
  void print_tree(ostream& os,string prefix,bool last)override{
    // TODO: Finish this function
    ast_line(os, prefix, last, "Compound Statement");
    string child_prefix = prefix + (last ? "    " : "│   ");
    for(auto i = 0; i < (int)statements.size(); i++) {
      if(i == (int)statements.size() - 1) {
        statements[i]->print_tree(os, child_prefix, true);
      } else {
        statements[i]->print_tree(os, child_prefix, false);
      }
    }
    
  };

  // Member Function to Interpret
  void interpret(ostream& out)override{
    // TODO: Finish this function
    for(auto i = 0; i < (int)statements.size(); i++) {
      if(statements[i]) statements[i]->interpret(out); 
    }
  };
};

//block → [ VAR IDENT COLON ( REAL | INTEGER ) SEMICOLON { IDENT COLON ( REAL | INTEGER ) SEMICOLON } ] compound
struct Block
{
  // TODO: Declare Any Member Variables
  unique_ptr<Compound> compound;

  // Member Function to Print
  void print_tree(ostream& os,string prefix,bool last){
    // TODO: Finish this function
    ast_line(os, prefix, last, "Block");
    string child_prefix = prefix + (last ? "    " : "│   ");
    if (compound) compound->print_tree(os, child_prefix, true);
    else 
    { 
      ast_line(os, child_prefix, true, "Compound Statement");
      ast_line(os, child_prefix + "    ", true, "empty");
    }
  };

  // Member Function to Interpret
  void interpret(ostream& out){
    // TODO: Finish this function
    if (compound) compound->interpret(out); 
  };
};

// You do not need to edit this struct, but can if you choose
//program → PROGRAM IDENT SEMICOLON block
struct Program 
{
  // Member Variables
  string name; 
  unique_ptr<Block> block;

  // Member Functions
  void print_tree(ostream& os)
  {
    cout << "Program\n";
    ast_line(os, "", false, "name: " + name);
    if (block) block->print_tree(os, "", true);
    else 
    { 
      ast_line(os, "", true, "Block"); 
      ast_line(os, "    ", true, "(empty)");
    }
  }

  void interpret(ostream& out) 
  { 
    if (block) block->interpret(out); 
  }
};