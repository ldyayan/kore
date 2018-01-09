/***************************************************************************
 * Code to handle timed events: either countdown events in nnn ticks, or   *
 * stuff that happens at a predefined time                                 *
 ***************************************************************************/


/* * * * * * *   How it works:   * * * * * * *
The list of countdown events is stored in a linked list, pointed to by
countdown. Every time countdown_timer_tick() is called, the timer value
of each event is decremented, and any events that reach zero are inserted
into a command_queue, then deleted from the countdown queue.

At the end of countdown_timer_tick, if there are commands ready to execute,
the timekeeper mob is loaded to the right room. All the commands in the
command queue are sent to command_interpreter as if they were typed by the
mob, and then the mob is immediatly purged.

Note that while imm commands cannot be used, mobprog commands will work.
The timekeeper mob should never actually be visible to players - use mpsilent
to prevent this if neccessary.
* * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"

/***** Definitions *****/
/* vnum for the timekeeper : the mob that actually executes the commands */
/* NOTE - this is also defined at the top of interpreter.c! */
#define TIMEKEEPER 1206

/***** Internal structures *****/
/* The list of countdown events that are waiting to happen */
struct countdown_queue {
  char *command;
  unsigned int timer;
  room_num room;
  struct countdown_queue *next;
};
/* The list of commands that need to be executed */
struct command_queue {
  char *command;
  room_num room;
  struct command_queue *next;
};

/***** Local globals *****/
struct countdown_queue *countdown;
struct command_queue *commands, *commands_bot;

/***** External globals *****/
extern int top_of_world;
extern struct room_data *world;

/***** Internal function prototypes *****/
void boot_event_queue();
void countdown_timer_tick();
void insert_commands(char *text, room_num room);
void execute_commands();
void insert_one_command(char *cmd, room_num room);
void event_to_queue(int room, int time, char *cmd);
ACMD(do_delay);

/***** The actual code *****/
void boot_event_queue() {
  countdown = NULL;
  commands = commands_bot = NULL;
}

void countdown_timer_tick() {
  struct countdown_queue *i, *prev;
  
  prev = NULL;
  
  for (i = countdown; i; i = i->next) {
    if (--(i->timer) <= 0) {
      insert_commands(i->command, i->room);
      if (prev) {
        prev->next = i->next;
      } else {
        prev = i;
        countdown = i->next;
      }
      if (i->command) free(i->command);
      free(i);
    } else prev = i;
  }
  
  if (commands) execute_commands();
}

void insert_commands(char *text, room_num room) {
  char *bufptr;
  
  if (!text) return;

  bufptr = buf;

  while (*text) {
    if (*text == ';') {
      *bufptr = '\0';
      insert_one_command(buf, room);
      bufptr = buf;
    } else {
      *bufptr = *text;
      bufptr++;
    }
    text++;
  }
  *bufptr = '\0';
  insert_one_command(buf, room);
}

void insert_one_command(char *cmd, room_num room) {
  struct command_queue *new;
  
  if (!cmd) return;
  if (!*cmd) return;
  CREATE(new, struct command_queue, 1);
  new->next = NULL;
  new->command = strdup(cmd);
  new->room = room;
  if (commands) {
    commands_bot->next = new;
    commands_bot = new;
  } else {
    commands = commands_bot = new;
  }
}

void execute_commands() {
  struct char_data *tk; /* The timekeeper */
  struct command_queue *q, *nxt;
  int rnum, vnum;
  if (!commands) return;   /* Don't let's be silly, now */
  
  tk = read_mobile(TIMEKEEPER, VIRTUAL);
  if (!tk) {
    mudlog("Fatal error: Unable to create the timekeeper!", NRM, LVL_DEITY, TRUE);
    /* Ugh...better purge the queue now, that's a shame */
    for (q = commands; q;) {
      nxt = q->next;
      if (q->command) free (q->command);
      free(q);
      q = nxt;
    }
    return;
  }
  
  vnum = -1;           /* Make sure the tk moves on the first command */
  char_to_room(tk, 0); /* Gotta start somewhere, the void seems good */
  
  for (q = commands; q;) {
    if (q->room != vnum) {     /* Gotta move the timekeeper */
      rnum = real_room(q->room);
      if (rnum < 0) {
        /* The command is going to happen anyway, just not where you want */
        sprintf(buf2, "Error: room %d does not exist for timed command '%s'!",
                q->room, q->command);
        mudlog(buf2, NRM, LVL_DEITY, TRUE);
      } else {
        char_from_room(tk);
        char_to_room(tk, rnum);
        vnum = q->room;
      }
    }
    command_interpreter(tk, q->command);   /* Execute the command now */
    
    /* Update the queue */
    nxt = q->next;
    if (q->command) free (q->command);
    free(q);
    q = nxt;
  }
  
  /* And....we're done */
  commands = commands_bot = NULL;   /* Clear the queue */
  extract_char(tk);    /* lose the timekeeper, we're done with him */
}

void show_timer_queue(struct char_data *ch) {
  struct countdown_queue *i;

  if (!ch) return;
  if (!ch->desc) return;
  
  sprintf(buf, "%s%-7s %-7s %s\r\n", "", "Room", "Time", "Command");
  sprintf(buf, "%s--------------------------------------------\r\n", buf);
  
  for (i = countdown; i; i = i->next) {
    sprintf(buf, "%s%-7d %-7d %s\r\n", buf, i->room, i->timer, i->command);
  }
  
  page_string(ch->desc, buf, 1);
}

ACMD(do_delay) {
  /* Syntax: "delay <time> <command>". Room is set to the imm's room */
  int time;
  
  if (IS_NPC(ch)) return;
  
  argument = one_argument(argument, buf);
  skip_spaces(&argument);
  if (!*argument) {
    send_to_char("Syntax: delay <time> <command>\r\nTime is in minutes.\r\n"
                 "Multiple commands may be seperated by semicolons.\r\n"
                 "The event will take place in the room you are in.\r\n", ch);
    return;
  }
  
  time = atoi(buf);
  if (time <= 0) {
    send_to_char("Syntax: delay <time> <command>\r\n"
                 "Error : time must be greater than zero!\r\n", ch);
    return;
  }
  
  event_to_queue(world[ch->in_room].number, time, argument);
  send_to_char("Ok.\r\n", ch);
}

void event_to_queue(int room, int time, char *cmd) {
  struct countdown_queue *new;
  
  if (room == NOWHERE || room >= top_of_world) {
    mudlog("Illegal room rnum in event_to_queue", NRM, LVL_GRGOD, TRUE);
    return;
  }
  
  CREATE(new, struct countdown_queue, 1);
  
  new->next = countdown;
  new->room = room;
  new->timer = time;
  new->command = strdup(cmd);
  countdown = new;
}

  
