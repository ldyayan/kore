
/* ***********************************************************************
*  File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "olc.h"

#define TIMEKEEPER 1206

extern const struct title_type titles[NUM_CLASSES][LVL_IMPL+1];
extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct char_data *character_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern char *color_codes[];
extern char *credits;
extern char *connected_types[];

/* external functions */
void echo_on(struct descriptor_data * d);
void echo_off(struct descriptor_data * d);
void do_start(struct char_data * ch);
void init_char(struct char_data * ch);
int create_entry(char *name);
int special(struct char_data * ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void clanlog(char *str, struct char_data * ch);
void color_setup(struct char_data * ch);
void oedit_parse(struct descriptor_data *d, char *arg);
void redit_parse(struct descriptor_data *d, char *arg);
void zedit_parse(struct descriptor_data *d, char *arg);
void medit_parse(struct descriptor_data *d, char *arg);
void sedit_parse(struct descriptor_data *d, char *arg);
int mprog_command_trigger(struct char_data * ch, int cmd, char *arg);


/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_addrestrict);
ACMD(do_add_storage_room);
ACMD(do_advance);
ACMD(do_affects);
ACMD(do_alias);
ACMD(do_aquest);
ACMD(do_areahelp);
ACMD(do_areas);
ACMD(do_arena);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_attribute);
ACMD(do_auctalk);
ACMD(do_auction);
ACMD(do_auto);
ACMD(do_avenging_blow);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bandage);
ACMD(do_banner);
ACMD(do_bash);
ACMD(do_battle);
ACMD(do_berserk);
ACMD(do_bid);
ACMD(do_bite);
ACMD(do_blacklist);
ACMD(do_block);
ACMD(do_board);
ACMD(do_breathe);
ACMD(do_brew);
ACMD(do_bury);
ACMD(do_buy);
ACMD(do_cast);
ACMD(do_changes);
ACMD(do_check_gold);
ACMD(do_check_values);
ACMD(do_circle);
ACMD(do_claim);
ACMD(do_close);
ACMD(do_clancomm);
ACMD(do_clanhelp);
ACMD(do_clanleav);
ACMD(do_clanleave);
ACMD(do_clanlevel);
ACMD(do_climb);
ACMD(do_color);
ACMD(do_commands);
ACMD(do_compare);
ACMD(do_compel);
ACMD(do_conceal);
/* ACMD(do_consider); */ 
ACMD(do_consider2);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_deathpost);
ACMD(do_defend);
ACMD(do_delay);
/* ACMD(do_devour); */
ACMD(do_diagnose);
ACMD(do_dig);
ACMD(do_disabled);
ACMD(do_display);
ACMD(do_dm_scores);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_dsedit);
ACMD(do_disarm); 
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enroll);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exits);
ACMD(do_flee);
#if 0
ACMD(do_finger);
#endif
ACMD(do_findaff2);
ACMD(do_follow);
ACMD(do_foldarea);
ACMD(do_force);
ACMD(do_forgive);
ACMD(do_gaze);
ACMD(do_gbanner);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_gauge);
ACMD(do_hcontrol);
ACMD(do_heal);
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_info);
ACMD(do_initiate);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_jar);
ACMD(do_judge);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_link);
ACMD(do_list);
ACMD(do_load);
ACMD(do_loadmoney);
ACMD(do_load_storage_rooms);
ACMD(do_lock);
ACMD(do_look);
ACMD(do_make);
ACMD(do_map);
ACMD(do_mount);
ACMD(do_move);
ACMD(do_music);
ACMD(do_newbie);
ACMD(do_newwho);
ACMD(do_not_here);
ACMD(do_not_save);
ACMD(do_offer);
ACMD(do_open);
ACMD(do_olc);
ACMD(do_opstat);
ACMD(do_order);
ACMD(do_page);
ACMD(do_palm);
ACMD(do_pawnremove);
ACMD(do_peace);
ACMD(do_petcontrol);
ACMD(do_pick);
/*ACMD(do_planeshift);*/
/*ACMD(do_players);*/
ACMD(do_players2);
ACMD(do_pledge);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_pretitle);
ACMD(do_pull);
ACMD(do_purge);
ACMD(do_push);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quest);
ACMD(do_questify);
ACMD(do_quickdraw);
ACMD(do_quit);
ACMD(do_rage);
ACMD(do_ready);
ACMD(do_reboot);
ACMD(do_reimburse);
ACMD(do_remove);
ACMD(do_removerestrict);
ACMD(do_rename);
ACMD(do_rent);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_reroll);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_riposte);
ACMD(do_romscore);
ACMD(do_sacrifice);
ACMD(do_save);
ACMD(do_save_rooms);
ACMD(do_say);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_score2);
ACMD(do_screate);
ACMD(do_scrounge);
ACMD(do_search);
ACMD(do_sell);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_smite);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_socedit);
ACMD(do_socials);
ACMD(do_speak);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_sremove);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_stun);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_system);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_translate);
ACMD(do_trip);
ACMD(do_unban);
ACMD(do_unlock);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_valour);
ACMD(do_value);
ACMD(do_visible);
ACMD(do_vload);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_vtype);
ACMD(do_vwear);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizhelp);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_yank);
ACMD(do_zreset);
ACMD(do_mpstat);
ACMD(do_mpasound);
ACMD(do_mpjunk);
ACMD(do_mpecho);
ACMD(do_mpechoat);
ACMD(do_mpechoaround);
ACMD(do_mpkill);
ACMD(do_mpmload);
ACMD(do_mpoload);
ACMD(do_mppurge);
ACMD(do_mpgoto);
ACMD(do_mpat);
ACMD(do_mptransfer);
ACMD(do_mpforce);
ACMD(do_mpcallmagic);
ACMD(do_mppose);
ACMD(do_mpdamage);
ACMD(do_mpdrainmana);
ACMD(do_mpdrainmove);
ACMD(do_mpremember);
ACMD(do_mpforget);
ACMD(do_mpstopcommand);
ACMD(do_mptrigger);
ACMD(do_mpsilent);
ACMD(do_mptrackto);
ACMD(do_mpsteerto);
ACMD(do_mpstopscript);
ACMD(do_mplog);
ACMD(do_mpset);
ACMD(do_mpconceal);

/*
 * This is the Master Command List(tm).
 *
 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

const struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN },
  { "ne"       , POS_STANDING, do_move     , 0, SCMD_NORTHEAST },
  { "se"       , POS_STANDING, do_move     , 0, SCMD_SOUTHEAST },
  { "sw"       , POS_STANDING, do_move     , 0, SCMD_SOUTHWEST },
  { "nw"       , POS_STANDING, do_move     , 0, SCMD_NORTHWEST },
  { "northeast", POS_STANDING, do_move     , 0, SCMD_NORTHEAST },
  { "southeast", POS_STANDING, do_move     , 0, SCMD_SOUTHEAST },
  { "southwest", POS_STANDING, do_move     , 0, SCMD_SOUTHWEST },
  { "northwest", POS_STANDING, do_move     , 0, SCMD_NORTHWEST },
  { "somewhere", POS_STANDING, do_move     , LVL_GOD, SCMD_SOMEWHERE },

  /* OLC */
/*{ "olc"      , POS_DEAD    , do_olc      , LVL_IMPL, 0 }, */
  { "olc"      , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_SAVEINFO },
  { "oedit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_OEDIT},
  { "redit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_REDIT},
  { "oldsedit" , POS_DEAD    , do_olc      , LVL_IMPL, SCMD_OLC_SEDIT},
  { "sedit"    , POS_DEAD    , do_dsedit   , LVL_IMPL - 1, 0},
  { "zedit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_ZEDIT},
  { "medit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_MEDIT},

/*{ "link"     , POS_DEAD    , do_link     , LVL_BUILDER, 0 }, */

  /* now, the main list */
  { "affects"  , POS_DEAD    , do_affects  , 0, 0 },
  { "afk"      , POS_RESTING , do_gen_tog  , 0, SCMD_AWAY },
  { "areas"    , POS_RESTING , do_areas    , 0, 0 },
  { "arena"    , POS_DEAD    , do_arena    , LVL_GRGOD, 0 },
  { "at"       , POS_DEAD    , do_at       , LVL_DEITY, 0 },
  { "addrestrict",POS_DEAD   , do_addrestrict, LVL_DEITY, 0 },
  { "addstorage",POS_DEAD    , do_add_storage_room, LVL_GRGOD, 0 },
  { "advance"  , POS_DEAD    , do_advance  , LVL_IMPL, 0 },
  { "alias"    , POS_DEAD    , do_alias    , 0, 0 },
  { "ahelp"    , POS_DEAD    , do_areahelp , 0, 0 },
  { "aquest"   , POS_RESTING , do_aquest   , 0, 0 },
  { "areahelp" , POS_DEAD    , do_areahelp , 0, 0 },
  { "assist"   , POS_FIGHTING, do_assist   , 0, 0 },
  { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK },
  { "attribute", POS_DEAD    , do_attribute, LVL_GRGOD, 0 },
  { "auction"  , POS_RESTING , do_auction  , 0, 0 },
  { "auctalk"  , POS_RESTING , do_auctalk  , 0, 0 },
  { "auto"     , POS_DEAD    , do_auto     , 0, 0 },
  { "anonymous", POS_DEAD    , do_gen_tog  , 0, SCMD_ANONYMOUS },
  { "autoassist",POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOASSIST },
  { "autodirs" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTODIRS },
  { "autoexits", POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEXIT },
  { "autogold" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOGOLD },
  { "autoloot" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOLOOT },
  { "autoreport",POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOGROUP },
  { "autosac"  , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOSAC },
  { "autosplit", POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOSPLIT },
/*
  { "autoscan" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOSCAN },
*/
  { "automap"  , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOMAP },
  { "avenging blow",POS_SLEEPING,do_avenging_blow, 0, SCMD_TEST_AVENGING_BLOW},
  { "away"     , POS_RESTING , do_gen_tog  , 0, SCMD_AWAY },

  { "backstab" , POS_STANDING, do_backstab , 0, 0 },
  { "ban"      , POS_DEAD    , do_ban      , LVL_GOD, 0 },
  { "banner"   , POS_RESTING , do_banner   , LVL_GOD, 0 },
  { "balance"  , POS_STANDING, do_not_here , 0, 0 },
  { "bandage"  , POS_STANDING, do_bandage  , 0, 0 },
  { "bash"     , POS_FIGHTING, do_bash     , 0, 0 },
  { "battle"   , POS_RESTING , do_battle   , 0, SCMD_BATTLE },
  { "battlebrief", POS_DEAD  , do_gen_tog  , 0, SCMD_BATTLEBRIEF },
  { "berserk"  , POS_FIGHTING, do_berserk  , 0, 0 },
  { "bid"      , POS_SLEEPING, do_bid      , 0, 0 },
  { "bite"     , POS_FIGHTING, do_bite     , 0, 0 }, 
  { "blacklist", POS_DEAD    , do_blacklist, 0, 0 },
  { "block"    , POS_FIGHTING, do_block    , 0, 0 },
  { "board"    , POS_STANDING, do_board    , 0, 0 },
  { "breathe"  , POS_FIGHTING, do_breathe  , 0, 0 },
  { "brew"     , POS_STANDING, do_brew     , 20, 0 },
  { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
  { "bury"     , POS_RESTING , do_bury     , 0, 0 },
  { "buy"      , POS_STANDING, do_buy      , 0, 0 },
  { "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG },

  { "cast"     , POS_FIGHTING, do_cast     , 0, SCMD_CAST },
  { "changes"  , POS_STANDING, do_changes  , 0, 0 },
  { "check"    , POS_STANDING, do_not_here , 0, 0 },
  { "checkgold" , POS_STANDING, do_check_gold , LVL_GRGOD, 0 },
  { "checkvals", POS_DEAD    , do_check_values, /*LVL_GRGOD*/60, 0 },
  { "circle"   , POS_FIGHTING, do_circle   , 0, 0 },
  { "claim"    , POS_STANDING, do_claim    , 0, 0 },
  { "clanhelp" , POS_DEAD    , do_clanhelp , 0, 0 },
  { "clanleav" , POS_RESTING , do_clanleav , 0, 0 },
  { "clanleave", POS_RESTING , do_clanleave, 0, 0 },
  { "climb"    , POS_STANDING, do_climb    , 0, 0 },
  { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "close"    , POS_SITTING , do_close    , 0, 0 },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "coins"    , POS_RESTING , do_gold     , 0, 0 },
  { "consider" , POS_RESTING , do_consider2, 0, 0 },
  { "congrat"  , POS_RESTING , do_gen_comm , 0, SCMD_GRATZ },
  { "color"    , POS_DEAD    , do_color    , 0, 0 },
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS },
  { "compare"  , POS_RESTING , do_compare  , 0, 0 },
  { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT },
  { "compel"   , POS_SLEEPING, do_compel   , LVL_GRGOD, 0 },
/*  { "conceal"  , POS_RESTING , do_conceal  , LVL_IMPL, 0 },*/
  { "conceal",   POS_STANDING, do_conceal  , LVL_IMMORT, 0 },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS },
  { "csay"     , POS_DEAD    , do_clancomm , 0, SCMD_CLANSAY},
  { "ctalk"    , POS_DEAD    , do_clancomm , 0, SCMD_CLANSAY},

  { "date"     , POS_DEAD    , do_date     , 0, SCMD_DATE },
  { "dc"       , POS_DEAD    , do_dc       , LVL_GOD, 0 },
  { "deathmatch",POS_DEAD    , do_gen_tog  , LVL_GOD, SCMD_DEATHMATCH },
  { "deathpost", POS_DEAD    , do_deathpost, LVL_GOD, 0 },
  { "defend"   , POS_FIGHTING, do_defend   , 0, 0 },                           
  { "delay"    , POS_DEAD    , do_delay    , LVL_GRGOD, 0 },
  { "demote"   , POS_RESTING , do_clanlevel, 0, SCMD_DEMOTE },
  { "deposit"  , POS_STANDING, do_not_here , 0, 0 },
/*{ "devour"   , POS_RESTING , do_devour   , 0, 0 }, */
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0 },
  { "dig"      , POS_DEAD    , do_dig      , LVL_BUILDER, 0 },
  { "display"  , POS_DEAD    , do_display  , 0, 0 },
  { "disarm"   , POS_FIGHTING, do_disarm   , 0, 0 },
  { "disembark", POS_STANDING, do_not_here , 0, 0 },
  { "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE },
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK },
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP },

  { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
  { "echo"     , POS_SLEEPING, do_echo     , LVL_DEITY, SCMD_ECHO },
  { "emote"    , POS_RESTING , do_echo     , 0, SCMD_EMOTE },
  { ":"        , POS_RESTING,  do_echo     , 0, SCMD_EMOTE },
  { "enter"    , POS_STANDING, do_enter    , 0, 0 },
  { "enroll"   , POS_DEAD    , do_enroll   , LVL_GOD, 0 },
  { "equip"    , POS_SLEEPING, do_equipment, 0, 0 },
  { "exits"    , POS_RESTING , do_exits    , 0, 0 },
  { "examine"  , POS_SITTING , do_examine  , 0, 0 },

  { "force"    , POS_SLEEPING, do_force    , LVL_GOD, 0 },
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
#if 0
  { "finger"   , POS_DEAD    , do_finger   , 0, 0 }, 
#endif
  { "findaff2" , POS_DEAD    , do_findaff2 , LVL_GOD, 0 },
  { "flee"     , POS_FIGHTING, do_flee     , 0, 0 },
  { "follow"   , POS_RESTING , do_follow   , 0, 0 },
  { "foldarea" , POS_RESTING , do_foldarea , LVL_IMPL, 0 },
  { "forgive"  , POS_DEAD    , do_forgive  , 0, 0 },
  { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE },
  
  { "get"      , POS_RESTING , do_get      , 0, 0 },
  { "gaze"     , POS_FIGHTING, do_gaze     , 0, 0 },
  { "gauge"    , POS_STANDING, do_gauge    , 0, 0 }, 
  { "gbanner"  , POS_RESTING , do_gbanner  , LVL_GOD, 0 },
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_GOD, 0 },
  { "give"     , POS_RESTING , do_give     , 0, 0 },
  { "goto"     , POS_SLEEPING, do_goto     , LVL_IMMORT, 0 },
  { "gold"     , POS_RESTING , do_gold     , 0, 0 },
  { "gossip"   , POS_RESTING    , do_gen_comm , 0, SCMD_GOSSIP },
  { "group"    , POS_DEAD    , do_group    , 0, 0 },
  { "grab"     , POS_RESTING , do_grab     , 0, 0 },
  { "grats"    , POS_RESTING , do_gen_comm , 0, SCMD_GRATZ },
  { "gsay"     , POS_DEAD    , do_gsay     , 0, 0 },
  { "gtell"    , POS_DEAD    , do_gsay     , 0, 0 },

  { "help"     , POS_DEAD    , do_help     , 0, 0 },
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_HANDBOOK },
  { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_GOD, 0 },
  { "heal"     , POS_RESTING , do_heal     , 0, 0 },
  { "hide"     , POS_RESTING , do_hide     , 0, 0 },
  { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
  { "hold"     , POS_RESTING , do_grab     , 0, 0 },
/*
  { "holler"   , POS_RESTING , do_gen_comm , 0, SCMD_HOLLER },
*/
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_HOLYLIGHT },
  { "house"    , POS_RESTING , do_house    , 0, 0 },

  { "inventory", POS_DEAD    , do_inventory, 0, 0 },
  { "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA },
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD },
  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "immtalk"  , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
  { "initiate" , POS_DEAD    , do_initiate , 0, 0 },
  { "insult"   , POS_RESTING , do_insult   , 0, 0 },
  { "invis"    , POS_DEAD    , do_invis    , LVL_IMMORT, 0 },
  
  { "jar"      , POS_DEAD    , do_jar      , LVL_DEITY, 0},
  { "judge"    , POS_RESTING , do_judge    , 0, 0 },
  { "junk"     , POS_RESTING , do_drop     , 0, SCMD_JUNK },

  { "kill"     , POS_FIGHTING, do_kill     , 0, 0 },
  { "kick"     , POS_FIGHTING, do_kick     , 0, 0 },

  { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK },
  { "last"     , POS_DEAD    , do_last     , 0, 0 },
  { "leave"    , POS_STANDING, do_leave    , 0, 0 },
  { "levels"   , POS_DEAD    , do_levels   , 0, 0 },
  { "list"     , POS_STANDING, do_list     , 0, 0 },
  { "lock"     , POS_SITTING , do_lock     , 0, 0 },
  { "load"     , POS_DEAD    , do_load     , LVL_IMMORT, 0 },
  { "loadmoney", POS_DEAD    , do_loadmoney, LVL_GRGOD, 0 },
  { "loadrooms", POS_DEAD    , do_load_storage_rooms,LVL_IMPL, 0 },
  { "lookout"  , POS_RESTING , do_not_here , 0, 0 },

  { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
  { "mail"     , POS_STANDING, do_not_here , 0, 0 },
  { "make"     , POS_RESTING , do_make     , LVL_IMPL, 0 },
  { "map"      , POS_RESTING , do_map      , 0, 0 },
  { "mount"    , POS_STANDING, do_mount    , 59, 0 },
  { "music"    , POS_RESTING , do_music    , 0, 0 },
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_LGOD, SCMD_SQUELCH },
  { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER },

  { "newbie"   , POS_RESTING , do_newbie   , 0, 0 },
  { "newwho"   , POS_DEAD    , do_newwho   , LVL_IMPL, 0 },
  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS },
  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION },
  { "noclan"   , POS_DEAD    , do_gen_tog  , 0, SCMD_NOCLAN },
  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP },
  { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ },
  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOHASSLE },
  { "nomusic"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOMUSIC },
  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
  { "nosummon" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOSUMMON },
  { "noshout"  , POS_SLEEPING, do_gen_tog  , 0, SCMD_DEAF },
  { "notell"   , POS_DEAD    , do_gen_tog  , 0, SCMD_NOTELL },
  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE },
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOWIZ },

  { "oldscore" , POS_DEAD    , do_score    , 0, 0 },
  { "order"    , POS_RESTING , do_order    , 0, 0 },
  { "offer"    , POS_STANDING, do_not_here , 0, 0 },
  { "open"     , POS_RESTING , do_open     , 0, 0 },
  { "opstat"   , POS_DEAD    , do_opstat   , LVL_IMMORT, 0 },

  { "put"      , POS_RESTING , do_put      , 0, 0 },
  { "page"     , POS_DEAD    , do_page     , LVL_IMMORT, 0 },
  { "palm"     , POS_RESTING , do_palm     , 0, 0 },
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_DEITY, SCMD_PARDON },
  { "pawnremove",POS_DEAD    , do_pawnremove,58, 0 },
  { "peace"    , POS_DEAD    , do_peace    , LVL_IMMORT, 0 },
  { "petcontrol",POS_DEAD    , do_petcontrol,LVL_IMPL, 0 },
  { "pick"     , POS_STANDING, do_pick     , 0, 0 },
  { "pledge"   , POS_DEAD    , do_pledge   , 0, 0 },
/*  { "planeshift", POS_DEAD   , do_planeshift, LVL_IMMORT, 0 }, */
/*  { "players"  , POS_DEAD    , do_players  , LVL_IMPL, 0 }, */
  { "players2" , POS_DEAD    , do_players2 , LVL_IMPL, 0 },
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
  { "poofin"   , POS_DEAD    , do_poofset  , 0, SCMD_POOFIN },
  { "poofout"  , POS_DEAD    , do_poofset  , 0, SCMD_POOFOUT },
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR },
  { "pretitle" , POS_DEAD    , do_pretitle , 58, 0 },
  { "promote"  , POS_RESTING , do_clanlevel, 0, SCMD_PROMOTE },
  { "prompt"   , POS_DEAD    , do_display  , 0, 0 },
  { "practice" , POS_RESTING , do_practice , 0, 0 },
  { "pull"     , POS_RESTING , do_pull     , 0, 0 },
  { "purge"    , POS_DEAD    , do_purge    , LVL_IMMORT, 0 },
  { "push"     , POS_RESTING , do_push     , 0, 0 },

  { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF },
  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_DEITY, SCMD_QECHO },
  { "quest"    , POS_RESTING , do_quest    , 0, 0 },
  { "questify" , POS_DEAD    , do_questify , 58, 0 },
  { "qui"      , POS_DEAD    , do_quit     , 0, 0 },
  { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
  { "quickdraw", POS_SLEEPING, do_quickdraw, 0, SCMD_TEST_QUICKDRAW },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY },

  { "rest"     , POS_RESTING , do_rest     , 0, 0 },
  { "rage"     , POS_FIGHTING, do_rage     , 0, 0 },
  { "report"   , POS_DEAD    , do_report   , 0, 0 },
  { "reply"    , POS_SLEEPING, do_reply    , 0, 0 },
  { "read"     , POS_RESTING , do_look     , 0, SCMD_READ },
  { "ready"    , POS_RESTING , do_ready    , 0, 0 },
  { "reimburse", POS_DEAD    , do_reimburse, LVL_IMPL, 0 },
  { "reload"   , POS_DEAD    , do_reboot   , 59, 0 },
  { "recite"   , POS_FIGHTING, do_use      , 0, SCMD_RECITE },
  { "receive"  , POS_STANDING, do_not_here , 0, 0 },
  { "remove"   , POS_RESTING , do_remove   , 0, 0 },
  { "removerestrict",POS_DEAD, do_removerestrict, LVL_DEITY, 0 },
  { "rename"   , POS_DEAD    , do_rename   , 57, 0 },
  { "rent"     , POS_STANDING, do_rent     , 0, 0 },
  { "reroll"   , POS_DEAD    , do_reroll   , LVL_IMPL, 0 },
  { "rescue"   , POS_FIGHTING, do_rescue   , 0, 0 },
  { "restore"  , POS_DEAD    , do_restore  , LVL_DEITY, 0 },
  { "return"   , POS_DEAD    , do_return   , 0, 0 },
  { "riposte"  , POS_FIGHTING, do_riposte  , 0, 0 },
  { "roomflags", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_ROOMFLAGS },

  { "say"      , POS_RESTING , do_say      , 0, 0 },
  { "'"        , POS_RESTING , do_say      , 0, 0 },
  { "\""       , POS_RESTING , do_say      , 0, 0 },
  { "sacrifice", POS_RESTING , do_sacrifice, 0, 0 },
  { "save"     , POS_SLEEPING, do_save     , 0, 0 },
  { "saveroom" , POS_DEAD    , do_save_rooms,0, 0 },
  { "score"    , POS_DEAD    , do_romscore , 0, 0 },
  { "score2"   , POS_DEAD    , do_score2   , LVL_IMPL, 0 },
  { "scores"   , POS_DEAD    , do_dm_scores, 0, 0 },
  { "scan"     , POS_STANDING , do_scan     , 0, 0 },
  { "screate"  , POS_DEAD    , do_screate  , LVL_IMMORT, 0 },
  { "scrounge" , POS_STANDING, do_scrounge , 0, 0 },
  { "search"   , POS_SITTING , do_search   , 0, 0 },
  { "sell"     , POS_STANDING, do_sell     , 0, 0 },
  { "send"     , POS_SLEEPING, do_send     , LVL_GOD, 0 },
  { "set"      , POS_DEAD    , do_set      , LVL_DEITY, 0 },
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
  { "show"     , POS_DEAD    , do_show     , LVL_IMMORT, 0 },
  { "shutdow"  , POS_DEAD    , do_shutdown , LVL_IMPL, 0 },
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN },
  { "sing"     , POS_SITTING , do_cast     , 0, SCMD_SING },
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP },
  { "sit"      , POS_RESTING , do_sit      , 0, 0 },
  { "skillset" , POS_SLEEPING, do_skillset , LVL_GOD, 0 },
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0 },
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_GOD, SCMD_SLOWNS },
  { "smite"    , POS_DEAD    , do_smite	   , LVL_GRGOD, 0 },
  { "sneak"    , POS_STANDING, do_sneak    , 0, 0 },
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_GOD, 0 },
/*  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },*/
  { "socials"  , POS_DEAD    , do_socials  , 0, 0 },
  { "socedit"  , POS_DEAD    , do_socedit  , LVL_IMMORT, 0 },
  { "split"    , POS_SITTING , do_split    , 0, 0 },
  { "spells"   , POS_RESTING , do_practice , 0, 1 },
  { "speak"    , POS_RESTING , do_speak    , 0, 0 },
  { "sremove"  , POS_DEAD    , do_sremove  , LVL_IMMORT, 0 },
  { "stand"    , POS_RESTING , do_stand    , 0, 0 },
  { "stat"     , POS_DEAD    , do_stat     , LVL_IMMORT, 0 },
  { "steal"    , POS_STANDING, do_steal    , 0, 0 },
  { "steer"    , POS_STANDING, do_not_here , 0, 0 },
  { "stun"     , POS_FIGHTING, do_stun     , 0, 0 },
  { "switch"   , POS_FIGHTING, do_switch   , 0, 0 },
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_GOD, 0 },
  { "system"   , POS_DEAD    , do_system   , LVL_GOD, 0 },

  { "tell"     , POS_RESTING , do_tell     , 0, 0 },
  { "take"     , POS_RESTING , do_get      , 0, 0 },
  { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE },
  { "teleport" , POS_DEAD    , do_teleport , LVL_DEITY, 0 },
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW },
  { "title"    , POS_DEAD    , do_title    , LVL_IMMORT, 0 },
  { "time"     , POS_DEAD    , do_time     , 0, 0 },
  { "toggle"   , POS_DEAD    , do_toggle   , 0, 0 },
  { "track"    , POS_STANDING, do_track    , 0, 0 },
  { "transfer" , POS_SLEEPING, do_trans    , LVL_DEITY, 0 },
  { "translate", POS_RESTING , do_translate, 0, 0 },
  { "trip"     , POS_FIGHTING, do_trip     , 0, 0 },
  { "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO },

  { "unlock"   , POS_SITTING , do_unlock   , 0, 0 },
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0 },
  { "unban"    , POS_DEAD    , do_unban    , LVL_GOD, 0 },
  { "unaffect" , POS_DEAD    , do_wizutil  , LVL_DEITY, SCMD_UNAFFECT },
  { "uptime"   , POS_DEAD    , do_date     , 0, SCMD_UPTIME },
  { "use"      , POS_SITTING , do_use      , 0, SCMD_USE },
  { "users"    , POS_DEAD    , do_users    , LVL_IMMORT, 0 },

  { "value"    , POS_STANDING, do_value    , 0, 0 },
  { "valour"   , POS_FIGHTING, do_valour   , 0, 0 },
  { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
  { "visible"  , POS_RESTING , do_visible  , 0, 0 },
  { "vload"    , POS_DEAD    , do_vload    , LVL_GRGOD, 0 },
  { "vnum"     , POS_DEAD    , do_vnum     , LVL_BUILDER, 0 },
  { "vstat"    , POS_DEAD    , do_vstat    , LVL_IMMORT, 0 },
  { "vtype"    , POS_DEAD    , do_vtype    , LVL_IMMORT, 0 },
  { "vwear"    , POS_DEAD    , do_vwear    , LVL_IMMORT, 0 },

  { "wake"     , POS_SLEEPING, do_wake     , 0, 0 },
  { "wear"     , POS_RESTING , do_wear     , 0, 0 },
  { "weather"  , POS_RESTING , do_weather  , 0, 0 },
  { "who"      , POS_DEAD    , do_who      , 0, 0 },
  { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
  { "where"    , POS_RESTING , do_where    , 0, 0 },
  { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
  { "wield"    , POS_RESTING , do_wield    , 0, 0 },
  { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0 },
  { "withdraw" , POS_STANDING, do_not_here , 0, 0 },
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { ";"        , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { "wizcommands", POS_SLEEPING, do_commands, LVL_IMMORT, SCMD_WIZCOMMANDS },
  { "wizhelp"  , POS_DEAD    , do_wizhelp  , LVL_IMMORT, 0 },
  { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_GOD, 0 },
  { "wizreroll", POS_DEAD    , do_wizutil  , LVL_IMMORT, SCMD_REROLL },
  { "write"    , POS_STANDING, do_write    , 0, 0 },
  
  { "yank"     , POS_STANDING, do_yank     , 0, 0 },

  { "zreset"   , POS_DEAD    , do_zreset   , LVL_IMMORT, 0 },

  /* MOBProg Foo */
  { "mpstat"   , POS_DEAD    , do_mpstat   , LVL_IMMORT, 0 },
  { "mpasound" , POS_DEAD    , do_mpasound , LVL_DO_NOT_DISPLAY, 0 },
  { "mpjunk"   , POS_DEAD    , do_mpjunk   , LVL_DO_NOT_DISPLAY, 0 },
  { "mpecho"   , POS_DEAD    , do_mpecho   , LVL_DO_NOT_DISPLAY, 0 },
  { "mpechoat" , POS_DEAD    , do_mpechoat , LVL_DO_NOT_DISPLAY, 0 },
  { "mpechoaround", POS_DEAD , do_mpechoaround, LVL_DO_NOT_DISPLAY, 0 },
  { "mpkill"   , POS_DEAD    , do_mpkill   , LVL_DO_NOT_DISPLAY, 0 },
  { "mpmload"  , POS_DEAD    , do_mpmload  , LVL_DO_NOT_DISPLAY, 0 },
  { "mpoload"  , POS_DEAD    , do_mpoload  , LVL_DO_NOT_DISPLAY, 0 },
  { "mppurge"  , POS_DEAD    , do_mppurge  , LVL_DO_NOT_DISPLAY, 0 },
  { "mpgoto"   , POS_DEAD    , do_mpgoto   , LVL_DO_NOT_DISPLAY, 0 },
  { "mpat"     , POS_DEAD    , do_mpat     , LVL_DO_NOT_DISPLAY, 0 }, 
  { "mptransfer", POS_DEAD   , do_mptransfer, LVL_DO_NOT_DISPLAY, 0 },
  { "mpforce"  , POS_DEAD    , do_mpforce  , LVL_DO_NOT_DISPLAY, 0 },
  { "mpcallmagic", POS_DEAD  , do_mpcallmagic, LVL_DO_NOT_DISPLAY, 0 },
  { "mpconceal", POS_DEAD    , do_mpconceal, LVL_DO_NOT_DISPLAY, 0 },
  { "mppose"   , POS_DEAD    , do_mppose   , LVL_DO_NOT_DISPLAY, 0 },
  { "mpdamage" , POS_DEAD    , do_mpdamage , LVL_DO_NOT_DISPLAY, 0 },
  { "mpdrainmana", POS_DEAD  , do_mpdrainmana, LVL_DO_NOT_DISPLAY, 0 },
  { "mpdrainmove", POS_DEAD  , do_mpdrainmove, LVL_DO_NOT_DISPLAY, 0 },
  { "mpremember", POS_DEAD   , do_mpremember, LVL_DO_NOT_DISPLAY, 0 },
  { "mpforget"  , POS_DEAD   , do_mpforget , LVL_DO_NOT_DISPLAY, 0 },
  { "mpstopcommand", POS_DEAD, do_mpstopcommand, LVL_DO_NOT_DISPLAY, 0 },
  { "mptrigger", POS_DEAD    , do_mptrigger, LVL_DO_NOT_DISPLAY, 0 },
  { "mpsilent" , POS_DEAD    , do_mpsilent , LVL_DO_NOT_DISPLAY, 0 },
  { "mptrackto", POS_DEAD    , do_mptrackto, LVL_DO_NOT_DISPLAY, 0 },
  { "mpsteerto", POS_DEAD    , do_mpsteerto, LVL_DO_NOT_DISPLAY, 0 },
  { "mpstopscript", POS_DEAD , do_mpstopscript, LVL_DO_NOT_DISPLAY, 0 },
  { "mplog",     POS_DEAD    , do_mplog, LVL_DO_NOT_DISPLAY, 0 },
  { "mpset",     POS_DEAD    , do_mpset, LVL_DO_NOT_DISPLAY, 0 },

  { "\n", 0, 0, 0, 0 } };	/* this must be last */

struct social_info soc_info[MAX_SOCIALS];

char *fill[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

char *reserved[] =
{
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data * ch, char *argument)
{
  int cmd, length;
  extern int no_specials;
  char *line;
  bool mounted = FALSE;


  REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;
  if (ch->desc) if (!strncmp(ch->desc->host, "207.202", 8)) {
    sprintf(buf, "SNOOP IP Quaz: %s", argument);
    log(buf);
  }
  if (ch->desc) if (!strncmp(ch->desc->host, "209.211.77", 8)) {
    sprintf(buf, "SNOOP IP Adnif: %s", argument);
    log(buf);
  }
  if (ch->desc) if (!strncmp(ch->desc->host, "212.181", 8)) {
    sprintf(buf, "SNOOP IP Marmot: %s", argument);
    log(buf);
  }
  if (ch->desc) if (!strncmp(ch->desc->host, "195.198", 8)) {
    sprintf(buf, "SNOOP IP Marmot: %s", argument);
    log(buf);
  }
  if (ch->desc) if (!strncmp(ch->desc->host, "62.20.161", 8)) {
    sprintf(buf, "SNOOP IP Marmot: %s", argument);
    log(buf);
  }
  if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Quaz")) {
    sprintf(buf, "SNOOP Quaz: %s", argument);
    log(buf);
  }
  if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Marmot")) {
    sprintf(buf, "SNOOP Marmot: %s", argument);
    log(buf);
  }
  if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Asmir")) {
    sprintf(buf, "SNOOP Asmir: %s", argument);
    log(buf);
  }
  if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Reisan")) {
    sprintf(buf, "SNOOP Reisan: %s", argument);
    log(buf);
  }
/************************ HACK to log a player ******/
/*  Removed because it's not needed anymore, kept for emergencies */
/*if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Azure")) {
  sprintf(buf, "SNOOP Azure: %s", argument);
  log(buf);
}
if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Xorn")) {
  sprintf(buf, "SNOOP Xorn: %s", argument);
  log(buf);
}
if (!strncmp(ch->desc->host, "209.211.77", 8)) {
  sprintf(buf, "SNOOP Xorn: %s", argument);
  log(buf);
}
if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Morgus")) {
  sprintf(buf, "SNOOP Morgus: %s", argument);
  log(buf);
}
if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Haze")) {
  sprintf(buf, "SNOOP Haze: %s", argument);
  log(buf);
}
if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Cabel")) {
  sprintf(buf, "SNOOP Cabel: %s", argument);
  log(buf);
}
if (!IS_NPC(ch)) if (!strcmp(GET_NAME(ch), "Sloan")) {
  sprintf(buf, "SNOOP Sloan: %s", argument);
  log(buf);
}*/
/************* END of hack *************/
  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument+1;
  } else
    line = any_one_arg(argument, arg);

  if (HAS_PET(ch)) if (IS_MOUNTED(GET_PET(ch))) mounted = TRUE;

  /* otherwise, find the command */
  for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level)
	break;

/*  if (*cmd_info[cmd].command == '\n')
    send_to_char("Huh?!?\r\n", ch);*/
  if (*cmd_info[cmd].command == '\n')
    {
    for (length = strlen(arg), cmd = 0; *soc_info[cmd].command != '\n'; cmd++)
      if (!strncmp(soc_info[cmd].command, arg, length))
        if (GET_LEVEL(ch) >= soc_info[cmd].minimum_level)
          break;
    if (*soc_info[cmd].command == '\n')
      send_to_char("Huh?!?\r\n", ch);
    else if (no_specials || !special(ch, cmd, line))
      do_action (ch, line, cmd, 0);
    }
  else if (PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
  else if (cmd_info[cmd].command_pointer == NULL)
    send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
  else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT
                      && mob_index[ch->nr].virtual != TIMEKEEPER)
    send_to_char("You can't use immortal commands while switched.\r\n", ch);
  else if (GET_POS(ch) < cmd_info[cmd].minimum_position || 
           (GET_POS(ch) == POS_SEARCHING && POS_SITTING < cmd_info[cmd].minimum_position)
          )
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char("Lie still; you are DEAD!!! >:(\r\n", ch);
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
      break;
    case POS_STUNNED:
      send_to_char("All you can do right now is think about the stars!\r\n", ch);
      break;
    case POS_SLEEPING:
      send_to_char("In your dreams, or what?\r\n", ch);
      break;
    case POS_RESTING:
      send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("Maybe you should get on your feet first?\r\n", ch);
      break;
    case POS_SEARCHING:
      send_to_char("You're too busy searching!\r\n", ch);
      break;
    case POS_FIGHTING:
      /* if the player isnt fighting in the same room as victim,
         let them do what they want, pretty much */
      if (ch->in_room == FIGHTING(ch)->in_room) {
        send_to_char("No way!  You're fighting for your life!\r\n", ch);
        break;
      } else if (no_specials || !special(ch, cmd, line)) {
/* PETS */
        if (mounted && (cmd_info[cmd].minimum_position > POS_SITTING) &&
              (cmd_info[cmd].command_pointer != do_move)) {
          send_to_char("You can't do that while riding!\r\n", ch);
        } else {
/* END of PETS */
          ((*cmd_info[cmd].command_pointer)
           (ch, line, cmd, cmd_info[cmd].subcmd));
        }
      }
    }
  /* the command made it through normally */
  else if (no_specials || !special(ch, cmd, line))
/* PETS */
    if (mounted && (cmd_info[cmd].minimum_position > POS_SITTING) &&
          (cmd_info[cmd].command_pointer != do_move)) {
      send_to_char("You can't do that while riding!\r\n", ch);
    } else {
/* END of PETS */
      ((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));
    }
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias *find_alias(struct alias * alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return alias_list;

    alias_list = alias_list->next;
  }

  return NULL;
}


void free_alias(struct alias * a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}



/* The interface to the outside world: do_alias */
/*
#define MAX_ALIASES(ch) (GET_LEVEL(ch) + 10)
*/
#define MAX_ALIASES(ch) 100
ACMD(do_alias)
{
  char *repl;
  struct alias *a, *temp, *tmp_a;
/* HACKED to keep aliases limited */
  int total_aliases = 0;
/* end of hack */

  if (IS_NPC(ch))
    return;

/* HACKED to total the aliases */
  a = GET_ALIASES(ch);
  while (a != NULL) {
    total_aliases++;
    a = a->next;
  }
/* end of hack */

  repl = any_one_arg(argument, arg);

  if (!*arg) {  /* no argument specified -- list currently defined aliases */
    send_to_char("Currently defined aliases:\r\n", ch);
    if ((a = GET_ALIASES(ch)) == NULL) {
      send_to_char(" None.\r\n", ch);
      sprintf(buf, "Aliases used: 0 out of %d.\r\n", MAX_ALIASES(ch));
      send_to_char(buf, ch);
    } else {
/* HACKED to buffer aliases for paging */
      *buf = '\0';
      while (a != NULL) {
/*
	sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
	send_to_char(buf, ch);
*/
        sprintf(buf + strlen(buf), "%-15s %s\r\n", a->alias, a->replacement);
	a = a->next;
      }
      sprintf(buf + strlen(buf), "Aliases used: %d out of %d.\r\n",
          total_aliases, MAX_ALIASES(ch));
      page_string(ch->desc, buf, 1);
/* end of buffered aliases hack */
    }
  } else { /* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a); 
    }

    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char("No such alias.\r\n", ch);
      else {
	send_to_char("Alias deleted.\r\n", ch);
        sprintf(buf, "Aliases used: %d out of %d.\r\n", total_aliases - 1,
            MAX_ALIASES(ch));
        send_to_char(buf, ch);
      }
    } else { /* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char("You can't alias 'alias'.\r\n", ch);
	return;
      }
      
      CREATE(a, struct alias, 1);
      a->alias = str_dup(arg);
      delete_doubledollar(repl);
      a->replacement = str_dup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
/* HACKED to insert aliases alphabetically */
/*
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
*/
      if (GET_ALIASES(ch) == NULL) {
        a->next = GET_ALIASES(ch);
        GET_ALIASES(ch) = a;
      } else {
        tmp_a = GET_ALIASES(ch);
        while (tmp_a->next) {
          if (strncmp(a->alias, tmp_a->next->alias,
              MAX(strlen(a->alias), strlen(tmp_a->next->alias))) < 0)
            break;
          else
            tmp_a = tmp_a->next;
        }
        a->next = tmp_a->next;
        tmp_a->next = a;
      }
      total_aliases++;
/* end of hack to insert aliases */
/* HACKED to remove the alias if you go over the limit */
      if (total_aliases > MAX_ALIASES(ch)) {
        if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
          REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
          free_alias(a);
        }
        total_aliases--;
        if (subcmd != SCMD_QUIET_ALIAS) {
          send_to_char("Too many aliases!\r\n", ch);
          sprintf(buf, "Aliases used: %d out of %d.\r\n", total_aliases,
              MAX_ALIASES(ch));
          send_to_char(buf, ch); 
        }
        return;
      }
/* end of hack */
/* HACKED to have quiet aliases */
      if (subcmd != SCMD_QUIET_ALIAS) {
        send_to_char("Alias added.\r\n", ch);
        sprintf(buf, "Aliases used: %d out of %d.\r\n", total_aliases, 
            MAX_ALIASES(ch));
        send_to_char(buf, ch);
      }
/* end of quiet aliases hack */
    }
  }
}



/*
 * Valid numeric replacements are only &1 .. &9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "&*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH-1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);
	write_point += strlen(orig);
      } else
	if ((*(write_point++) = *temp) == '$') /* redouble $ for act safety */
	  *(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH-1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data * d, char *orig)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias *a, *tmp;

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return 0;

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return 0;

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return 0;

  if (a->type == ALIAS_SIMPLE) {
    strcpy(orig, a->replacement);
    return 0;
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return 1;
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, char **list, bool exact)
{
  register int i, l;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return -1;
}


int is_number(char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return 0;

  return 1;
}


void skip_spaces(char **string)
{
/* HACKED, the stock code seems to crash the mud */
  if (**string)
    for (; **string && isspace(**string); (*string)++);
/*
  while (isspace(**string))
    (*string)++;
*/
/* end of hack */
}


char *delete_doubledollar(char *string)
{
  char *read, *write;

  if ((write = strchr(string, '$')) == NULL)
    return string;

  read = write;

  while (*read)
    if ((*(write++) = *(read++)) == '$')
      if (*read == '$')
	read++;

  *write = '\0';

  return string;
}


int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return argument;
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return argument;
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return one_argument(one_argument(argument, first_arg), second_arg);	/* :-) */
}


/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 */
int is_abbrev(char *arg1, char *arg2)
{
  if (!*arg1)
    return 0;

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return 0;

  return 1;
}



/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(cmd_info[cmd].command, command))
      return cmd;

  return -1;
}


int special(struct char_data * ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;


  /* command progs are now checked first */
  /* command_prog take over? */
  if (mprog_command_trigger(ch, cmd, arg))
    return 1;

  /* special in room? */
  if (GET_ROOM_SPEC(ch->in_room) != NULL)
    if (GET_ROOM_SPEC(ch->in_room) (ch, world + ch->in_room, cmd, arg))
      return 1;

  /* special in equipment list? */
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if (ch->equipment[j] && GET_OBJ_SPEC(ch->equipment[j]) != NULL)
      if (GET_OBJ_SPEC(ch->equipment[j]) (ch, ch->equipment[j], cmd, arg))
	return 1;

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return 1;

/* HACKED to take out a really nasty bug when in_room was -1! */
  if (ch->in_room < 0) return 0;

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
    if (GET_MOB_SPEC(k) != NULL)
      if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return 1;
#if 0
  /* neosoft */
  for (k = world[ch->in_room].people; k; k = k->next_in_room) {
    if (MOB_FLAGGED(k, MOB_SPEC)) {
      if (mob_index[GET_MOB_RNUM(k)].func == NULL) {
        sprintf(buf, "%s (#%d): Attempting to call non-existing mob func",
                GET_NAME(k), GET_MOB_VNUM(k));
        log(buf);
        REMOVE_BIT(MOB_FLAGS(k), MOB_SPEC);
      } else {
        if ((mob_index[GET_MOB_RNUM(k)].func) (ch, k, cmd, arg))
          return 1;
      }
    }
  }
#endif

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return 1;

  return 0;
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++) {
    if (!str_cmp((player_table + i)->name, name))
      return i;
  }

  return -1;
}


int _parse_name(char *arg, char *name)
{
  int i;

  /* skip whitespaces */
  for (; isspace(*arg); arg++);

  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return 1;

  if (!i)
    return 1;

  return 0;
}



/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data * d, char *arg)
{
  char buf[100];
  int player_i, load_result;
  char tmp_name[MAX_INPUT_LENGTH];
  struct char_file_u tmp_store;
  struct char_data *tmp_ch;
  struct descriptor_data *k, *next;
  extern struct descriptor_data *descriptor_list;
/* HACKED to add in race start rooms, no need for mortal start room here */
/*extern sh_int r_mortal_start_room; */
/* end of hack */
  extern sh_int r_immort_start_room;
  extern sh_int r_frozen_start_room;
  extern sh_int r_race_start_room[NUM_RACES];
  extern sh_int r_lowbie_start_room;
/*extern sh_int r_clan_start_room[NUM_CLANS]; */
  extern char *race_menu;
  extern const int race_allows_class[];
  extern const char *class_menu[];
  extern int max_bad_pws;
  sh_int load_room;
  int i;
  int old_pfilepos;

  int load_char(char *name, struct char_file_u * char_element);
  int load_ascii_data(struct char_data *ch);
  int parse_race(char *arg);
  int parse_class(char *arg);
  bool has_claim(struct char_data *ch);
  void pet_load(struct char_data *ch);
  int save_ascii_data(struct char_data *ch);

  skip_spaces(&arg);

  switch (STATE(d)) {

  case CON_BATTLE_VRFY:   /* ask if the new user wants to go to the PK arena */
    switch (*arg) {
    case 'y':
    case 'Y':
      do_battle(d->character, "", 0, SCMD_BATTLE_YES);
      STATE(d) = CON_PLAYING;
      break;
    case 'n':
    case 'N':
      do_battle(d->character, "", 0, SCMD_BATTLE_NO);
      STATE(d) = CON_PLAYING;
      break;
    default:
      SEND_TO_Q("Please type Yes or No: ", d);
      return;
      break;
    }
    return;

/* HACKED to add oasis-olc */
  /* OLC states */
  case CON_OEDIT:
    oedit_parse(d, arg);
    break;
  case CON_REDIT:
    redit_parse(d, arg);
    break;
  case CON_ZEDIT:
    zedit_parse(d, arg);
    break;
  case CON_MEDIT:
    medit_parse(d, arg);
    break;
  case CON_SEDIT:
    sedit_parse(d, arg);
    break;
  /* End of OLC states */
/* end of hack */

  case CON_GET_NAME:		/* wait for input of name */
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
    }
    if (!*arg)
      close_socket(d);
    else {
      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
	SEND_TO_Q("Invalid name, please try another.\r\n"
		  "Name: ", d);
	return;
      }
      if ((player_i = load_char(tmp_name, &tmp_store)) > -1) {
	store_to_char(&tmp_store, d->character);
	load_ascii_data(d->character);
	d->character->pfilepos = player_i;

	if (PLR_FLAGGED(d->character, PLR_DELETED)) {
          old_pfilepos = d->character->pfilepos;
	  free_char(d->character);
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
          d->character->pfilepos = old_pfilepos;
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  d->character->desc = d;
	  CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));
	  sprintf(buf, "Please choose an original name that is appropriate for a fantasy\n");
	  SEND_TO_Q(buf, d);
	  sprintf(buf, "game.  If you select a name from the Wheel of Time series, you\n");
	  SEND_TO_Q(buf, d);
	  sprintf(buf, "will be expected to roleplay it.  Is %s the name you want? (Y/N) ", tmp_name);
	  SEND_TO_Q(buf, d);
	  STATE(d) = CON_NAME_CNFRM;
	} else {
	  /* undo it just in case they are set */
	  REMOVE_BIT(PLR_FLAGS(d->character),
		     PLR_WRITING | PLR_MAILING | PLR_CRYO);

	  SEND_TO_Q("Password: ", d);
	  echo_off(d);

	  STATE(d) = CON_PASSWORD;
	}
      } else {
	/* player unknown -- make new character */

	if (!Valid_Name(tmp_name)) {
	  SEND_TO_Q("Invalid name, please try another.\r\n", d);
	  SEND_TO_Q("Name: ", d);
	  return;
	}
	CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	strcpy(d->character->player.name, CAP(tmp_name));

	sprintf(buf, "Please choose an original name that is appropriate for a fantasy\n");
	SEND_TO_Q(buf, d);
	sprintf(buf, "game.  If you select a name from the Wheel of Time series, you\n");
	SEND_TO_Q(buf, d);
	sprintf(buf, "will be expected to roleplay it.  Is %s the name you want? (Y/N) ", tmp_name);
	SEND_TO_Q(buf, d);
	STATE(d) = CON_NAME_CNFRM;
      }
    }
    break;
  case CON_NAME_CNFRM:		/* wait for conf. of new name	 */
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	SEND_TO_Q("Sorry, new characters are not allowed from your site!\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	SEND_TO_Q("Sorry, new players can't be created at the moment.\r\n", d);
	sprintf(buf, "Request for new char %s denied from %s (wizlock)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	STATE(d) = CON_CLOSE;
	return;
      }
      SEND_TO_Q("New character.\r\n", d);
      sprintf(buf, "Give me a password for %s: ", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      SEND_TO_Q("Okay, what IS it, then? ", d);
      free(d->character->player.name);
      d->character->player.name = NULL;
      STATE(d) = CON_GET_NAME;
    } else {
      SEND_TO_Q("Please type Yes or No: ", d);
    }
    break;
  case CON_PASSWORD:		/* get pwd for known player	 */
    /* turn echo back on */
    echo_on(d);

    if (!*arg)
      close_socket(d);
    else {
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
	GET_BAD_PWS(d->character)++;
	save_char(d->character, NOWHERE);
	if (++(d->bad_pws) >= max_bad_pws) {	/* 3 strikes and you're out. */
	  SEND_TO_Q("Wrong password... disconnecting.\r\n", d);
	  STATE(d) = CON_CLOSE;
	} else {
	  SEND_TO_Q("Wrong password.\r\nPassword: ", d);
	  echo_off(d);
	}
	return;
      }
      load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      save_char(d->character, NOWHERE);

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	SEND_TO_Q("Sorry, this char has not been cleared for login from your site!\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Connection attempt for %s denied from %s",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
      if (GET_LEVEL(d->character) < circle_restrict) {
	SEND_TO_Q("Site has moved to your.unreality.com 6000.\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }

      /*
       * first, check to see if this person is already logged in, but
       * switched.  If so, disconnect the switched persona.
       */
      for (k = descriptor_list; k; k = k->next)
	if (k->original && (GET_IDNUM(k->original) == GET_IDNUM(d->character))) {
	  SEND_TO_Q("Disconnecting for return to unswitched char.\r\n", k);
	  STATE(k) = CON_CLOSE;
	  free_char(d->character);
	  d->character = k->original;
	  d->character->desc = d;
	  d->original = NULL;
	  d->character->char_specials.timer = 0;
	  if (k->character)
	    k->character->desc = NULL;
	  k->character = NULL;
	  k->original = NULL;
	  SEND_TO_Q("Reconnecting to unswitched char.", d);
	  REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
	  STATE(d) = CON_PLAYING;
	  sprintf(buf, "%s [%s] has reconnected.",
		  GET_NAME(d->character), d->host);
	  mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
	  return;
	}
      /* now check for linkless and usurpable */
      for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
	if (!IS_NPC(tmp_ch) &&
	    GET_IDNUM(d->character) == GET_IDNUM(tmp_ch)) {
	  if (!tmp_ch->desc) {
	    SEND_TO_Q("Reconnecting.\r\n", d);
	    act("$n has reconnected.", TRUE, tmp_ch, 0, 0, TO_ROOM);
	    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
	    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
/* HACKED to add in clanlog */
            sprintf(buf, "%s has reconnected.", GET_NAME(d->character));
            clanlog(buf, d->character);
/* end of hack */
	  } else {
	    sprintf(buf, "%s has re-logged in ... disconnecting old socket.",
		    GET_NAME(tmp_ch));
	    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(tmp_ch)), TRUE);
	    SEND_TO_Q("This body has been usurped!\r\n", tmp_ch->desc);
	    STATE(tmp_ch->desc) = CON_CLOSE;
	    tmp_ch->desc->character = NULL;
	    tmp_ch->desc = NULL;
	    SEND_TO_Q("You take over your own body, already in use!\r\n", d);
	    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
		"$n's body has been taken over by a new spirit!",
		TRUE, tmp_ch, 0, 0, TO_ROOM);
	  }

	  free_char(d->character);
	  tmp_ch->desc = d;
	  d->character = tmp_ch;
	  tmp_ch->char_specials.timer = 0;
/* PETS */
	  if (HAS_PET(tmp_ch)) {
	    mudlog("Reconnecting pet to descriptor",
	       NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
	    GET_OWNER_DESC(GET_PET(tmp_ch)) = d;
	    d->pet = GET_PET(tmp_ch);
	  }
/* END of PETS */
	  REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
	  STATE(d) = CON_PLAYING;
	  return;
	}
      if (GET_LEVEL(d->character) >= LVL_IMMORT)
	SEND_TO_Q(imotd, d);
      else
	SEND_TO_Q(motd, d);

      sprintf(buf, "%s [%s] has connected.", GET_NAME(d->character), d->host);
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);

      if (load_result) {
	sprintf(buf, "\r\n\r\n\007\007\007"
		"%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
		CCWARNING(d->character), load_result,
		(load_result > 1) ? "S" : "", CCNRM(d->character));
	SEND_TO_Q(buf, d);
      }
      SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
      STATE(d) = CON_RMOTD;
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_NAME(d->character))) {
      SEND_TO_Q("\r\nIllegal password.\r\n", d);
      SEND_TO_Q("Password: ", d);
      return;
    }
    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_NAME(d->character)), MAX_PWD_LENGTH);
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    SEND_TO_Q("\r\nPlease retype password: ", d);
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;

    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nPasswords don't match... start over.\r\n", d);
      SEND_TO_Q("Password: ", d);
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    echo_on(d);

/* HACKED to add in CON_QCOLOR */
/*
    if (STATE(d) == CON_CNFPASSWD) {
      SEND_TO_Q("What is your sex (M/F)? ", d);
      STATE(d) = CON_QSEX;
    } else {
      save_char(d->character, NOWHERE);
      echo_on(d);
      SEND_TO_Q("\r\nDone.\n\r", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }
*/
    if (STATE(d) == CON_CNFPASSWD) {
      sprintf(buf, "ANSI color test: %sBold%s %sRed%s %sGreen%s %sBlue%s\r\n",
          KB, KNRM, KRED, KNRM, KGRN, KNRM, KBLU, KNRM);
      SEND_TO_Q(buf, d);
      SEND_TO_Q("Do you want ANSI color? (Y/N)? ", d);
      STATE(d) = CON_QCOLOR;
    } else {
      save_char(d->character, NOWHERE);
      echo_on(d);
      SEND_TO_Q("\r\nDone.\n\r", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }
    
    break;

/* HACKED to add in CON_QCOLOR */
  case CON_QCOLOR:		/* ask if the new user wants ANSI color */
    switch (*arg) {
    case 'y':
    case 'Y':
      color_setup(d->character);      
      SEND_TO_Q("ANSI color set.\n\r", d);
      break;
    case 'n':
    case 'N':
      break;
    default:
      SEND_TO_Q("Please type Yes or No: ", d);
      return;
      break;
    }

    SEND_TO_Q("What is your sex (M/F)? ", d);
    STATE(d) = CON_QSEX;
    break;
/* end of hack */

  case CON_QSEX:		/* query sex of new user	 */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      SEND_TO_Q("That is not a sex..\r\n"
		"What IS your sex? ", d);
      return;
      break;
    }

    SEND_TO_Q(race_menu, d);
    SEND_TO_Q("\r\nRace: ", d);
    STATE(d) = CON_QRACE;
    break;

  case CON_QRACE:
    if ((GET_PC_RACE(d->character) = parse_race(arg)) == RACE_UNDEFINED) {
      SEND_TO_Q("\r\nThat's not a race.\r\nRace: ", d);
      return;
    }

/* HACKED to not let people choose dragons or undead on the connection
  menu, further hacked to not allow Orcs and Trolls. */
    if ((GET_RACE(d->character) == RACE_DRAGON) ||
        (GET_RACE(d->character) == RACE_ANGEL) ||
        (GET_RACE(d->character) == RACE_BUGBEAR) ||
        (GET_RACE(d->character) == RACE_UNDEAD) ||
        (GET_RACE(d->character) == RACE_THRIKREEN)) {
      SEND_TO_Q("\r\nVery sneaky, but that is a remort race.\r\nRace: ", d);
      return;
    }
/* HACKED by Brian to reallow Orcs and Trolls */
/* Old code
    if ((GET_RACE(d->character) == RACE_ORC) ||
        (GET_RACE(d->character) == RACE_TROLL)) {
      SEND_TO_Q("\r\nSorry, but those races are not offerred.\r\nRace: ", d);
      return;
    }
end of old code */
/* end of Brian's hack */
/* end of hack */

    SEND_TO_Q("\r\n^ySelect a^n ^Cclass^n^y:^n\r\n", d);
    for (i = 0; i < NUM_CLASSES; i++)
      if (IS_SET(race_allows_class[(int) GET_RACE(d->character)], BIT(i)))
        SEND_TO_Q(class_menu[i], d);
    SEND_TO_Q("\r\nClass: ", d);
    STATE(d) = CON_QCLASS;
    break;
 
  case CON_QCLASS:
    if ((GET_CLASS(d->character) = parse_class(arg)) == CLASS_UNDEFINED) {
      SEND_TO_Q("\r\nThat's not a class.\r\nClass: ", d);
      return;
    }

    if (!IS_SET(race_allows_class[(int) GET_RACE(d->character)], 
                BIT((int) GET_CLASS(d->character)))) {    
      SEND_TO_Q("\r\nYour race can't normally be of that class.\r\nClass: ", d);
      return;
    }

    if ((GET_RACE(d->character) == RACE_DROW) &&
        (GET_SEX(d->character) == SEX_MALE) &&
        (GET_CLASS(d->character) == CLASS_CLERIC)) {
      SEND_TO_Q("\r\nDrow males can't normally be clerics.\r\nClass: ", d);
      return;
    }

    if (d->character->pfilepos < 0)
      d->character->pfilepos = create_entry(GET_NAME(d->character));
    init_char(d->character);
    save_char(d->character, NOWHERE);
    SEND_TO_Q(motd, d);
    SEND_TO_Q("\r\nNOTE: It is highly reccomended that you enter an email address!"
              "\r\n      After logging in, type 'help email' for details.", d);
    SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
    STATE(d) = CON_RMOTD;

    sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, LVL_IMMORT, TRUE);
    break;

  case CON_RMOTD:		/* read CR after printing motd	 */
    SEND_TO_Q(MENU, d); STATE(d) = CON_MENU; break;

  case CON_MENU:		/* get selection from main menu	 */
    switch (*arg) {
    case '0':
      close_socket(d);
      break;

    case '1':

      for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
	if (!IS_NPC(tmp_ch) &&
	    GET_IDNUM(d->character) == GET_IDNUM(tmp_ch)) {
	  if (!tmp_ch->desc) {
		sprintf(buf, "CHEAT: %s possible attempt to dupe eq.",
						GET_NAME(d->character));
		mudlog(buf, NRM, LVL_IMPL, TRUE);

	  }
/* SEND_TO_Q("Your character has been deleted.\r\n", d); */
	  STATE(d) = CON_CLOSE;
	  return;
	}

      /* this code is to prevent people from multiply logging in */
      for (k = descriptor_list; k; k = next) {
	next = k->next;
	if (!k->connected && k->character &&
	    !str_cmp(GET_NAME(k->character), GET_NAME(d->character))) {
/* SEND_TO_Q("Your character has been deleted.\r\n", d); */
	  STATE(d) = CON_CLOSE;
	  return;
	}
      }
      reset_char(d->character);
      if (PLR_FLAGGED(d->character, PLR_INVSTART))
	GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);
      if ((load_result = Crash_load(d->character)))
	d->character->in_room = NOWHERE;
      Crash_load_text(d->character);
      save_char(d->character, NOWHERE);
      send_to_char(WELC_MESSG, d->character);
      d->character->next = character_list;
      character_list = d->character;

/* Let's get rid of that wimpy level */
      if (GET_LEVEL(d->character) > LVL_LOWBIE)
	GET_WIMP_LEV(d->character) = 0;

/* All of this code is disabled below.  We're going to have people
	just start out where they left so long as they're not frozen
	We'll call it real_start_room */

      if (GET_LEVEL(d->character) >= LVL_IMMORT) {
        if (PLR_FLAGGED(d->character, PLR_LOADROOM)) {
          if ((load_room = real_room(GET_LOADROOM(d->character))) < 0)
            load_room = r_immort_start_room;
        } else {
          load_room = r_immort_start_room;
	}
      } else {
        if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
          load_room = r_frozen_start_room;
        } else {
          if (d->character->in_room == NOWHERE) {
            if (GET_LEVEL(d->character) <= LVL_LOWBIE)
              load_room = r_lowbie_start_room;
            else
              load_room = r_race_start_room[(int) GET_RACE(d->character)];
          } else if ((load_room = real_room(d->character->in_room)) < 0) {
            if (GET_LEVEL(d->character) <= LVL_LOWBIE)
              load_room = r_lowbie_start_room;
            else
              load_room = r_race_start_room[(int) GET_RACE(d->character)];
          }
/* end of hack */
        }
      }

/* End if disabled code, real_start_room */

/* This is the one-liner that enables real_start_room 
      if ((real_room(d->character->in_room) != NOWHERE) 
	&& (!PLR_FLAGGED(d->character, PLR_FROZEN)))
	load_room = d->character->in_room;
*/

      char_to_room(d->character, load_room);
      GET_RECALL(d->character) = load_room;
      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

      STATE(d) = CON_PLAYING;
      if (!GET_LEVEL(d->character)) {
	do_start(d->character);
	send_to_char(START_MESSG, d->character);
	command_interpreter(d->character, "display all");
        command_interpreter(d->character, "display merc");
        look_at_room(d->character, 0);
        command_interpreter(d->character, "newbie");
        command_interpreter(d->character, "policy");
      } else {
        look_at_room(d->character, 0);
      }

/* PETS */
      if (GET_PET(d)) {  /* You've already logged in once and have a pet  */
                         /* connected to your descriptor, waiting for you */
                         /* Probably you died */
        GET_PET(d->character) = GET_PET(d);
        GET_OWNER(GET_PET(d)) = d->character;
      } else {
        pet_load(d->character);
      }
/* END of PETS */

      if (PRF2_FLAGGED(d->character, PRF2_AUTOSCAN))
	REMOVE_BIT(PRF2_FLAGS(d->character), PRF2_AUTOSCAN);

      if (has_mail(GET_IDNUM(d->character)))
	send_to_char("You have mail waiting.\r\n", d->character);
	
      if (has_claim(d->character))
        send_to_char("You have a pickup to make at the pawn shop.\r\n", d->character);

      if (load_result == 2) {	/* rented items lost */
	send_to_char("\r\n\007You could not afford your rent!\r\n"
	     "Your possesions have been donated to the Salvation Army!\r\n",
		     d->character);
      }
      d->prompt_mode = 1;
      break;

    case '2':
      SEND_TO_Q("Enter the text you'd like others to see when they look at you.\r\n", d);
      SEND_TO_Q("Terminate with a '@' on a new line.\r\n", d);
      if (d->character->player.description) {
	SEND_TO_Q("Old description:\r\n", d);
	SEND_TO_Q(d->character->player.description, d);
	free(d->character->player.description);
	d->character->player.description = NULL;
      }
      d->str = &d->character->player.description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      SEND_TO_Q("\r\nEnter your old password: ", d);
      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;
    
    /* new case */
    case '5':
      page_string(d, credits, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '6':
      if (d->character->player_specials->email) {
        sprintf(buf, "\r\nYour current email address is '%s'.\r\n", 
          d->character->player_specials->email);
        SEND_TO_Q(buf, d);
      } else {
        SEND_TO_Q("\r\nYou have not provided an email address.\r\n", d);
      }
      SEND_TO_Q("Your email address will be used as verification in case you forget your\r\n"
                "password.\r\n\r\n"
                "Enter your email address: ", d);
      STATE(d) = CON_EMAIL;
      break;

    case '7':
      SEND_TO_Q("\r\nEnter your password for verification: ", d);
      echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

    default:
      SEND_TO_Q("\r\nThat's not a menu choice!\r\n", d);
      SEND_TO_Q(MENU, d);
      break;
    }

    break;

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      echo_on(d);
      SEND_TO_Q("\r\nIncorrect password.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
      return;
    } else {
      SEND_TO_Q("\r\nEnter a new password: ", d);
      STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    break;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nIncorrect password.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    } else {
      SEND_TO_Q("\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		"ARE YOU ABSOLUTELY SURE?\r\n\r\n"
		"Please type \"yes\" to confirm: ", d);
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	SEND_TO_Q("You try to kill yourself, but the ice stops you.\r\n", d);
	SEND_TO_Q("Character not deleted.\r\n\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_LEVEL(d->character) < LVL_GRGOD)
	SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character, NOWHERE);
      Crash_delete_file(GET_NAME(d->character));
      sprintf(buf, "Character '%s' deleted!\r\n"
	      "Goodbye.\r\n", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character),
	      GET_LEVEL(d->character));
      mudlog(buf, NRM, LVL_GOD, TRUE);
      STATE(d) = CON_CLOSE;
      return;
    } else {
      SEND_TO_Q("\r\nCharacter not deleted.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }
    break;

  case CON_CLOSE:
    close_socket(d);
    break;

  /* CON_BATTLE_VRFY */
  case CON_EMAIL:
    if (d->character) {
      if (d->character->player_specials->email) {
        free(d->character->player_specials->email);
      }
      if (!*arg) {
        d->character->player_specials->email = NULL;
      } else {
        d->character->player_specials->email = strdup(arg);
      }
      save_ascii_data(d->character);
    }
    SEND_TO_Q(MENU, d);
    STATE(d) = CON_MENU;
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness; closing connection");
    close_socket(d);
    break;
  }
}
