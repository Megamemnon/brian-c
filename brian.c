/*********************************
* Brian
* Copyright (c) 2023 Brian O'Dell
*
**********************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#define DEBUG
#define MAX_TOKENS 1024
#define MAX_IDENTIFIER_LENGTH 30
#define TEMP_BUFFER_SIZE 1000

/***********************************************
 * Structs and Globals
************************************************/

typedef enum {
  BINARYOP, VARIABLE, CONSTANT, IMPLY, QUOTED,
  PAREN, BRACKET, CURLY, END
} termtype;

typedef struct TOKENNODE
{
  char *identifier;
  termtype type;
  struct TOKENNODE *next;
} tokennode;

typedef struct ASTNODE
{
  int serial;
  char *identifier;
  termtype type;
  struct ASTNODE *left;
  struct ASTNODE *right;
} astnode;

typedef struct STATEMENTNODE{
  astnode *statement;
  struct STATEMENTNODE *next;
} statementnode;

typedef struct UNIFIER{
  astnode *var;
  astnode *term;
  struct UNIFIER *next;
} unifier;

typedef struct MATCHEDNODE{
  astnode *node;
  unifier *unifiers;
  struct MATCHEDNODE *next;
} matchednode;

char *memfile=NULL;
tokennode *tokens[MAX_TOKENS];
int tokenindex=0;
tokennode *postfix[MAX_TOKENS];
int postfixindex=0;
tokennode *ops[MAX_TOKENS];
int opsindex=0;
astnode *connectives[MAX_TOKENS];
int connectivesindex=0;
astnode *output[MAX_TOKENS];
int outputindex=0;
statementnode *rules;
statementnode *program;

/*************************************************
 * Node Creators and Destroyers
**************************************************/

tokennode *createToken(char *identifier, termtype type){
  tokennode *t=malloc(sizeof(tokennode));
  t->type=type;
  t->identifier=malloc(strlen(identifier)+1);
  strcpy(t->identifier, identifier);
  t->next=NULL;
  return t;
}

void freeToken(tokennode *token){
  if(!token) return;
  tokennode *t=token->next;
  free(token);
  while(t){
    token=t;
    t=token->next;
    free(token);
  }
}

astnode *createAST(char *identifier, termtype type, int serial){
  astnode *a=malloc(sizeof(astnode));
  a->type=type;
  a->identifier=malloc(strlen(identifier)+1);
  strcpy(a->identifier, identifier);
  a->serial=serial;
  a->left=NULL;
  a->right=NULL;
  return a;
}

void freeAST(astnode *node){
  if(!node) return;
  if(node->left) freeAST(node->left);
  if(node->right) freeAST(node->right);
  free(node);
}

statementnode *createStatement(astnode *stmnt){
  statementnode *s=malloc(sizeof(statementnode));
  s->statement=stmnt;
  s->next=NULL;
  return s;
}

void freeStatement(statementnode *stmnt){
  if(!stmnt) return;
  statementnode *s=stmnt->next;
  free(stmnt);
  while(s){
    stmnt=s;
    s=stmnt->next;
    free(stmnt);
  }
}

unifier *createUnifier(astnode *term, astnode *var){
  unifier *u=malloc(sizeof(unifier));
  u->var=var,
  u->term=term;
  u->next=NULL;
  return u;
}

void freeUnifier(unifier *u){
  if(!u) return;
  unifier *u1=u->next;
  free(u);
  while(u1){
    u=u1;
    u1=u->next;
    free(u);
  }
}

matchednode *createMatchedNode(astnode *node, unifier *u){
  matchednode *m=malloc(sizeof(matchednode));
  m->node=node;
  m->unifiers=u;
  return m;
}

void freeMatchedNode(matchednode *m){
  if(!m) return;
  matchednode *m1=m->next;
  freeUnifier(m->unifiers);
  free(m);
  while(m1){
    m=m1;
    m1=m->next;
    free(m);
  }
}
/*************************************************
 * Stack and List Operations
**************************************************/

void appendToken(tokennode *token){
  tokens[tokenindex++]=token;
}

tokennode *popToken(){
  return tokens[--tokenindex];
}

void appendPostfix(tokennode *token){
  postfix[postfixindex++]=token;
}

tokennode *popPostfix(){
  return postfix[--postfixindex];
}

void appendOps(tokennode *token){
  ops[opsindex++]=token;
}

tokennode *popOps(){
  return ops[--opsindex];
}

void appendOutput(astnode *ast){
  output[outputindex++]=ast;
}

astnode *popOutput(){
  return output[--outputindex];
}

void appendConnective(astnode *ast){
  connectives[connectivesindex++]=ast;
}

astnode *popConnective(){
  return connectives[--connectivesindex];
}

void appendProgram(statementnode *prog){
  if(program==NULL){
    program=prog;
  }
  else {
    statementnode *p=program;
    while(p->next!=NULL){
      p=p->next;
    }
    p->next=prog;
  }
}

void appendRule(statementnode *rule){
  if(rules==NULL){
    rules=rule;
  }
  else {
    statementnode *r=rules;
    while(r->next!=NULL){
      r=r->next;
    }
    r->next=rule;
  }
}
/**********************************************
 * Abstract Syntax Tree
***********************************************/

astnode *copydeepASTNode(astnode *node){
  astnode *copy = createAST(node->identifier, node->type, node->serial);
  if(node->left) copy->left=copydeepASTNode(node->left);
  if(node->right) copy->left=copydeepASTNode(node->right);
  return copy;  
}

void replaceVariable(astnode *term, unifier *u){
  if(term->left){
    if(!strcmp(term->left->identifier, u->var->identifier)){
      term->left=copydeepASTNode(u->term);
    }
  }
  if(term->right){
    if(!strcmp(term->right->identifier, u->var->identifier)){
      term->right=copydeepASTNode(u->term);
    }
  }
}

bool equivalent(astnode *term, astnode *rulehead){
  bool left=true;
  bool right=true;
  if(rulehead->type==VARIABLE) return true;
  if(term->type!=rulehead->type) return false;
  if(strcmp(term->identifier,rulehead->identifier)) return false;
  if(term->left!=NULL){
    if(rulehead->left==NULL) return false;
    left=equivalent(term->left, rulehead->left);
  }
  else if(rulehead->left!=NULL){
    return false;
  }
  if(term->right!=NULL){
    if(rulehead->right==NULL) return false;
    right=equivalent(term->right, rulehead->right);
  }
  else if(rulehead->right!=NULL){
    return false;
  }
  return left && right;
}

unifier *unify(astnode *term, astnode *rulenode){
  if(rulenode->type==VARIABLE) return(createUnifier(term, rulenode));
  if(strcmp(term->identifier, rulenode->identifier)) return NULL;
  if(term->type!=rulenode->type) return NULL;
  unifier *u=NULL;
  if(term->left){
    if(!rulenode->left) return NULL;
    unifier *left=unify(term->left, rulenode->left);
    if(left) u=left;
  }
  else if(rulenode->left){
    return NULL;
  }
  if(term->right){
    if(!rulenode->right) return NULL;
    unifier *right=unify(term->right, rulenode->right);
    if(right){
      if(u){
        unifier *u1=u;
        while(u1->next){
          u1=u1->next;
        }
        u1->next=right;
      }
      else{
        u=right;
      }
    }
  }
  else if(rulenode->right){
    return NULL;
  }
}

matchednode *resolve(astnode *term, astnode *rulehead){
  matchednode *m=NULL;
  if(equivalent(term, rulehead)){
    unifier *u=unify(term, rulehead);
    if(u){
      m=createMatchedNode(term, u);
    }
  }
  if(term->left){
    matchednode *l=resolve(term->left, rulehead);
    if(l){
      if(m){
        m->next=l;
      }
      else{
        m=l;
      }
    }
  }
  if(term->right){
    matchednode *r=resolve(term->right, rulehead);
    if(r){
      if(m){
        matchednode *m1=m;
        while(m1->next){
          m1=m1->next;
        }
        m1->next=r;
      }
      else{
        m=r;
      }
    }
  }
  return m;
}

char *getFormula(astnode *ast, bool paren){
  char begin[2]="\0\0";
  char end[2]="\0\0";
  char *left=NULL;
  char *mid=NULL;
  char *right=NULL;
  switch (ast->type)
  {
  case BINARYOP:
  case IMPLY:
    if(paren && ast->identifier[0]!=','){
      begin[0]='(';
    }
    left=getFormula(ast->left, true);
    mid=ast->identifier;
    right=getFormula(ast->right, true);
    if(paren && ast->identifier[0]!=','){
      end[0]=')';
    }
    break;
  case VARIABLE:
  case CONSTANT:
    mid=ast->identifier;
    break;
  case BRACKET:
    begin[0]='[';
    right=getFormula(ast->right, false);
    end[0]=']';
    break;
  case CURLY:
    begin[0]='{';
    right=getFormula(ast->right, false);
    end[0]='}';
    break;
  
  default:
    break;
  }

  int len=1;
  if(begin[0]!=0) len+=1;
  if(left!=NULL) len+=strlen(left);
  len+=strlen(ast->identifier);
  if(right!=NULL) len+=strlen(right);
  if(end[0]!=0) len+=1;
  char *formula=(char *) calloc(1, len);
  if(begin[0]!=0) strcat(formula, begin);
  if(left!=NULL) strcat(formula, left);
  if(mid!=NULL) strcat(formula, mid);
  if(right!=NULL) strcat(formula, right);
  if(end[0]!=0) strcat(formula, end);
  return formula;
}

void astTokens(){
  int i=0;
  int nodecounter=0;
  while(i<postfixindex){
    tokennode *tnode=postfix[i];
    astnode *ast=NULL;
    switch (tnode->type)
    {
    case BRACKET:
      if(tnode->identifier[0]==']'){
        astnode *temp[MAX_TOKENS];
        int tempi=0;
        astnode *right=popOutput();
        while(right->identifier[0]!='['){
          if(connectivesindex>0){
            ast=popConnective();
            ast->right=right;
            temp[tempi++]=ast;
          }
          right=popOutput();
        }
        for(int tempj=0;tempj<tempi;tempj++){
          appendOutput(temp[tempj]);
        }
      }
      else {
        ast=createAST(tnode->identifier, tnode->type, nodecounter++);
        appendOutput(ast);
        appendConnective(ast);
      }
      break;
    case CURLY:
      if(tnode->identifier[0]=='}'){
        astnode *temp[MAX_TOKENS];
        int tempi=0;
        astnode *right=popOutput();
        while(right->identifier[0]!='{'){
          if(connectivesindex>0){
            ast=popConnective();
            ast->right=right;
            temp[tempi++]=ast;
          }
          right=popOutput();
        }
        for(int tempj=0;tempj<tempi;tempj++){
          appendOutput(temp[tempj]);
        }
      }
      else {
        ast=createAST(tnode->identifier, tnode->type, nodecounter++);
        appendOutput(ast);
        appendConnective(ast);
      }
      break;
    case CONSTANT:
    case VARIABLE:
      ast=createAST(tnode->identifier, tnode->type, nodecounter++);
      appendOutput(ast);
      break;
    case BINARYOP:
    case IMPLY:
      astnode *right=popOutput();
      astnode *left=popOutput();
      ast=createAST(tnode->identifier, tnode->type, nodecounter++);
      ast->right=right;
      ast->left=left;
      appendOutput(ast);
      break;

    case END:
      ast=popOutput();
      statementnode *p=createStatement(ast);
      appendProgram(p);
      break;
    
    default:
      break;
    }
    //freeToken(tnode);
    i++;
  }
}

void postfixTokens(){
  int t=0;
  while(t<tokenindex){
    tokennode *tnode=tokens[t];
    switch (tnode->type)
    {
    case PAREN:
      if(tnode->identifier[0]==')'){
        tokennode *op=popOps();
        while(op->identifier[0]!='('){
          appendPostfix(op);
          op=popOps();
        }
        // freeToken(op);
        appendPostfix(tnode);
      }
      else {
        appendPostfix(tnode);
        appendOps(tnode);
      }
      break;
    case BRACKET:
      if(tnode->identifier[0]==']'){
        tokennode *op=popOps();
        while(op->identifier[0]!='['){
          appendPostfix(op);
          op=popOps();
        }
        // freeToken(op);
        appendPostfix(tnode);
      }
      else {
        appendPostfix(tnode);
        appendOps(tnode);
      }
      break;
    case CURLY:
      if(tnode->identifier[0]=='}'){
        tokennode *op=popOps();
        while(op->identifier[0]!='{'){
          appendPostfix(op);
          op=popOps();
        }
        // freeToken(op);
        appendPostfix(tnode);
      }
      else {
        appendPostfix(tnode);
        appendOps(tnode);
      }
      break;
    case VARIABLE:
    case CONSTANT:
    case QUOTED:
      appendPostfix(tnode);
      break;
    case BINARYOP:
    case IMPLY:
      if(opsindex>0){
        if(tnode->identifier[0]==','){
          tokennode *op=popOps();
          if(op->identifier[0]==','){
            // comma is right associative
            appendOps(op);
          }
          else {
            while((op->type!=PAREN || op->identifier[0]!='(')
            && (op->type!=BRACKET || op->identifier[0]!='[')
            && (op->type!=CURLY || op->identifier[0]!='{') 
            && opsindex>0){
              appendPostfix(op);
              op=popOps();
            }
            if((op->type==PAREN && op->identifier[0]=='(')
            || (op->type==BRACKET && op->identifier[0]=='[')
            || (op->type==CURLY && op->identifier[0]=='{') ){
              appendOps(op);
            }
          }
        }
        else {
          // handle all left associative binary operators
          tokennode *op=popOps();
          if((op->type!=PAREN || op->identifier[0]!='(')
          && (op->type!=BRACKET || op->identifier[0]!='[')
          && (op->type!=CURLY || op->identifier[0]!='{') ){
            appendPostfix(op);
          }
          else {
            appendOps(op);
          }        
        }
      }
      appendOps(tnode);        
      break;
    case END:
      while(opsindex>0){
        appendPostfix(popOps());
      }
      appendPostfix(tnode);
      break;
    
    default:
      break;
    }
    t++;
  }
// replace quoted strings with comma-delimited lists of characters
// comma is the only right-associative binary operator to force 
// head/tail functionality to behave as expected
  tokennode *temp[MAX_TOKENS];
  int tempi=0;
  int pfi=0;
  while(pfi<postfixindex){
    tokennode *node=postfix[pfi++];
    if(node->type==QUOTED){
      tokennode *brack=createToken("[",BRACKET);
      temp[tempi++]=brack;
      for(int j=1;j<strlen(node->identifier)-1;j++){
        char tc[2]="\0\0";
        tc[0]=node->identifier[j];
        tokennode *character=createToken(tc, CONSTANT);
        temp[tempi++]=character;
      }
      for(int j=1;j<strlen(node->identifier)-2;j++){
        tokennode *comma=createToken(",",BINARYOP);
        temp[tempi++]=comma;
      }
      temp[tempi++]=createToken("]",BRACKET);
    }
    else {
      temp[tempi++]=node;
    }
  }
  for(int tempj=0;tempj<tempi;tempj++){
    postfix[tempj]=temp[tempj];
  }
  postfixindex=tempi;


  astTokens();
}

void tokenizeMemFile(long memfilelength){
  int i=0;
  bool inword=false;
  bool inop=false;
  char identifier[MAX_IDENTIFIER_LENGTH+1];
  int identindex=0;
  termtype wordtype;
  tokennode *tnode=NULL;
  while(i<memfilelength){
    char c=memfile[i];
    char nc='\0';
    if(i+1<memfilelength){
      nc=memfile[i+1];
    }
    if(c==' ' || c=='\n' || c=='\t' || c=='(' || c==')' 
     || c=='[' || c==']' || c=='{' || c=='}' || c==',' 
     || c=='.' || c=='"' || c=='#'){
      if(inword){
        identifier[identindex]=0;
        tnode=createToken(identifier, wordtype);
        appendToken(tnode);
        inword=false;
      } 
      if(inop){
        identifier[identindex]=0;
        tnode=createToken(identifier, BINARYOP);
        appendToken(tnode);
        inop=false;
      }
    }
    if(c=='(' || c==')'){
      char id[2];
      id[0]=c;
      id[1]='\0';
      tnode=createToken(id, PAREN);
      appendToken(tnode);
    }
    else if(c=='[' || c==']'){
      char id[2];
      id[0]=c;
      id[1]='\0';
      tnode=createToken(id, BRACKET);      
      appendToken(tnode);
    }
    else if(c=='{' || c=='}'){
      char id[2];
      id[0]=c;
      id[1]='\0';
      tnode=createToken(id, CURLY);      
      appendToken(tnode);
    }
    else if(c==','){
      char id[2];
      id[0]=c;
      id[1]='\0';
      tnode=createToken(id, BINARYOP);      
      appendToken(tnode);
    }
    else if(c=='-' && nc=='>'){
      tnode=createToken("->", IMPLY);
      appendToken(tnode);
      i++;
    }
    else if(c=='#'){
      while(c!='\n' && i<memfilelength) c=memfile[++i];
    }
    else if(c=='"'){
      char buffer[TEMP_BUFFER_SIZE];
      int bi=0;
      int quotecount=0;
      while(quotecount<2){
        buffer[bi++]=c;
        if(c=='"') quotecount++;
        if(i+1>=memfilelength){
          buffer[bi++]='"';
          buffer[bi]=0;
        }
        else {
          c=memfile[++i];
        }
      }
      i--;
      tnode=createToken(buffer, QUOTED);
      appendToken(tnode);
    }
    else if(c=='.'){
      char id[2];
      id[0]=c;
      id[1]='\0';
      tnode=createToken(id, END);      
      appendToken(tnode);
      postfixTokens();
      for(int t=0;t<tokenindex;t++){
        freeToken(tokens[t]);
      }
      tokenindex=0;
      postfixindex=0;
      opsindex=0;
      outputindex=0;
      connectivesindex=0;
    }
    else if(isalnum(c)){
      if(inword){
        identifier[identindex++]=c;
      }
      else{
        if(inop){
          identifier[identindex]=0;
          tnode=createToken(identifier, BINARYOP);
          appendToken(tnode);
          inop=false;
        }
        inword=true;
        if(isupper(c)){
          wordtype=VARIABLE;
        }
        else {
          wordtype=CONSTANT;
        }
        identindex=0;
        identifier[identindex++]=c;
      }
    }
    else if(c!=' ' && c!='\n' && c!='\t'){
      if(inop){
        identifier[identindex++]=c;
      }
      else {
        if(inword){
          identifier[identindex]=0;
          tnode=createToken(identifier, wordtype);
          appendToken(tnode);
          inword=false;
        }
        inop=true;
        identindex=0;
        identifier[identindex++]=c;
      }
    }
    i++;
  }
  // postfixTokens();
}

/*********************************************************
 * FILE I/O 
**********************************************************/
long getFileSize(const char *pathname){
  long sz;
  FILE *f;

  f = fopen(pathname, "r");
  if(f == NULL) return 0;
  fseek(f, 0L, SEEK_END);
  sz = ftell(f);
  fclose(f);
  return sz;
}

char *loadMemFile(const char *pathname){
  char c;
  long sz = getFileSize(pathname);
  long i = 0;
  memfile = malloc(sz+1);
  if(memfile == NULL) return NULL;
  FILE *f = fopen(pathname, "r");
  if(f == NULL) return NULL;
  do
  {
    c = fgetc(f);
    memfile[i++] = c;
  } while (c != EOF);
  
  fclose(f);
  tokenizeMemFile(sz);
}

void runProgram(){
  statementnode *stmnt=program;
  while(stmnt!=NULL){
    astnode *prog=stmnt->statement;
    if(prog){
      // put Rules in the Rules list
      if(!strcmp(prog->identifier,"->")){
        statementnode *newstmnt=createStatement(prog);
        appendRule(newstmnt);
      }
    }
    else{
      // reduce program line
      bool changed=true;
      while(changed){
        changed=false;
        statementnode *rulelist=rules;
        while(rulelist!=NULL){
          astnode *rule=rulelist->statement;
          if(rule){
            astnode *rulehead=rule->left;
            if(rulehead){
              matchednode *mn=resolve(prog, rulehead);
              if(mn){
                matchednode *mnx=mn;
                while(mnx){
                  astnode *rulebody=copydeepASTNode(rule->right);
                  unifier *u=mnx->unifiers;
                  while(u){
                    if(!strcmp(rulehead->identifier, u->var->identifier)){
                      rulehead=copydeepASTNode(u->term);
                    }
                    else{
                      replaceVariable(rulehead, u);
                    }
                    u=u->next;
                  }
                  mnx=mnx->next;
                }
              }
            }
          }
          rulelist=rulelist->next;
        }
      }
    }
    stmnt=stmnt->next;
  }

}

int main(int argc, char const *argv[]){
  printf("Brian\nCopyright (c) 2023 Brian O'Dell\n\n");
#ifdef DEBUG
  loadMemFile("/home/brian/git/brian-c/test");
#else
  if(argc != 2){
    printf("usage: brian programfile\n");
    return 1;
  }
#endif
  statementnode *s=program;
  while(s!=NULL){
    char *f=getFormula(s->statement, false);
    printf("%s.\n", f);
    s=s->next;
  }
  runProgram();

}