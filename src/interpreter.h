/* ************************************************************************
*   File: interpreter.h                                 Part of CircleMUD *
*  Usage: header file: public procs, macro defs, subcommand defines       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "circle.h"

#define ACMD(name)  \
   void (name)(struct char_data *ch, char *argument, int cmd, int subcmd)

#define CMD_NAME (cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strcmp(cmd_name, cmd_info[cmd].command))
#define IS_MOVE(cmdnum) (cmdnum >= 1 && cmdnum <= 11)  /* 11 = NUM_OF_DIRS */

void	command_interpreter(struct char_data *ch, char *argument);
int	search_block(char *arg, char **list, bool exact);
char	lower( char c );
char	*one_argument(char *argument, char *first_arg);
char	*any_one_arg(char *argument, char *first_arg);
char	*two_arguments(char *argument, char *first_arg, char *second_arg);
int	fill_word(char *argument);
void	half_chop(char *string, char *arg1, char *arg2);
void	nanny(struct descriptor_data *d, char *arg);
int	is_abbrev(char *arg1, char *arg2);
int	is_number(char *str);
int	find_command(char *command);
void	skip_spaces(char **string);
char	*delete_doubledollar(char *string);

/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments(a, b, c)


struct command_info {
   char *command;
   byte minimum_position;
   void	(*command_pointer)
   (struct char_data *ch, char * argument, int cmd, int subcmd);
   sh_int minimum_level;
   int	subcmd;
};

/* necessary for CMD_IS macro */
#ifndef __INTERPRETER_C__
extern struct command_info cmd_info[];
#endif

/* Maximum number of socials allowed */
#define MAX_SOCIALS 300
#define MAX_SOCIAL_LEN 15

struct social_info {
   char command[MAX_SOCIAL_LEN+1];
   byte minimum_position;
   sh_int minimum_level;
};

struct alias {
  char *alias;
  char *replacement;
  int type;
  struct alias *next;
};

#define ALIAS_SIMPLE	0
#define ALIAS_COMPLEX	1

#define ALIAS_SEP_CHAR	';'
#define ALIAS_VAR_CHAR	'$'
#define ALIAS_GLOB_CHAR	'*'

/*
 * SUBCOMMANDS
 *   You can define these however you want to, and the definitions of the
 *   subcommands are independent from function to function.
 */

/* directions */
#define SCMD_NORTH	1
#define SCMD_EAST	2
#define SCMD_SOUTH	3
#define SCMD_WEST	4
#define SCMD_UP		5
#define SCMD_DOWN	6
#define SCMD_NORTHEAST  7
#define SCMD_SOUTHEAST  8
#define SCMD_SOUTHWEST  9
#define SCMD_NORTHWEST  10
#define SCMD_SOMEWHERE	11	/* special purpose exit, good for traps etc */

/* do_battle */
#define SCMD_BATTLE	0
#define SCMD_BATTLE_YES	1
#define SCMD_BATTLE_NO	2

/* do_gen_ps */
#define SCMD_INFO       0
#define SCMD_HANDBOOK   1 
#define SCMD_CREDITS    2
#define SCMD_NEWS       3
#define SCMD_WIZLIST    4
#define SCMD_POLICIES   5
#define SCMD_VERSION    6
#define SCMD_IMMLIST    7
#define SCMD_MOTD	8
#define SCMD_IMOTD	9
#define SCMD_CLEAR	10
#define SCMD_WHOAMI	11
#define SCMD_CHANGES	12

/* do_gen_tog */
#define SCMD_NOSUMMON   0
#define SCMD_NOHASSLE   1
#define SCMD_BRIEF      2
#define SCMD_COMPACT    3
#define SCMD_NOTELL	4
#define SCMD_NOAUCTION	5
#define SCMD_DEAF	6
#define SCMD_NOGOSSIP	7
#define SCMD_NOGRATZ	8
#define SCMD_NOWIZ	9
#define SCMD_ROOMFLAGS	10
#define SCMD_NOREPEAT	11
#define SCMD_HOLYLIGHT	12
#define SCMD_SLOWNS	13
#define SCMD_AUTOEXIT	14
#define SCMD_AUTOLOOT	15
#define SCMD_AUTOSAC	16
#define SCMD_AUTOGOLD	17
#define SCMD_AUTOSPLIT	18
#define SCMD_AUTODIRS	19
#define SCMD_AUTOASSIST	20
#define SCMD_NOCLAN     21
#define SCMD_AWAY       22
#define SCMD_ANONYMOUS	23
#define SCMD_AUTOSCAN	24
#define SCMD_BATTLEBRIEF 25
#define SCMD_AUTOGROUP	26
#define SCMD_DEATHMATCH 27
#define SCMD_NOMUSIC	28
#define SCMD_AUTOMAP    29

/* do_wizutil */
#define SCMD_REROLL	0
#define SCMD_PARDON     1
#define SCMD_NOTITLE    2
#define SCMD_SQUELCH    3
#define SCMD_FREEZE	4
#define SCMD_THAW	5
#define SCMD_UNAFFECT	6

/* do_spec_com */
#define SCMD_WHISPER	0
#define SCMD_ASK	1

/* do_gen_com */
#define SCMD_HOLLER	0
#define SCMD_SHOUT	1
#define SCMD_GOSSIP	2
#define SCMD_AUCTION	3
#define SCMD_GRATZ	4

/* do_shutdown */
#define SCMD_SHUTDOW	0
#define SCMD_SHUTDOWN   1

/* do_quit */
#define SCMD_QUI	0
#define SCMD_QUIT	1

/* do_date */
#define SCMD_DATE	0
#define SCMD_UPTIME	1

/* do_commands */
#define SCMD_COMMANDS	0
#define SCMD_SOCIALS	1
#define SCMD_WIZCOMMANDS 2

/* do_drop */
#define SCMD_DROP	0
#define SCMD_JUNK	1
#define SCMD_DONATE	2

/* do_gen_write */
#define SCMD_BUG	0
#define SCMD_TYPO	1
#define SCMD_IDEA	2

/* do_look */
#define SCMD_LOOK	0
#define SCMD_READ	1

/* do_qcomm */
#define SCMD_QSAY	0
#define SCMD_QECHO	1

/* do_clancomm */
#define SCMD_CLANSAY	0

/* do_pour */
#define SCMD_POUR	0
#define SCMD_FILL	1

/* do_poof */
#define SCMD_POOFIN	0
#define SCMD_POOFOUT	1

/* do_hit */
#define SCMD_HIT	0
#define SCMD_MURDER	1

/* do_eat */
#define SCMD_EAT	0
#define SCMD_TASTE	1
#define SCMD_DRINK	2
#define SCMD_SIP	3

/* do_use */
#define SCMD_USE	0
#define SCMD_QUAFF	1
#define SCMD_RECITE	2
#define SCMD_EAT_PILL	3

/* do_echo */
#define SCMD_ECHO	0
#define SCMD_EMOTE	1

/* do_gen_door */
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4

/* do_alias */
#define SCMD_QUIET_ALIAS 1	/* set alias without saying 'Alias added.' */

/* do_quickdraw */
#define SCMD_TEST_QUICKDRAW	0
#define SCMD_REAL_QUICKDRAW	1

/* do_bash */
/* left empty, SCMD_BASH 0 */
#define SCMD_BASH_NO_SAVE 1

/* HACKED to add oasis-olc */
/* do_olc */
#define SCMD_OLC_REDIT		0
#define SCMD_OLC_OEDIT		1
#define SCMD_OLC_ZEDIT		2
#define SCMD_OLC_MEDIT		3
#define SCMD_OLC_SEDIT		4
#define SCMD_OLC_SAVEINFO	5
/* end of hack */

/* do_avenging_blow */
#define SCMD_TEST_AVENGING_BLOW	0
#define SCMD_REAL_AVENGING_BLOW	1

/* do_save */
#define SCMD_QUIET_SAVE		1

/* do_trip */
#define SCMD_TRIP_NO_SAVE	1

/* do_peace */
#define SCMD_QUIET_PEACE	1

/* do_cast */
#define SCMD_CAST		0
#define SCMD_PRAY		1
#define SCMD_SING		2

/* do_clanlevel */
#define SCMD_PROMOTE		0
#define SCMD_DEMOTE		1

/* do_flee */
#define SCMD_FLEE_ALWAYS	1
