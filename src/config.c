/* ************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __CONFIG_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#define TRUE	1
#define YES	1
#define FALSE	0
#define NO	0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/


/* GAME PLAY OPTIONS */

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
int pk_allowed = NO;

/* is playerthieving allowed? */
int pt_allowed = NO;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 1;

/* number of movement points it costs to holler */
int holler_move_cost = 20;

/* maximum amount of hitpoints a persons wimpy can be set to */
int max_wimpy_lev = 200;

/* exp change limits */
int max_exp_gain = 200000;	/* max gainable per kill */
int max_exp_loss = 5000000;	/* max losable per death */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 20;

/* should items in death traps automatically be junked? */
int dts_are_dumps = YES;

/* "okay" etc. */
char *OK = "Okay.\r\n";
char *NOPERSON = "No-one by that name here.\r\n";

/* is the auction system on? */
int auction_on = YES;

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.
 */
int free_rent = YES;

/* maximum number of items players are allowed to rent */
int max_obj_save = 100;

/* receptionist's surcharge on top of item costs */
int min_rent_cost = 100;

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.
 */
int auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int autosave_time = 5;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int crash_file_timeout = 180;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 360;


/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that mortals should enter at */
/* note: the race start rooms are in race.c in the race_start_room array */
/* note: the clan start rooms are in clan.c in the clan_start_room array */
/* this is just a default room if all else fails (mostly) */
sh_int mortal_start_room = 3005;

/* virtual number of room that immorts should enter at by default */
sh_int immort_start_room = 1204;

/* virtual number of room that frozen players should enter at */
sh_int frozen_start_room = 1202;

/* virtual number of room that players bodies (and players) enter at */
sh_int mortuary_start_room = 3001;

/* virtual number of room that players under LEVEL_LOWBIE enter at */
sh_int lowbie_start_room = 3005;  /* used to be 302 */

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.obj1.c if you change the number of non-NOWHERE
 * donation rooms.
 */
const sh_int donation_room_1 = 3063;
const sh_int donation_room_2 = 5510;	/* unused - room for expansion */
const sh_int donation_room_3 = NOWHERE;	/* unused - room for expansion */


/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/* default port the game should run on if no port given on command-line */
int DFLT_PORT = 6666;
int port = 5000;

/* creation port, a port where lower level people can build */
int creation_port = 4500;

/* default directory to use as data directory */
char *DFLT_DIR = "lib";

/* maximum number of players allowed before game starts to turn people away */
int MAX_PLAYERS = 300;

/* maximum size of bug, typo and idea files (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 1;

/*
 * Some nameservers are very slow and cause the game to lag terribly every 
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = YES;

/* And of course we have the default mode for the arena: Deathmatch OFF */

int arena_deathmatch_mode = NO;
int arena_deathmatch_level = 50;

char *MENU =
"\r\n"
"+-------------------------------+       -= Heroes of Kore MUD =-\r\n"
"|                               |       Based on Dikumud and Circle 3.0\r\n"
"|    0  Exit Heroes of Kore     |       Dikumud programmed by:\r\n"
"|    1  Enter game              |         Hans Henrik Staerfeldt,\r\n"
"|    2  Enter description       |         Katja Nyboe, Tom Madsen,\r\n"
"|    3  Read background         |         Michael Seifert, and\r\n"
"|    4  Change password         |         Sebastian Hammer\r\n"
"|    5  Read credits            |       Circlemud programmed by:\r\n"
"|    6  Change e-mail address   |         Jeremy Eleson\r\n"
"|    7  Delete this character   |       Mobprogramming by:\r\n"
"|    Make your choice           |         N'Atas-Ha\r\n"
"|                               |       On-line coding by:\r\n"
"+-------------------------------+         Harvey Gilpin and Levork\r\n";

/*
"\r\n"
"    You stand in a cold and dank place, all around you is nothingness.\r\n"
"You can see yourself, but aside from that, nothing. Before panic has a\r\n"
"chance to set in, you notice something changing in the darkness directly\r\n"
"before you. As you stare in awe the area slowly begins to coalesce into a\r\n"
"form. It is not long before you can make out what it is: a tattered old\r\n"
"manuscript covered in cryptic runes. You do not know the language, but\r\n" 
"somehow you can understand the strange writings. You feel an overwelming\r\n" 
"urge to choose one of the options (how do you know that they're options???).\r\n"
"\r\n"
"^y     __^^__                                              __^^__^n\r\n"
"^y    ( ___ )--------------------------------------------( ___ )^n\r\n"
"^y     | / |                                              | \\ |^n\r\n"
"^y     | / |           ^n^MWelcome to Heroes of Kore^n^y          | \\ |^n\r\n"
"^y     | / |                                              | \\ |^n\r\n"
"^y     | / |           ^n^r(^n^RL^n^r)^n ^beave Heroes of Kore^n^y            | \\ |^n\r\n"
"^y     | / |           ^n^r(^n^RE^n^r)^n ^bnter the MUD^n^y                   | \\ |^n\r\n"
"^y     | / |           ^n^r(^n^RW^n^r)^n ^brite your description^n^y          | \\ |^n\r\n"
"^y     | / |           ^n^r(^n^RR^n^r)^n ^bead background^n^y                 | \\ |^n\r\n"
"^y     | / |           ^n^r(^n^RC^n^r)^n ^bhange your password^n^y            | \\ |^n\r\n"
"^y     | / |           ^n^r(^n^RA^n^r)^n ^bbout CircleMUD^n^y                 | \\ |^n\r\n"
"^y     |___|                                              |___|^n\r\n"
"^y    (_____)--------------------------------------------(_____)^n\r\n"
"^N"
"\r\n"
"\r\n";
*/


char *GREETINGS =
"\r\n"
"          @@@  @@@  @@@@@@@@  @@@@@@@    @@@@@@   @@@@@@@@   @@@@@@\r\n"
"          @@@  @@@  @@@@@@@@  @@@@@@@@  @@@@@@@@  @@@@@@@@  @@@@@@@\r\n"
"          @@!  @@@  @@!       @@!  @@@  @@!  @@@  @@!       !@@\r\n"
"          !@!  @!@  !@!       !@!  @!@  !@!  @!@  !@!       !@!\r\n"
"          @!@!@!@!  @!!!:!    @!@!!@!   @!@  !@!  @!!!:!    !!@@!!\r\n"
"          !!!@!!!!  !!!!!:    !!@!@!    !@!  !!!  !!!!!:     !!@!!!\r\n"
"          !!:  !!!  !!:       !!: :!!   !!:  !!!  !!:            !:!\r\n"
"          :!:  !:!  :!:       :!:  !:!  :!:  !:!  :!:           !:!\r\n"
"          ::.  :::  .::.::::  ::.   ::: :::::.::  .::.::::  ::::.::\r\n"
"          .:.  :.:  :.::.::.  .:.    :.: :.:..:   :.::.::.  ::.:.:\r\n"
"\r\n"
"                               @@@@@@   @@@@@@@@\r\n"
"                              @@@@@@@@  @@@@@@@@\r\n"
"                              @@!  @@@  @@!\r\n"
"                              !@!  @!@  !@!\r\n"
"                              @!@  !@!  @!!!:!\r\n"
"                              !@!  !!!  !!!!!:\r\n"
"                              !!:  !!!  !!:\r\n"
"                              :!:  !:!  :!:\r\n"
"                              :::::.::  .::\r\n"
"                               :.:..:   .:.\r\n"
"\r\n"
"                   @@@    @@@  @@@@@@   @@@@@@@   @@@@@@@@\r\n"
"                   @@@   @@@  @@@@@@@@  @@@@@@@@  @@@@@@@@\r\n"
"                   @@!  !@@   @@!  @@@  @@!  @@@  @@!\r\n"
"                   !@!  @!!   !@!  @!@  !@!  @!@  !@!\r\n"
"                   @!@@!@!    @!@  !@!  @!@!!@!   @!!!:!\r\n"
"                   !!@!!!     !@!  !!!  !!@!@!    !!!!!:\r\n"
"                   !!: :!!    !!:  !!!  !!: :!!   !!:\r\n"
"                   :!:  !:!   :!:  !:!  :!:  !:!  :!:\r\n"
"                   .::   :::  :::::.::  ::.   ::: .::.::::\r\n"
"                   .:.    :::  :.:..:   .:.    :.::.::.::.\r\n"
"\r\n"
"By what name do you wish to be known? ";


char *WELC_MESSG =
"\r\n"
"Welcome back to Kore..."
"\r\n\r\n";

char *START_MESSG =
"\r\n"
"Welcome to Heroes of Kore MUD!"
"\r\n"
"If you are new to muds, get help on these commands:\r\n"
"help, help prompt, help compact, help auto exits\r\n"
"\r\n\r\n";

/****************************************************************************/
/****************************************************************************/

/* AUTOWIZ OPTIONS */

/* Should the game automatically create a new wizlist/immlist every time
   someone immorts, or is promoted to a higher (or lower) god level? */
int use_autowiz = NO;

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
int min_wizlist_lev = LVL_GOD;
