/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*       ROM 2.4 is copyright 1993-1995 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@pacinfo.com)                              *
*           Gabrielle Taylor (gtaylor@pacinfo.com)                         *
*           Brian Moore (rom@rom.efn.org)                                  *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

/***************************************************************************
*  Automated Quest code written by Vassago of MOONGATE, moongate.ams.com   *
*  4000. Copyright (c) 1996 Ryan Addams, All Rights Reserved. Use of this  * 
*  code is allowed provided you add a credit line to the effect of:        *
*  "Quest Code (c) 1996 Ryan Addams" to your logon screen with the rest    *
*  of the standard diku/rom credits. If you use this or a modified version *
*  of this code, let me know via email: moongate@moongate.ams.com. Further *
*  updates will be posted to the rom mailing list. If you'd like to get    *
*  the latest version of quest.c, please send a request to the above add-  *
*  ress. Quest Code v2.00.                                                 *
***************************************************************************/

/***************************************************************************
*  Ported to SMAUG by Vir of Eternal Struggle (es.mudservices.com 4321)    *
*  Additional changes to make life easier also by Vir.  Quest Code         *
*  originally (C)opyright 1996 Ryan Addams of MOONGATE.  Thanx for the     *
*  code, Ryan!! For more SMAUG code, e-mail "leckey@rogers.wave.ca"        *
***************************************************************************/

/***************************************************************************
*  Ported to SMAUG 1.4 by Karn.                                            *
***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "handler.h"

/* Object vnums for Quest Rewards */

#define QUEST_ITEM1 670
#define QUEST_ITEM2 671
#define QUEST_ITEM3 672
#define QUEST_ITEM4 673
#define QUEST_ITEM5 674

/* Quest point costs for above objects */

#define QUEST_VALUE1 1000
#define QUEST_VALUE2 1000
#define QUEST_VALUE3 1000
#define QUEST_VALUE4 1000
#define QUEST_VALUE5 1000

/* Object vnums for object quest 'tokens'. In Moongate, the tokens are
   things like 'the Shield of Moongate', 'the Sceptre of Moongate'. These
   items are worthless and have the rot-death flag, as they are placed
   into the world when a player receives an object quest. */

/* I've set these to newbie objects from newdark.are so you won't get
   errors when using with stock smaug 1.4.  You need to redefine these
   for your mud  - Karn */

#define QUEST_OBJQUEST1 690
#define QUEST_OBJQUEST2 691
#define QUEST_OBJQUEST3 692
#define QUEST_OBJQUEST4 693
#define QUEST_OBJQUEST5 694


/* external functions */

ACMD(do_say);
extern struct char_data *get_char_world( struct char_data *ch, char *argument );
extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *character_list;
sh_int find_target_room(struct char_data * ch, char *rawroomstr);
int number_range(int from, int to);
struct char_player_data *get_mob_index (int vnum);
void raw_kill(struct char_data * ch, struct char_data * killer);

/* Local functions */

void generate_quest(struct char_data *ch, struct char_data *questman );
void quest_update( void );
bool qchance( int num );

bool qchance( int num )
{
 if (number_range(1,100) <= num) return TRUE;
 else return FALSE;
}

/* The main quest function */

void do_aquest(struct char_data *ch, char *argument)
{
    struct char_data *questman;
    struct obj_data *obj=NULL, *obj_next;
    struct obj_data *obj1, *obj2, *obj3, *obj4, *obj5;
    struct obj_data *questinfoobj;
    struct char_data *questinfo;
    char buf [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    int r_num;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!strcmp(arg1, "info"))
    {
        if (IS_AFFECTED2(ch, AFF2_QUESTOR))
	{
/*
	    if ((ch)->player_specials->saved.questmob == -1 && ch->questgiver->player.short_descr != NULL)
*/
	    if ((ch)->player_specials->saved.questmob == -1 )
	    {
		sprintf(buf, "Your quest is ALMOST complete!\n\rGet back to Gortok before your time runs out!\n\r");
		send_to_char(buf, ch);
	    }
	    else if ((ch)->player_specials->saved.questobj > 0)
	    {

		r_num = real_object((ch)->player_specials->saved.questobj);
		questinfoobj = read_object(r_num, REAL);

		if (questinfoobj != NULL)
		{
		    sprintf(buf, "You are on a quest to recover the fabled %s!\n\r",questinfoobj->name);
		    send_to_char(buf, ch);
		    extract_obj(questinfoobj);
		}
		else send_to_char("You aren't currently on a quest.\n\r",ch);
		return;
	    }
	    else if ((ch)->player_specials->saved.questmob > 0)
	    {
		r_num = real_mobile((ch)->player_specials->saved.questmob);
		questinfo = read_mobile(r_num, REAL);

		if (questinfo != NULL)
		{
	            sprintf(buf, "You are on a quest to slay the dreaded %s!\n\r",questinfo->player.short_descr);
		    send_to_char(buf, ch);
		    char_to_room(questinfo, 0);
		    extract_char(questinfo);
		}
		else send_to_char("You aren't currently on a quest.\n\r",ch);
		return;
	    }
	}
	else
	    send_to_char("You aren't currently on a quest.\n\r",ch);
	return;
    }
    if (!strcmp(arg1, "points"))
    {
	sprintf(buf, "You have %ld quest points.\n\r",(ch)->player_specials->saved.questpoints);
	send_to_char(buf, ch);
	return;
    }
    else if (!strcmp(arg1, "giveup"))
    {
        if (!IS_AFFECTED2(ch, AFF2_QUESTOR))
	{
	    send_to_char("You aren't currently on a quest.\n\r",ch);
	    if ((ch)->player_specials->saved.nextquest > 1)
	    {
		sprintf(buf, "There are %d minutes remaining until you can go on another quest.\n\r",(ch)->player_specials->saved.nextquest);
		send_to_char(buf, ch);
	    }
	    else if ((ch)->player_specials->saved.nextquest == 1)
	    {
		sprintf(buf, "There is less than a minute remaining until you can go on another quest.\n\r");
		send_to_char(buf, ch);
	    }
	}
        else if ((ch)->player_specials->saved.countdown > 0)
        {
	    act("$n abandons $s quest.",FALSE,ch,NULL,NULL,TO_ROOM); 
	    send_to_char("You abandon your quest.\n\r", ch);
	    send_to_char("There are 30 minutes remaining until you can go on another quest.\n\r", ch);
            REMOVE_BIT(AFF2_FLAGS(ch), AFF2_QUESTOR);
            (ch)->player_specials->saved.countdown = 0;
            (ch)->player_specials->saved.questmob = 0;
            (ch)->player_specials->saved.questobj = 0;
	    (ch)->player_specials->saved.nextquest = 30;
	}
	return;
    }
/*
    else if (!strcmp(arg1, "fixme"))
    {
        if (!IS_AFFECTED2(ch, AFF2_QUESTOR) &&
		((ch)->player_specials->saved.questobj != 0 ||
		(ch)->player_specials->saved.questmob != 0))
	{
	    send_to_char("You aren't broken.\n\r",ch);
	    return;
	}

        REMOVE_BIT(AFF2_FLAGS(ch), AFF2_QUESTOR);
        (ch)->player_specials->saved.countdown = 0;
        (ch)->player_specials->saved.questmob = 0;
        (ch)->player_specials->saved.questobj = 0;
	(ch)->player_specials->saved.nextquest = 30;
	send_to_char("Fixed.\n\r", ch);
	send_to_char("There are 30 minutes remaining until you can go on another quest.\n\r", ch);
	return;
    }
*/
    else if (!strcmp(arg1, "time"))
    {
        if (!IS_AFFECTED2(ch, AFF2_QUESTOR))
	{
	    send_to_char("You aren't currently on a quest.\n\r",ch);
	    if ((ch)->player_specials->saved.nextquest > 1)
	    {
		sprintf(buf, "There are %d minutes remaining until you can go on another quest.\n\r",(ch)->player_specials->saved.nextquest);
		send_to_char(buf, ch);
	    }
	    else if ((ch)->player_specials->saved.nextquest == 1)
	    {
		sprintf(buf, "There is less than a minute remaining until you can go on another quest.\n\r");
		send_to_char(buf, ch);
	    }
	}
        else if ((ch)->player_specials->saved.countdown > 0)
        {
	    sprintf(buf, "Time left for current quest: %d\n\r",(ch)->player_specials->saved.countdown);
	    send_to_char(buf, ch);
	}
	return;
    }

/* Checks for a character in the room with spec_questmaster set. This special
   procedure must be defined in special.c. You could instead use an 
   ACT_QUESTMASTER flag instead of a special procedure. */

    for ( questman = world[ch->in_room].people; questman; questman = questman->next_in_room )
    {
	if (!IS_NPC(questman))
	    continue;
	if (MOB_FLAGGED(questman, MOB_QMASTER))
	    break;
    }

    if (questman == NULL)
    {
        send_to_char("You can't do that here.\n\r",ch);
        return;
    }

    if ( GET_POS(questman) == POS_FIGHTING)
    {
	send_to_char("Wait until the fighting stops.\n\r",ch);
        return;
    }

/*
    ch->questgiver = questman;
*/

/* And, of course, you will need to change the following lines for YOUR
   quest item information. Quest items on Moongate are unbalanced, very
   very nice items, and no one has one yet, because it takes awhile to
   build up quest points :> Make the item worth their while. */

    obj1 = read_object(QUEST_ITEM1, VIRTUAL);
    obj2 = read_object(QUEST_ITEM2, VIRTUAL);
    obj3 = read_object(QUEST_ITEM3, VIRTUAL);
    obj4 = read_object(QUEST_ITEM4, VIRTUAL);
    obj5 = read_object(QUEST_ITEM5, VIRTUAL);

    if ( obj1 == NULL || obj2 == NULL || obj3 == NULL || obj4 == NULL || obj5 == NULL )
    {
	sprintf(buf, "Error loading quest objects.  Char: %s", GET_NAME(ch));
	mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
	return;
    }

    extract_obj(obj1);
    extract_obj(obj2);
    extract_obj(obj3);
    extract_obj(obj4);
    extract_obj(obj5);


    if (!strcmp(arg1, "list"))
    {

	obj1 = read_object(QUEST_ITEM1, VIRTUAL);
	obj2 = read_object(QUEST_ITEM2, VIRTUAL);
	obj3 = read_object(QUEST_ITEM3, VIRTUAL);
	obj4 = read_object(QUEST_ITEM4, VIRTUAL);
	obj5 = read_object(QUEST_ITEM5, VIRTUAL);


        act("$n asks $N for a list of quest items.",FALSE,ch,NULL,questman,TO_ROOM); 
	act("You ask $N for a list of quest items.",FALSE,ch,NULL,questman,TO_CHAR);
	sprintf(buf, "Current Quest Items available for Purchase:\n\r\n\r\
[1] %dqp.........%s\n\r\
[2] %dqp.........%s\n\r\
[3] %dqp.........%s\n\r\
[4] %dqp.........%s\n\r\
[5] %dqp.........%s\n\r",
QUEST_VALUE1, obj1->short_description, QUEST_VALUE2, obj2->short_description, 
QUEST_VALUE3, obj3->short_description, QUEST_VALUE4, obj4->short_description, 
QUEST_VALUE5, obj5->short_description);

	extract_obj(obj1);
	extract_obj(obj2);
	extract_obj(obj3);
	extract_obj(obj4);
	extract_obj(obj5);

	send_to_char(buf, ch);
	return;
    }

    else if (!strcmp(arg1, "buy"))
    {
	if (arg2[0] == '\0')
	{
	    send_to_char("To buy an item, type 'QUEST BUY <item>'.\n\r",ch);
	    return;
	}

        if (!strcmp(arg2, "1"))
	{
            if ((ch)->player_specials->saved.questpoints >= QUEST_VALUE1)
	    {
                (ch)->player_specials->saved.questpoints -= QUEST_VALUE1;
	        obj = read_object(QUEST_ITEM1, VIRTUAL);
	    }
	    else
	    {
		sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",GET_NAME(ch));
		do_say(questman,buf, 0, 0);
		return;
	    }
	}
        else if (!strcmp(arg2, "2"))
	{
            if ((ch)->player_specials->saved.questpoints >= QUEST_VALUE2)
	    {
                (ch)->player_specials->saved.questpoints -= QUEST_VALUE2;
	        obj = read_object(QUEST_ITEM2, VIRTUAL);
	    }
	    else
	    {
		sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",GET_NAME(ch));
		do_say(questman,buf, 0, 0);
		return;
	    }
	}
        else if (!strcmp(arg2, "3"))
	{
            if ((ch)->player_specials->saved.questpoints >= QUEST_VALUE3)
	    {
                (ch)->player_specials->saved.questpoints -= QUEST_VALUE3;
	        obj = read_object(QUEST_ITEM3, VIRTUAL);
	    }
	    else
	    {
		sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",GET_NAME(ch));
		do_say(questman,buf, 0, 0);
		return;
	    }
	}
        else if (!strcmp(arg2, "4"))
	{
            if ((ch)->player_specials->saved.questpoints >= QUEST_VALUE4)
	    {
                (ch)->player_specials->saved.questpoints -= QUEST_VALUE4;
	        obj = read_object(QUEST_ITEM4, VIRTUAL);
	    }
	    else
	    {
		sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",GET_NAME(ch));
		do_say(questman,buf, 0, 0);
		return;
	    }
	}
        else if (!strcmp(arg2, "5"))
	{
            if ((ch)->player_specials->saved.questpoints >= QUEST_VALUE5)
	    {
                (ch)->player_specials->saved.questpoints -= QUEST_VALUE5;
	        obj = read_object(QUEST_ITEM5, VIRTUAL);
	    }
	    else
	    {
		sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",GET_NAME(ch));
		do_say(questman,buf, 0, 0);
		return;
	    }
	}
/*
        else if (!strcmp(arg2, "6"))
	{
	    if ((ch)->player_specials->saved.questpoints >= 500)
	    {
		(ch)->player_specials->saved.questpoints -= 500;
	        ch->practice += 30;
    	        act("$N gives 30 practices to $n.", FALSE, ch, NULL, questman, 
			TO_ROOM );
    	        act("$N gives you 30 practices.", FALSE,   ch, NULL, questman, 
			TO_CHAR );
	        return;
	    }
	    else
	    {
		sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",GET_NAME(ch));
		do_say(questman,buf, 0, 0);
		return;
	    }
	}
        else if (!strcmp(arg2, "7"))
	{
	    if ((ch)->player_specials->saved.questpoints >= 500)
	    {
		(ch)->player_specials->saved.questpoints -= 500;
                GET_GOLD(ch) += 10000000;
                act("$N gives 10,000,000 gold pieces to $n.", FALSE, ch, NULL, 
			questman, TO_ROOM );
                act("$N hands you a pouch of gold pieces.", FALSE,   ch, NULL, 
			questman, TO_CHAR );
	        return;
	    }
	    else
	    {
		sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.",GET_NAME(ch));
		do_say(questman,buf, 0, 0);
		return;
	    }
	}
*/
	else
	{
	    sprintf(buf, "I don't have that item, %s.",GET_NAME(ch));
	    do_say(questman, buf, 0, 0);
	}
	if (obj != NULL)
	{
            act("$N gives something to $n.", FALSE , ch, obj, questman, TO_ROOM );
            act("$N gives you your reward.", FALSE ,  ch, obj, questman, TO_CHAR );
	    obj_to_char(obj, ch);
	}
	return;
    }
    else if (!strcmp(arg1, "request"))
    {
        act("$n asks $N for a quest.", FALSE, ch, NULL, questman, TO_ROOM); 
	act("You ask $N for a quest.", FALSE, ch, NULL, questman, TO_CHAR);
        if (IS_AFFECTED2(ch, AFF2_QUESTOR))
	{
            sprintf(buf, "But you're already on a quest!\n\rBetter hurry up and finish it!");
	    do_say(questman, buf, 0, 0);
	    return;
	}
	if ((ch)->player_specials->saved.nextquest > 0)
	{
	    sprintf(buf, "You're very brave, %s, but let someone else have a chance.",GET_NAME(ch));
	    do_say(questman, buf, 0, 0);
	    sprintf(buf, "Come back later.");
	    do_say(questman, buf, 0, 0);
	    return;
	}

	if (IS_NPC(ch) && IS_AFFECTED2(ch, AFF2_JARRED)) {
	    send_to_char("That's not a good idea.\n\r", ch);
	    raw_kill(ch, ch); 
	    return;
	}

	if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM) && ch->master) {
	    send_to_char("You really shouldn't have done that.\n\r", ch->master);
	    GET_HIT(ch->master) = -5;
	    GET_MANA(ch->master) = -5;
	    GET_MOVE(ch->master) = -5;
	    update_pos(ch->master);
	    return;
	}

	sprintf(buf, "Thank you, brave %s!",GET_NAME(ch));
	do_say(questman, buf, 0, 0);

	generate_quest(ch, questman);

        if ((ch)->player_specials->saved.questmob > 0 || (ch)->player_specials->saved.questobj > 0)
	{
            (ch)->player_specials->saved.countdown = number_range(10,30);
            SET_BIT(AFF2_FLAGS(ch), AFF2_QUESTOR);
	    sprintf(buf, "You have %d minutes to complete this quest.",(ch)->player_specials->saved.countdown);
	    do_say(questman, buf, 0, 0);
	    sprintf(buf, "May the gods go with you!");
	    do_say(questman, buf, 0, 0);
	}
	return;
    }
    else if (!strcmp(arg1, "complete"))
    {
        act("$n informs $N $e has completed $s quest.", FALSE, ch, NULL, questman, 
		TO_ROOM); 
        act("You inform $N you have completed $s quest.", FALSE ,ch, NULL, 
		questman, TO_CHAR);
/*
	if (ch->questgiver != questman)
	{
	    sprintf(buf, "I never sent you on a quest! Perhaps you're thinking of someone else.");
	    do_say(questman,buf, 0, 0);
	    return;
	}
*/

        if (IS_AFFECTED2(ch, AFF2_QUESTOR))
	{
	    if ((ch)->player_specials->saved.questmob == -1 && (ch)->player_specials->saved.countdown > 0)
	    {
		int reward, pointreward;
		/* int pracreward; */

                reward = number_range(1000,5000);
                pointreward = number_range(15,75);

		sprintf(buf, "Congratulations on completing your quest!");
		do_say(questman,buf, 0, 0);
		sprintf(buf,"As a reward, I am giving you %d quest points, and %d gold.",pointreward,reward);
		do_say(questman,buf, 0, 0);
/*
                if (qchance(15))
		{
                    pracreward = number_range(1,5);
		    sprintf(buf, "You gain %d practices!\n\r",pracreward);
		    send_to_char(buf, ch);
		    ch->practice += pracreward;
		}
*/

		REMOVE_BIT(AFF2_FLAGS(ch), AFF2_QUESTOR);
/*
	        ch->questgiver = NULL;
*/
	        (ch)->player_specials->saved.countdown = 0;
	        (ch)->player_specials->saved.questmob = 0;
		(ch)->player_specials->saved.questobj = 0;
	        (ch)->player_specials->saved.nextquest = 30;
		GET_GOLD(ch) += reward;
		(ch)->player_specials->saved.questpoints += pointreward;

	        return;
	    }
	    else if ((ch)->player_specials->saved.questobj > 0 && (ch)->player_specials->saved.countdown > 0)
	    {
		bool obj_found = FALSE;

		for (obj = ch->carrying; obj; obj = obj_next)
    		{
                    obj_next = obj->next_content;
        
		    if (obj != NULL && GET_OBJ_VNUM(obj) == (ch)->player_specials->saved.questobj)
		    {
			obj_found = TRUE;
            	        break;
		    }
        	}
		if (obj_found == TRUE)
		{
		    int reward, pointreward;
		    /* int pracreward; */

                    reward = number_range(1000,5000);
                    pointreward = number_range(10,50);

		    act("You hand $p to $N.", FALSE,ch, obj, questman, TO_CHAR);
		    act("$n hands $p to $N.", FALSE ,ch, obj, questman, TO_ROOM);

	    	    sprintf(buf, "Congratulations on completing your quest!");
		    do_say(questman,buf, 0, 0);
		    sprintf(buf,"As a reward, I am giving you %d quest points, and %d gold.",pointreward,reward);
		    do_say(questman,buf, 0, 0);
/*
                    if (qchance(15))
		    {
		        pracreward = number_range(1,6);
		        sprintf(buf, "You gain %d practices!\n\r",pracreward);
		        send_to_char(buf, ch);
		        ch->practice += pracreward;
		    }
*/

		    REMOVE_BIT(AFF2_FLAGS(ch), AFF2_QUESTOR);
/*
	            ch->questgiver = NULL;
*/
	            (ch)->player_specials->saved.countdown = 0;
	            (ch)->player_specials->saved.questmob = 0;
		    (ch)->player_specials->saved.questobj = 0;
	            (ch)->player_specials->saved.nextquest = 30;
		    GET_GOLD(ch) += reward;
		    (ch)->player_specials->saved.questpoints += pointreward;
		    extract_obj(obj);
		    return;
		}
		else
		{
		    sprintf(buf, "You haven't completed the quest yet, but there is still time!");
		    do_say(questman, buf, 0, 0);
		    return;
		}
		return;
	    }
	    else if (((ch)->player_specials->saved.questmob > 0 || (ch)->player_specials->saved.questobj > 0) && (ch)->player_specials->saved.countdown > 0)
	    {
		sprintf(buf, "You haven't completed the quest yet, but there is still time!");
		do_say(questman, buf, 0, 0);
		return;
	    }
	}
	if ((ch)->player_specials->saved.nextquest > 0)
	    sprintf(buf,"But you didn't complete your quest in time!");
	else sprintf(buf, "You have to REQUEST a quest first, %s.",GET_NAME(ch));
	do_say(questman, buf, 0, 0);
	return;
    }

    send_to_char("QUEST commands: POINTS INFO TIME REQUEST COMPLETE LIST BUY GIVEUP.\n\r",ch);
    send_to_char("For more information, type 'HELP AQUEST'.\n\r",ch);
    return;
}

void generate_quest(struct char_data *ch, struct char_data *questman)
{
    struct char_data *victim;
    struct char_data *vsearch;
    sh_int room;
    struct obj_data *questitem;
    char buf[MAX_STRING_LENGTH];
    long mcounter;
    int level_diff, mob_vnum;

    /*  Randomly selects a mob from the world mob list. If you don't
	want a mob to be selected, make sure it is immune to summon.
	Or, you could add a new mob flag called ACT_NOQUEST. The mob
	is selected for both mob and obj quests, even tho in the obj
	quest the mob is not used. This is done to assure the level
	of difficulty for the area isn't too great for the player. */

    for (mcounter = 0; mcounter < 99999; mcounter ++)
    {
	mob_vnum = number_range(50, 30499);

	if ( (vsearch = read_mobile(mob_vnum, VIRTUAL) ) != NULL )
	{
	    level_diff = GET_LEVEL(vsearch) - GET_LEVEL(ch);

		/* Level differences to search for. Moongate has 350
		   levels, so you will want to tweak these greater or
		   less than statements for yourself. - Vassago */

            if (((level_diff < 10 && level_diff > -10)
                || (GET_LEVEL(ch) > 30 && GET_LEVEL(ch) < 40 && GET_LEVEL(vsearch) > 30 && GET_LEVEL(vsearch) < 50)
                || (GET_LEVEL(ch) > 40 && GET_LEVEL(vsearch) > 40))
		&& !MOB_FLAGGED(vsearch, MOB_NOTTHERE)
		&& !MOB_FLAGGED(vsearch, MOB_SHOPKEEPER)
		&& !MOB_FLAGGED(vsearch, MOB_DSHOPKEEPER)
                && !MOB_FLAGGED(vsearch, MOB_SAFE)
                && qchance(35))
	    {
		char_to_room(vsearch, 0);
		extract_char(vsearch);
		break;
	    } else {
		char_to_room(vsearch, 0);
		extract_char(vsearch);
		vsearch = NULL;
	    }
	}
    }

    if ( vsearch == NULL || ( victim = get_char_world(ch, GET_NAME(vsearch)) ) == NULL || !IS_NPC(victim))
    {
	sprintf(buf, "I'm sorry, but I don't have any quests for you at this time.");
	do_say(questman, buf, 0, 0);
	sprintf(buf, "Try again later.");
	do_say(questman, buf, 0, 0);
	(ch)->player_specials->saved.nextquest = 5;
        return;
    }

    if ( ( (room = victim->in_room) == NOWHERE )
	|| (ROOM_FLAGGED(victim->in_room, ROOM_PEACEFUL))
	|| (ROOM_FLAGGED(victim->in_room, ROOM_GODROOM))
	|| (!ZONE_FLAGGED(victim->in_room, ZONE_ACTIVE)) )
    {
	sprintf(buf, "I'm sorry, but I don't have any quests for you at this time.");
	do_say(questman, buf, 0, 0);
	sprintf(buf, "Try again later.");
	do_say(questman, buf, 0, 0);
	(ch)->player_specials->saved.nextquest = 5;
        return;
    }

    /*  40% chance it will send the player on a 'recover item' quest. */

    if (qchance(40))
    {
	int objvnum = 0;

	switch(number_range(0,4))
	{
	    case 0:
	    objvnum = QUEST_OBJQUEST1;
	    break;

	    case 1:
	    objvnum = QUEST_OBJQUEST2;
	    break;

	    case 2:
	    objvnum = QUEST_OBJQUEST3;
	    break;

	    case 3:
	    objvnum = QUEST_OBJQUEST4;
	    break;

	    case 4:
	    objvnum = QUEST_OBJQUEST5;
	    break;
	}

        questitem = read_object(objvnum, VIRTUAL);

	obj_to_room(questitem, victim->in_room);
	(ch)->player_specials->saved.questobj = GET_OBJ_VNUM(questitem);

        sprintf(buf, "Vile pilferers have stolen %s from the treasury!",questitem->short_description);
	do_say(questman, buf, 0, 0);
	do_say(questman, "My court wizard, with his crystal ball, has pinpointed its location.", 0, 0);

	/* I changed my area names so that they have just the name of the area
	   and none of the level stuff. You may want to comment these next two
	   lines. - Vassago */

	sprintf(buf, "Look in the general area of %s!", world[victim->in_room].name);
	do_say(questman, buf, 0, 0);
	return;
    }

    /* Quest to kill a mob */

    else 
    {
    switch(number_range(0,1))
    {
	case 0:
        sprintf(buf, "An enemy of mine, %s, is making vile threats against the city.",victim->player.short_descr);
        do_say(questman, buf, 0, 0);
        sprintf(buf, "This threat must be eliminated!");
        do_say(questman, buf, 0, 0);
	break;

	case 1:
        sprintf(buf, "Kore's most heinous criminal, %s, has escaped from the dungeon!",victim->player.short_descr);
	do_say(questman, buf, 0, 0);
	sprintf(buf, "Since the escape, %s has murdered %d civillians!",victim->player.short_descr, number_range(2,20));
	do_say(questman, buf, 0, 0);
	do_say(questman,"The penalty for this crime is death, and you are to deliver the sentence!", 0, 0);
	break;
    }

    if (world[victim->in_room].name != NULL)
    {
        sprintf(buf, "Seek %s out somewhere in the vicinity of %s!",victim->player.short_descr, world[victim->in_room].name);
        do_say(questman, buf, 0, 0);

	/* I changed my area names so that they have just the name of the area
	   and none of the level stuff. You may want to comment these next two
	   lines. - Vassago */

	sprintf(buf, "That location is in the general area of %s.",zone_table[world[victim->in_room].zone].name);
	do_say(questman, buf, 0, 0);
    }
    (ch)->player_specials->saved.questmob = GET_MOB_VNUM(victim);
    }
    return;
}

/* Called from update_handler() by pulse_area */

void quest_update(void)
{
    struct char_data *ch;

    for (ch = character_list; ch; ch = ch->next)
    {

	if (IS_NPC(ch)) continue;

	if ((ch)->player_specials->saved.nextquest > 0) 
	{
	    (ch)->player_specials->saved.nextquest--;

	    if ((ch)->player_specials->saved.nextquest == 0)
	    {
	        send_to_char("You may now quest again.\n\r",ch);
	        continue;
	    }
	}
	else if (IS_AFFECTED2(ch, AFF2_QUESTOR))
        {
	    if ((--(ch)->player_specials->saved.countdown) <= 0)
	    {
    	        char buf [MAX_STRING_LENGTH];

	        (ch)->player_specials->saved.nextquest = 30;
	        sprintf(buf, "You have run out of time for your quest!\n\rYou may quest again in %d minutes.\n\r",(ch)->player_specials->saved.nextquest);
	        send_to_char(buf, ch);
		REMOVE_BIT(AFF2_FLAGS(ch), AFF2_QUESTOR);
/*
                ch->questgiver = NULL;
*/
                (ch)->player_specials->saved.countdown = 0;
                (ch)->player_specials->saved.questmob = 0;
                (ch)->player_specials->saved.questobj = 0;
	    }
	    if ((ch)->player_specials->saved.countdown > 0 && (ch)->player_specials->saved.countdown < 6)
	    {
	        send_to_char("Better hurry, you're almost out of time for your quest!\n\r",ch);
	        return;
	    }
        }
    }
    return;
}
