/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern char *color_codes[];
extern char *clan_names[];

/* extern functions */
extern void mprog_speech_trigger(char *txt, struct char_data *mob);



ACMD(do_say)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
  else {
    sprintf(buf, "$n says, '%s'", argument);
    MOBTrigger = FALSE;
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      sprintf(buf, "You say, '%s'", argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }
/* HACKED to let mobs trigger says on each other a potential infinite loop */
/*  if (!IS_NPC(ch)) */
      mprog_speech_trigger(argument, ch);
/* end of hack */
  }
}



ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  skip_spaces(&argument);

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not the member of a group!\r\n", ch);
    return;
  }
  if (!*argument)
    send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
  else {
    if (ch->master)
      k = ch->master;
    else
      k = ch;

    sprintf(buf, "$n tells the group, '%s'", argument);

    if (IS_AFFECTED(k, AFF_GROUP) && (k != ch)) {
      send_to_char(CCGSAY(k), k);
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
      send_to_char(CCNRM(k), k);
    }
    for (f = k->followers; f; f = f->next)
      if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower != ch)) {
        send_to_char(CCGSAY(f->follower), f->follower);
	act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);
        send_to_char(CCNRM(f->follower), f->follower);
      }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "%sYou tell the group, '%s'%s", CCGSAY(ch), argument, 
          CCNRM(ch));
      act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    }
  }
}



void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
/* HACKED to give an away message */
  if (PRF2_FLAGGED(vict, PRF2_AWAY)) {
    send_to_char(CCALERT(ch), ch);
    strcpy(buf1, PERS(vict, ch));
    CAP(buf1);
    sprintf(buf, "(%s is away) ", buf1);
    send_to_char(buf, ch);
    send_to_char(CCNRM(ch), ch);

    send_to_char(CCALERT(vict), vict);
    send_to_char("While you were away: ", vict);
    send_to_char(CCNRM(ch), ch);
  }
/* end of hack */
  if (arg[0] != ':') {
    send_to_char(CCTELL(vict), vict);
    sprintf(buf, "$n tells you, '%s'", arg);
    act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
    send_to_char(CCNRM(vict), vict);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      send_to_char(CCTELL(ch), ch);
      sprintf(buf, "You tell $N, '%s'", arg);
      act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      send_to_char(CCNRM(ch), ch);
    }
  } else {
    send_to_char(CCTELL(vict), vict);
    sprintf(buf, "From afar, $n %s", ++arg);
    arg--;
    act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
    send_to_char(CCNRM(vict), vict);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      send_to_char(CCTELL(ch), ch);
      sprintf(buf, "Long distance to $N: $n %s", ++arg);
      arg--;
      act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      send_to_char(CCNRM(ch), ch);
    }
  }
  GET_LAST_TELL(vict) = GET_IDNUM(ch);
}



/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict;

  half_chop(argument, buf, buf2);

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char("You may not use channels.\n\r", ch);
    return;
  }

  if (!*buf || !*buf2)
    send_to_char("Who do you wish to tell what??\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (IS_NPC(vict) && !vict->desc)	/* linkless */
    send_to_char(NOPERSON, ch);
  else if (ch == vict)
    send_to_char("You try to tell yourself something.\r\n", ch);
  else if (PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char("You can't tell other people while you have notell on.\r\n", ch);
  else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) && GET_LEVEL(ch) < 
           LVL_GRGOD)
    send_to_char("The walls seem to absorb your words.\r\n", ch);
  else if (!IS_NPC(vict) && !vict->desc)	/* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.",
	FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ( ((PRF_FLAGGED(vict, PRF_NOTELL)) || !(AWAKE(vict))
      || (ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF)))
              && (GET_LEVEL(ch) < LVL_GRGOD) )
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    perform_tell(ch, vict, buf2);
}



ACMD(do_reply)
{
  struct char_data *tch = character_list;

  skip_spaces(&argument);

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char("You may not use channels.\n\r", ch);
    return;
  }

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char("You have no-one to reply to!\r\n", ch);
  else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) && GET_LEVEL(ch) < 
           LVL_GRGOD)
    send_to_char("The walls seem to absorb your words.\r\n", ch);
  else if (!*argument)
    send_to_char("What is your reply?\r\n", ch);
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     */
				     
    while (tch != NULL && GET_IDNUM(tch) != GET_LAST_TELL(ch))
      tch = tch->next;

    if (tch == NULL)
      send_to_char("They are no longer playing.\r\n", ch);
    else
      perform_tell(ch, tch, argument);
  }
}



ACMD(do_spec_comm)
{
  struct char_data *vict;
  char *action_sing, *action_plur, *action_others;

  void mprog_social_trigger(struct char_data *vict, struct char_data *ch,
      char *social_name);


  if (subcmd == SCMD_WHISPER) {
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
  } else {
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    sprintf(buf, "Whom do you want to %s.. and what??\r\n", action_sing);
    send_to_char(buf, ch);
  } else if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (vict == ch)
    send_to_char("You can't get your mouth close enough to your ear...\r\n", ch);
  else {
    sprintf(buf, "$n %s you, '%s'", action_plur, buf2);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
    if (subcmd == SCMD_WHISPER) {
      sprintf(buf, " whisper %s", buf2);
      mprog_social_trigger(vict, ch, buf);
    } else {
      sprintf(buf, " ask %s", buf2);
      mprog_social_trigger(vict, ch, buf);
    }
  }
}



#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
  struct obj_data *paper = 0, *pen = 0;
  char *papername, *penname;
  int wear_hold = WEAR_HOLD;
  
  if (IS_THRIKREEN(ch)) wear_hold = THRI_WEAR_HOLD_R;

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\r\n", ch);
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", penname);
      send_to_char(buf, ch);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "There is no %s in your inventory.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = 0;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char("That thing has nothing to do with writing.\r\n", ch);
      return;
    }
    /* One object was found.. now for the other one. */
    if (!ch->equipment[wear_hold]) {
      sprintf(buf, "You can't write with %s %s alone.\r\n", AN(papername),
	      papername);
      send_to_char(buf, ch);
      return;
    }
    if (!CAN_SEE_OBJ(ch, ch->equipment[wear_hold])) {
      send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n", ch);
      return;
    }
    if (pen)
      paper = ch->equipment[wear_hold];
    else
      pen = ch->equipment[wear_hold];
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else if (paper->action_description)
    send_to_char("There's something written on it already.\r\n", ch);
  else {
    /* we can write - hooray! */
    send_to_char("Write your note.  End with '@' on a new line.\r\n", ch);
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    ch->desc->str = &paper->action_description;
    ch->desc->max_str = MAX_NOTE_LENGTH;
  }
}



ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char("Monsters can't page.. go away.\r\n", ch);
  else if (!*arg)
    send_to_char("Whom do you wish to page?\r\n", ch);
  else {
    sprintf(buf, "\007\007*%s* %s\r\n", GET_NAME(ch), buf2);
    if (!str_cmp(arg, "all")) {
      if (GET_LEVEL(ch) > LVL_GOD) {
	for (d = descriptor_list; d; d = d->next)
	  if (!d->connected && d->character) {
            send_to_char(CCPAGE(d->character), d->character);
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
            send_to_char(CCNRM(d->character), d->character);
          }
      } else
	send_to_char("You will never be godly enough to do that!\r\n", ch);
      return;
    }
    if ((vict = get_char_vis(ch, arg)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(OK, ch);
      else {
        send_to_char(CCPAGE(ch), ch);
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
        send_to_char(CCNRM(ch), ch);
      }
      return;
    } else
      send_to_char("There is no such person in the game!\r\n", ch);
  }
}



/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  extern int level_can_shout;
/* HACKED to remove mv cost for holler
  extern int holler_move_cost;
 * END of hack */
  struct descriptor_data *i;

  /* Array of flags which must _not_ be set in order for comm to be heard */
  static long channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   */
  static char *com_msgs[][3] = {
    {"You cannot holler!!\r\n",
      "holler",
      ""},

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n"},

    {"You cannot gossip!!\r\n",
      "gossip",
      "You aren't even on the channel!\r\n"},

    {"You cannot auction!!\r\n",
      "auction",
      "You aren't even on the channel!\r\n"},

    {"You cannot congratulate!!\r\n",
      "congrat",
      "You aren't even on the channel!\r\n"},

  };

  /* to keep pets, etc from being ordered to shout */
  if ((!ch->desc) && AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char("You may not use channels.\n\r", ch);
    return;
  }

  if ((ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) && (GET_LEVEL(ch) < LVL_GRGOD)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < level_can_shout) {
    sprintf(buf1, "You must be at least level %d before you can %s.\r\n",
	    level_can_shout, com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  /* make sure the char is on the channel */
  if (PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(com_msgs[subcmd][2], ch);
    return;
  }
  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument) {
    sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n",
	    com_msgs[subcmd][1], com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  /* HACKED to remove hollver MV cost *
  if (subcmd == SCMD_HOLLER) {
    if (GET_MOVE(ch) < holler_move_cost) {
      send_to_char("You're too exhausted to holler.\r\n", ch);
      return;
    } else
      GET_MOVE(ch) -= holler_move_cost;
  }
  * END of hack */
  
  /* first, set up strings to be given to the communicator */
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    sprintf(buf1, "You %s, '%s'", com_msgs[subcmd][1], argument);
    switch (subcmd) {
      case SCMD_HOLLER:  send_to_char(CCHOLLER(ch), ch); break;
      case SCMD_SHOUT:   send_to_char(CCSHOUT(ch), ch); break;
      case SCMD_GOSSIP:  send_to_char(CCGOSSIP(ch), ch); break;
      case SCMD_AUCTION: send_to_char(CCAUCTION(ch), ch); break;
      case SCMD_GRATZ:   send_to_char(CCGRATZ(ch), ch); break;
      default:           break;
    }
    act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    send_to_char(CCNRM(ch), ch);
  }

  sprintf(buf, "$n %ss, '%s'", com_msgs[subcmd][1], argument);

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i != ch->desc && i->character &&
	!PRF_FLAGGED(i->character, channels[subcmd]) &&
	!PLR_FLAGGED(i->character, PLR_WRITING) &&
	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
	  ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
	   GET_POS(i->character) < POS_RESTING))
	continue;

      switch (subcmd) {
        case SCMD_HOLLER:  send_to_char(CCHOLLER(i->character), i->character);
                           break;
        case SCMD_SHOUT:   send_to_char(CCSHOUT(i->character), i->character);
                           break;
        case SCMD_GOSSIP:  send_to_char(CCGOSSIP(i->character), i->character);
                           break;
        case SCMD_AUCTION: send_to_char(CCAUCTION(i->character), i->character);
                           break;
        case SCMD_GRATZ:   send_to_char(CCGRATZ(i->character), i->character);
                           break;
        default:           break;
      }
      act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
      send_to_char(CCNRM(i->character), i->character);
    }
  }
}



ACMD(do_qcomm)
{
  struct descriptor_data *i;

  if ((ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) && (GET_LEVEL(ch) < LVL_GRGOD)
      && (subcmd != SCMD_QECHO)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char("You may not use channels.\n\r", ch);
    return;
  }
  if (GET_QUEST(ch) == QUEST_OFF && subcmd != SCMD_QECHO) {
    send_to_char("You aren't even part of the quest!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "%s%s?  Yes, fine, %s we must, but WHAT??%s\r\n",
        CCALERT(ch), CMD_NAME, CMD_NAME, CCNRM(ch));
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      if (subcmd == SCMD_QSAY) {
        sprintf(buf, "%sYou quest-say, '%s'%s", CCQSAY(ch), argument,
            CCNRM(ch));
      } else {
        strcpy(buf, argument);
      }
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    if (subcmd == SCMD_QSAY)
      sprintf(buf, "$n quest-says, '%s'", argument);
    else
      strcpy(buf, argument);

    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i != ch->desc &&
          GET_QUEST(i->character) > QUEST_OFF) {
        send_to_char(CCQSAY(i->character), i->character);
        act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
        send_to_char(CCNRM(i->character), i->character);
      }
  }
}



ACMD(do_clancomm)
{
  struct descriptor_data *i;
  char clanbuf[MAX_INPUT_LENGTH];
  char clanprefix[MAX_INPUT_LENGTH];
  char openb[MAX_INPUT_LENGTH];
  char closeb[MAX_INPUT_LENGTH];

  if (GET_CLAN(ch) <= CLAN_NOCLAN) {
    send_to_char(CCWARNING(ch), ch);
    send_to_char("You aren't even in a clan!!\r\n", ch);
    send_to_char(CCNRM(ch), ch);
    return;
  }
  if ((ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) && (GET_LEVEL(ch) < LVL_GRGOD)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  if (GET_CLAN(ch) == CLAN_PLEDGE) {
    send_to_char(CCALERT(ch), ch);
    send_to_char("You aren't in a clan yet, "
                 "you have to be initiated first.\r\n", ch);
    send_to_char(CCNRM(ch), ch);
    return;
  }
  if (GET_CLAN(ch) == CLAN_BLACKLISTED) {
    send_to_char(CCWARNING(ch), ch);
    send_to_char("SHAME! You dare to invoke the wrath of the Gods?!\r\n", ch);
    send_to_char("The ways of the clans are closed to you.\r\n", ch);
    send_to_char(CCNRM(ch), ch);
    return;
  }
  if (PRF2_FLAGGED(ch, PRF2_NOCLAN)) {
    send_to_char(CCALERT(ch), ch);
    send_to_char("You aren't even on the clan channel!!\r\n", ch);
    send_to_char(CCNRM(ch), ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "%s%s?  Yes, fine, %s we must, but WHAT??%s\r\n",
        CCALERT(ch), CMD_NAME, CMD_NAME, CCNRM(ch));
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "%sYou clan-say, '%s'%s", CCCLANSAY(ch), argument,
          CCNRM(ch));
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    sprintf(buf, "$n clan-says, '%s'", argument);


    for (i = descriptor_list; i; i = i->next)

	if(!i->connected) {

	    if (( i != ch->desc &&
	    !PRF2_FLAGGED(i->character, PRF2_NOCLAN) &&
	    !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) &&
	    GET_CLAN(i->character) == GET_CLAN(ch)) ||
	    (GET_LEVEL(i->character) >= LVL_GRGOD &&
	    !PRF2_FLAGGED(i->character, PRF2_NOCLAN) &&
	    !PLR_FLAGGED(i->character, PLR_WRITING) &&
	    !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) &&
	    i != ch->desc) ) {

		if (GET_LEVEL(i->character) >= LVL_GRGOD) {
		    strcpy(openb, "["); strcpy(closeb, "]");
		    sprinttype(GET_CLAN(ch), clan_names, clanbuf);
		    sprintf(clanprefix, "%s%s%s ", openb, clanbuf, closeb);
		    send_to_char(CCCLANSAY(i->character), i->character);
		    send_to_char(clanprefix, i->character);
		    send_to_char(CCNRM(i->character), i->character);
		}

		send_to_char(CCCLANSAY(i->character), i->character);
		act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
		send_to_char(CCNRM(i->character), i->character);
	    }

	}

  }
}

ACMD(do_music)
{
  struct descriptor_data *i;

  if (PRF2_FLAGGED(ch, PRF2_NOMUSIC)) {
    send_to_char("You aren't even on the music channel!\r\n", ch);
    return;
  }

   if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
     send_to_char("You may not use channels.\n\r", ch);
     return;
   }
 
  if ((ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) && (GET_LEVEL(ch) < LVL_GRGOD)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "%s%s?  Yes, fine, %s we must, but WHAT??%s\r\n",
        CCALERT(ch), CMD_NAME, CMD_NAME, CCNRM(ch));
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
     sprintf(buf, "%sYou sing, '%s'%s", CCGOSSIP(ch), argument, CCNRM(ch));
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    sprintf(buf, "$n sings, '%s'", argument);

    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i != ch->desc &&
          !PRF2_FLAGGED(i->character, PRF2_NOMUSIC)) {
        send_to_char(CCGOSSIP(i->character), i->character);
        act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
        send_to_char(CCNRM(i->character), i->character);
      }
  }
}

