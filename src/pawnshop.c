/***********************************************************************
 ***   A pawn shop where players can sell stuff at a price they set  ***
 ***********************************************************************/


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"

int Obj_to_store(struct obj_data * obj, FILE * fl);
void perform_tell(struct char_data * ch, struct char_data * vict, char * arg);
struct char_data * find_shopkeeper(struct char_data * ch);
extern struct index_data *obj_index;
ACMD(do_save);
extern struct char_data *character_list;
extern struct index_data *mob_index;


struct forsale_data {
  struct obj_data *obj;
  char *objname;
  char *seller;
  char *buyer;
  int price;
  bool sold;
  struct forsale_data *next;
};

struct forsale_data *forsale;

void boot_pawnshop() {
  struct char_data *keeper;
  FILE *fl;
  char instr[2], temp1[MAX_NAME_LENGTH+1], temp2[100], line[256];
  struct forsale_data *temp;
  int item_number;
  int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  long tmp8, tmp21;
  int tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17, tmp18,
        tmp19, tmp20;
  struct obj_data *obj;
  int n;
  struct char_data *i;
  int found;

  
  fl = fopen( PAWNSHOP_FILE, "r" );
  if (!fl) {
    log("ERROR: Missing pawn shop file!");
    return;
  }

  /* find the keeper */
  found = 0;
  for (i = character_list; i; i=i->next) {
    if (GET_MOB_VNUM(i) == PAWNSHOP_KEEPER_VNUM) {
      keeper = i; 
      found = 1;
      break;
    }
  }
  if (!found) {
    log("ERROR: Pawnshop keeper not found!");
    return;
  }


  do {
    fscanf(fl, "%s\n", instr);
    temp = malloc(sizeof(struct forsale_data));
    temp->next = forsale;
    switch (instr[0]) {
      case '~':
        break;
        free(temp);
      case 'x':   /* An object that's BEEN sold */
        forsale = temp;
        fscanf(fl, "%s %d\n", temp1, &temp->price);
        temp->seller = strdup(temp1);
        temp->sold = TRUE;
        temp->obj = NULL;
        get_line(fl, temp2);
        temp->objname = strdup(temp2);
        fscanf(fl, "%s\n", temp1);
        temp->buyer = strdup(temp1);
        break;
      case 's':
      case 'r':
        obj = malloc(sizeof(struct obj_data));
        forsale = temp;
        fscanf(fl, "%s %d\n", temp1, &temp->price);
        temp->seller = strdup(temp1);
        if (instr[0] == 'r') {
          fscanf(fl, "%s\n", temp1);
          temp->buyer = strdup(temp1);
          temp->sold = TRUE;
        } else {
          temp->sold = FALSE;
          temp->buyer = NULL;
        }
        get_line(fl, temp2);
        temp->objname = strdup(temp2);
        fscanf(fl, "%d\n", &item_number);
        obj = read_object(real_object(item_number), REAL);
        obj_to_char(obj, keeper);
        get_line(fl, line);
        /* watch out for MAX_OBJ_AFFECTS!! :( */
    if (obj == NULL) {
      sprintf(buf, "SYSERR: obj #%d does not exist", item_number);
      log(buf);
      return;
    }
        n = sscanf(line, "%d %d %d %d %d %d %d %ld %d %d %d %d %d %d %d %d %d %d %d %d %ld",
            &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7, &tmp8,
            &tmp9, &tmp10, &tmp11, &tmp12, &tmp13, &tmp14, 
            &tmp15, &tmp16, &tmp17, &tmp18, &tmp19, &tmp20, &tmp21);
        obj->obj_flags.value[0] = tmp1;
        obj->obj_flags.value[1] = tmp2;
        obj->obj_flags.value[2] = tmp3;
        obj->obj_flags.value[3] = tmp4;
        obj->obj_flags.extra_flags = tmp5;
  /* HACKED by Culvan...weight won't change (will it?) and this screws up
     containers */
  /*      obj->obj_flags.weight = tmp6; */
        obj->obj_flags.timer = tmp7;

        if (n > 7)
            obj->obj_flags.bitvector = tmp8;
        else
            obj->obj_flags.bitvector = 0;
        if (n > 20)
            obj->obj_flags.bitvector2 = tmp21;
        else
            obj->obj_flags.bitvector2 = 0;

        obj->affected[0].location = tmp9;
        obj->affected[0].modifier = tmp10;
        obj->affected[1].location = tmp11;
        obj->affected[1].modifier = tmp12;
        obj->affected[2].location = tmp13;
        obj->affected[2].modifier = tmp14;
        obj->affected[3].location = tmp15;
        obj->affected[3].modifier = tmp16;
        obj->affected[4].location = tmp17;
        obj->affected[4].modifier = tmp18;
        obj->affected[5].location = tmp19;
        obj->affected[5].modifier = tmp20;
/*        free(obj->name);
        free(obj->description);
        free(obj->short_description);*/
        obj->short_description = strdup(temp->objname);
        get_line(fl, line);
        obj->name = strdup(line);
        get_line(fl, line);
        obj->description = strdup(line);
        temp->obj = obj;
        break;
      default:
        log("ERROR: invalid load type in pawnshop file!");
    }
  } while (instr[0] != '~' && !feof(fl));
  fclose(fl);
}

void save_pawnshop() {
  FILE *fl;
  struct forsale_data *cur = forsale;
  struct obj_data *obj;
  int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  long tmp8, tmp21;
  int tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17, tmp18,
        tmp19, tmp20;
  extern struct index_data *obj_index;

  
  fl = fopen( PAWNSHOP_FILE, "w" );
  
  if (!fl) {
    mudlog("Error saving pawnshop file!", NRM, LVL_IMMORT, TRUE);
    return;
  }
  
  while (cur) {
    if (cur->sold && cur->price != 0) {
      /* Item has been sold */
      fprintf(fl, "x\n%s %d\n", cur->seller, cur->price);
      fprintf(fl, "%s\n%s\n", cur->objname, cur->buyer);
    } else {
      if (!cur->sold) {
        /* Item is still for sale */
        fprintf(fl, "s\n%s %d\n", cur->seller, cur->price);
      } else {
        /* Item has been removed from sale */
        fprintf(fl, "r\n%s %d\n", cur->seller, cur->price);
        fprintf(fl, "%s\n", cur->buyer);
      }
      fprintf(fl, "%s\n", cur->objname);
      obj = cur->obj;
      fprintf(fl, "%d\n", GET_OBJ_VNUM(obj));
      tmp1 = obj->obj_flags.value[0];
      tmp2 = obj->obj_flags.value[1];
      tmp3 = obj->obj_flags.value[2];
      tmp4 = obj->obj_flags.value[3];
      tmp5 = obj->obj_flags.extra_flags;
      tmp6 = obj->obj_flags.weight;
      tmp7 = obj->obj_flags.timer;

      tmp8 = obj->obj_flags.bitvector;
      tmp21 = obj->obj_flags.bitvector2;

      tmp9 = obj->affected[0].location;
      tmp10 = obj->affected[0].modifier;
      tmp11 = obj->affected[1].location;
      tmp12 = obj->affected[1].modifier;
      tmp13 = obj->affected[2].location;
      tmp14 = obj->affected[2].modifier;
      tmp15 = obj->affected[3].location;
      tmp16 = obj->affected[3].modifier;
      tmp17 = obj->affected[4].location;
      tmp18 = obj->affected[4].modifier;
      tmp19 = obj->affected[5].location;
      tmp20 = obj->affected[5].modifier;

      fprintf(fl, "%d %d %d %d %d %d %d %ld %d %d %d %d %d %d %d %d %d %d %d %d %ld\n",
          tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8,
          tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, 
          tmp15, tmp16, tmp17, tmp18, tmp19, tmp20, tmp21);
      fprintf(fl, "%s\n%s\n", obj->name, obj->description);
    }
    cur = cur->next;
  }
  fprintf(fl, "~\n");
  fclose(fl);
}

int get_pawnshop_num(char *str) {
  /* Returns an item number, given the alias. Returns -1 if the item
     isn't found. The item number refers to items still for sale only */
  int i = 1;
  struct forsale_data *cur = forsale;
  
  while (cur) {
    if (!cur->sold) {
      if (isname(str, cur->obj->name)) return i;
      i++;
    }
    cur = cur->next;
  }
  
  return -1;
}

void ident_pawn_item( struct char_data *ch, int num ) {
  /* This assumes that num exists! */
  int i = 1;
  struct forsale_data *cur = forsale;
  ASPELL(spell_identify);
  
  if (num < 1) {
    send_to_char("This should never happen!\r\n"
                 "Please notify an immmortal.\r\n"
                 "ERROR: Invalid number to ident_pawn_item\r\n",
                 ch);
    log("ERROR: ident_pawn_item: item number too small!");
    return;
  }
  while (i != num && cur) {
    if (!cur->sold) i++;
    cur = cur->next;
  }
  if (!cur) {
    send_to_char("This should never happen!\r\n"
                 "Please notify an immmortal.\r\n"
                 "ERROR: Bad item passed to ident_pawn_item\r\n",
                 ch);
    log("ERROR: ident_pawn_item: item number too large!"); 
    return;
  }
  
  spell_identify(50, ch, NULL, cur->obj, "");
}

ACMD(do_pawnlist) {
  ASPELL(spell_identify);
  struct forsale_data *temp;
  struct char_data *keeper;
  int i, num;

  if (!(keeper = find_shopkeeper(ch))) {
    send_to_char("Sorry but there is nothing to list, no shopkeeper!\r\n", ch);
    return;
  }
  
  argument = one_argument(argument, arg);
  if (*arg) {
    if (*arg >= '0' && *arg <= '9') {
      i = 1;
      temp = forsale;
      num = atoi(arg);
      if (num <= 0) {
        perform_tell(keeper, ch, "Whaddya think I am, stupid? Try somethin' I got!");
        return;
      }
      while (temp) {
        if (!temp->sold) {
          if (i == num) break;
          i++;
        }
        temp = temp->next;
      }
      if (!temp) {
        perform_tell(keeper, ch, "Try somethin' I got in stock! I look psychic, maybe?");
        return;
      }
      if (GET_GOLD(ch) < PAWNSHOP_IDENT) {
        perform_tell(keeper, ch, "Hey, I'm not in dis for my health, ya know!");
        perform_tell(keeper, ch, "Come back when ya can afford da information!");
        return;
      }
      GET_GOLD(ch) -= PAWNSHOP_IDENT;
      sprintf(buf, "$n takes %d coins for the service.", PAWNSHOP_IDENT);
      act(buf, FALSE, keeper, NULL, ch, TO_VICT);
      spell_identify(50, ch, NULL, temp->obj, "");
      return;
    }
  }
  
/*  send_to_char(
    "       Item                                          Price    Sold by"
    "\r\n"
    "---------------------------------------------------------------------"
    "\r\n",ch
  );*/
  send_to_char (
    "Number   Price   Item                                            Sold by"
    "\r\n"
    "-------------------------------------------------------------------------"
    "\r\n",ch
  );
  for (temp = forsale, i = 1; temp; temp = temp->next) {
    if (!temp->sold) {
      sprintf(buf, " %-5d   %-7d %-47s %s\r\n", i, temp->price, temp->objname, temp->seller);
      send_to_char(buf, ch);
      i++;
    }
  }
}

ACMD(do_pawn) {
  struct char_data *keeper;
  struct obj_data *obj = NULL;
  struct forsale_data *temp;
  int count, price;

  if (!(keeper = find_shopkeeper(ch))) {
    send_to_char("Sorry but there is no shopkeeper to sell to!\r\n", ch);
    return;
  }

  /* hey a shopkeep */
  argument = one_argument(argument, arg);

  if(IS_NPC(ch)) {
    perform_tell(keeper, ch, "Hey, goway! No monsters in dis joint!");
    return;
  }
  
  if (!*arg) {
    perform_tell(keeper, ch, "Sure, whaddya wanna sell??");
    return;
  } 

  if (is_number(arg)) {
    count = atoi(arg);
    argument = one_argument(argument, arg);
  } else {
    count = 1;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    perform_tell(keeper, ch, "Ya can't sell what ya don't got.");
    return;
  } 
  
  if (PLR_FLAGGED(ch, PLR_NOPAWN)) {
    perform_tell(keeper, ch, "Hey, I'm not dealin' wit you now more!");
    return;
  }
  
  if (GET_OBJ_VNUM(obj) <= 0 || IS_OBJ_STAT(obj, ITEM_NODROP) ||
     GET_OBJ_TYPE(obj) == ITEM_KEY || GET_OBJ_TYPE(obj) == ITEM_FOOD ||
     GET_OBJ_TYPE(obj) == ITEM_TRASH ||
     IS_OBJ_STAT(obj, ITEM_SUPERCURSED) ||
     IS_OBJ_STAT(obj, ITEM_NOSELL)) {
    perform_tell(keeper, ch, "Hey, I don't want none of dat stuff!");
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_SEED) {
    perform_tell(keeper, ch,
    "Hey, da last thing I need is dat thing crackin' open in my shop!");
    return;
  }

  if (GET_OBJ_TIMER(obj) != 0) {
    perform_tell(keeper, ch, "Hey, none of dat stuff that'll rot on da shelf!");
    return;
  }
  
  argument = one_argument(argument, arg);
  if (!*arg) {
    perform_tell(keeper, ch, "How much ya think you can pawn that off for?");
    return;
  }
  
  price = atoi(arg);
  if (price < 1) {
    perform_tell(keeper, ch, "If yer that desperate to get rid of it, donate it!");
    return;
  }
  
  if (price < MIN_PAWN_PRICE) {
    perform_tell(keeper, ch, "Hey, dis is a class establishment. We don't deal"
                 "wit small-time stuff like dat!");
    return;
  }
  
  perform_tell(keeper, ch, "Okie dokie, I'll see what I kin do fer ya.");
  sprintf(buf2, "Don't forget, da shop takes a %d%% commission!", PAWN_COMMISSION);
  perform_tell(keeper, ch, buf2);
  
  obj_from_char(obj);
  temp = malloc(sizeof(struct forsale_data));
  temp->obj = obj;
  temp->objname = strdup(obj->short_description);
  temp->seller = strdup(ch->player.name);
  temp->buyer = NULL;
  temp->sold = FALSE;
  temp->price = price;
  temp->next = forsale;
  forsale = temp;
  do_save(ch, "", 0, SCMD_QUIET_SAVE);
  save_pawnshop();
  act("$n puts $p up for sale.", TRUE, ch, obj, NULL, TO_ROOM);
  obj_to_char(obj, keeper);
}

void delete_pawn_item( struct forsale_data *obj ) {
  if (obj->seller) free (obj->seller);
  if (obj->buyer) free (obj->buyer);
  if (obj->obj) free (obj->obj);
  if (obj->objname) free (obj->objname);
  free (obj);
}

ACMD(do_pawnbuy) {
  struct char_data *keeper;
/*  struct obj_data *obj = NULL;*/
  struct forsale_data *obj, *prev;
  int count, price, targ;

  if (!(keeper = find_shopkeeper(ch))) {
    send_to_char("Sorry but there is no shopkeeper to sell to!\r\n", ch);
    return;
  }

  if(IS_NPC(ch)) {
    perform_tell(keeper, ch, "Hey, goway! No monsters in dis joint!");
    return;
  }
  
  if (!*arg) {
    perform_tell(keeper, ch, "Ya, great, whaddya wanna buy?");
    return;
  }
  
  argument = one_argument(argument, arg);

  if (arg[0] == '#') strcpy(arg, arg+1);
  
  if (arg[0] < '0' || arg[0] > '9') {
/*    perform_tell(keeper, ch, "Ya gotta buy stuff by number around here.");*/
    targ = get_pawnshop_num(arg);
    if (targ == -1) {
      perform_tell(keeper, ch, "We don't got none of dem!");
      return;
    }
  } else {
    targ = atoi(arg);
  }
  
  if (targ <= 0) {
    perform_tell(keeper, ch, "Hey, don't get smart wit me!");
    return;
  }
 
  prev = NULL;
  for (count = 0, obj = forsale; obj; obj = obj->next) {
    if (!obj->sold) count++;
    if (count == targ) break;
    prev = obj;
  }

  if (!obj) {
    perform_tell(keeper, ch, "We don't got none of them!");
    return;
  }
  
  if (!str_cmp(obj->seller, GET_NAME(ch))) {
    perform_tell(keeper, ch, "Hey dat's yours!");
    perform_tell(keeper, ch, "Here, take it, it was scaring away da clientele!");
    sprintf(buf, "You receive your %s.\r\n", obj->objname);
    send_to_char(buf, ch);
    act("$n gives $N $p.", TRUE, keeper, obj->obj, ch, TO_NOTVICT);
    obj_from_char(obj->obj);
    obj_to_char(obj->obj, ch);
    obj->obj = NULL;
    if (prev) {
      prev->next = obj->next;
    } else {
      forsale = obj->next;
    }
    delete_pawn_item( obj );
    save_pawnshop();
    do_save(ch, "", 0, SCMD_QUIET_SAVE);
    return;
  }
  
  price = obj->price;
  if (GET_GOLD(ch) < price) {
    perform_tell(keeper, ch, "Nice, huh? Come back when you got da cash, ya bum!");
    return;
  }
  
  perform_tell(keeper, ch, "Great deal, I know. Ya come back to us fer all yer shopping needs!");
  sprintf(buf, "Congratulations! You now have %s!\r\n", obj->objname);
  send_to_char(buf, ch);
  act("$n buys $p.", TRUE, ch, obj->obj, 0, TO_ROOM);
  
  GET_GOLD(ch) -= price;
 
 
  obj->sold = TRUE;
  obj_from_char(obj->obj);
  obj_to_char(obj->obj, ch);
  obj->obj = NULL;
  obj->buyer = strdup(GET_NAME(ch));
  do_save(ch, "", 0, SCMD_QUIET_SAVE);
  save_pawnshop();
}

ACMD(do_claim) {
  extern struct room_data *world;
  struct char_data *keeper;
  struct forsale_data *cur, *prev, *tmp;
  int price;
  bool any;
  
  if (world[ch->in_room].number != PAWNSHOP_VNUM) {
    send_to_char("You can only claim in the pawn shop!\r\n", ch);
    return;
  }

  if (!(keeper = find_shopkeeper(ch))) {
    send_to_char("Sorry but there is no shopkeeper to deal with!\r\n", ch);
    return;
  }
  
  if (IS_NPC(ch)) {
    perform_tell(keeper, ch, "Hey, no monsters! Git out!");
    return;
  }
  
  any = FALSE;
  for (cur = forsale, prev = NULL; cur;) {
    if (!str_cmp(cur->seller, GET_NAME(ch))) {
      any = TRUE;
      if (cur->sold) {
        if (cur->price > 0) {
          sprintf(buf2, "%s bought %s for %d coins.", cur->buyer,
             cur->objname, cur->price);
          perform_tell(keeper, ch, buf2);
          perform_tell(keeper, ch, "Here's your cut.");
          price = (float)cur->price * (1 - (float)PAWN_COMMISSION / 100.0);
          sprintf(buf, "$n gives you %d coins.", price);
          act(buf, TRUE, keeper, 0, ch, TO_VICT);
          act("$n gives $N some money.", TRUE, keeper, 0, ch, TO_NOTVICT);
          GET_GOLD(ch) += price;
        } else {
          sprintf(buf2, "I had to pull %s from my shelves! Think before you sell!",
                  cur->objname);
          perform_tell(keeper, ch, buf2);
          act("$n gives $N $p.", TRUE, keeper, cur->obj, ch, TO_NOTVICT);
          obj_from_char(cur->obj);
          obj_to_char(cur->obj, ch);
          act("$n gives you $p.", FALSE, keeper, cur->obj, ch, TO_VICT);
          cur->obj = NULL;
        }
        tmp = cur->next;
        if (prev) {
          prev->next = cur->next;
        } else {
          forsale = cur->next;
        }
        delete_pawn_item(cur);
        cur = tmp;
      } else {
        sprintf(buf2, "No one bought %s.", cur->objname);
        perform_tell(keeper, ch, buf2);
        prev = cur;
        cur = cur->next;
      }
    } else {
      prev = cur;
      cur = cur->next;
    }
  }
  if (!any) {
    perform_tell(keeper, ch, "Hey, ya ain't dealt wit me yet! Nothin' to claim!");
  }
  do_save(ch, "", 0, SCMD_QUIET_SAVE);
  save_pawnshop();
}

void show_pawnshop(struct char_data *ch) {
  int curnum, forsalenum;
  struct forsale_data *cur = forsale;
  
  curnum = forsalenum = 0;
  
  send_to_char("Position    Status       Sold by      Item\r\n"
"--------------------------------------------------------------------------\r\n"
  ,ch);
  
  while (cur) {
    curnum++;
    if (cur->sold) {
      sprintf(buf, "%-7d    %s%-12s %-12s %s\r\n", curnum,
              (cur->price > 0) ? " " : "*",
              cur->buyer, cur->seller, cur->objname);
    } else {
      forsalenum++;
      sprintf(buf, "%-7d      #%-10d %-12s %s\r\n", curnum, forsalenum,
              cur->seller, cur->objname);
    }
    send_to_char(buf, ch);
    cur = cur->next;
  }
}

ACMD(do_pawnremove) {
  int i = 1, num;
  struct forsale_data *cur = forsale;
  extern struct room_data *world;
  
  if (IS_NPC(ch)) {
    send_to_char("I don't THINK so!\r\n", ch);
    return;
  }
  
  if (world[ch->in_room].number != PAWNSHOP_VNUM) {
    /* Just to stop people from accidentally removing stuff */
    sprintf(buf, "This only works in the pawnshop (#%d)\r\n", PAWNSHOP_VNUM);
    send_to_char(buf, ch);
    return;
  }
  
  argument = one_argument(argument, arg);
  
  if (!arg) num = 0;
  else if (!*arg) num = 0;
  else num = atoi(arg);
  
  if (num <= 0 ) {
    send_to_char ("Remove which item?\r\n", ch);
    return;
  }
  
  while (cur) {
    if (!cur->sold) {
      if (i == num) break;
      i++;
    }
    cur = cur->next;
  }
  
  if (!cur) {
    send_to_char("That item does not exist!\r\n", ch);
    return;
  }
  
  cur->sold = TRUE;
  cur->price = 0;
  cur->buyer = strdup(GET_NAME(ch));
  send_to_char("Item removed from sale.\r\n", ch);
  save_pawnshop();
}

bool has_claim(struct char_data *ch) {
  struct forsale_data *cur;
  
  for (cur = forsale; cur; cur = cur->next)
    if (!str_cmp(cur->seller, GET_NAME(ch)) && cur->sold)
      return TRUE;
      
  return FALSE;
}
