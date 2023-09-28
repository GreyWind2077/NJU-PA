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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char *expr;
  word_t ret;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

static WP* new_wp() {
  assert(free_);
  WP *ret = free_;
  free_ = free_->next;
  ret->next = head;
  head = ret;
  return ret;
}

static void free_wp(WP *wp) {
  WP* h = head;
  if (h == wp) head = NULL;
  else {
    while (h && h->next != wp) h = h->next;
    assert(h);
    h->next = wp->next;
  }
  wp->next = free_;
  free_ = wp;
}

void wp_add(char *expr, word_t res) {
  WP* wp = new_wp();
  strcpy(wp->expr, expr);
  wp->ret = res;
  printf("watchpoint set. %d: %s\n", wp->NO, wp->expr);
}

void wp_remove(int num) {
  assert(num < NR_WP);
  WP* wp = &wp_pool[num];
  free_wp(wp);
  printf("delete watchpoint. %d: %s", wp->NO, wp->expr);
}

void wp_display() {
  WP *wp = head;
  if (wp == NULL) {
    puts("no watchpoints.");
    return;
  }
  printf("%-8s%-8s\n", "Number", "Expr");
  while (wp) {
    printf("%-8d%-8s\n", wp->NO, wp->expr);
    wp = wp->next;
  }
}

void wp_difftest() {
  WP* wp = head;
  while (wp) {
    bool success;
    word_t new = expr(wp->expr, &success);
    if (wp->ret != new) {
      printf("Watchpoint %d: %s\n"
        "Old value = %u\n"
        "New value = %u\n"
        , wp->NO, wp->expr, wp->ret, new);
      wp->ret = new;
    }
    wp = wp->next;
  }
}
