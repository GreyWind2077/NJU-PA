/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>


enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NUM, // 10, 16
  TK_REG,
  TK_VAR,


};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"-", '-'},
  {"\\(", '('},
  {"\\)", ')'},
  {"\\*", '*'},
  {"/", '/'},

  {"[0-9]+", TK_NUM},
  {"\\$\\w+", TK_REG},
  {"[a-Az-z_]\\w*", TK_VAR}

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {

      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if (rules[i].token_type == TK_NOTYPE) break;
        // add to token
        tokens[nr_token].type = rules[i].token_type;

        switch (rules[i].token_type) {
          case TK_NUM:
          case TK_REG:
          case TK_VAR:
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';

          //default: TODO();
        }
        nr_token++;

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


bool check_parentheses(int p, int q) {

  if (tokens[p].type == '(' && tokens[q].type == ')') {
    int par = 0;
    for (int i = 0; i <= q; i++) {
      if (tokens[i].type == '(') par++;
      else if (tokens[i].type == ')') par--;

      if (par == 0) return i == q;
    }
  }
  return false;
}


int get_position(int p, int q) {
  int  ret = -1, par = 0, op_type = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_NUM) {
      continue;
    }

    if (tokens[i].type == '(') {
      par++;
    } else if (tokens[i].type == ')') {
      if (par == 0) {
        return -1;
      }
      par--;
    } else if (par > 0) {
      continue;
    } else {
      int type = 0;
      switch (tokens[i].type)
      {
      case '*':
      case '/':
        type = 1;
        break;
      case '+':
      case '-':
        type = 2;
        break;
      default:
        assert(0);
      }
      if (type >= op_type) {
        op_type = type;
        ret = i;
      }

    }
  }


  if (par != 0) return -1;

  return ret;

}

word_t eval(int p, int q, bool *success) {
  *success = true;
  if (p > q) {
    *success = false;
    return 0;
  } else if (p == q) {
    if (tokens[p].type != TK_NUM) {
      *success = false;
      return 0;
    }
    word_t ret = strtol(tokens[p].str, NULL, 10);
    return ret;
  } else if (check_parentheses(p, q)) {
    return eval(p + 1, q -1, success);
  } else {
    int op = get_position(p, q);
    if (op < 0) {
      *success = false;
      return 0;
    }
    word_t val1 = eval(p, op - 1, success);
    if (!*success) return 0;
    word_t val2 = eval(op + 1, q, success);
    if (!*success) return 0;
    
    switch (tokens[op].type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/': 
      if (val2 == 0) {
        *success = false;
        return 0;
      }
      return (sword_t)val1 / (sword_t)val2;
    
    default:
      assert(0);
    
  }
}



__attribute__((unused)) word_t expr(char *e, bool *success){
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  return eval(0, nr_token - 1, success);
  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
}
}
