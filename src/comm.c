/* ************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __COMM_C__

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "screen.h"
#include "olc.h"

/* externs */
extern int restrict;
extern int mini_mud;
extern int no_rent_check;
extern FILE *player_fl;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int MAX_PLAYERS;
extern int MAX_DESCRIPTORS_AVAILABLE;

extern struct room_data *world;	/* In db.c */
extern int top_of_world;	/* In db.c */
extern struct time_info_data time_info;	/* In db.c */
extern char help[];
extern char *true_color_codes[];
extern char *color_codes[];

/* local globals */
struct descriptor_data *descriptor_list = NULL;	/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int no_specials = 0;		/* Suppress ass. of special routines */
int avail_descs = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
bool MOBTrigger = TRUE;		/* for MOBProg */
int MOBHandled = 0;		/* for MOBProg */
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern int autosave_time;	/* see config.c */
struct timeval null_time;	/* zero-valued time structure */
extern struct title_type titles[NUM_CLASSES][LVL_IMPL + 1];
/* HACKED to show experience needed on the prompt */
/* extern int experience_table[LVL_IMPL + 1]; */
/* end of hack */

/* functions in this file */
int get_from_q(struct txt_q * queue, char *dest, int *aliased);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_avail_descs(void);
int process_output(struct descriptor_data * t);
int process_input(struct descriptor_data * t);
void close_socket(struct descriptor_data * d);
struct timeval timediff(struct timeval * a, struct timeval * b);
void flush_queues(struct descriptor_data * d);
void nonblock(int s);
int perform_subst(struct descriptor_data * t, char *orig, char *subst);
int perform_alias(struct descriptor_data * d, char *orig);
void record_usage(void);
void make_prompt(struct descriptor_data * point);

/* extern fcnts */
void boot_db(void);
void zone_update(void);
void affect_update(void);	/* In spells.c */
void quest_update(void);	/* In quest.c */
void point_update(void);	/* In limits.c */
void obj_update(void);		/* In limits.c */
void update_specials(void);     /* In limits.c */
void mobile_activity(void);
/* AUTOQUEST */
void string_add(struct descriptor_data * d, char *str);
void perform_violence(void);
void show_string(struct descriptor_data * d, char *input);
int isbanned(char *hostname);
void weather_and_time(int mode);
void mprog_act_trigger( char *buf, struct char_data *mob, struct char_data *ch,
			struct obj_data *obj, void *vo);
/* HACKED to add diagnose to the prompt */
const char *diag_char_to_char_short(struct char_data *ch);
/* end of hack */
/* extern fctns special to NeXTs */
#ifdef NeXT
void srandom(int seed);		/* in /NextDeveloper/Headers/bsd/libc.h */
#endif
void perform_scripts(void);
void perform_pulse_functions(void);
void perform_all_search(void);
void auction_update(void);



/* *********************************************************************
*  main game loop and related stuff                                    *
********************************************************************* */

int main(int argc, char **argv)
{
  extern int port;
  char buf[512];
  int pos = 1;
  char *dir;


  port = DFLT_PORT;
  dir = DFLT_DIR;

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'd':
      if (*(argv[pos] + 2))
	dir = argv[pos] + 2;
      else if (++pos < argc)
	dir = argv[pos];
      else {
	log("Directory arg expected after option -d.");
	exit(1);
      }
      break;
    case 'm':
      mini_mud = 1;
      no_rent_check = 1;
      log("Running in minimized mode & with no rent check.");
      break;
    case 'q':
      no_rent_check = 1;
      log("Quick boot mode -- rent check supressed.");
      break;
    case 'r':
      restrict = 1;
      log("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      log("Suppressing assignment of special routines.");
      break;
    default:
      sprintf(buf, "SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
      log(buf);
      break;
    }
    pos++;
  }

  if (pos < argc)
    if (!isdigit(*argv[pos])) {
      fprintf(stderr, "Usage: %s [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      fprintf(stderr, "Illegal port number.\n");
      exit(1);
    }
  sprintf(buf, "Running game on port %d.", port);
  log(buf);

  if (chdir(dir) < 0) {
    perror("Fatal error changing to data directory");
    exit(1);
  }
  sprintf(buf, "Using %s as data directory.", dir);
  log(buf);

  init_game(port);

  return 0;
}



/* Init sockets, run game, and cleanup sockets */
void init_game(int port)
{
  int mother_desc;
  void my_srand(unsigned long initial_seed);

  my_srand(time(0));

  log("Opening mother connection.");
  mother_desc = init_socket(port);

  avail_descs = get_avail_descs();

  boot_db();

  log("Signal trapping.");
  signal_setup();

  log("Entering game loop.");

  game_loop(mother_desc);

  log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  close(mother_desc);
  fclose(player_fl);

  if (circle_reboot) {
    log("Rebooting.");
    exit(52);			/* what's so great about HHGTTG, anyhow? */
  }
  log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(int port)
{
  int s, opt;
  struct sockaddr_in sa;

  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so ths point is (hopefully) moot.
   */

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Create socket");
    exit(1);
  }
#if defined(SO_SNDBUF)
  opt = LARGE_BUFSIZE + GARBAGE_SPACE;
  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt SNDBUF");
    exit(1);
  }
#endif

#if defined(SO_REUSEADDR)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt REUSEADDR");
    exit(1);
  }
#endif

#if defined(SO_REUSEPORT)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt REUSEPORT");
    exit(1);
  }
#endif

#if defined(SO_LINGER)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld)) < 0) {
      perror("setsockopt LINGER");
      exit(1);
    }
  }
#endif

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(s, (struct sockaddr *) & sa, sizeof(sa)) < 0) {
    perror("bind");
    close(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return s;
}


int get_avail_descs(void)
{
  int max_descs = 0;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.
 */
#if defined (RLIMIT_NOFILE) || defined (RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
  {
    struct rlimit limit;

    getrlimit(RLIMIT_NOFILE, &limit);
    max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
    limit.rlim_cur = max_descs;
    setrlimit(RLIMIT_NOFILE, &limit);
  }
#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
  max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
				 * OPEN_MAX */
#else
  /*
   * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
   * use the POSIX sysconf() function.  (See Stevens' _Advanced Programming
   * in the UNIX Environment_).
   */
  errno = 0;
  if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
    if (errno == 0)
      max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
    else {
      perror("Error calling sysconf");
      exit(1);
    }
  }
#endif

  max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    log("Non-positive max player limit!");
    exit(1);
  }
  sprintf(buf, "Setting player limit to %d.", max_descs);
  log(buf);
  return max_descs;
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(int mother_desc)
{
  void countdown_timer_tick();

  fd_set input_set, output_set, exc_set;
  struct timeval last_time, now, timespent, timeout, opt_time;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int pulse = 0, mins_since_crashsave = 0, maxdesc, aliased;

  /* initialize various time values */
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;
  opt_time.tv_sec = 0;
  gettimeofday(&last_time, (struct timezone *) 0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

    /* Sleep if we don't have any connections */
    if (descriptor_list == NULL) {
      log("No connections.  Going to sleep.");
      FD_ZERO(&input_set);
      FD_SET(mother_desc, &input_set);
      if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
	if (errno == EINTR)
	  log("Waking up to process signal.");
	else
	  perror("Select coma");
      } else
	log("New connection.  Waking up.");
      gettimeofday(&last_time, (struct timezone *) 0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);
    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next) {
      if (d->descriptor > maxdesc)
	maxdesc = d->descriptor;
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    /*
     * At this point, the original Diku code set up a signal mask to avoid
     * block all signals from being delivered.  I believe this was done in
     * order to prevent the MUD from dying with an "interrupted system call"
     * error in the event that a signal be received while the MUD is dormant.
     * However, I think it is easier to check for an EINTR error return from
     * this select() call rather than to block and unblock signals.
     */
    do {
      errno = 0;		/* clear error condition */

      /* figure out for how long we have to sleep */
      gettimeofday(&now, (struct timezone *) 0);
      timespent = timediff(&now, &last_time);
      timeout = timediff(&opt_time, &timespent);

      /* sleep until the next 0.1 second mark */
      if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0)
	if (errno != EINTR) {
	  perror("Select sleep");
	  exit(1);
	}
    } while (errno);

    /* record the time for the next pass */
    gettimeofday(&last_time, (struct timezone *) 0);

    /* poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time)<0) {
      perror("Select poll");
      return;
    }
    /* New connection waiting for us? */
    if (FD_ISSET(mother_desc, &input_set))
      new_descriptor(mother_desc);

    /* kick out the freaky folks in the exception set */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) {
	FD_CLR(d->descriptor, &input_set);
	FD_CLR(d->descriptor, &output_set);
	close_socket(d);
      }
    }

    /* process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &input_set))
	if (process_input(d) < 0)
	  close_socket(d);
    }

    /* process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

      if ((--(d->wait) <= 0) && get_from_q(&d->input, comm, &aliased)) {
	if (d->character) {
	  d->character->char_specials.timer = 0;
/* HACKED so if a player unidles send them back */
	  if (!d->connected && GET_WAS_IN(d->character) != NOWHERE) {
	    if (d->character->in_room != NOWHERE)
	      char_from_room(d->character);
	    char_to_room(d->character, GET_WAS_IN(d->character));
	    GET_WAS_IN(d->character) = NOWHERE;
	    act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
	  } 
	}
/* end of hack */
	d->wait = 1;
	d->prompt_mode = 1;

	if (d->str)			/* writing boards, mail, etc.	*/
	  string_add(d, comm);
	else if (d->showstr_point)	/* reading something w/ pager	*/
	  show_string(d, comm);
	else if (d->connected != CON_PLAYING)	/* in menus, etc.	*/
	  nanny(d, comm);
	else {				/* else: we're playing normally */
	  if (aliased) /* to prevent recursive aliases */
	    d->prompt_mode = 0;
	  else {
	    if (perform_alias(d, comm))	/* run it through aliasing system */
	      get_from_q(&d->input, comm, &aliased);
	  }
	  command_interpreter(d->character, comm); /* send it to interpreter */
	}
      }
    }

    /* send queued output out to the operating system (ultimately to user) */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &output_set) && *(d->output))
	if (process_output(d) < 0)
	  close_socket(d);
	else
	  d->prompt_mode = 1;
    }

    /* kick out folks in the CON_CLOSE state */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (STATE(d) == CON_CLOSE)
	close_socket(d);
    }

    /* give each descriptor an appropriate prompt */
    for (d = descriptor_list; d; d = d->next) {
      if (d->prompt_mode) {
	make_prompt(d);
	d->prompt_mode = 0;
      }
    }

    /* handle heartbeat stuff */
    /* Note: pulse now changes every 0.10 seconds  */

    pulse++;

    if (!(pulse % PULSE_ZONE))
      zone_update();

    if (!(pulse % PULSE_MOBILE))
      mobile_activity();

    if (!(pulse % PULSE_VIOLENCE)) {
      perform_violence();
      point_update();
    }

    /* MOBProg foo */
    if (!(pulse % PULSE_MOBILE))
      perform_scripts();

    /* auction foo */
    if (!(pulse % (15 * PASSES_PER_SEC))) 
      auction_update(); 
    
    /* timed events */
    if (!(pulse % (60 * PASSES_PER_SEC)))  /* 1 minute */
      countdown_timer_tick();

/*
 * Since I plan to add more pulse-based functions in the future, it makes
 * sense to add a new class and function call here.  I could add the calls
 * to perform_scripts or perform_violence, both of which also poll in two
 * second intervals, but it's messy to do it that way.  This will slightly
 * increase overall cpu usage, but should be negligible.
 */

    if (!(pulse % (PULSE_ACT)))
      perform_pulse_functions();

    if (!(pulse % (PULSE_SEARCH)))
      perform_all_search();

    if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
      weather_and_time(1);
      affect_update();
      obj_update();
      quest_update();
      fflush(player_fl);
    }
    if (auto_save)
      if (!(pulse % (60 * PASSES_PER_SEC)))	/* 1 minute */
	if (++mins_since_crashsave >= autosave_time) {
	  mins_since_crashsave = 0;
	  Crash_save_all();
	  House_save_all();
	}
    if (!(pulse % (300 * PASSES_PER_SEC)))	/* 5 minutes */
      record_usage();
    if (!(pulse % (600 * PASSES_PER_SEC)))      /* 10 minutes */
      update_specials();
    if (pulse >= (30 * 60 * PASSES_PER_SEC)) {	/* 30 minutes */
      pulse = 0;
    }
    tics++;			/* tics since last checkpoint signal */
  }
}



/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
struct timeval timediff(struct timeval * a, struct timeval * b)
{
  struct timeval rslt;

  if (a->tv_sec < b->tv_sec)
    return null_time;
  else if (a->tv_sec == b->tv_sec) {
    if (a->tv_usec < b->tv_usec)
      return null_time;
    else {
      rslt.tv_sec = 0;
      rslt.tv_usec = a->tv_usec - b->tv_usec;
      return rslt;
    }
  } else { /* a->tv_sec > b->tv_sec */
    rslt.tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_usec < b->tv_usec) {
      rslt.tv_usec = a->tv_usec + 1000000 - b->tv_usec;
      rslt.tv_sec--;
    } else
      rslt.tv_usec = a->tv_usec - b->tv_usec;
    return rslt;
  }
}


void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;
  char buf[256];

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (!d->connected)
      sockets_playing++;
  }

  sprintf(buf, "nusage: %-3d sockets connected, %-3d sockets playing",
	  sockets_connected, sockets_playing);
  log(buf);

#ifdef RUSAGE
  {
    struct rusage ru;

    getrusage(0, &ru);
    sprintf(buf, "rusage: %d %d %d %d %d %d %d",
	    ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss,
	    ru.ru_ixrss, ru.ru_ismrss, ru.ru_idrss, ru.ru_isrss);
    log(buf);
  }
#endif

}



/* HACKED to add the diagnose option to the prompt */
#define GET_EXP_NEEDED(ch) \
    (titles[(int) GET_CLASS(ch)][GET_LEVEL(ch) + 1].exp - GET_EXP(ch))
#define GET_EXP_PROGRESS(ch) \
    (GET_EXP(ch) - titles[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)].exp)
#define GET_EXP_GAP(ch) \
    (titles[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch) + 1].exp - \
    titles[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)].exp)
#define DIAG(ch) diag_char_to_char_short(FIGHTING(ch))

void make_custom_prompt(struct char_data *ch, char *prompt) {
  char *src, *dest;
  extern const char *position_types[];
  
  dest = prompt;
  for (src = ch->player_specials->prompt; *src; src++) {
    if (*src == '$') {
      src++;
      switch(*src) {
        case '$':
          *dest = '$';
          dest++;
          break;
        case 'h':
          dest += sprintf(dest, "%d", GET_HIT(ch));
          break;
        case 'H':
          dest += sprintf(dest, "%d", GET_MAX_HIT(ch));
          break;
        case 'm':
          dest += sprintf(dest, "%d", GET_MANA(ch));
          break;
        case 'M':
          dest += sprintf(dest, "%d", GET_MAX_MANA(ch));
          break;
        case 'v':
          dest += sprintf(dest, "%d", GET_MOVE(ch));
          break;
        case 'V':
          dest += sprintf(dest, "%d", GET_MAX_MOVE(ch));
          break;
        case 'g':
          dest += sprintf(dest, "%d", GET_GOLD(ch));
          break;
        case 'b':
          dest += sprintf(dest, "%d", GET_BANK_GOLD(ch));
          break;
        case 'G':
          dest += sprintf(dest, "%d", GET_GOLD(ch) + GET_BANK_GOLD(ch));
          break;
        case 'x':
          dest += sprintf(dest, "%d", GET_EXP(ch));
          break;
        case 'X':
          dest += sprintf(dest, "%d", GET_EXP_NEEDED(ch));
          break;
        case 'l':
          dest += sprintf(dest, "%d", GET_LEVEL(ch));
          break;
        case 'n':
          dest += sprintf(dest, "%s", GET_NAME(ch));
          break;
        case 'a':
          dest += sprintf(dest, "%d", GET_AGE(ch));
          break;
        case 'R':
          if (GET_LEVEL(ch) < 25) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_AC(ch));
          }
          break;
        case 't':
          if (GET_LEVEL(ch) < 15) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_HITROLL(ch));
          }
          break;
        case 'd':
          if (GET_LEVEL(ch) < 15) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_DAMROLL(ch));
          }
          break;
        case 'S':
          if (GET_LEVEL(ch) <= LVL_LOWBIE) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_STR(ch));
          }
          break;
        case 'D':
          if (GET_LEVEL(ch) <= LVL_LOWBIE) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_DEX(ch));
          }
          break;
        case 'N':
          if (GET_LEVEL(ch) <= LVL_LOWBIE) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_CON(ch));
          }
          break;
        case 'I':
          if (GET_LEVEL(ch) <= LVL_LOWBIE) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_INT(ch));
          }
          break;
        case 'W':
          if (GET_LEVEL(ch) <= LVL_LOWBIE) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_WIS(ch));
          }
          break;
        case 'C':
          if (GET_LEVEL(ch) <= LVL_LOWBIE) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_CHA(ch));
          }
          break;
        case 'A':
          if (GET_LEVEL(ch) < 10) {
            dest += sprintf(dest, "?");
          } else {
            dest += sprintf(dest, "%d", GET_ALIGNMENT(ch));
          }
          break;
        case 'r':
          if (IS_IMMORT(ch)) {
            dest += sprintf(dest, "%d", world[ch->in_room].number);
          } else {
            *dest = '?';
            dest++;
          }
          break;
        case 'i':
          if (IS_IMMORT(ch)) {
            dest += sprintf(dest, "%d", GET_INVIS_LEV(ch));
          } else {
            *dest = '?';
            dest++;
          }
          break;
        case 'L':
          if (IS_IMMORT(ch)) {
            if (GET_INVIS_LEV(ch) > 0) {
              dest += sprintf(dest, "%dI ", GET_INVIS_LEV(ch));
            }
          } else {
            *dest = '?';
            dest++;
          }
          break;
        case 'p':
          dest += sprintf(dest, "%s", position_types[(int)GET_POS(ch)]);
          break;
        case 'f':
          if (FIGHTING(ch)) {
            dest += sprintf(dest, "%s ", GET_NAME(FIGHTING(ch)));
          }
          break;
        case 'c':
          if (FIGHTING(ch)) {
            dest += sprintf(dest, "(%s) ", DIAG(ch));
          }
          break;
        default:
          *dest = '?';
          dest++;
          break;
      }
    } else if (*src == '^') {
      src++;
      switch(*src) {
        case '^':
          *dest = '^';
          dest++;
          break;
        case '\\':
          *dest = '\r';
          dest++;
          *dest = '\n';
          dest++;
          break;
        case 'n':
          dest += sprintf(dest, CCNRM(ch));
          break;
        case 'r':
          dest += sprintf(dest, CCRED(ch));
          break;
        case 'g':
          dest += sprintf(dest, CCGRN(ch));
          break;
        case 'y':
          dest += sprintf(dest, CCYEL(ch));
          break;
        case 'b':
          dest += sprintf(dest, CCBLU(ch));
          break;
        case 'm':
          dest += sprintf(dest, CCMAG(ch));
          break;
        case 'c':
          dest += sprintf(dest, CCCYN(ch));
          break;
        case 'w':
          dest += sprintf(dest, CCWHT(ch));
          break;
        case 'R':
          dest += sprintf(dest, CCBRED(ch));
          break;
        case 'G':
          dest += sprintf(dest, CCBGRN(ch));
          break;
        case 'Y':
          dest += sprintf(dest, CCBYEL(ch));
          break;
        case 'B':
          dest += sprintf(dest, CCBBLU(ch));
          break;
        case 'M':
          dest += sprintf(dest, CCBMAG(ch));
          break;
        case 'C':
          dest += sprintf(dest, CCBCYN(ch));
          break;
        case 'W':
          dest += sprintf(dest, CCBWHT(ch));
          break;
        default:
          *dest = '?';
          dest++;
          break;
      }
    } else {
      *dest = *src;
      dest++;
    }
    *dest = '\0';
  }
}

void make_prompt(struct descriptor_data * d)
{

  if (d->str)
    write_to_descriptor(d->descriptor, "] ");
  else if (d->showstr_point)
    write_to_descriptor(d->descriptor,
			"\r\n*** Press return to continue, q to quit ***");
  else if (!d->connected) {
    char prompt[MAX_INPUT_LENGTH];

    *prompt = '\0';

    /* make the prompt reset to absolute normal */
    sprintf(prompt, KNRM);
    GET_COLOR_STACK_INDEX(d->character) = -1;
    fix_color_stack(d->character);

/* HACKED to improve the prompt a LOT */ 
/* Rehacked to improve it even more (custom prompts) */
    if (!IS_NPC(d->character) && d->character->player_specials->prompt) {
      make_custom_prompt(d->character, prompt);
    } else {
      /* away flag and preprompt */
      if (PRF2_FLAGGED(d->character, PRF2_AWAY)) {
        if (PRF_FLAGGED(d->character, PRF_COLORPROMPT))
          sprintf(prompt, "%s%s(AFK)%s < ", prompt, CCTRUEALERT(d->character),
              CCNRM(d->character));
        else
          sprintf(prompt, "%s(AFK) < ", prompt);
      } else {
        sprintf(prompt, "%s< ", prompt);
      }

      /* imm-invis */
      if (GET_INVIS_LEV(d->character))
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%dI ", prompt, GET_INVIS_LEV(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s(%di) ", prompt, GET_INVIS_LEV(d->character));
            break;
        }

      /* hit points */
      if (PRF_FLAGGED(d->character, PRF_DISPHP)) {
        sprintf(prompt, "%s%s", prompt, CCTRUETHERMO(d->character,
            GET_HIT(d->character), GET_MAX_HIT(d->character)));
        if (!PRF_FLAGGED(d->character, PRF_DISPMINMAX))
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%dH ", prompt, GET_HIT(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%dhp ", prompt, GET_HIT(d->character));
            break;
        }
        else
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%d/%dH ", prompt, GET_HIT(d->character),
                    GET_MAX_HIT(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%d/%dhp ", prompt, GET_HIT(d->character),
                    GET_MAX_HIT(d->character));
            break;
        }
        sprintf(prompt, "%s%s", prompt, CCNRM(d->character));
      }

      /* mana */
      if (PRF_FLAGGED(d->character, PRF_DISPMANA)) {
        sprintf(prompt, "%s%s", prompt, CCTRUETHERMO(d->character,
            GET_MANA(d->character), GET_MAX_MANA(d->character)));
        if (!PRF_FLAGGED(d->character, PRF_DISPMINMAX))
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%dM ", prompt, GET_MANA(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%dma ", prompt, GET_MANA(d->character));
            break;
        }
        else
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%d/%dM ", prompt, GET_MANA(d->character),
                    GET_MAX_MANA(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%d/%dma ", prompt, GET_MANA(d->character),
                    GET_MAX_MANA(d->character));
            break;
        }
        sprintf(prompt, "%s%s", prompt, CCNRM(d->character));
      }

      /* movement */
      if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) {
        sprintf(prompt, "%s%s", prompt, CCTRUETHERMO(d->character,
            GET_MOVE(d->character), GET_MAX_MOVE(d->character)));
        if (!PRF_FLAGGED(d->character, PRF_DISPMINMAX))
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%dV ", prompt, GET_MOVE(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%dmv ", prompt, GET_MOVE(d->character));
            break;
        }
        else
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%d/%dV ", prompt, GET_MOVE(d->character),
                    GET_MAX_MOVE(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%d/%dmv ", prompt, GET_MOVE(d->character),
                    GET_MAX_MOVE(d->character));
            break;
        }
        sprintf(prompt, "%s%s", prompt, CCNRM(d->character));
      }

      /* players gold */
      if (PRF_FLAGGED(d->character, PRF_DISPGOLD))
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%dG ", prompt, GET_GOLD(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%dgo ", prompt, GET_GOLD(d->character));
            break;
        }

      /* experience required to next level */
      if (PRF_FLAGGED(d->character, PRF_DISPEXP)) {
        sprintf(prompt, "%s%s", prompt, CCTRUETHERMO(d->character,
            GET_EXP_PROGRESS(d->character),
            GET_EXP_GAP(d->character)));
        if (GET_LEVEL(d->character) < LVL_IMMORT)
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%dX ", prompt, GET_EXP_NEEDED(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%dex ", prompt, GET_EXP_NEEDED(d->character));
            break;
        }
        else
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s%s*X ", prompt, CCNRM(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%s*ex ", prompt, CCNRM(d->character));
            break;
        }
        sprintf(prompt, "%s%s", prompt, CCNRM(d->character));
      }

      /* mob condition */
      if (PRF_FLAGGED(d->character, PRF_DISPDIAG) && FIGHTING(d->character)) {
        sprintf(prompt, "%s%s", prompt, CCTRUETHERMO(d->character,
            GET_HIT(FIGHTING(d->character)),
            GET_MAX_HIT(FIGHTING(d->character))));
        switch (GET_PROMPT_STYLE(d->character)) {
          case PROMPT_HOLO:
            sprintf(prompt, "%s/ %s ", prompt, DIAG(d->character));
            break;
          case PROMPT_MERC:
  	default:
            sprintf(prompt, "%s%s ", prompt, DIAG(d->character));
            break;
        }
        sprintf(prompt, "%s%s", prompt, CCNRM(d->character));
      }

      /* post prompt */
      sprintf(prompt, "%s>", prompt);
    }
    
    
    /* and absolute normal code again */
    sprintf(prompt, "%s%s ", prompt, CCNRM(d->character));
    GET_COLOR_STACK_INDEX(d->character) = -1;
    fix_color_stack(d->character);
 
    write_to_descriptor(d->descriptor, prompt);
  }
}



void write_to_q(char *txt, struct txt_q * queue, int aliased)
{
  struct txt_block *new;

  CREATE(new, struct txt_block, 1);
  CREATE(new->text, char, strlen(txt) + 1);
  strcpy(new->text, txt);
  new->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    new->next = NULL;
    queue->head = queue->tail = new;
  } else {
    queue->tail->next = new;
    queue->tail = new;
    new->next = NULL;
  }
}



int get_from_q(struct txt_q * queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return 0;

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return 1;
}



/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data * d)
{
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy));
}



/* special color source from Zigg */
int count_chars(const char *txt, char character)
{
  int i;
  int count = 0;
  int length = strlen(txt);

   
  for (i = 0; i < length; i++)
    if(txt[i] == character)
      count++;
   
  return count;
}



#define COLOR_CONTROL_CHAR     '^'
char *parse_color(const char *txt, struct descriptor_data *t)
{
  char *new_txt;
  char *toret;
  register int i = 0;
  int control_chars = 0;
  int j = 0;

  /* count how many control-chars there are in the string */ 
  control_chars = count_chars(txt, COLOR_CONTROL_CHAR);

  /*
   * no ansi-escape code is larger that 7 bytes so a 
   * 7 * times the number of control chars + strlen(txt) + 1
   * character big buffer would seem big enough
   * but now with extended codes, the biggest it could go:
   * normal (4 chars) + background (5 chars) + bold fore (up to 9)
   * so thats 18 chars per color code max
   * why not malloc 25 chars per color code just to be ultra safe? :))
   */
  new_txt = malloc((control_chars * 25) + strlen(txt) + 1);

  /* the parser.. Huge but fast */
  for (i = 0; txt[i] != '\0'; i++) {
    if (txt[i] == COLOR_CONTROL_CHAR) {
      i++;
       
      switch (txt[i]) {
        case '\\': strcpy(new_txt + j, "\r\n");
                   j += 2;
            break;
        /* make uppercase N be absolute normal.. drop and clear the stack */
        case 'N' :
            GET_COLOR_STACK_INDEX(t->character) = -1;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCTRUESTACK(t->character));
            if (clr(t->character))
              j += strlen(CCTRUESTACK(t->character));
            break;
        /* make lowercase n merely drop the stack a level */
        case 'n' :
            GET_COLOR_STACK_INDEX(t->character)--;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCTRUESTACK(t->character)); 
            if (clr(t->character))
              j += strlen(CCTRUESTACK(t->character));
            break;
        case 'r' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLRED;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCRED(t->character));
            if (clr(t->character))
              j += 7;
            break;
        case 'g' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLGRN;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCGRN(t->character)); 
            if (clr(t->character))
              j += 7; 
            break;
        case 'y' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLYEL;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCYEL(t->character)); 
            if (clr(t->character))
              j += 7;
            break;
        case 'b' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLBLU;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCBLU(t->character)); 
            if (clr(t->character))
              j += 7; 
            break;
        case 'm' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLMAG;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCMAG(t->character)); 
            if (clr(t->character))
              j += 7;  
            break;
        case 'c' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLCYN;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCCYN(t->character)); 
            if (clr(t->character))
              j += 7;  
            break;
        case 'w' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLWHT;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCWHT(t->character)); 
            if (clr(t->character))
              j += 7;  
            break;
        case 'R' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLBRED;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCBRED(t->character));
            if (clr(t->character))
               j += 7;
            break;
        case 'G' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLBGRN;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCBGRN(t->character));
            if (clr(t->character))
               j += 7;
            break;
        case 'Y' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLBYEL;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCBYEL(t->character));
            if (clr(t->character))
               j += 7;
            break;
        case 'B' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLBBLU;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCBBLU(t->character));
            if (clr(t->character))
               j += 7;
            break;
        case 'M' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLBMAG;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCBMAG(t->character));
            if (clr(t->character))
               j += 7;
            break;
        case 'C' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLBCYN;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCBCYN(t->character));
            if (clr(t->character))
               j += 7;
            break;
        case 'W' :
            GET_COLOR_STACK_INDEX(t->character)++;
            GET_COLOR_STACK(t->character) = KLBWHT;
            fix_color_stack(t->character);
            strcpy(new_txt + j, CCBWHT(t->character));
            if (clr(t->character))
               j += 7;
            break;
        case COLOR_CONTROL_CHAR :
               new_txt[j] = txt[i];
               j++;
            break;
        default:
               new_txt[j] = txt[i];
               j++;
            break;
      }
         
    } else {
      new_txt[j] = txt[i]; 
      j++;
    }
  }


  new_txt[j] = '\0'; /* terminate the string */
  toret = strdup(new_txt); /* create a new string with no eventual memoryloss */
  free(new_txt); /* free the old buffer */
   
  return toret; /* the colorized buffer */
}
/* end of special color source */



void write_to_output(const char *txt, struct descriptor_data * t)
{
  int size;
  char *new_txt;


  /* MOBProg foo */
  if (STATE(t) == CON_DEAF)
    return;

  /* continue like normal.. */

  new_txt = parse_color(txt, t);

  /* from here on down, using new_txt instead of txt */

  size = strlen(new_txt);

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size) {
    strcpy(t->output + t->bufptr, new_txt);
    t->bufspace -= size;
    t->bufptr += size;
  } else {			/* otherwise, try switching to a lrg buffer */
    if (t->large_outbuf || ((size + strlen(t->output)) > LARGE_BUFSIZE)) {
      /*
       * we're already using large buffer, or even the large buffer isn't big
       * enough -- switch to overflow state
       */
      t->bufptr = -1;
      buf_overflows++;
      return;
    }
    buf_switches++;

    /* if the pool has a buffer in it, grab it */
    if (bufpool != NULL) {
      t->large_outbuf = bufpool;
      bufpool = bufpool->next;
    } else {			/* else create a new one */
      CREATE(t->large_outbuf, struct txt_block, 1);
      CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
      buf_largecount++;
    }

    strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
    t->output = t->large_outbuf->text;	/* make big buffer primary */
    strcat(t->output, new_txt);	/* now add new text */

    /* calculate how much space is left in the buffer */
    t->bufspace = LARGE_BUFSIZE - 1 - strlen(t->output);

    /* set the pointer for the next write */
    t->bufptr = strlen(t->output);
  }

  free(new_txt); 
}





/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


int new_descriptor(int s)
{
  int desc, sockets_connected = 0;
  unsigned long addr;
  int i;
  static int last_desc = 0;	/* last descriptor number */
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from;
  extern char *GREETINGS;

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) & peer, &i)) < 0) {
    perror("Accept");
    return -1;
  }
  /* keep it from blocking */
  nonblock(desc);

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= avail_descs) {
    write_to_descriptor(desc, "Sorry, CircleMUD is full right now... try again later!  :-)\r\n");
    close(desc);
    return 0;
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *) newd, 0, sizeof(struct descriptor_data));

  /* find the sitename */
  if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
					 sizeof(peer.sin_addr), AF_INET))) {
    if (!nameserver_is_slow)
      perror("gethostbyaddr");
    addr = ntohl(peer.sin_addr.s_addr);
    sprintf(newd->host, "%d.%d.%d.%d", (int)((addr & 0xFF000000) >> 24),
	    (int)((addr & 0x00FF0000) >> 16), (int)((addr & 0x0000FF00) >> 8),
	    (int)((addr & 0x000000FF)));
  } else {
    strncpy(newd->host, from->h_name, HOST_LENGTH);
    *(newd->host + HOST_LENGTH) = '\0';
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    close(desc);
    sprintf(buf2, "Connection attempt denied from [%s]", newd->host);
    mudlog(buf2, CMP, LVL_GOD, TRUE);
    free(newd);
    return 0;
  }

#if 0
  /* Log new connections - probably unnecessary, but you may want it */
  sprintf(buf2, "New connection from [%s]", newd->host);
  mudlog(buf2, CMP, LVL_GOD, FALSE);
#endif

  /* initialize descriptor data */
  newd->descriptor = desc;
  newd->connected = CON_GET_NAME;
  newd->wait = 1;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->next = descriptor_list;
  newd->login_time = time(0);

  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  /* prepend to list */
  descriptor_list = newd;

  SEND_TO_Q(GREETINGS, newd);

  return 0;
}



int process_output(struct descriptor_data * t)
{
  static char i[LARGE_BUFSIZE + GARBAGE_SPACE];
  static int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n");

  /* now, append the 'real' output */
  strcpy(i + 2, t->output);

  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
    strcat(i, "**OVERFLOW**");

  /* add the extra CRLF if the person isn't in compact mode */
  if (!t->connected && t->character && !PRF_FLAGGED(t->character, PRF_COMPACT))
    strcat(i + 2, "\r\n");

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (!t->prompt_mode)		/* && !t->connected) */
    result = write_to_descriptor(t->descriptor, i);
  else
    result = write_to_descriptor(t->descriptor, i + 2);

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by) {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';

  return result;
}



int write_to_descriptor(int desc, char *txt)
{
  int total, bytes_written;

  total = strlen(txt);

  do {
    if ((bytes_written = write(desc, txt, total)) < 0) {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
	errno = EAGAIN;
#endif
      if (errno == EAGAIN)
	log("process_output: socket write would block, about to close");
      else
	perror("Write to socket");
      return -1;
    } else {
      txt += bytes_written;
      total -= bytes_written;
    }
  } while (total > 0);

  return 0;
}


/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(struct descriptor_data * t)
{
  int buf_length, bytes_read, space_left, failed_subst;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH + 8];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;

  do {
    if ((bytes_read = read(t->descriptor, read_point,
			   MAX_RAW_INPUT_LENGTH - buf_length - 1)) < 0) {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
	errno = EAGAIN;
#endif
      if (errno != EAGAIN) {
	perror("process_input: about to lose connection");
	return -1;		/* some error condition was encountered on
				 * read */
      } else
	return 0;		/* the read would have blocked: just means no
				 * data there */
    } else if (bytes_read == 0) {
      log("EOF on socket read (connection broken by peer)");
      return -1;
    }
    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0';	/* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;

    read_point += bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
#if !defined(POSIX_NONBLOCK_BROKEN)
  } while (nl_pos == NULL);
#else
  } while (0);

  if (nl_pos == NULL)
    return 0;
#endif

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

    for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b') {	/* handle backspacing */
	if (write_point > tmp) {
	  if (*(--write_point) == '$') {
	    write_point--;
	    space_left += 2;
	  } else
	    space_left++;
	}
      } else if (isascii(*ptr) && isprint(*ptr)) {
	if ((*(write_point++) = *ptr) == '$') {	/* copy one character */
	  *(write_point++) = '$';	/* if it's a $, double it */
	  space_left -= 2;
	} else
	  space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
	return -1;
    }
    if (t->snoop_by) {
      SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(tmp, t->snoop_by);
      SEND_TO_Q("\r\n", t->snoop_by);
    }
    failed_subst = 0;

    if (*tmp == '!')
      strcpy(tmp, t->last_input);
/* HACKED dont perform substitution with ^
    else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	strcpy(t->last_input, tmp);
    }
*/
    else
      strcpy(t->last_input, tmp);

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return 1;
}



/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data * t, char *orig, char *subst)
{
  char new[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }

  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }

  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(new, orig, (strpos - orig));
  new[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(new, second, (MAX_INPUT_LENGTH - strlen(new) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(new, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(new) - 1));

  /* terminate the string in case of an overflow from strncat */
  new[MAX_INPUT_LENGTH-1] = '\0';
  strcpy(subst, new);

  return 0;
}

void clanlog(char *str, struct char_data * ch);

void close_socket(struct descriptor_data * d)
{
  void extract_pet(struct char_data *pet);
  
  struct descriptor_data *temp;
  char buf[100];

  close(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
    d->snoop_by->snooping = NULL;
  }

/* HACKED to add in oasis-olc */
 /* Kill any OLC stuff */
 switch(d->connected) {
   case CON_OEDIT:
   case CON_REDIT:
   case CON_ZEDIT:
   case CON_MEDIT:
   case CON_SEDIT:
       cleanup_olc(d, CLEANUP_ALL);
   default:
       break;
 }
/* end of hack */

  if (d->character) {
    if (d->connected == CON_PLAYING) {
      save_char(d->character, NOWHERE);
      act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
/* HACKED to add clanlog */
      sprintf(buf, "Closing link to: %s.", GET_NAME(d->character));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
/* End of hack */
/* PETS */
      if (HAS_PET(d)) {
        GET_OWNER_DESC(GET_PET(d)) = NULL;
        GET_PET(d) = NULL; /* unneccesary, the descriptor is destroyed later */
        mudlog("Keeping pet.", NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
      }
/* END of PETS */
      d->character->desc = NULL;
    } else {
      sprintf(buf, "Losing player: %s.",
	      GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
      mudlog(buf, CMP, LVL_IMMORT, TRUE);
/* PETS */
      if (HAS_PET(d)) {
        /* Hmm apparantly we're NOT playing, but we HAVEN'T rented...
           probably died. Well, we're gonna lose our pet. */
        /* on second thought, make pet go away */
/*        GET_OWNER_DESC(GET_PET(d)) = NULL;
        GET_PET(d) = NULL;
        if (d->character) GET_PET(d->character) = NULL;
        GET_OWNER(GET_PET(d)) = NULL;  */
        extract_pet(GET_PET(d));
        mudlog("Losing pet also.", CMP, LVL_IMMORT, TRUE);
      }
/* END of PETS */
      free_char(d->character);
    }
  } else
    mudlog("Losing descriptor without char.", CMP, LVL_IMMORT, TRUE);

  /* JE 2/22/95 -- part of my enending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

  REMOVE_FROM_LIST(d, descriptor_list, next);

  if (d->showstr_head)
    free(d->showstr_head);
  free(d);
}

/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(int s)
{
  int flags;

  flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}


/* ******************************************************************
*  signal-handling functions (formerly signals.c)                   *
****************************************************************** */


void checkpointing()
{
  if (!tics) {
    log("SYSERR: CHECKPOINT shutdown: tics not updated");
    abort();
  } else
    tics = 0;
}


void reread_wizlists()
{
  void reboot_wizlists(void);

  mudlog("Rereading wizlists.", CMP, LVL_IMMORT, FALSE);
  reboot_wizlists();
}


void unrestrict_game()
{
  extern struct ban_list_element *ban_list;
  extern int num_invalid;

  mudlog("Received SIGUSR2 - completely unrestricting game (emergent)",
	 BRF, LVL_IMMORT, TRUE);
  ban_list = NULL;
  restrict = 0;
  num_invalid = 0;
}


void hupsig()
{
  log("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  exit(0);			/* perhaps something more elegant should
				 * substituted */
}


/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted because BSD systems do not restart
 * select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 */

#if defined(NeXT)
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

  if (sigaction(signo, &act, &oact) < 0)
    return SIG_ERR;

  return oact.sa_handler;
}
#endif /* NeXT */

void signal_setup(void)
{
/* HACKED to disable SIGVTALRM
  struct itimerval itime;
  struct timeval interval;
*/

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  my_signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes
   */
/* HACKED to disable SIGVTALRM
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);
*/

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
}



/* ****************************************************************
*       Public routines for system-to-player-communication        *
*******************************************************************/

void send_to_char(char *messg, struct char_data * ch)
{
  if (ch->desc && messg)
    SEND_TO_Q(messg, ch->desc);
}


void send_to_all(char *messg)
{
  struct descriptor_data *i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected)
	SEND_TO_Q(messg, i);
}


void send_to_outdoor(char *messg)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character && AWAKE(i->character) &&
	OUTSIDE(i->character))
      SEND_TO_Q(messg, i);
}



void send_to_room(char *messg, int room)
{
  struct char_data *i;

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if (i->desc)
	SEND_TO_Q(messg, i->desc);
}



char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(char *orig, struct char_data * ch, struct obj_data * obj,
		      void *vict_obj, struct char_data * to)
{
  register char *i, *buf;
  static char lbuf[MAX_STRING_LENGTH];

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'n':
	i = PERS(ch, to);
	break;
      case 'N':
	CHECK_NULL(vict_obj, PERS((struct char_data *) vict_obj, to));
	break;
      case 'm':
	i = HMHR(ch);
	break;
      case 'M':
	CHECK_NULL(vict_obj, HMHR((struct char_data *) vict_obj));
	break;
      case 's':
	i = HSHR(ch);
	break;
      case 'S':
	CHECK_NULL(vict_obj, HSHR((struct char_data *) vict_obj));
	break;
      case 'e':
	i = HSSH(ch);
	break;
      case 'E':
	CHECK_NULL(vict_obj, HSSH((struct char_data *) vict_obj));
	break;
      case 'o':
	CHECK_NULL(obj, OBJN(obj, to));
	break;
      case 'O':
	CHECK_NULL(vict_obj, OBJN((struct obj_data *) vict_obj, to));
	break;
      case 'p':
	CHECK_NULL(obj, OBJS(obj, to));
	break;
      case 'P':
	CHECK_NULL(vict_obj, OBJS((struct obj_data *) vict_obj, to));
	break;
      case 'a':
	CHECK_NULL(obj, SANA(obj));
	break;
      case 'A':
	CHECK_NULL(vict_obj, SANA((struct obj_data *) vict_obj));
	break;
      case 'T':
	CHECK_NULL(vict_obj, (char *) vict_obj);
	break;
      case 'F':
	CHECK_NULL(vict_obj, fname((char *) vict_obj));
	break;
      case '$':
	i = "$";
	break;
      default:
	log("SYSERR: Illegal $-code to act():");
	strcpy(buf1, "SYSERR: ");
	strcat(buf1, orig);
	log(buf1);
	break;
      }
      while ((*buf = *(i++)))
	buf++;
      orig++;
    } else if (!(*(buf++) = *(orig++)))
      break;
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

/* HACKED to add mobprog code */
/*
  this is older code, disabled, changed the act_trigger call
  to use lbuf instead of just buf, hoping that that would work,
  it didnt work, now all this code is completely disabled
  the problem is that mob_act_triggers are going off all the time
*/
  if (to->desc) {
    SEND_TO_Q(CAP(lbuf), to->desc);
/*
    if (MOBTrigger)
      mprog_act_trigger(lbuf, to, ch, obj, vict_obj);
*/
  }
  MOBTrigger = TRUE;
/*
  this is supposedly new mobprog code, but it looks tooooooo
  buggy, something seems to make this check far too often, lagging
  the mud
*/
/*
  if (to->desc)
    SEND_TO_Q(CAP(lbuf), to->desc);
  if (MOBTrigger)
    mprog_act_trigger(buf, to, ch, obj, vict_obj);
*/
/* end of hack */
}


#define SENDOK(ch) ((AWAKE(ch) || sleep) && \
		    !PLR_FLAGGED((ch), PLR_WRITING))

void act(char *str, int hide_invisible, struct char_data * ch,
	      struct obj_data * obj, void *vict_obj, int type)
{
  struct char_data *to;
  static int sleep;

/*  if (!str || !*str) {
    MOBTrigger = TRUE;
    return;
  }*/
  if (!str) {
    MOBTrigger = TRUE;
    return;
  }
  if (!*str) {
    MOBTrigger = TRUE;
    return;
  }

  /*
   * Warning: the following TO_SLEEP code is a hack.
   * 
   * I wanted to be able to tell act to deliver a message regardless of sleep
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
   * command.  It's not legal to combine TO_x's with each other otherwise.
   */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((sleep = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;


  if (type == TO_CHAR) {
    if (ch && SENDOK(ch))
      perform_act(str, ch, obj, vict_obj, ch);
    MOBTrigger = TRUE;
    return;
  }
  if (type == TO_VICT) {
    if ((to = (struct char_data *) vict_obj) && SENDOK(to))
      perform_act(str, ch, obj, vict_obj, to);
    MOBTrigger = TRUE;
    return;
  }

  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */
  /* or TO_BATTLE */
  if (ch && ch->in_room != NOWHERE)
    to = world[ch->in_room].people;
  else if (obj && obj->in_room != NOWHERE)
    to = world[obj->in_room].people;
  else {
    log("SYSERR: no valid target to act()!");
    sprintf(buf, "SYSERR: ...'%s'", str);
    log(buf);
    return;
  }

  for (; to; to = to->next_in_room)
    if (SENDOK(to) && !(hide_invisible && ch && !CAN_SEE(to, ch)) &&
      (to != ch) && (type == TO_ROOM || (to != vict_obj))) {
          if (type != TO_BATTLE || !PRF2_FLAGGED(to, PRF2_BATTLEBRIEF))
          perform_act(str, ch, obj, vict_obj, to);
      }
  MOBTrigger = TRUE;
}
