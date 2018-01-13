/* ************************************************************************
*   File: screen.h                                      Part of CircleMUD *
*  Usage: header file with ANSI color codes for online color              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define KNRM	"\x1B[0m"
#define KRED	"\x1B[0;31m"
#define KGRN	"\x1B[0;32m"
#define KYEL	"\x1B[0;33m"
#define KBLU	"\x1B[0;34m"
#define KMAG	"\x1B[0;35m"
#define KCYN	"\x1B[0;36m"
#define KWHT	"\x1B[0;37m"
#define KB	"\x1B[1m"
#define KBRED	"\x1B[1;31m"
#define KBGRN	"\x1B[1;32m"
#define KBYEL	"\x1B[1;33m"
#define KBBLU	"\x1B[1;34m"
#define KBMAG	"\x1B[1;35m"
#define KBCYN	"\x1B[1;36m"
#define KBWHT	"\x1B[1;37m"
#define KNUL	""

/* these are for the parse_color color system ... no turning off previous
  codes, use the normal code for that, turn on bold with the special
  bold code */
#define KFORERED    "\x1B[31m"
#define KFOREGRN    "\x1B[32m"
#define KFOREYEL    "\x1B[33m"
#define KFOREBLU    "\x1B[34m"
#define KFOREMAG    "\x1B[35m"
#define KFORECYN    "\x1B[36m"
#define KFOREWHT    "\x1B[37m"
#define KBACKRED    "\x1B[41m"
#define KBACKGRN    "\x1B[42m"
#define KBACKYEL    "\x1B[43m"
#define KBACKBLU    "\x1B[44m"
#define KBACKMAG    "\x1B[45m"
#define KBACKCYN    "\x1B[46m"
#define KBACKWHT    "\x1B[47m"

/* these are the 4-bit nibble codes for colors set using the color
  command, not parse color, and saved to the playerfile */
#define KLNRM	0
#define KLRED	1
#define KLGRN	2
#define KLYEL	3
#define KLBLU	4
#define KLMAG	5
#define KLCYN	6
#define KLWHT	7
#define KLB	8
#define KLBRED	9
#define KLBGRN	10
#define KLBYEL	11
#define KLBBLU	12
#define KLBMAG	13
#define KLBCYN	14
#define KLBWHT	15

#define clr(ch) (PRF_FLAGGED((ch), PRF_COLOR) ? 1 : 0)

#define CCNRM(ch) (clr((ch))?KNRM:KNUL)
#define CCRED(ch) (clr((ch))?KRED:KNUL)
#define CCGRN(ch) (clr((ch))?KGRN:KNUL)
#define CCYEL(ch) (clr((ch))?KYEL:KNUL)
#define CCBLU(ch) (clr((ch))?KBLU:KNUL)
#define CCMAG(ch) (clr((ch))?KMAG:KNUL)
#define CCCYN(ch) (clr((ch))?KCYN:KNUL)
#define CCWHT(ch) (clr((ch))?KWHT:KNUL)
#define CCB(ch)   (clr((ch))?KB:KNUL)
#define CCBRED(ch) (clr((ch))?KBRED:KNUL)
#define CCBGRN(ch) (clr((ch))?KBGRN:KNUL)
#define CCBYEL(ch) (clr((ch))?KBYEL:KNUL)
#define CCBBLU(ch) (clr((ch))?KBBLU:KNUL)
#define CCBMAG(ch) (clr((ch))?KBMAG:KNUL)
#define CCBCYN(ch) (clr((ch))?KBCYN:KNUL)
#define CCBWHT(ch) (clr((ch))?KBWHT:KNUL)
#define CCSTACK(ch) (clr((ch))?color_codes[GET_COLOR_STACK((ch))]:KNUL)
#define CCTRUESTACK(ch) (clr((ch))?true_color_codes[GET_COLOR_STACK((ch))]:KNUL)

/* more parse_color defines */
#define CCFORERED(ch) (clr((ch))?KFORERED:KNUL)
#define CCFOREGRN(ch) (clr((ch))?KFOREGRN:KNUL)
#define CCFOREYEL(ch) (clr((ch))?KFOREYEL:KNUL)
#define CCFOREBLU(ch) (clr((ch))?KFOREBLU:KNUL)
#define CCFOREMAG(ch) (clr((ch))?KFOREMAG:KNUL)
#define CCFORECYN(ch) (clr((ch))?KFORECYN:KNUL)
#define CCFOREWHT(ch) (clr((ch))?KFOREWHT:KNUL)
#define CCBACKRED(ch) (clr((ch))?KBACKRED:KNUL)
#define CCBACKGRN(ch) (clr((ch))?KBACKGRN:KNUL)
#define CCBACKYEL(ch) (clr((ch))?KBACKYEL:KNUL)
#define CCBACKBLU(ch) (clr((ch))?KBACKBLU:KNUL)
#define CCBACKMAG(ch) (clr((ch))?KBACKMAG:KNUL)
#define CCBACKCYN(ch) (clr((ch))?KBACKCYN:KNUL)
#define CCBACKWHT(ch) (clr((ch))?KBACKWHT:KNUL)


/* note, color_prefs has a max of 40 ints, change in structs.h if you need
  to, but be careful. */
#define CCWARNING(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[0])]:KNUL)
#define CCALERT(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[1])]:KNUL)
#define CCINFO(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[2])]:KNUL)
#define CCHOLLER(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[3])]:KNUL)
#define CCSHOUT(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[4])]:KNUL)
#define CCGOSSIP(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[5])]:KNUL)
#define CCAUCTION(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[6])]:KNUL)
#define CCGRATZ(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[7])]:KNUL)
#define CCTELL(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[8])]:KNUL)
#define CCPAGE(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[9])]:KNUL)
#define CCGSAY(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[10])]:KNUL)
#define CCQSAY(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[11])]:KNUL)
#define CCCLANSAY(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[12])]:KNUL)
#define CCROOMNAME(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[13])]:KNUL)
#define CCROOMDESC(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[14])]:KNUL)
#define CCOBJECTS(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[15])]:KNUL)
#define CCPLAYERS(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[16])]:KNUL)
#define CCGODS(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[17])]:KNUL)
#define CCEXITS(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[18])]:KNUL)
#define CCRUNES(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[19])]:KNUL)
#define CCBLADE(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[20])]:KNUL)
#define CCQUILLIONS(ch) (clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[21])]:KNUL)
#define CCJEWELS(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[22])]:KNUL)
#define CCHILT(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[23])]:KNUL)
#define CCPOMMEL(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[24])]:KNUL)
#define CCWIZNET(ch)	(clr((ch))?color_codes[((ch)->player_specials->saved.color_prefs[25])]:KNUL)

#define CCTRUEWARNING(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[0])]:KNUL)
#define CCTRUEALERT(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[1])]:KNUL)
#define CCTRUEINFO(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[2])]:KNUL)
#define CCTRUEHOLLER(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[3])]:KNUL)
#define CCTRUESHOUT(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[4])]:KNUL)
#define CCTRUEGOSSIP(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[5])]:KNUL)
#define CCTRUEAUCTION(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[6])]:KNUL)
#define CCTRUEGRATZ(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[7])]:KNUL)
#define CCTRUETELL(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[8])]:KNUL)
#define CCTRUEPAGE(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[9])]:KNUL)
#define CCTRUEGSAY(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[10])]:KNUL)
#define CCTRUEQSAY(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[11])]:KNUL)
#define CCTRUECLANSAY(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[12])]:KNUL)
#define CCTRUEROOMNAME(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[13])]:KNUL)
#define CCTRUEROOMDESC(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[14])]:KNUL)
#define CCTRUEOBJECTS(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[15])]:KNUL)
#define CCTRUEPLAYERS(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[16])]:KNUL)
#define CCTRUEGODS(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[17])]:KNUL)
#define CCTRUEEXITS(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[18])]:KNUL)
#define CCTRUERUNES(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[19])]:KNUL)
#define CCTRUEBLADE(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[20])]:KNUL)
#define CCTRUEQUILLIONS(ch) (clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[21])]:KNUL)
#define CCTRUEJEWELS(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[22])]:KNUL)
#define CCTRUEHILT(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[23])]:KNUL)
#define CCTRUEPOMMEL(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[24])]:KNUL)
#define CCTRUEWIZNET(ch)	(clr((ch))?true_color_codes[((ch)->player_specials->saved.color_prefs[25])]:KNUL)

/* a special define for a dashboard effect:
  takes two values: a current and a max, and returns
  CCNRM if they are equal or if current is > than max,
  CCINFO if current is > than 1/2 max,
  CCALERT if current is > than 1/4 max,
  otherwise CCWARNING!!! */
#define CCTHERMO(ch, current, max) \
  (!PRF_FLAGGED(ch, PRF_COLORPROMPT) ? CCNRM(ch) : \
  (current) >= ((max)) ? CCNRM(ch) : \
  (current) > ((max) / 2) ? CCINFO(ch) : \
  (current) > ((max) / 4) ? CCALERT(ch) : \
   CCWARNING(ch))

#define CCTRUETHERMO(ch, current, max) \
  (!PRF_FLAGGED(ch, PRF_COLORPROMPT) ? CCNRM(ch) : \
  (current) >= ((max)) ? CCNRM(ch) : \
  (current) > ((max) / 2) ? CCTRUEINFO(ch) : \
  (current) > ((max) / 4) ? CCTRUEALERT(ch) : \
   CCTRUEWARNING(ch))

/*
 *  normal_color stores the value (default is 0, or KLNRM) to reset
 *  a characters color values to.  It's very temporary, has to be set
 *  by parse_color in comm.c
 *  It works by storing the previous color value used.
 *  Hence is a 'normal'
 *  Yikes this is a wierd pair.
 */
#define GET_COLOR_STACK(ch)		((ch)->char_specials.color_stack[(ch)->char_specials.color_stack_index])
#define GET_COLOR_STACK_INDEX(ch)	((ch)->char_specials.color_stack_index)



/* function prototypes */
void fix_color_stack(struct char_data * ch);
void strip_color(char *instr);
