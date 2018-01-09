/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - medit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "olc.h"

/*-------------------------------------------------------------------*/
/* external variables */
extern struct index_data *mob_index;			/*. db.c    	.*/
extern struct char_data *mob_proto;			/*. db.c    	.*/
extern struct char_data *character_list;		/*. db.c    	.*/
extern int top_of_mobt;					/*. db.c    	.*/
extern struct zone_data *zone_table;			/*. db.c    	.*/
extern int top_of_zone_table;				/*. db.c    	.*/
extern struct player_special_data dummy_mob;		/*. db.c    	.*/	
extern struct attack_hit_type attack_hit_text[]; 	/*. fight.c 	.*/
extern char *action_bits[];				/*. constants.c .*/
extern char *affected_bits[];				/*. constants.c .*/
extern char *affected2_bits[];
extern char *position_types[];				/*. constants.c .*/
extern char *genders[];					/*. constants.c .*/
extern int top_shop;					/*. shop.c	.*/
extern struct shop_data *shop_index;			/*. shop.c	.*/
extern struct descriptor_data *descriptor_list;		/*. comm.c	.*/

/*-------------------------------------------------------------------*/
/*. Handy  macros .*/

#define GET_NDD(mob) ((mob)->mob_specials.damnodice)
#define GET_SDD(mob) ((mob)->mob_specials.damsizedice)
#define GET_ALIAS(mob) ((mob)->player.name)
#define GET_SDESC(mob) ((mob)->player.short_descr)
#define GET_LDESC(mob) ((mob)->player.long_descr)
#define GET_DDESC(mob) ((mob)->player.description)
#define GET_ATTACK(mob) ((mob)->mob_specials.attack_type)
#define S_KEEPER(shop) ((shop)->keeper)

/*-------------------------------------------------------------------*/
/*. Function prototypes .*/

void medit_parse(struct descriptor_data * d, char *arg);
void medit_disp_menu(struct descriptor_data * d);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(struct descriptor_data *d);
void init_mobile(struct char_data *mob);
void copy_mobile(struct char_data *tmob, struct char_data *fmob);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

void medit_setup_new(struct descriptor_data *d)
{ struct char_data *mob;

  /*. Alloc some mob shaped space .*/
  CREATE(mob, struct char_data, 1);
  init_mobile(mob);
  
  GET_MOB_RNUM(mob) = -1;
  /*. default strings .*/
  GET_ALIAS(mob) = strdup("mob unfinished");
  GET_SDESC(mob) = strdup("the unfinished mob");
  GET_LDESC(mob) = strdup("An unfinished mob stands here.\r\n");
  GET_DDESC(mob) = strdup("It looks, err, unfinished.\r\n");

  OLC_MOB(d) = mob;
  OLC_VAL(d) = 0;   /*. Has changed flag .*/
  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num)
{ struct char_data *mob;

  /*. Alloc some mob shaped space .*/
  CREATE(mob, struct char_data, 1);
  copy_mobile(mob, mob_proto + rmob_num);
  OLC_MOB(d) = mob;
  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/
/*. Copy one mob struct to another .*/

void copy_mobile(struct char_data *tmob, struct char_data *fmob)
{
  /*. Free up any used strings .*/
  if (GET_ALIAS(tmob))
    free(GET_ALIAS(tmob));
  if (GET_SDESC(tmob))
    free(GET_SDESC(tmob));
  if (GET_LDESC(tmob))
    free(GET_LDESC(tmob));
  if (GET_DDESC(tmob))
    free(GET_DDESC(tmob));
  
  /*.Copy mob .*/
  *tmob = *fmob;
 
  /*. Realloc strings .*/
  if (GET_ALIAS(fmob))
    GET_ALIAS(tmob) = strdup(GET_ALIAS(fmob));

  if (GET_SDESC(fmob))
    GET_SDESC(tmob) = strdup(GET_SDESC(fmob));

  if (GET_LDESC(fmob))
    GET_LDESC(tmob) = strdup(GET_LDESC(fmob));

  if (GET_DDESC(fmob))
    GET_DDESC(tmob) = strdup(GET_DDESC(fmob));

}


/*-------------------------------------------------------------------*/
/*. Ideally, this function should be in db.c, but I'll put it here for
    portability.*/

void init_mobile(struct char_data *mob)
{
  clear_char(mob);

  GET_HIT(mob) = 1;
  GET_MANA(mob) = 1;
  GET_MAX_MANA(mob) = 100;
  GET_MAX_MOVE(mob) = 100;
  GET_NDD(mob) = 1;
  GET_SDD(mob) = 1;
  GET_WEIGHT(mob) = 200;
  GET_HEIGHT(mob) = 198;

  mob->real_abils.str = 11;
  mob->real_abils.intel = 11;
  mob->real_abils.wis = 11;
  mob->real_abils.dex = 11;
  mob->real_abils.con = 11;
  mob->real_abils.cha = 11;
  mob->aff_abils = mob->real_abils;

  SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
  mob->player_specials = &dummy_mob;

/*
  GET_RACE(mob) = RACE_UNDEFINED;
  GET_CLASS(mob) = CLASS_UNDEFINED;
*/
}

/*-------------------------------------------------------------------*/
/*. Save new/edited mob to memory .*/

#define ZCMD zone_table[zone].cmd[cmd_no]

void medit_save_internally(struct descriptor_data *d)
{
  int rmob_num, found = 0, new_mob_num = 0, zone, cmd_no, shop;
  struct char_data *new_proto;
  struct index_data *new_index;
  struct char_data *live_mob;
  struct descriptor_data *dsc;

  rmob_num = real_mobile(OLC_NUM(d));

  /*. Mob exists? Just update it .*/
  if (rmob_num != -1)
  { copy_mobile((mob_proto + rmob_num), OLC_MOB(d));
    /*. Update live mobiles .*/
    for(live_mob = character_list; live_mob; live_mob = live_mob->next)
      if(IS_MOB(live_mob) && GET_MOB_RNUM(live_mob) == rmob_num)
      { /*. Only really need update the strings, since these can cause
            protection faults.  The rest can wait till a reset/reboot .*/
        GET_ALIAS(live_mob) = GET_ALIAS(mob_proto + rmob_num);
        GET_SDESC(live_mob) = GET_SDESC(mob_proto + rmob_num);
        GET_LDESC(live_mob) = GET_LDESC(mob_proto + rmob_num);
        GET_DDESC(live_mob) = GET_DDESC(mob_proto + rmob_num);
      }
  } 
  /*. Mob does not exist, hafta add it .*/
  else
  {
    CREATE(new_proto, struct char_data, top_of_mobt + 2);
    CREATE(new_index, struct index_data, top_of_mobt + 2);

    for (rmob_num = 0; rmob_num <= top_of_mobt; rmob_num++)
    { if (!found)
      { /*. Is this the place?  .*/
        if ((rmob_num > top_of_mobt) ||
            (mob_index[rmob_num].virtual > OLC_NUM(d)))
        { /*. Yep, stick it here .*/
          found = 1;
          new_index[rmob_num].virtual = OLC_NUM(d);
          new_index[rmob_num].number = 0;
          new_index[rmob_num].func = NULL;
          new_mob_num = rmob_num;
          GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
          copy_mobile((new_proto + rmob_num), OLC_MOB(d));
          /*. Copy the mob that should be here on top .*/
          new_index[rmob_num + 1] = mob_index[rmob_num];
          new_proto[rmob_num + 1] = mob_proto[rmob_num];
          GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
        } else
        { /*. Nope, copy over as normal.*/
          new_index[rmob_num] = mob_index[rmob_num];
          new_proto[rmob_num] = mob_proto[rmob_num];
        }
      } else
      { /*. We've already found it, copy the rest over .*/
        new_index[rmob_num + 1] = mob_index[rmob_num];
        new_proto[rmob_num + 1] = mob_proto[rmob_num];
        GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
      }
    }
    if (!found)
    { /*. Still not found, must add it to the top of the table .*/
      new_index[rmob_num].virtual = OLC_NUM(d);
      new_index[rmob_num].number = 0;
      new_index[rmob_num].func = NULL;
      new_mob_num = rmob_num;
      GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
      copy_mobile((new_proto + rmob_num), OLC_MOB(d));
    }

    /*. Replace tables .*/
#if (0)
    /*** Culvan hacked out this line because it crashed the mud.   ***\
    |*** It's just a flaky pointer that I can't debug. I think it  ***|
    |*** works out to a memory leak of a whole 14 bytes or         ***|
    |*** something silly like that. Not worth the effort, IMO. You ***|
    \*** can now edit mobs without a crash.                        ***/
    free(mob_index);
#endif
    
    free(mob_proto);
    mob_index = new_index;
    mob_proto = new_proto;
    top_of_mobt++;

    /*. Update live mobile rnums .*/
    for(live_mob = character_list; live_mob; live_mob = live_mob->next)
      if(GET_MOB_RNUM(live_mob) > new_mob_num)
        GET_MOB_RNUM(live_mob)++;
    
    /*. Update zone table .*/
    for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) 
        if (ZCMD.command == 'M')
          if (ZCMD.arg1 > new_mob_num)
            ZCMD.arg1++;

    /*. Update shop keepers .*/
    for(shop = 0; shop <= top_shop; shop++)
      if(SHOP_KEEPER(shop) > new_mob_num)
        SHOP_KEEPER(shop)++;

    /*. Update keepers in shops being edited .*/
    for(dsc = descriptor_list; dsc; dsc = dsc->next)
      if(dsc->connected == CON_SEDIT)
        if(S_KEEPER(OLC_SHOP(dsc)) >= new_mob_num)
          S_KEEPER(OLC_SHOP(dsc))++;
  }
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_MOB);
}


/*-------------------------------------------------------------------*/
/*. Save ALL mobiles for a zone to their .mob file, mobs are all 
    saved in Extended format, regardless of whether they have any
    extended fields.  Thanks to Samedi for ideas on this bit of code.*/

void medit_save_to_disk(struct descriptor_data *d)
{ 
  int i, rmob_num, zone, top;
  FILE *mob_file;
  char fname[64];
  struct char_data *mob;

  zone = zone_table[OLC_ZNUM(d)].number; 
  top = zone_table[OLC_ZNUM(d)].top; 

  sprintf(fname, "%s/%.3i.mob", MOB_PREFIX, zone);

  if(!(mob_file = fopen(fname, "w")))
  { mudlog("SYSERR: OLC: Cannot open mob file!", BRF, LVL_BUILDER, TRUE);
    return;
  }

  /*. Seach database for mobs in this zone and save em .*/
  for(i = zone * 100; i <= top; i++)
  { rmob_num = real_mobile(i);
    
    if(rmob_num != -1) 
    { if(fprintf(mob_file, "#%d\n", i) < 0)
      { mudlog("SYSERR: OLC: Cannot write mob file!\r\n", BRF, LVL_BUILDER, TRUE);
        fclose(mob_file);
        return;
      }
      mob = (mob_proto + rmob_num);

      /*. Clean up strings .*/
      strcpy (buf1, GET_LDESC(mob) ? GET_LDESC(mob) : "undefined");
      strip_string(buf1);
      strcpy(buf2, GET_DDESC(mob) ? GET_DDESC(mob) : "undefined");
      strip_string(buf2);

      fprintf(mob_file, 
	"%s~\n"
	"%s~\n"
	"%s~\n"
	"%s~\n"
	"%ld %ld %i S\n" 
	"%d %d %i %dd%d+%d %dd%d+%d\n" 
	"%ld %ld\n"  /*. Gold & Exp are longs in my mud, ignore any warning .*/
	"%d %d %d",
	GET_ALIAS(mob) ? GET_ALIAS(mob) : "undefined",
	GET_SDESC(mob) ? GET_SDESC(mob) : "undefined",
	buf1,
	buf2,
	MOB_FLAGS(mob),
	AFF_FLAGS(mob), 
  	GET_ALIGNMENT(mob), 
  	GET_LEVEL(mob),
  	20 - GET_HITROLL(mob), /*. Convert hitroll to thac0 .*/
        GET_AC(mob) / 10,
	GET_HIT(mob),
	GET_MANA(mob),
	GET_MOVE(mob),
	GET_NDD(mob),
	GET_SDD(mob),
	GET_DAMROLL(mob),
	(long)GET_GOLD(mob),
	(long)GET_EXP(mob),
	GET_POS(mob),
	GET_DEFAULT_POS(mob),
	GET_SEX(mob)
      );

      /*. Deal with Extra stats in case they are there .*/
      if (GET_ATTACK(mob) != 0)
        fprintf(mob_file, " %d\n", GET_ATTACK(mob));
      else
        fprintf(mob_file, "\n");
/*
      if(GET_ATTACK(mob) != 0)
	fprintf(mob_file, "BareHandAttack: %d\n", GET_ATTACK(mob));
      if(GET_STR(mob) != 11)
        fprintf(mob_file, "Str: %d\n", GET_STR(mob));
      if(GET_ADD(mob) != 0)
        fprintf(mob_file, "StrAdd: %d\n", GET_ADD(mob));
      if(GET_DEX(mob) != 11)
        fprintf(mob_file, "Dex: %d\n", GET_DEX(mob));
      if(GET_INT(mob) != 11)
        fprintf(mob_file, "Int: %d\n", GET_INT(mob));
      if(GET_WIS(mob) != 11)
        fprintf(mob_file, "Wis: %d\n", GET_WIS(mob));
      if(GET_CON(mob) != 11)
        fprintf(mob_file, "Con: %d\n", GET_CON(mob));
      if(GET_CHA(mob) != 11)
        fprintf(mob_file, "Cha: %d\n", GET_CHA(mob));
*/
      /*. Add E-mob handlers here .*/

/*
      fprintf(mob_file, "E\n");
*/
    }
  }
  fprintf(mob_file, "#99999\n$~\n");
  fclose(mob_file);
  olc_remove_from_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_MOB);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/
/*. Display poistions (sitting, standing etc) .*/

void medit_disp_positions(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; *position_types[i] != '\n'; i++)
  {  sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
     send_to_char(buf, d->character);
  }
  send_to_char("Enter position number : ", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display sex (Oooh-err).*/

void medit_disp_sex(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_GENDERS; i++)
  {  sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
     send_to_char(buf, d->character);
  }
  send_to_char("Enter gender number : ", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display attack types menu .*/

void medit_disp_attack_types(struct descriptor_data *d)
{
  int i;
   
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_ATTACK_TYPES; i++)
  {  sprintf(buf, "%s%2d%s) %s\r\n", 
	grn, i, nrm, attack_hit_text[i].singular
     );
     send_to_char(buf, d->character);
  }
  send_to_char("Enter attack type : ", d->character);
}
 

/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/

void medit_disp_mob_flags(struct descriptor_data *d)
{
  int i, columns = 0;
  
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_MOB_FLAGS; i++)
  {  sprintf(buf, "%s%2d%s) %-20.20s  ",
	grn, i+1, nrm, action_bits[i]
     );
     if(!(++columns % 2))
       strcat(buf, "\r\n");
     send_to_char(buf, d->character);
  }
  sprintbit(MOB_FLAGS(OLC_MOB(d)), action_bits, buf1);
  sprintf(buf, "\r\n"
	"Current flags : %s%s%s\r\n"
	"Enter mob flags (0 to quit) : ",
        cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void medit_disp_aff_flags(struct descriptor_data *d)
{
  int i, columns = 0;
  
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_AFF_FLAGS; i++)
  {  sprintf(buf, "%s%2d%s) %-20.20s  ", 
	grn, i+1, nrm, affected_bits[i]
     );
     if(!(++columns % 2))
       strcat(buf, "\r\n");
     send_to_char(buf, d->character);
  }
  sprintbit(AFF_FLAGS(OLC_MOB(d)), affected_bits, buf1);
  sprintf(buf, "\r\n"
	"Current flags   : %s%s%s\r\n"
	"Enter aff flags (0 to quit) : ",
        cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}
  
/*-------------------------------------------------------------------*/
/*. Display main menu .*/

void medit_disp_menu(struct descriptor_data * d)
{
  struct char_data *mob;

  mob = OLC_MOB(d);
  get_char_cols(d->character);

  sprintf(buf, "[H[J"
	"-- Mob Number:  [%s%d%s]\r\n"
        "%s1%s) Keywords : %s%s\r\n"
        "%s2%s) Name     : %s%s\r\n"
	"%s3%s) Look     :-\r\n%s%s"
	"%s4%s) Desc     :-\r\n%s%s",

        cyn, OLC_NUM(d), nrm,
        grn, nrm, yel, GET_ALIAS(mob),
        grn, nrm, yel, GET_SDESC(mob),
        grn, nrm, yel, GET_LDESC(mob),
        grn, nrm, yel, GET_DDESC(mob) 
  );
  send_to_char(buf, d->character);

  sprintbit(MOB_FLAGS(mob), action_bits, buf1);
  sprintbit(AFF_FLAGS(mob), affected_bits, buf2);
  sprintf(buf,
        "%s5%s) NPC Flags : %s%s\r\n"
        "%s6%s) AFF Flags : %s%s\r\n",
        grn, nrm, cyn, buf1,
        grn, nrm, cyn, buf2 
  );
  send_to_char(buf, d->character);

  sprintf(buf, 
	"%s7%s) Alignment : %s%-12d%s %s8%s) Level: %s%-8d%s "
        "%s9%s) Hitroll : %s%-10d%s\r\n"
        "%sABC%s) Barehand damage : %s%dd%d+%d%s\r\n"
	"%sDEF%s) Hit points :      %s%dd%d+%d%s\r\n"
	"%sG%s) Armor Class : %s%-10d%s %sH%s) Exp : %s%-10ld%s "
        "%sI%s) Gold : %s%-10ld%s\r\n",

	grn, nrm, cyn, GET_ALIGNMENT(mob), nrm,
        grn, nrm, cyn, GET_LEVEL(mob), nrm,
	grn, nrm, cyn, GET_HITROLL(mob), nrm,
	grn, nrm, cyn, GET_NDD(mob),
	               GET_SDD(mob),
                       GET_DAMROLL(mob), nrm,
	grn, nrm, cyn, GET_HIT(mob),
	               GET_MANA(mob),
	               GET_MOVE(mob), nrm,
	grn, nrm, cyn, GET_AC(mob), nrm, 
        /* Gold & Exp are longs in my mud, ignore any warnings */
	grn, nrm, cyn, (long)GET_EXP(mob), nrm,
	grn, nrm, cyn, (long)GET_GOLD(mob), nrm
  );
  send_to_char(buf, d->character);

  sprintf(buf,
	"%sJ%s) Position  : %s%s\r\n"
        "%sK%s) Default   : %s%s\r\n"
        "%sL%s) Sex       : %s%-7.7s%s\r\n"
        "%sM%s) Attack    : %s%s\r\n"
        "%sQ%s) Quit\r\n"
        "Enter choice : ",

	grn, nrm, yel, position_types[(int)GET_POS(mob)],
	grn, nrm, yel, position_types[(int)GET_DEFAULT_POS(mob)],
        grn, nrm, yel, genders[(int)GET_SEX(mob)], nrm,
        grn, nrm, yel, attack_hit_text[GET_ATTACK(mob)].singular,
        grn, nrm
  );
  send_to_char(buf, d->character);

  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void medit_parse(struct descriptor_data * d, char *arg)
{ int i;

  if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE)
  { if(!*arg || (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))))
    { send_to_char("Field must be numerical, try again : ", d->character);
      return;
    }
  }

  switch (OLC_MODE(d)) 
  {
/*-------------------------------------------------------------------*/
  case MEDIT_CONFIRM_SAVESTRING:
    /*. Ensure mob has MOB_ISNPC set or things will go pair shaped .*/
    SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
    switch (*arg) {
    case 'y':
    case 'Y':
      /*. Save the mob in memory and to disk  .*/
      send_to_char("Saving mobile to memory.\r\n", d->character);
      medit_save_internally(d);
      sprintf(buf, "OLC: %s edits mob %d", GET_NAME(d->character),
               OLC_NUM(d));	      
      mudlog(buf, CMP, LVL_BUILDER, TRUE);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save the mobile? : ", d->character);
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  case MEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) 
    { case 'q':
      case 'Q':
        if (OLC_VAL(d)) /*. Anything been changed? .*/
        { send_to_char("Do you wish to save the changes to the mobile? (y/n) : ", d->character);
          OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
        } else
          cleanup_olc(d, CLEANUP_ALL);
        return;
      case '1': /* keywords */
        OLC_MODE(d) = MEDIT_ALIAS;
        i--;
        send_to_char("Enter keywords : ", d->character);
        break;
      case '2': /* name */
        OLC_MODE(d) = MEDIT_S_DESC;
        i--;
        send_to_char("Enter name : ", d->character);
        break;
      case '3': /* look */
        OLC_MODE(d) = MEDIT_L_DESC;
        i--;
        send_to_char("Enter look : ", d->character);
        break;
      case '4': /* desc */
        send_to_char("Enter description :-\r\n", d->character);
        /*. Pass control to modify.c .*/
        OLC_MODE(d) = MEDIT_D_DESC;
        d->str = (char **) malloc(sizeof(char *));
        *(d->str) = NULL;
        d->max_str = MAX_MOB_DESC;
        d->mail_to = 0;
        OLC_VAL(d) = 1;
        return;
      case '5':
        OLC_MODE(d) = MEDIT_NPC_FLAGS;
        medit_disp_mob_flags(d);
        return;
      case '6':
        OLC_MODE(d) = MEDIT_AFF_FLAGS;
        medit_disp_aff_flags(d);
        return;
      case '7':
        OLC_MODE(d) = MEDIT_ALIGNMENT;
        i++;
        send_to_char("Enter alignment : ", d->character);
        break;
      case '8':
        OLC_MODE(d) = MEDIT_LEVEL;
        i++;
        send_to_char("Enter level : ", d->character);
        break;
      case '9':
        OLC_MODE(d) = MEDIT_HITROLL;
        i++;
        send_to_char("Enter hitroll : ", d->character);
        break;
      case 'a':
      case 'A':
        OLC_MODE(d) = MEDIT_NDD;
        i++;
        send_to_char("Enter number damage dice : ", d->character);
        break;
      case 'b':
      case 'B':
        OLC_MODE(d) = MEDIT_SDD;
        i++;
        send_to_char("Enter sides damage dice : ", d->character);
        break;
      case 'c':
      case 'C':
        OLC_MODE(d) = MEDIT_DAMROLL;
        i++;
        send_to_char("Enter damroll : ", d->character);
        break;
      case 'd':
      case 'D':
        OLC_MODE(d) = MEDIT_NUM_HP_DICE;
        i++;
        send_to_char("Enter number hp dice : ", d->character);
        break;
      case 'e':
      case 'E':
        OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
        i++;
        send_to_char("Enter sides hp dice : ", d->character);
        break;
      case 'f':
      case 'F':
        OLC_MODE(d) = MEDIT_ADD_HP;
        i++;
        send_to_char("Enter bonus hp : ", d->character);
        break;
      case 'g':
      case 'G':
        OLC_MODE(d) = MEDIT_AC;
        i++;
        send_to_char("Enter armor class : ", d->character);
        break;
      case 'h':
      case 'H':
        OLC_MODE(d) = MEDIT_EXP;
        i++;
        send_to_char("Enter exps : ", d->character);
        break;
      case 'i':
      case 'I':
        OLC_MODE(d) = MEDIT_GOLD;
        i++;
        send_to_char("Enter gold : ", d->character);
        break;
      case 'j':
      case 'J':
        OLC_MODE(d) = MEDIT_POS;
        medit_disp_positions(d);
        return;
      case 'k':
      case 'K':
        OLC_MODE(d) = MEDIT_DEFAULT_POS;
        medit_disp_positions(d);
        return;
      case 'l':
      case 'L':
        OLC_MODE(d) = MEDIT_SEX;
        medit_disp_sex(d);
        return;
      case 'm':
      case 'M':
        OLC_MODE(d) = MEDIT_ATTACK;
        medit_disp_attack_types(d);
        return;
      default:
        medit_disp_menu(d);
	return;
    }
    /* new custom messages above */
    if (i==1)
    {
       /* send_to_char("\r\nEnter new value : ", d->character); */
       return;
    }
    if (i==-1)
    {
       /* send_to_char("\r\nEnter new text :\r\n| ", d->character); */
       return;
    }
    break; 

/*-------------------------------------------------------------------*/
  case MEDIT_ALIAS:
    if(GET_ALIAS(OLC_MOB(d)))
      free(GET_ALIAS(OLC_MOB(d)));
    GET_ALIAS(OLC_MOB(d)) = strdup(arg); 
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_S_DESC:
    if(GET_SDESC(OLC_MOB(d)))
      free(GET_SDESC(OLC_MOB(d)));
    GET_SDESC(OLC_MOB(d)) = strdup(arg); 
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_L_DESC:
    if(GET_LDESC(OLC_MOB(d)))
      free(GET_LDESC(OLC_MOB(d)));
    strcpy(buf, arg);
    strcat(buf, "\r\n");
    GET_LDESC(OLC_MOB(d)) = strdup(buf); 
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_D_DESC:
    /*. We should never get here .*/
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!",BRF,LVL_BUILDER,TRUE);
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_NPC_FLAGS:
    i = atoi(arg);
    if (i==0)
      break;
    if (!((i < 0) || (i > NUM_MOB_FLAGS)))
    { i = 1 << (i - 1);
      if (IS_SET(MOB_FLAGS(OLC_MOB(d)), i))
        REMOVE_BIT(MOB_FLAGS(OLC_MOB(d)), i);
      else
        SET_BIT(MOB_FLAGS(OLC_MOB(d)), i);
    }
    medit_disp_mob_flags(d);
    return;
/*-------------------------------------------------------------------*/
  case MEDIT_AFF_FLAGS:
    i = atoi(arg);
    if (i==0)
      break;
    if (!((i < 0) || (i > NUM_AFF_FLAGS)))
    { i = 1 << (i - 1);
      if (IS_SET(AFF_FLAGS(OLC_MOB(d)), i))
        REMOVE_BIT(AFF_FLAGS(OLC_MOB(d)), i);
      else
        SET_BIT(AFF_FLAGS(OLC_MOB(d)), i);
    }
    medit_disp_aff_flags(d);
    return;
/*-------------------------------------------------------------------*/
/*. Numerical responses .*/

  case MEDIT_SEX:
    GET_SEX(OLC_MOB(d)) = MAX(0, MIN(NUM_GENDERS -1, atoi(arg)));
    break;

  case MEDIT_HITROLL:
    GET_HITROLL(OLC_MOB(d)) = MAX(0, MIN(50, atoi(arg)));
    break;

  case MEDIT_DAMROLL:
    GET_DAMROLL(OLC_MOB(d)) = MAX(0, MIN(50, atoi(arg)));
    break;

  case MEDIT_NDD:
    GET_NDD(OLC_MOB(d)) = MAX(0, MIN(30, atoi(arg)));
    break;

  case MEDIT_SDD:
    GET_SDD(OLC_MOB(d)) = MAX(0, MIN(127, atoi(arg)));
    break;

  case MEDIT_NUM_HP_DICE:
    GET_HIT(OLC_MOB(d)) = MAX(0, MIN(30, atoi(arg)));
    break;

  case MEDIT_SIZE_HP_DICE:
    GET_MANA(OLC_MOB(d)) = MAX(0, MIN(1000, atoi(arg)));
    break;

  case MEDIT_ADD_HP:
    GET_MOVE(OLC_MOB(d)) = MAX(0, MIN(30000, atoi(arg)));
    break;

  case MEDIT_AC:
    GET_AC(OLC_MOB(d)) = MAX(-200, MIN(200, atoi(arg)));
    break;

  case MEDIT_EXP:
    GET_EXP(OLC_MOB(d)) = MAX(0, atol(arg));
    break;

  case MEDIT_GOLD:
    GET_GOLD(OLC_MOB(d)) = MAX(0, atol(arg));
    break;

  case MEDIT_POS:
    GET_POS(OLC_MOB(d)) = MAX(0, MIN(NUM_POSITIONS-1, atoi(arg)));
    break;

  case MEDIT_DEFAULT_POS:
    GET_DEFAULT_POS(OLC_MOB(d)) = MAX(0, MIN(NUM_POSITIONS-1, atoi(arg)));
    break;

  case MEDIT_ATTACK:
    GET_ATTACK(OLC_MOB(d)) = MAX(0, MIN(NUM_ATTACK_TYPES-1, atoi(arg)));
    break;

  case MEDIT_LEVEL:
    GET_LEVEL(OLC_MOB(d)) = MAX(1, MIN(100, atoi(arg)));
    break;

  case MEDIT_ALIGNMENT:
    GET_ALIGNMENT(OLC_MOB(d)) = MAX(-1000, MIN(1000, atoi(arg)));
    break;

/*-------------------------------------------------------------------*/
  default:
    /*. We should never get here .*/
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached default case!",BRF,LVL_BUILDER,TRUE);
    break;
  }
/*-------------------------------------------------------------------*/
/*. END OF CASE 
    If we get here, we have probably changed something, and now want to
    return to main menu.  Use OLC_VAL as a 'has changed' flag .*/

  OLC_VAL(d) = 1;
  medit_disp_menu(d);
}
/*. End of medit_parse() .*/

