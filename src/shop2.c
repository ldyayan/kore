/* ************************************************************************
*  File: shop2.c                                                          *
*  Usage: shopkeepers: simple shopkeepers, done much like the circle      *
*      pet shop boy.                                                      *
*                                                                         *
*  Parts of this code are surely:                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  Heroes of Kore's new-style simple shopcode by Aule                     *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

#define IS_GOD(ch)              (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GOD))


/* external variables */
extern struct room_data *world;
extern struct index_data *obj_index;
extern struct str_app_type str_app[];

/* external function declarations */
void perform_tell(struct char_data * ch, struct char_data * vict, char * arg);
char *fname(char *namelist);

/* local variables */



/* 
 *  if the shopkeep sells these items out of his shop, then just let
 *  me know.  useful for junking duplicate eq people might be selling
 *  back to the shopkeep (among other things)
 */
bool sells_unlimited(struct obj_data * obj, struct char_data * keeper)
{
  struct obj_data *k;
  int shop_room;


  shop_room = keeper->in_room + 1;

  for (k = world[shop_room].contents; k; k = k->next_content)
    if (GET_OBJ_VNUM(obj) == GET_OBJ_VNUM(k))
      return TRUE;

  return FALSE;
}



/*
 *  shopkeepers sell the items they have at a rate determined by their
 *  alignment.  if they are really good (align 1000) they will sell items
 *  for just their basic cost.  if they are really evil (align -1000) they
 *  sell their items at double the cost.  all other alignments are on the
 *  slope of those two points.  1/2 way for example, at align 0, the selling
 *  cost is 150%.
 */
int sell_cost(struct obj_data * obj, struct char_data * keeper)
{
  int sell_for;
  float rate;


  rate = (GET_ALIGNMENT(keeper) - 1000);        /* 0 to -2000 */
  rate = -1 * rate;                             /* 0 to 2000 */
  rate = rate / 20;                             /* 0 to 100 */
  rate = 100 + rate;                            /* 100 to 200 */
  rate = rate / 100;                            /* multiply cost 1.0 to 2.0
                                                times, depending on alignment */
  sell_for = (int) (GET_OBJ_COST(obj) * rate);

  return(sell_for);
}



/*
 *  shopkeepers dont buy items if their type isnt like others that are
 *  in the shop_room.  they also wont buy cursed items, and they wont
 *  buy !sell items.  if they wont buy an item, the value is -1;
 *  but any negative value is bad, so don't check for == -1, check for < 0.
 *  as a special case, if the item is all used up, like a wand or staff
 *  that's out of charges, return a -2.
 */
int buy_value(struct obj_data * obj, struct char_data * keeper)
{
  struct obj_data *k;
  int shop_room;
  int found = 0;
  int buy_for;
  float rate;


  if (GET_OBJ_COST(obj) < 1)
    return -1;

  if (IS_OBJ_STAT(obj, ITEM_NOSELL))
    return -1;

  if (IS_OBJ_STAT(obj, ITEM_NODROP))
    return -1;

  if (((GET_OBJ_TYPE(obj) == ITEM_WAND) ||
       (GET_OBJ_TYPE(obj) == ITEM_STAFF)) &&
      (GET_OBJ_VAL(obj, 2) == 0))
    return -2;

  shop_room = keeper->in_room + 1;
 
  for (k = world[shop_room].contents; k; k = k->next_content) {
    if (GET_OBJ_TYPE(obj) == GET_OBJ_TYPE(k)) {
      found = 1;
      break;
    }
  }

  if (!found)
    return -1;

  /* otherwise the shopkeeper sells objects like these */
  rate = (GET_ALIGNMENT(keeper) + 1000);        /* 2000 to 0 */
  rate = rate / 100;				/* 20% to 0% of cost */;
  rate = rate / 100;                            /* multiply cost 0.2 to 0.0
                                                times, depending on alignment */
  buy_for = (int) (GET_OBJ_COST(obj) * rate);

  return(buy_for);
}



/*
 *  count_check returns how many objects of that type it counts in
 *  a seller's inventory
 */
int count_check_seller(struct char_data * ch, struct obj_data * obj)
{
  struct obj_data *k;
  int count = 0;


  for (k = ch->carrying; k; k = k->next_content)
    if (GET_OBJ_VNUM(k) == GET_OBJ_VNUM(obj))
      count++;

  return count;
}



/*
 *  appraise_obj sends messages to ch about how much keeper might
 *  pay for obj and returns how much keeper says he'll pay.
 *  count winds up holding how many he'll actually buy, so that needs to
 *  be checked too...
 */
int appraise_obj(struct char_data * ch, struct obj_data * obj,
    struct char_data * keeper, int * count)
{
  int value;
  int total_value;
  char buf[MAX_STRING_LENGTH];  /* without this, perform_tell messes up */
  int how_many;


  /* fix count in case its a mess */
  if ((*count) < 1) {
    (*count) = 0;
    perform_tell(keeper, ch, "How many??");
    return -1;
  }

  how_many = count_check_seller(ch, obj);
  if ((*count) > how_many) {
    (*count) = how_many;
    sprintf(buf, "You only seem to have %d of those...", (*count));
    perform_tell(keeper, ch, buf);
  }

  /* calc value and return if its bad */
  value = buy_value(obj, keeper);
  if (value < 0) {
    if (value == -2)
      perform_tell(keeper, ch, "I don't buy used up items.");
    else
      perform_tell(keeper, ch, "I don't buy such items.");
    return value;
  }

  /* work on total value */
  total_value = value * (*count);

  if (total_value > GET_GOLD(keeper)) {
    if ((*count) == 1) {
      sprintf(buf, "You'd get %d gold coins for it,", value);
      perform_tell(keeper, ch, buf);
      perform_tell(keeper, ch, "but I don't have enough gold."); 
    } else {	/* count > 1 */
      if (total_value == 0) { /* watch out for division by zero below */
        sprintf(buf, "You'd get %d gold coins for each,", value);
        perform_tell(keeper, ch, buf);
        perform_tell(keeper, ch, "but strangely I don't have enough gold.");
      } else {
        (*count) = GET_GOLD(keeper) / value;
        if ((*count) == 0) {
          sprintf(buf, "You'd get %d gold coins for each,", value);
          perform_tell(keeper, ch, buf);
          perform_tell(keeper, ch, "but I dont have enough gold for any.");
        } else {
          sprintf(buf, "You'd get %d gold coins for each,", value);
          perform_tell(keeper, ch, buf);
          sprintf(buf, "but I only have enough gold to buy %d.", (*count));
          perform_tell(keeper, ch, buf);
        }
      }
    }
    return -1;
  }

  if ((*count) == 1) {
    sprintf(buf, "I'll give you %d gold coins for that!", value);
    perform_tell(keeper, ch, buf);
  } else {
    sprintf(buf, "I'll give you %d gold coins each,", value);
    perform_tell(keeper, ch, buf);
    sprintf(buf, "%d gold coins total!", total_value);
    perform_tell(keeper, ch, buf);
  }

  return total_value;
}



struct obj_data * get_hash_keeper_obj(struct char_data * keeper,
    int index_to_find, int * unlimited)
{
  struct obj_data *obj, *first_obj;
  struct obj_data *found_obj;
  int index;
  int shop_room;


  (*unlimited) = 0;

  if (!keeper || index_to_find < 1)
    return NULL;

  /* hey a shopkeep */
  shop_room = keeper->in_room + 1;

  /* this is a lot like do_list's function, naturally */
  found_obj = NULL;
  index = 0;
  if (keeper->carrying) {
    /* sort the list */
    /* clear the count */
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      GET_OBJ_COUNT(obj) = 0;
    /* count out how many of the first object of that vnum, and store there */
    for (obj = keeper->carrying, first_obj = obj;
         obj; obj = obj->next_content) {
      if (GET_OBJ_VNUM(obj) == GET_OBJ_VNUM(first_obj)) {
        GET_OBJ_COUNT(first_obj)++;
      } else {
        first_obj = obj;
        GET_OBJ_COUNT(first_obj)++;
      }
    }
    /* look for index_to_find */
    for (obj = keeper->carrying; obj; obj = obj->next_content) {
      if (GET_OBJ_COUNT(obj) > 0) {
        index++;
        if (index == index_to_find) {
          found_obj = obj;
        }
      }
    }
    /* clear the count back to zero */
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      GET_OBJ_COUNT(obj) = 0;
  }

  /* check the unlimiteds in the shop */
  for (obj = world[shop_room].contents; obj; obj = obj->next_content) {
    index++;
    if (index == index_to_find) {
      found_obj = obj;
      (*unlimited) = 1;
    }
  }

  return found_obj;
}



/*
 *  this just checks to see if there's a shopkeeper in the room,
 *  the new kind of course, with the shopkeeper flag.
 */
struct char_data * find_shopkeeper(struct char_data * ch)
{
  struct char_data *keeper;


  for (keeper = world[ch->in_room].people;
       keeper;
       keeper = keeper->next_in_room) {
    if (IS_MOB(keeper) && !FIGHTING(keeper)
        && AWAKE(keeper) && !IS_AFFECTED(keeper, AFF_CHARM)) {
      if (MOB_FLAGGED(keeper, MOB_SHOPKEEPER))
        break;
      else
        continue;
    }
  }

  return keeper;
}



/*
 * some improvments that could be done:
 *   1) fix up the mass slow kludge <- keyword kludge
 *        that gets the objects out of the limited inventory of the keeper
 *        and gives them to the player
 *   2) work it out so that buy 2.sword searches both the main inventory
 *        of the keeper and their storage room and does it right.. carrying
 *        the number across..
 */
ACMD(do_buy)
{
  struct char_data *keeper;
  int shop_room;
  int count;
  int how_many;
  struct obj_data *obj = NULL;
  int bits;
  struct char_data *dummy;
  int keeper_room;
  char buf[MAX_STRING_LENGTH];  /* without this, perform_tell messes up */
  int index;
  int unlimited = 0;
  struct obj_data *o = NULL;
  int i;
  int cost;
  extern struct room_data *world;
  ACMD(do_pawnbuy);
  ACMD(do_dbuy);

  if (world[ch->in_room].number == PAWNSHOP_VNUM) {
    do_pawnbuy(ch, argument, cmd, subcmd);
    return;
  }

  if (!(keeper = find_shopkeeper(ch))) {
/*    send_to_char("Sorry but you can't buy stuff here, no shopkeeper!\r\n", ch);*/
    do_dbuy(ch, argument, cmd, subcmd);
    return;
  }

  /* hey a shopkeep */
  keeper_room = keeper->in_room;
  shop_room = keeper->in_room + 1;

  argument = one_argument(argument, arg);

  if (!*arg) {
    perform_tell(keeper, ch, "What do you want to buy??");
    return;
  } 

  if (is_number(arg)) {
    count = atoi(arg);
    argument = one_argument(argument, arg);
  } else {
    count = 1;
  }

  if (count == 0) {
    perform_tell(keeper, ch, "What do you want to buy??");
    return;
  } else if (count < 0) {
    perform_tell(keeper, ch, "A negative amount?  Try selling me something.");
    return;
  }

  /* look for the object the player wants to buy */
  if (arg[0] == '#') {
    index = atoi(arg + 1);
    if (!(obj = get_hash_keeper_obj(keeper, index, &unlimited))) {
      perform_tell(keeper, ch, "I haven't got that in stock -- try list.");
      return;
    }
  } else {
    /*
     * this else is so bad, but the way out (double-threading the lists, 
     * keeping counts on the lists and then setting it back) is so harsh, I
     * will just leave it. UGH!! 
     * I tell them to try buying by stock # 
     */
    if (strstr(arg, ".") > 0) {
      perform_tell(keeper, ch, "Please buy by unique name or by stock #.");
      return;
    }
    char_from_room(keeper);
    char_to_room(keeper, shop_room);
    bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM,
               keeper, &dummy, &obj);
    char_from_room(keeper);
    char_to_room(keeper, keeper_room);
  } 

  if (obj == NULL) {
    perform_tell(keeper, ch, "I haven't got that in stock -- try list.");
    return;
  }

  if (sells_unlimited(obj, keeper))
    unlimited = 1;

  /* 
   * check and see if the shopkeeper has as many of the objects as the 
   * buyer wants
   */
  if (!unlimited) {
    how_many = count_check_seller(keeper, obj);
    if (how_many < count) {
      sprintf(buf, "I only have %d to sell you.", how_many);
      perform_tell(keeper, ch, buf);
      count = how_many;
      /* no return, just sell ch fewer objects than they want */
    }
  }

  /* have enough gold? gods no have to pay */
  if (GET_GOLD(ch) < sell_cost(obj, keeper)) {
      perform_tell(keeper, ch, "You can't afford it!");
      return;
  } else if (GET_GOLD(ch) < (sell_cost(obj, keeper) * count)) {
      /* only enough gold for a few obj's, not as many as ch would like */
      count = GET_GOLD(ch) / sell_cost(obj, keeper);
      sprintf(buf, "You can only afford %d.", count);
      perform_tell(keeper, ch, buf); 
  }

  /* check carrying limits and make sure ch doesn go over max number */
  how_many = CAN_CARRY_N(ch) - IS_CARRYING_N(ch);
  if (how_many <= 0) {   /* shouldnt ever be less than 0 ... but... */
    sprintf(buf, "%s: You can't carry any more items.\r\n",
        fname(obj->name));
    send_to_char(buf, ch);
    return;
  }
  if (count > how_many) {
    count = how_many;
    sprintf(buf, "You can only carry %d.", count);
    perform_tell(keeper, ch, buf);
  }

  /* check carrying limits and make sure ch doesnt go over max weight */
  if (GET_OBJ_WEIGHT(obj) > 0) {
    how_many = (CAN_CARRY_W(ch) - IS_CARRYING_W(ch)) / GET_OBJ_WEIGHT(obj);
    if (how_many <= 0) {   /* shouldnt ever be less than 0 ... but ... */
      sprintf(buf, "%s: You can't carry that much weight.\r\n",
          fname(obj->name));
      send_to_char(buf, ch);
      return;
    }
    if (count > how_many) {
      count = how_many;
      sprintf(buf, "You can only carry %d.", count);
      perform_tell(keeper, ch, buf);
    }
  }

  /*
   * actually buy the item, either by taking it from the keepers inventory
   * or by cloning an unlimited item from his shop_room
   * dont necessarily take all the objects from keepers inventory
   * if the objects are limited
   */
  if (!unlimited) {
    /* take the objects right out of the shopkeeps inventory */
    /* this is a MASS SLOW and inefficient kludge!!! */
    for (i = 0; i < count; i++) {
      for (o = keeper->carrying; o; o = o->next_content) {
        if (GET_OBJ_VNUM(o) == GET_OBJ_VNUM(obj)) {
          obj_from_char(o);
          obj_to_char(o, ch);
          break;
        }
      }
    }
  } else {
    /* otherwise just clone and give as many objects as needed */
    for (i = 0; i < count; i++) {
      o = read_object(GET_OBJ_RNUM(obj), REAL);
      obj_to_char(o, ch);
    }
  }

  /* charge them the gold - but with gods no money is exchanged */
  cost = count * sell_cost(obj, keeper);
  GET_GOLD(ch) -= cost;
  GET_GOLD(keeper) += cost;

  if (cost == 1)
    sprintf(buf, "That'll be 1 coin.");
  else
    sprintf(buf, "That'll be %d coins.", cost);
  perform_tell(keeper, ch, buf);
 
  if (count > 1)
    sprintf(buf, "You now have %s (x %d).\r\n", obj->short_description, count); 
  else
    sprintf(buf, "You now have %s.\r\n", obj->short_description);
  send_to_char(buf, ch);

  if (count > 1)
    sprintf(buf, "$n buys $p (x %d).", count);
  else
    sprintf(buf, "$n buys $p.");
  act(buf, FALSE, ch, obj, keeper, TO_ROOM);

  return;
}



/*
 *  list the items the shopkeep is selling.
 *  some improvements might include sorting the list before coalescing
 *  his inventory.
 *  other improvements might be to show if an item (like a wand/staff)
 *  is '(slightly used)'
 *  another improvement would be if the item is a drink container, to say
 *  what beverage it holds, but this basically works.
 */
ACMD(do_list)
{
  struct char_data *keeper;
  int index;
  struct obj_data *obj;
  struct obj_data *first_obj;
  int shop_room;
  extern struct room_data *world;
  ACMD(do_pawnlist);
  ACMD(do_dlist);

  if (world[ch->in_room].number == PAWNSHOP_VNUM) {
    do_pawnlist(ch, argument, cmd, subcmd);
    return;
  }

  if (!(keeper = find_shopkeeper(ch))) {
/*    send_to_char("Sorry but there is nothing to list, no shopkeeper!\r\n", ch); */
    do_dlist(ch, argument, cmd, subcmd);
    return;
  }

  /* hey a shopkeep */
  shop_room = keeper->in_room + 1;

  send_to_char(
      " ##   Available   Item                    "
      "                           Cost\r\n"
      "------------------------------------------"
      "-------------------------------\r\n", ch);

  index = 0;
  if (keeper->carrying) {
    /* sort the list */
    /* clear the count */
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      GET_OBJ_COUNT(obj) = 0;
    /* count out how many of the first object of that vnum, and store there */
    for (obj = keeper->carrying, first_obj = obj;
         obj; obj = obj->next_content) {
      if (GET_OBJ_VNUM(obj) == GET_OBJ_VNUM(first_obj)) {
        GET_OBJ_COUNT(first_obj)++;
      } else {
        first_obj = obj;
        GET_OBJ_COUNT(first_obj)++;
      }
    }
    /* print */
    for (obj = keeper->carrying; obj; obj = obj->next_content) {
      if (GET_OBJ_COUNT(obj) > 0) {
        index++;
        sprintf(buf, "%3d)  %5d       %-40s%15d\r\n", index, GET_OBJ_COUNT(obj),
            obj->short_description, sell_cost(obj, keeper));
        send_to_char(buf, ch);
      }
    }
    /* clear the count back to zero */
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      GET_OBJ_COUNT(obj) = 0;
  }
  /* list the unlimiteds in the shop */
  for (obj = world[shop_room].contents; obj; obj = obj->next_content) {
    index++;
    sprintf(buf, "%3d)  Unlimited   %-40s%15d\r\n", index,
        obj->short_description, sell_cost(obj, keeper));
    send_to_char(buf, ch);
  }

  return;
}



/*
 *  seems to work,
 *  some improvements would be:
 *  to let you sell a number of items all at once, like 'sell 5 bread'
 *  oh and to give a little message to others that you just sold some stuff.
 */
ACMD(do_sell)
{
  struct char_data *keeper;
  struct obj_data *obj = NULL;
  int count;
  int value;
  int vnum, i;
  struct obj_data *k;
  extern struct room_data *world;
  ACMD(do_pawn);
  ACMD(do_dsell);

  if (world[ch->in_room].number == PAWNSHOP_VNUM) {
    do_pawn(ch, argument, cmd, subcmd);
    return;
  }

  if (!(keeper = find_shopkeeper(ch))) {
/*    send_to_char("Sorry but there is no shopkeeper to sell to!\r\n", ch);*/
    do_dsell(ch, argument, cmd, subcmd);
    return;
  }

  /* hey a shopkeep */
  argument = one_argument(argument, arg);

  if (!*arg) {
    perform_tell(keeper, ch, "What do you want to sell??");
    return;
  } 

  if (is_number(arg)) {
    count = atoi(arg);
    argument = one_argument(argument, arg);
  } else {
    count = 1;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    perform_tell(keeper, ch, "You don't seem to have that.");
    return;
  } 

  if ((value = appraise_obj(ch, obj, keeper, &count)) >= 0) {
    if (count > 1)
      sprintf(buf, "$n now has $p (x %d).", count);
    else
      sprintf(buf, "$n now has $p.");
    act(buf, FALSE, keeper, obj, ch, TO_VICT);

    if (count > 1)
      sprintf(buf, "$n sells $p (x %d).", count);
    else
      sprintf(buf, "$n sells $p.");
    act(buf, FALSE, ch, obj, keeper, TO_ROOM);

    vnum = GET_OBJ_VNUM(obj);
    for (i = 0; i < count; i++) {
      for (k = ch->carrying; k; k = k->next_content) {
        if (GET_OBJ_VNUM(k) == vnum) {
          obj_from_char(k); 
          if (sells_unlimited(k, keeper))
            extract_obj(k);
          else
            obj_to_char(k, keeper);
          break;
        }
      }
    }
    GET_GOLD(keeper) -= value;
    GET_GOLD(ch) += value;
    return;
  }

  return;
}



ACMD(do_value)
{
  struct char_data *keeper;
  struct obj_data *obj = NULL;
  int count;
  ACMD(do_dvalue);


  if (!(keeper = find_shopkeeper(ch))) {
/*    send_to_char("Sorry but there is no shopkeeper "
                 "to valuate your items!\r\n", ch);*/
    do_dvalue(ch, argument, cmd, subcmd);
    return;
  }

  /* hey a shopkeep */
  argument = one_argument(argument, arg);

  if (!*arg) {
    perform_tell(keeper, ch, "What do you want me to valuate??");
    return;
  }

  if (is_number(arg)) {
    count = atoi(arg);
    argument = one_argument(argument, arg);
  } else {
    count = 1;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    perform_tell(keeper, ch, "You don't seem to have that.");
    return;
  }

  appraise_obj(ch, obj, keeper, &count);

  return;
}
