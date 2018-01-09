/* Must add:
    * structs to structs.h
    * "struct shopkeeper_info *shopinfo" to struct char_data
    * "ch->shopinfo = NULL" to db.c:clear_char()
    * free_shopkeeper code, called by db.c:free_char()
    * Some sort of flag to indicate a shopkeeper
    * code to call load_shopkeeper
    - imm command to list shopkeepers
    * list
    * buy (by name or number, 1 or more)
    * sell (by name, 1 or more)
    * value (by name)
    * "type" code...what kind of stuff the shopkeeper deals in.
    * custom messages
    * Is the shop open at night?
    * Does the shopkeeper have limited gold?
    - Will the shopkeep identify (a la pawnshop)?
    * Just make a SHOP_FLAGS int.
    * watch out for used up / partially used wands & staffs
    * check for cursed / !SELL items
    X do I have to do an inflation check?
    * OLC
    * stat_shop (called from do_stat_character)
    X invis check!
    * RICH shopkeepers shouldn't gain money when selling
    * shop money different from mob money
    - change value of partially used staffs/wands
    * memory leak in free_shopkeeper...objects aren't purged from limited_list
    * go-home code for shopkeeps (set !THERE if no go-home)
      - Actually we've got time_progs, they work fine for this.
    * custom open/close times
    * shop money loads seperate
*/

/****************************************************************************
 *  New dynamic shop code. Attaches shops to mobs in such a way that the    *
 *  shopkeeper can walk around and be killed without anything breaking, and *
 *  there's no "second storage room" to potentially screw things up         *
 *                                                                          *
 *                                               - Culvan                   *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "comm.h"
#include "handler.h"
#include "string.h"

#define DSHOP_CLOSES 	(1 << 0) /* shop closes at night 	*/
#define DSHOP_RICH 	(1 << 1) /* shop has infinite gold	*/
#define DSHOP_IDENT	(1 << 2) /* shop will identify stuff	*/
#define DSHOP_ZONEBUY   (1 << 3) /* Only buys stuff from his zone */
#define NUM_DSHOP_FLAGS 4

char *shopflags[] = {
  "CLOSES",
  "RICH",
  "IDENTS",
  "ZONEBUY"
};

/* BE SURE to change this in shopkeeper_info in structs.h as well! */
#define MSGS 24

#define MSG(k, i) ((k)->shopinfo->msgs[i])
#define DMSG(k, i, d) (MSG(k, i) ? MSG(k, i) : d)

#define MSG_BUY_WHAT(k) (DMSG(k, 0, "What do you want to buy??"))
#define MSG_HAVE_NONE(k) (DMSG(k, 1, "I haven't got that in stock -- try list."))
#define MSG_ARMS_FULL(k) (DMSG(k, 2, "You can't carry any more items!"))
#define MSG_CANT_AFFORD(k) (DMSG(k, 3, "You can't afford it!"))
#define MSG_TOO_HEAVY(k) (DMSG(k, 4, "You can't carry that much weight!"))
#define MSG_THANKS(k) (DMSG(k, 5, "Thanks!"))
#define MSG_THANKS2(k) (DMSG(k, 6, "Thanks!"))
#define MSG_SELL_WHAT(k) (DMSG(k, 7, "What do you want to sell?"))
#define MSG_GOT_NONE(k) (DMSG(k, 8, "You don't seem to have that."))
#define MSG_IM_BROKE(k) (DMSG(k, 9, "I can't afford that!"))
#define MSG_GOT_NO_MORE(k) (DMSG(k, 10, "That's all you have."))
#define MSG_THATS_ALL(k) (DMSG(k, 11, "That's all I can afford!"))
#define MSG_DONT_WANT(k) (DMSG(k, 12, "I don't want that!"))
#define MSG_CLOSED(k) (DMSG(k, 13, "I'm closed for the night!"))
#define MSG_WRONG_TYPE(k) (DMSG(k, 14, "I don't deal in that sort of thing!"))
#define MSG_NEG_BUY(k) (DMSG(k, 15, "A negative amount? Try selling me something!"))
#define MSG_NEG_SELL(k) (DMSG(k, 16, "A negative amount? Try buying something!"))
#define MSG_ONLY_HAVE(k) (DMSG(k, 17, "I only have %d to sell you!"))
#define MSG_ARMS_FULL2(k) (DMSG(k, 18, "You can only carry %d!"))
#define MSG_AFFORD(k) (DMSG(k, 19, "You can only afford %d!"))
#define MSG_WEIGHT2(k) (DMSG(k, 20, "You can only carry %d!"))
#define MSG_COST(k) (DMSG(k, 21, "That'll be %d coins."))
#define MSG_COST_ONE(k) (DMSG(k, 22, "That'll be one coin."))
#define MSG_VALUE(k) (DMSG(k, 23, "I'll give you %d coins for %s."))

char *textnames[] = {
  "buy_what",
  "i_have_none",
  "arms_full",
  "you_broke",
  "too_heavy",
  "thanks_buy",
  "thanks_sell",
  "sell_what",
  "you_have_none",
  "im_broke",
  "you_have_no_more",
  "all_i_can_buy",
  "dont_want",
  "closed",
  "wrong_kind",
  "neg_buy",
  "neg_sell",
  "_i_only_have",
  "_arms_full",
  "_you_broke",
  "_too_heavy",
  "_cost",
  "cost_one",
  "__value"
};

#define DSHOP_FLAGS(k) ((k)->shopinfo->flags)
#define DSHOP_FLAGGED(k, f) (DSHOP_FLAGS(k) & (f))

#define IS_NIGHT (weather_info.sunlight == SUN_DARK)

#if(0)
#define DSHOP_CLOSED(k) (DSHOP_FLAGGED(k, DSHOP_CLOSES) && IS_NIGHT)
// New custom hours
#define DSHOP_CLOSED(k) (DSHOP_FLAGGED(k, DSHOP_CLOSES) && \
  ((k->shopinfo->open < k->shopinfo->close) ? \
   (time_info->hours < k->shopinfo->open || \
    time_info->hours > k->shopinfo->close) : \
   (time_info->hours > k->shopinfo->close && \
    time_info->hours < k->shopinfo->open) \
  ))
#endif
#define DSHOP_CLOSED(k) (dshop_closed(k))
#define DSHOP_CHECK_CLOSED(k) if (DSHOP_CLOSED(keeper)) { \
    perform_tell(keeper, ch, MSG_CLOSED(keeper)); return; }

#define GET_SHOP_GOLD(k) ((k)->shopinfo->gold)

/* Actually this may be unneccessary, I think the libraries make this check
   automatically, but just in case.... */
#define FREE(x) if (x) free(x)

extern void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct time_info_data time_info;

bool dshop_closed(struct char_data *keeper) {
  struct shopkeeper_info *inf;
  
  if (!keeper) return TRUE;
  if (!(inf = keeper->shopinfo)) return TRUE;
  if (!DSHOP_FLAGGED(keeper, DSHOP_CLOSES)) return FALSE;
  if (inf->open < inf->close)
    return (time_info.hours < inf->open || time_info.hours > inf->close);
  return (time_info.hours < inf->open && time_info.hours > inf->close);
}

void init_new_shopkeeper(struct char_data *ch) {
  struct shopkeeper_info *info;
  int i;
  
  if (!IS_NPC(ch)) {
    log ("init_new_shopkeeper called on player character!");
    return;
  }
  CREATE(ch->shopinfo, struct shopkeeper_info, 1); info = ch->shopinfo;
  info->limited = NULL;
  info->unlimited = NULL;
  info->flags = 0;
  info->types = 0; /* Sell nothing by default */
  info->open = 7;
  info->close = 18;
  info->shop = info->home = -1;
  GET_SHOP_GOLD(ch) = GET_GOLD(ch);
  info->buy_rate = info->sell_rate = 1.0;
/*  info->buywhat = info->havenone = info->armsfull = info->ch_broke =
  info->tooheavy = info->thanks = info->sellwhat = info->thanks2 =
  info->gotnone = info->im_broke = info->gotnomore = info->thatsall =
  info->dontwant = info->closed = info->wrongtype = NULL;
  info->neg_buy = info->neg_sell = info->onlyhave = info->armsfull2 =
  info->afford = info->weight2 = info->cost = info->costone =
  info->value = NULL;*/
  for (i = 0; i < MSGS; i++) MSG(ch, i) = NULL;
  
}

void free_shopkeeper(struct char_data *ch) {
  struct shopkeeper_info *info;
  struct limited_sell_info *lim, *nlim;
  struct unlimited_sell_info *unlim, *nunlim;
  int i;
  
/*  if (!MOB_FLAGGED(ch, MOB_DSHOPKEEPER)) return;*/
  if (!ch->shopinfo) return;
  info = ch->shopinfo;
  for (lim = info->limited; lim; lim = nlim) {
    nlim = lim->next;
    if (lim->obj) extract_obj(lim->obj);
    free(lim);
  }
  for (unlim = info->unlimited; unlim; unlim = nunlim) {
    nunlim = unlim->next;
    free(unlim);
  }
  /*FREE(info->buywhat); FREE(info->havenone); FREE(info->armsfull);
  FREE(info->ch_broke); FREE(info->tooheavy); FREE(info->thanks);
  FREE(info->sellwhat); FREE(info->thanks2); FREE(info->gotnone);
  FREE(info->im_broke); FREE(info->gotnomore); FREE(info->thatsall);
  FREE(info->dontwant); FREE(info->closed); FREE(info->wrongtype);
  FREE(info->neg_buy); FREE(info->neg_sell); FREE(info->onlyhave);
  FREE(info->armsfull2); FREE(info->afford); FREE(info->weight2);
  FREE(info->cost); FREE(info->costone); FREE(info->value);*/
  
  for (i = 0; i < MSGS; i++) {
    FREE(MSG(ch, i));
  }
  
  free(ch->shopinfo);
  ch->shopinfo = NULL;
}

void shopkeeper_from_file(struct char_data *ch, FILE *fl, FILE *inv) {
/*  struct limited_sell_info **limited_ptr;
  struct unlimited_sell_info **unlimited_ptr; */
  struct limited_sell_info *limtemp;
  struct unlimited_sell_info *unlimtemp;
  int vnum, num, i;
  
/*  limited_ptr = &ch->shopinfo->limited;
  unlimited_ptr = &ch->shopinfo->unlimited; */
  
  for (i = 0; i < MSGS; i++) {
/*    fscanf(fl, "%s\n", buf);*/
    get_line(fl, buf);
    if (*buf != '-') {
      MSG(ch, i) = strdup(buf);
    }
  }
  
  fscanf(fl, "%f %f\n", &ch->shopinfo->buy_rate, &ch->shopinfo->sell_rate);
  fscanf(fl, "%d\n%d\n", &ch->shopinfo->flags, &ch->shopinfo->types);
  fscanf(fl, "%d %d\n", &ch->shopinfo->open, &ch->shopinfo->close);
  fscanf(fl, "%d\n", &ch->shopinfo->gold);
  
  for (;;) {
    fscanf(inv, "%d %d\n", &vnum, &num);
    if (vnum == -1) break;
    if (real_object(vnum) == -1) {
      sprintf(buf, "Invalid vnum %d for %s's inventory!", vnum, GET_NAME(ch));
      mudlog(buf, NRM, LVL_DEITY, TRUE);
    } else {
      if (num == -1) { /* unlimited */
/*        CREATE (*unlimited_ptr, struct unlimited_sell_info, 1);
        (*unlimited_ptr)->vnum = vnum;
        unlimited_ptr = &(*unlimited_ptr)->next; */
        CREATE(unlimtemp, struct unlimited_sell_info, 1);
        unlimtemp->vnum = vnum;
        unlimtemp->next = ch->shopinfo->unlimited;
        ch->shopinfo->unlimited = unlimtemp;
      } else {
        for (;num > 0; num--) {
/*          CREATE(*limited_ptr, struct limited_sell_info, 1);
          (*limited_ptr)->obj = read_object(vnum, VIRTUAL);
          limited_ptr = &(*limited_ptr)->next; */
          CREATE(limtemp, struct limited_sell_info, 1);
          limtemp->obj = read_object(vnum, VIRTUAL);
          limtemp->next = ch->shopinfo->limited;
          ch->shopinfo->limited = limtemp;
        }
      }
    }
  }
}

bool save_dshopkeeper(struct char_data *ch) {
  char filename[100];
  FILE *fl;
  int i;
  
  sprintf(filename, "world/dshops/%d.shp", mob_index[ch->nr].virtual);
  if (!(fl = fopen(filename, "w"))) {
    sprintf(buf, "Error opening control file %s for shopkeeper %s!", filename, GET_NAME(ch));
    mudlog(buf, NRM, LVL_DEITY, TRUE);
    return FALSE;
  }

  for (i = 0; i < MSGS; i++) {
    fprintf(fl, "%s\n", MSG(ch, i) ? MSG(ch, i) : "-");
  }
  
  fprintf(fl, "%#1.3f %#1.3f\n", ch->shopinfo->buy_rate, ch->shopinfo->sell_rate);
  fprintf(fl, "%d\n%d\n", ch->shopinfo->flags, ch->shopinfo->types);
  fprintf(fl, "%d %d\n", ch->shopinfo->open, ch->shopinfo->close);
  fprintf(fl, "%d\n", GET_SHOP_GOLD(ch));
  
  fclose(fl);
  
  return TRUE;
}

void load_shopkeeper(struct char_data *ch) {
  FILE *fl, *ifl;
  char filename[100];
  
  if (!IS_NPC(ch)) {
    log("load_shopkeeper() : Target is a player!");
    return;
  }
  sprintf(buf, "Loading shopkeeper %s.", GET_NAME(ch));
  log(buf);
  
  sprintf(filename, "world/dshops/%d.shp", mob_index[ch->nr].virtual);
  if (!(fl = fopen(filename, "r"))) {
    sprintf(buf, "Error opening control file %s for shopkeeper %s!", filename, GET_NAME(ch));    
    mudlog(buf, NRM, LVL_DEITY, TRUE);
    return;
  }

  sprintf(filename, "world/dshops/%d.inv", mob_index[ch->nr].virtual);
  if (!(ifl = fopen(filename, "r"))) {
    sprintf(buf, "Error opening inventory file %s for shopkeeper %s!", filename, GET_NAME(ch));    
    mudlog(buf, NRM, LVL_DEITY, TRUE);
    fclose(fl);
    return;
  }
  
  init_new_shopkeeper(ch);
  shopkeeper_from_file(ch, fl, ifl);
  fclose(fl);
  fclose(ifl);
}

void sort_limited_list(struct char_data *ch) {
  struct limited_sell_info *cur, *start, *min;
  struct obj_data *temp;
  
  if (!ch->shopinfo) {
    log ("sort_limited_list: no ch->shopinfo!");
    return;
  }
  
  /* Selection sort, of course */
  for (start = ch->shopinfo->limited; start; start = start->next) {
    min = start;
    for (cur = start->next; cur; cur = cur->next) {
      if (cur->obj->item_number < min->obj->item_number) min = cur;
    }
    if (start != min) {
      temp = start->obj;
      start->obj = min->obj;
      min->obj = temp;
    }
  }
}

ACMD(do_dlist) {
  struct char_data *keeper;
  ACMD(do_pawnlist);
  int count, index, rnum;
  struct unlimited_sell_info *unlim;
  struct limited_sell_info *lim;
  extern struct obj_data *obj_proto;

/* This is unneeded, because do_dlist is only called by do_list */
/*  if (world[ch->in_room].number == PAWNSHOP_VNUM) {
    do_pawnlist(ch, argument, cmd, subcmd);
    return;
  }*/
  
  if (!ch->desc) return;
  
  for (keeper = world[ch->in_room].people; keeper; keeper = keeper->next_in_room) {
    if (IS_NPC(keeper)) if (MOB_FLAGGED(keeper, MOB_DSHOPKEEPER)) break;
  }
  
  if (!keeper) {
    send_to_char("Nothing to list, no shopkeeper!\r\n", ch);
    return;
  }
  
  DSHOP_CHECK_CLOSED(keeper);  
  
  sort_limited_list(keeper);

  strcpy(buf, 
      " ##   Available   Item                    "
      "                           Cost\r\n"
      "------------------------------------------"
      "-------------------------------\r\n");
  
  index = 1;
  
  /* FIRST, we'll do the unlimited stuff */
  for (unlim = keeper->shopinfo->unlimited; unlim; unlim = unlim->next) {
    rnum = real_object(unlim->vnum);
    sprintf(buf, "%s%3d)  Unlimited   %-40s%15d\r\n", buf, index,
        obj_proto[rnum].short_description,
        (int)(GET_OBJ_COST(&obj_proto[rnum]) * keeper->shopinfo->sell_rate));
    index++;
  }
  
  /* NOW, we'll do the limited stuff...this must be grouped */
  for (lim = keeper->shopinfo->limited; lim; lim = lim->next) {
    rnum = lim->obj->item_number;
    count = 1;
    while (lim->next) {
      if (lim->next->obj->item_number == rnum) {
        count++;
        lim = lim->next;
      } else {
        break;
      }
    }
    sprintf(buf, "%s%3d)  %5d       %-40s%15d\r\n", buf, index, count,
            lim->obj->short_description,
            (int)(GET_OBJ_COST(lim->obj) * keeper->shopinfo->sell_rate));
    index++;
  }
 
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_dbuy) {
  struct char_data *keeper;
  struct shopkeeper_info *shopinfo;
  struct unlimited_sell_info *unlim;
  struct limited_sell_info *lim, *limprev, *tmplim;
  struct obj_data *obj, *nextobj, *original;
  extern struct obj_data *obj_proto;
  int count, index, lastnum, shophas, can_hold, numsold, cost;
  
  for (keeper = world[ch->in_room].people; keeper; keeper = keeper->next_in_room) {
    if (MOB_FLAGGED(keeper, MOB_DSHOPKEEPER)) break;
  }
  
  if (!keeper) {
    send_to_char("Sorry, you can't buy stuff here, no shopkeeper!\r\n", ch);
    return;
  }
  
  if (!(shopinfo = keeper->shopinfo)) {
    sprintf(buf, "Mob #%d (%s) flagged as DSHOPKEEPER but has no shopinfo!",
            keeper->nr, GET_NAME(keeper));
    mudlog(buf, NRM, LVL_DEITY, TRUE);
    send_to_char("Sorry, there is an error in this shop. Please inform an immortal.\r\n", ch);
    return;
  }

  DSHOP_CHECK_CLOSED(keeper);  

  argument = one_argument(argument, arg);

  if (!*arg) {
    perform_tell(keeper, ch, MSG_BUY_WHAT(keeper));
    return;
  } 

  if (is_number(arg)) {
    count = atoi(arg);
    argument = one_argument(argument, arg);
  } else {
    count = 1;
  }

  if (count == 0) {
    perform_tell(keeper, ch, MSG_BUY_WHAT(keeper));
    return;
  } else if (count < 0) {
    perform_tell(keeper, ch, MSG_NEG_BUY(keeper));
    return;
  }
  
  obj = NULL;
  lim = limprev = NULL;
  unlim = NULL;
  
  if (arg[0] == '#') {
    /* check by index number */
    index = atoi(arg+1);
    /* First check if it's an unlimited item */
    for (unlim = shopinfo->unlimited; unlim; unlim = unlim->next) {
      if (index == 1) {
        /* this is it */
        break;
      } else {
        index--;
      }
    }
    if (!unlim) {
      /* Now check if it's a limited item */
      for (lim = shopinfo->limited; lim; lim = lim->next) {
        if (index == 1) {
          /* this is it */
          break;
        } else {
          if (lastnum != lim->obj->item_number) {
            index--;
            lastnum = lim->obj->item_number;
          }
        }
        limprev = lim;
      }
    }
  } else {
    /* check by object name */
    for (unlim = shopinfo->unlimited; unlim; unlim = unlim->next) {
      if (isname(arg, obj_proto[real_object(unlim->vnum)].name)) break;
    }
    if (!unlim) {
      /* check the limited list */
      for (lim = shopinfo->limited; lim; lim = lim->next) {
        if (isname(arg, lim->obj->name)) break;
        limprev = lim;
      }
    }
  }
  if (unlim) {
    /* It's an unlimited item */
    obj = read_object(unlim->vnum, VIRTUAL);
  } else {
    if (lim) {
      /* It's a limited item */
      obj = lim->obj;
    } else {
      /* We don't have any! */
      perform_tell(keeper, ch, MSG_HAVE_NONE(keeper));
      return;
    }
  }
  
  /* double-check the count */
  if (count > 1) {
    if (!unlim) {
      /* check how many the shopkeeper has */
      shophas = 1;
      lastnum = obj->item_number;
      for (tmplim = lim->next; tmplim; tmplim = tmplim->next) {
        if (tmplim->obj->item_number == lastnum) shophas++;
        else break;
      }
      if (count > shophas) {
        count = shophas;
        sprintf(buf2, MSG_ONLY_HAVE(keeper), count);
        perform_tell(keeper, ch, buf2);
      }
    }
    can_hold = CAN_CARRY_N(ch) - IS_CARRYING_N(ch);
    if (can_hold <= 0) {
      perform_tell(keeper, ch, MSG_ARMS_FULL(keeper));
      count = 0;
    } else if (can_hold < count) {
      count = can_hold;
      sprintf(buf2, MSG_ARMS_FULL2(keeper), count);
      perform_tell(keeper, ch, buf2);
    }
  }
  /* Because limited objects can actually vary in weight and/or
     value, we can't do the full cost & weight checks until we actually
     try it. */
  if ((int)(GET_OBJ_COST(obj) * shopinfo->sell_rate) > GET_GOLD(ch)) {
    perform_tell(keeper, ch, MSG_CANT_AFFORD(keeper));
    count = 0;
  }
  if (GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch) - IS_CARRYING_W(ch)) {
    perform_tell(keeper, ch, MSG_TOO_HEAVY(keeper));
    count = 0;
  }
  /* pant pant....Ok we've actually got the potential merchandise in
     obj now...if it's a limited item, it still needs to be removed from
     the list, and watch out for selling more than one! */
  numsold = cost = 0;
  original = obj;
  while (count > 0) {
    if (unlim) {
      obj_to_char(obj, ch);
      nextobj = read_object(unlim->vnum, VIRTUAL);
    } else {
      if (limprev) limprev->next = lim->next;
      else shopinfo->limited = lim->next;
      obj_to_char(obj, ch);
      free(lim);
      if (limprev)
        lim = limprev->next; /* We assume the list is sorted */
      else
        lim = shopinfo->limited;
      if (lim) nextobj = lim->obj;
    }
    numsold++;
    count--;
    GET_GOLD(ch) -= (int)(GET_OBJ_COST(obj) * shopinfo->sell_rate);
    if (!DSHOP_FLAGGED(keeper, DSHOP_RICH))
      GET_SHOP_GOLD(keeper) += (int)(GET_OBJ_COST(obj) * shopinfo->sell_rate);
    cost += (int)(GET_OBJ_COST(obj) * shopinfo->sell_rate);
    /* Can we still buy more? */
    obj = nextobj;
    if (count > 0) {
      if (GET_GOLD(ch) < (int)(GET_OBJ_COST(obj) * shopinfo->sell_rate)) {
        sprintf(buf2, MSG_AFFORD(keeper), numsold);
        perform_tell(keeper, ch, buf2);
        count = 0;
      } else if (GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch) - IS_CARRYING_W(ch)) {
        sprintf(buf2, MSG_WEIGHT2(keeper), numsold);
        perform_tell(keeper, ch, buf2);
        count = 0;
      }
    }
  }
  /* Cleanup */
  if (unlim) extract_obj(obj);
  
  /* And finally, the messages */
  if (numsold > 0) {
    if (cost == 1) {
      strcpy(buf2, MSG_COST_ONE(keeper));
    } else {
      sprintf(buf2, MSG_COST(keeper), cost);
    }
    perform_tell(keeper, ch, buf2);
    perform_tell(keeper, ch, MSG_THANKS(keeper));
    if (numsold > 1) {
      sprintf(buf, "You now have %s (x %d)\r\n", original->short_description, numsold);
    } else {
      sprintf(buf, "You now have %s.\r\n", original->short_description);
    }
    send_to_char(buf, ch);
    if (numsold > 1) {
      sprintf(buf, "$n buys $p (x %d).", numsold);
    } else {
      strcpy(buf, "$n buys $p.");
    }
    act(buf, FALSE, ch, original, NULL, TO_ROOM);
  }
}

ACMD(do_dsell) {
  struct char_data *keeper;
  struct shopkeeper_info *shopinfo;
  int count, numsold, cost, price;
  struct obj_data *obj, *nextobj;
  struct limited_sell_info *lim;
  bool found;
  extern struct index_data *obj_index;
  extern struct index_data *mob_index;
  
  for (keeper = world[ch->in_room].people; keeper; keeper = keeper->next_in_room) {
    if (MOB_FLAGGED(keeper, MOB_DSHOPKEEPER)) break;
  }
  
  if (!keeper) {
    send_to_char("Sorry but there is no shopkeeper to sell to!\r\n", ch);
    return;
  }
  
  if (!(shopinfo = keeper->shopinfo)) {
    sprintf(buf, "Mob #%d (%s) flagged as DSHOPKEEPER but has no shopinfo!",
            keeper->nr, GET_NAME(keeper));
    mudlog(buf, NRM, LVL_DEITY, TRUE);
    send_to_char("Sorry, there is an error in this shop. Please inform an immortal.\r\n", ch);
    return;
  }
  
  DSHOP_CHECK_CLOSED(keeper);
  
  argument = one_argument(argument, arg);

  if (!*arg) {
    perform_tell(keeper, ch, MSG_SELL_WHAT(keeper));
    return;
  } 

  if (is_number(arg)) {
    count = atoi(arg);
    argument = one_argument(argument, arg);
  } else {
    count = 1;
  }
  
  if (count <= 0) {
    perform_tell(keeper, ch, MSG_NEG_SELL(keeper));
    return;
  }
  
  numsold = price = 0;
  obj = ch->carrying;
  found = FALSE;
  while (count > 0 && obj) {
    nextobj = obj->next_content;
    if (isname(arg, obj->name) && CAN_SEE_OBJ(ch, obj)) {
      found = TRUE;
      if (DSHOP_FLAGGED(keeper, DSHOP_ZONEBUY) && 
           (GET_OBJ_VNUM(obj) / 100 != GET_MOB_VNUM(keeper) / 100)
         ) {
/*        sprintf(buf, "%s: ", obj->short_description);
        send_to_char(buf, ch);                                      */
        perform_tell(keeper, ch, MSG_DONT_WANT(keeper));
        break;
      } else if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOSELL)) {
        sprintf(buf, "You can't sell %s!\r\n", obj->short_description);
        send_to_char(buf, ch);
        break;
      } else if (((GET_OBJ_TYPE(obj) == ITEM_WAND) ||
           (GET_OBJ_TYPE(obj) == ITEM_STAFF)) &&
           (GET_OBJ_VAL(obj, 2) == 0)) {
        sprintf(buf, "You can't sell %s, it's all used up!", obj->short_description);
        send_to_char(buf, ch);
        break;
      } else if (!(shopinfo->types & (1 << GET_OBJ_TYPE(obj)))) {
/*        sprintf(buf, "%s: ", obj->short_description);
        send_to_char(buf, ch);                                     */
        perform_tell(keeper, ch, MSG_WRONG_TYPE(keeper));
        break;
      } else {
        cost = (int)(GET_OBJ_COST(obj) * shopinfo->buy_rate);
        if (GET_SHOP_GOLD(keeper) < cost && !DSHOP_FLAGGED(keeper, DSHOP_RICH)) {
          count = -1;
          break;
        }
        if (!DSHOP_FLAGGED(keeper, DSHOP_RICH)) GET_SHOP_GOLD(keeper) -= cost;
        GET_GOLD(ch) += cost;
        price += cost;
        obj_from_char(obj);
        CREATE(lim, struct limited_sell_info, 1);
        lim->next = shopinfo->limited;
        shopinfo->limited = lim;
        lim->obj = obj;
        numsold++;
        count--;
        sprintf(buf, "You sell %s.\r\n", obj->short_description);
        send_to_char(buf, ch);
      }
    }
    obj = nextobj;
/*    count--; */
  }
  if (count > 0) {
    /* You didn't have enough! */
    if (numsold > 0) {
      perform_tell(keeper, ch, MSG_GOT_NO_MORE(keeper));
    } else {
      if (!found) perform_tell(keeper, ch, MSG_GOT_NONE(keeper));
    }
  } else if (count == -1) {
    /* The shopkeep couldn't afford them all */
    if (numsold > 0) {
      perform_tell(keeper, ch, MSG_THATS_ALL(keeper));
    } else {
      perform_tell(keeper, ch, MSG_IM_BROKE(keeper));
    }
  }
  if (numsold > 0) {
    sprintf(buf, "$n gives you %d coins.", price);
    act(buf, FALSE, keeper, NULL, ch, TO_VICT);
    act("$n gives $N some money.", TRUE, keeper, NULL, ch, TO_NOTVICT);
    perform_tell(keeper, ch, MSG_THANKS2(keeper));
  }
}    

ACMD(do_dvalue) {
  struct char_data *keeper;
  struct shopkeeper_info *shopinfo;
  struct obj_data *obj;
  
  for (keeper = world[ch->in_room].people; keeper; keeper = keeper->next_in_room) {
    if (MOB_FLAGGED(keeper, MOB_DSHOPKEEPER)) break;
  }
  
  if (!keeper) {
    send_to_char("Sorry but there is no shopkeeper to valuate your items!\r\n", ch);
    return;
  }
  
  if (!(shopinfo = keeper->shopinfo)) {
    sprintf(buf, "Mob #%d (%s) flagged as DSHOPKEEPER but has no shopinfo!",
            keeper->nr, GET_NAME(keeper));
    mudlog(buf, NRM, LVL_DEITY, TRUE);
    send_to_char("Sorry, there is an error in this shop. Please inform an immortal.\r\n", ch);
    return;
  }
  
  DSHOP_CHECK_CLOSED(keeper);
  
  argument = one_argument(argument, arg);

  if (!*arg) {
    perform_tell(keeper, ch, MSG_SELL_WHAT(keeper));
    return;
  } 
  
  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    perform_tell(keeper, ch, MSG_GOT_NONE(keeper));
    return;
  }
  
  if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOSELL) ||
     (((GET_OBJ_TYPE(obj) == ITEM_WAND) || (GET_OBJ_TYPE(obj) == ITEM_STAFF)) &&
     (GET_OBJ_VAL(obj, 2) == 0))) {
    perform_tell(keeper, ch, MSG_DONT_WANT(keeper));
    return;
  }
  
  if (!(shopinfo->types & GET_OBJ_TYPE(obj))) {
    perform_tell(keeper, ch, MSG_WRONG_TYPE(keeper));
    return;
  }
  
  sprintf(buf2, MSG_VALUE(keeper),
         (int)(GET_OBJ_COST(obj) * shopinfo->buy_rate), obj->short_description);
  perform_tell(keeper, ch, buf2);
}

void stat_shop(struct char_data *ch, struct char_data *k) {
  extern char *item_types[];
  
  if (!MOB_FLAGGED(k, MOB_DSHOPKEEPER)) return;
  if (!k->shopinfo) return;
  
  sprintf(buf, "Shop gold : %d\r\n", GET_SHOP_GOLD(k));
  send_to_char(buf, ch); 
  send_to_char("Shopkeeper flags : ", ch);
  sprintbit(k->shopinfo->flags, shopflags, buf);
  send_to_char(buf, ch);
  sprintf(buf, "\r\nBuys at %#1.3f, sells at %#1.3f.\r\n", k->shopinfo->buy_rate,
          k->shopinfo->sell_rate);
  send_to_char(buf, ch);
  send_to_char("This shopkeeper deals in : ", ch);
  sprintbit(k->shopinfo->types, item_types, buf);
  send_to_char(buf, ch);
  sprintf(buf, "\r\nOpens: %d. Closes: %d.\r\n", k->shopinfo->open, k->shopinfo->close);
  send_to_char(buf, ch);
  send_to_char("\r\n", ch);
}

ACMD(do_dsedit) {
  struct char_data *k;
  int i;
  char *c;
  extern char *item_types[];
  
  argument = one_argument(argument, arg);
  
  if (*arg) {
    k = get_char_room_vis(ch, arg);
    if (!k) {
      sprintf(buf, "I don't see anyone named %s around here, do you?", arg);
      send_to_char(buf, ch);
      return;
    }
    argument = one_argument(argument, arg);
  }
  
  if (!*arg) {
    send_to_char("Syntax: sedit <mobile> <command> [options]\r\n"
                 "where <command> is one of:\r\n"
                 "listspeech     speech         buyrate        sellrate\r\n"
                 "flags          types          opens          closes\r\n"
                 "save\r\n", ch);
    return;
  }
  
  if (!MOB_FLAGGED(k, MOB_DSHOPKEEPER)) {
    sprintf(buf, "%s isn't a shopkeeper!\r\n", GET_NAME(k));
    send_to_char(buf, ch);
    return;
  }
  
  if (!k->shopinfo) init_new_shopkeeper(k);
  
  if (is_abbrev(arg, "listspeech")) {
    *buf = '\0';
    for (i = 0; i < MSGS; i++)
      sprintf(buf, "%s%20s: %s\r\n", buf, textnames[i],
              MSG(k, i) ? MSG(k, i) : "-");
    page_string(ch->desc, buf, TRUE);
  } else if (is_abbrev(arg, "speech")) {
    argument = one_argument(argument, arg);
    c = argument;
    skip_spaces(&c);
    if (!*c || !*arg) {
      send_to_char("Syntax: sedit <mobile> speech <message> <text>\r\n"
                   "For a list of valid messages, try\r\n"
                   "   sedit <mobile> listspeech\r\n"
                   "Note that message names beginning with underscores require special\r\n"
                   "control codes!\r\n", ch);
      return;
    }
    for (i = 0; i < MSGS; i++) {
      if (!str_cmp(textnames[i], arg)) break;
    }
    if (i == MSGS) {
      sprintf(buf, "Invalid message name %s!\r\n"
                   "For a list of valid message names, try\r\n"
                   "   sedit <mobile> listspeech\r\n"
                   "Note that message names beginning with underscores require special\r\n"
                   "control codes!\r\n", arg);
      send_to_char(buf, ch);
      return;
    }
    FREE(MSG(k, i));
    MSG(k, i) = strdup(c);
  } else if (is_abbrev(arg, "buyrate")) {
    argument = one_argument(argument, arg);
    if (!*arg) {
      send_to_char("Set the buy rate to what?\r\n", ch);
      return;
    }
    k->shopinfo->buy_rate = atof(arg);
  } else if (is_abbrev(arg, "sellrate")) {
    argument = one_argument(argument, arg);
    if (!*arg) {
      send_to_char("Set the sell rate to what?\r\n", ch);
      return;
    }
    k->shopinfo->sell_rate = atof(arg);
  } else if (is_abbrev(arg, "flags")) {
    argument = one_argument(argument, arg);
    if (!*arg) {
      sprintf(buf, "Toggle which flag?\r\n"
                   "Valid flags are: \r\n");
      for (i = 0; i < NUM_DSHOP_FLAGS; i++)
        sprintf(buf, "%s%s\r\n", buf, shopflags[i]);
      page_string(ch->desc, buf, TRUE);
      return;
    }
    c = arg;
    skip_spaces(&c);
    for (i = 0; i < NUM_DSHOP_FLAGS; i++)
      if (!str_cmp(c, shopflags[i])) break;
    if (i == NUM_DSHOP_FLAGS) {
      sprintf(buf, "Unknown shop flag %s!\r\n"
                   "Valid flags are: \r\n", c);
      for (i = 0; i < NUM_DSHOP_FLAGS; i++)
        sprintf(buf, "%s%s\r\n", buf, shopflags[i]);
      page_string(ch->desc, buf, TRUE);
      return;
    }
    TOGGLE_BIT(DSHOP_FLAGS(k), 1 << i);
  } else if (is_abbrev(arg, "types")) {
    argument = one_argument(argument, arg);
    if (!*arg) {
      sprintf(buf, "Toggle which type?\r\n"
                   "Valid types are: \r\n");
      for (i = 0; i < NUM_ITEM_TYPES; i++)
        sprintf(buf, "%s%s\r\n", buf, item_types[i]);
      page_string(ch->desc, buf, TRUE);
      return;
    }
    c = arg;
    skip_spaces(&c);
    for (i = 1; i < NUM_ITEM_TYPES; i++)
      if (!str_cmp(c, item_types[i])) break;
    if (i == NUM_ITEM_TYPES) {
      sprintf(buf, "Unknown item type %s!\r\n"
                   "Valid types are: \r\n", c);
      for (i = 0; i < NUM_ITEM_TYPES; i++)
        sprintf(buf, "%s%s\r\n", buf, item_types[i]);
      page_string(ch->desc, buf, TRUE);
      return;
    }
    TOGGLE_BIT(k->shopinfo->types, 1 << i);
  } else if (is_abbrev(arg, "opens")) {
    argument = one_argument(argument, arg);
    if (!*arg) {
      send_to_char("Set the opening hour to what?\r\n", ch);
      return;
    }
    k->shopinfo->open = atoi(arg);
  } else if (is_abbrev(arg, "closes")) {
    argument = one_argument(argument, arg);
    if (!*arg) {
      send_to_char("Set the closing hour to what?\r\n", ch);
      return;
    }
    k->shopinfo->close = atoi(arg);
  } else if (is_abbrev(arg, "gold")) {
    argument = one_argument(argument, arg);
    if (!*arg) {
      send_to_char("How much gold does this shop get?\r\n", ch);
      return;
    }
    GET_SHOP_GOLD(k) = atoi(arg);
  } else if (!str_cmp(arg, "save")) {
    if (!save_dshopkeeper(k))
      send_to_char("Save failed!\r\n", ch);
    else
      send_to_char("Ok.\r\n", ch);
  } else {
    sprintf(buf, "Unknown command %s.", arg);
    send_to_char(buf, ch);
    return;
  }
}
