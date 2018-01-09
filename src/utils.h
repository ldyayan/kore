/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* external declarations and prototypes **********************************/

#include "circle.h"

extern struct weather_data weather_info;

#define log basic_mud_log

/* public functions in utils.c */
void	basic_mud_log(char *str);
int	touch(char *path);
void	mudlog(char *str, char type, sbyte level, byte file);
void    arealog(char *str, char type, sbyte level, byte file, int area);
void	log_death_trap(struct char_data *ch);
int	number(int from, int to);
int	dice(int number, int size);
/* HACKED to show all 32 options */
void	sprintbit(unsigned long vektor, char *names[], char *result);
/* end of hack */
void	sprinttype(int type, char *names[], char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *orig_name, char *filename, int mode);
struct time_info_data age(struct char_data *ch);

/*
 * Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists.
 */
#ifndef str_cmp
int str_cmp(const char *arg1, const char *arg2);
#endif
#ifndef strn_cmp
int strn_cmp(const char *arg1, const char *arg2, int n);
#endif

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);

#define UMIN(a, b)              ((a) < (b) ? (a) : (b))
#define UMAX(a, b)              ((a) > (b) ? (a) : (b))

/* in magic.c */
bool	circle_follow(struct char_data *ch, struct char_data * victim);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in limits.c */
int	mana_limit(struct char_data *ch);
int	hit_limit(struct char_data *ch);
int	move_limit(struct char_data *ch);
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	advance_level(struct char_data *ch);
void	demote_level(struct char_data *ch);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	obj_update(void);
void	update_pos(struct char_data *victim);


/* various constants *****************************************************/


/* defines for mudlog() */
#define OFF	0
#define ON	1
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE		0
#define ETEXT_FILE		1
#define ASCII_PLAYER_FILE	2
#define AUX_PLAYER_FILE		3
#define STOREROOM_FILE		4
/* PETS */
#define PET_FILE		5
/* END of PETS */
#define ASCII_PLAYER_DATA	6

/* breadth-first searching */
#define BFS_ERROR		-1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH		-3

/* mud-life time */
#define SECS_PER_MUD_HOUR	75
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(17*SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 
#define IF_STR(st) ((st) ? (st) : "\0")
#define CAP(st)  (*(st) = UPPER(*(st)), st)

#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")


/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
		{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure"); abort(); } } while(0)

/* the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))
#define BIT(bit)  (1 << (bit))

#define MOB_FLAGS(ch) ((ch)->char_specials.saved.act)
#define PLR_FLAGS(ch) ((ch)->char_specials.saved.act)
#define PRF_FLAGS(ch) ((ch)->player_specials->saved.pref)
#define PRF2_FLAGS(ch) ((ch)->player_specials->saved.pref2)
#define AFF_FLAGS(ch) ((ch)->char_specials.saved.affected_by)
#define AFF2_FLAGS(ch) ((ch)->char_specials.saved.affected_by2)
#define ROOM_FLAGS(loc) (world[(loc)].room_flags)

#define IS_NPC(ch)  (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)  (IS_NPC(ch) && ((ch)->nr >-1))

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET(AFF_FLAGS(ch), (flag)))
#define AFF2_FLAGGED(ch, flag) (IS_SET(AFF2_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET(PRF_FLAGS(ch), (flag)))
#define PRF2_FLAGGED(ch, flag) (IS_SET(PRF2_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS(loc), (flag)))
#define ZONE_FLAGGED(loc, flag) (IS_SET(zone_table[world[(loc)].zone].zone_flags, (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

#define IS_AFFECTED2(ch, skill) (AFF2_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define PRF2_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF2_FLAGS(ch), (flag))) & (flag))


/* room utils ************************************************************/


#define IS_DARK(room)  ( !world[room].light && \
                         (IS_SET(world[room].room_flags, ROOM_DARK) || \
                          ( ( world[room].sector_type != SECT_INSIDE && \
                              world[room].sector_type != SECT_CITY ) && \
                            (weather_info.sunlight == SUN_SET || \
			     weather_info.sunlight == SUN_DARK)) ) )

#define IS_LIGHT(room)  (!IS_DARK(room))

#define GET_ROOM_SPEC(room) ((room) >= 0 ? world[(room)].func : NULL)

#define IS_CHAOS_ROOM(room)  (ROOM_FLAGGED(room, ROOM_CHAOS) || \
                         ZONE_FLAGGED(room, ZONE_CHAOS) )

/* char utils ************************************************************/


#define IS_QUESTOR(ch)  (IS_AFFECTED2(ch, AFF2_QUESTOR))
#define IN_ROOM(ch)	((ch)->in_room)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch).year)

#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->player.short_descr : (ch)->player.name)
#define GET_REAL_TITLE(ch)   ((ch)->player.title)
#define HAS_PRETITLE(ch) (IS_NPC(ch) ? FALSE : strchr(GET_REAL_TITLE(ch), PRETITLE_SEP_CHAR) ? TRUE : FALSE )
#define GET_TITLE(ch) (HAS_PRETITLE(ch) ? \
                       strchr(GET_REAL_TITLE(ch), PRETITLE_SEP_CHAR) + 1 : \
                       GET_REAL_TITLE(ch))
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_PASSWD(ch)	((ch)->player.passwd)
#define GET_RECALL(ch)  ((ch)->player.recallpoint)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GET_PROMPT_STYLE(ch) (PRF_FLAGGED((ch), PRF_MERCPROMPT))

#define GET_MOB_RACE(ch)((ch)->mob_specials.race)
#define GET_PC_RACE(ch)	((ch)->player_specials->saved.race)
#define GET_RACE(ch)	(IS_NPC(ch) ? GET_MOB_RACE(ch) : GET_PC_RACE(ch))
#define GET_ARCHFOE(ch) ((ch)->player_specials->saved.archfoe)
#define GET_BESTKILL(ch) ((ch)->player_specials->saved.bestkill)
#define GET_ARCHFOERANK(ch) ((ch)->player_specials->saved.archfoerank)
#define GET_BESTKILLRANK(ch) ((ch)->player_specials->saved.bestkillrank)
#define GET_CLAN(ch)    ((ch)->player_specials->saved.clan)
#define	GET_CLAN_LEVEL(ch) ((ch)->player_specials->saved.clan_level)
#define GET_QUEST(ch)	((ch)->player_specials->saved.quest_enroll)
#define GET_IMAGES(ch)  ((ch)->char_specials.num_images)
#define GET_TEMPBESTKILL(ch) ((ch)->char_specials.tempbestkill)
#define GET_TEMPBESTKILLRANK(ch) ((ch)->char_specials.tempbestkillrank)
#define GET_TEMPARCHFOE(ch) ((ch)->char_specials.temparchfoe)
#define GET_TEMPARCHFOERANK(ch) ((ch)->char_specials.temparchfoerank)
#define GET_LAYERS(ch)  ((ch)->char_specials.num_layers)
#define GET_SPECIAL(ch) ((ch)->char_specials.num_specials)
#define GET_CLASS(ch)   ((ch)->player.class)
#define GET_HOME(ch)	((ch)->player.hometown)
#define GET_HEIGHT(ch)	((ch)->player.height)
#define GET_WEIGHT(ch)	((ch)->player.weight)
#define GET_SEX(ch)	((ch)->player.sex)

#define GET_STR(ch)     ((ch)->aff_abils.str)
#define GET_ADD(ch)     ((ch)->aff_abils.str_add)
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_EXP(ch)	  ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_HIT(ch)	  ((ch)->points.hit)
#define GET_MAX_HIT(ch)	  ((ch)->points.max_hit)
#define GET_MOVE(ch)	  ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)	  ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_GOLD(ch)	  ((ch)->points.gold)
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
#define GET_HITROLL(ch)	  ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)

#define GET_POS(ch)	  ((ch)->char_specials.position)
#define GET_BLOCKED(ch)   ((ch)->char_specials.blocked)
#define GET_CANT_WIMPY(ch) ((ch)->char_specials.cant_wimpy)
#define GET_IDNUM(ch)	  ((ch)->char_specials.saved.idnum)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define FIGHTING(ch)	  ((ch)->char_specials.fighting)
#define HUNTING(ch)	  ((ch)->char_specials.hunting)
#define GET_SAVE(ch, i)	  ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

#define GET_COND(ch, i)		((ch)->player_specials->saved.conditions[(i)])
#define GET_LOADROOM(ch)	((ch)->player_specials->saved.load_room)
#define GET_PRACTICES(ch)	((ch)->player_specials->saved.spells_to_learn)
#define GET_INVIS_LEV(ch)	((ch)->player_specials->saved.invis_level)
#define GET_WIMP_LEV(ch)	((ch)->player_specials->saved.wimp_level)
#define GET_FREEZE_LEV(ch)	((ch)->player_specials->saved.freeze_level)
#define GET_BAD_PWS(ch)		((ch)->player_specials->saved.bad_pws)
#define GET_TALK(ch, i)		((ch)->player_specials->saved.talks[i])
#define POOFIN(ch)		((ch)->player_specials->poofin)
#define POOFOUT(ch)		((ch)->player_specials->poofout)
#define GET_LAST_OLC_TARG(ch)	((ch)->player_specials->last_olc_targ)
#define GET_LAST_OLC_MODE(ch)	((ch)->player_specials->last_olc_mode)
#define GET_ALIASES(ch)		((ch)->player_specials->aliases)
#define GET_LAST_TELL(ch)	((ch)->player_specials->last_tell)
#define GET_KILLER_TO_FORGIVE(ch) ((ch)->player_specials->killer_to_forgive)

/* HACKED to remove the practices */
/*
#define GET_SKILL(ch, i)	((ch)->player_specials->saved.skills[i])
#define SET_SKILL(ch, i, pct)	{ (ch)->player_specials->saved.skills[i] = pct; }
*/
/* old define, replaced below
#define GET_SKILL(ch, i)	(95)
*/
#define GET_SKILL(ch, i)	(((GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)]) || IS_NPC(ch)) ? 95 : 0)
#define SET_SKILL(ch, i, pct)	{ /* empty */ }
/* end of hack */

#define GET_MOB_SPEC(ch) (IS_MOB(ch) ? (mob_index[(ch->nr)].func) : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].virtual : -1)

#define MEMORY(ch)		((ch)->mob_specials.memory)
#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define GET_MOBFORMAT(ch)	((ch)->mob_specials.mobformat)

#define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch)==0) || (GET_STR(ch) != 18)) ? MIN(GET_STR(ch), 25) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        )

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
#define CAN_CARRY_N(ch) (5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define HAS_LIGHT(ch) (IS_THRIKREEN(ch) ? ch->equipment[THRI_WEAR_LIGHT] : ch->equipment[WEAR_LIGHT] )
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || PRF_FLAGGED(ch, PRF_HOLYLIGHT) || \
    HAS_LIGHT(ch))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

#define GET_NUM_WEARS(ch) (IS_THRIKREEN(ch)? NUM_THRI_WEARS : NUM_WEARS)
#define GET_WEAR_WIELD(ch) (IS_THRIKREEN(ch)? THRI_WEAR_WIELD_R : WEAR_WIELD)
#define GET_WEAR_WIELD_2(ch) \
  (IS_THRIKREEN(ch)? THRI_WEAR_WIELD_L : WEAR_WIELD_2)
#define GET_WEAR_HOLD(ch) (IS_THRIKREEN(ch)? THRI_WEAR_HOLD_R : WEAR_HOLD)
#define GET_WEAR_LIGHT(ch) (IS_THRIKREEN(ch)? THRI_WEAR_LIGHT : WEAR_LIGHT)

/* PETS */
/* GET_PET works on both char_data and descriptor_data! */
#define GET_PET(ch)		(ch->pet)
#define GET_OWNER(pet)		(pet->owner)
#define GET_OWNER_DESC(pet)	(pet->ownerd)
#define HAS_PET(ch)		(GET_PET(ch) != NULL)
#define IS_PET(pet)		((GET_OWNER(pet) != NULL) || (GET_OWNER_DESC(pet) != NULL))
#define GET_LOYALTY(pet)	(pet->petdata->loyalty)
#define GET_PET_SKILL(pet, num)	(pet->petdata->skills[num])
#define GET_PETNUM(pet)		(pet->petdata->petnum)
#define IS_MOUNTED(pet)		(pet->petdata->mounted)
/* END of PETS */

/* descriptor-based utils ************************************************/


#define WAIT_STATE(ch, cycle) { if ((ch)->desc) (ch)->desc->wait = (cycle); }
#define CHECK_WAIT(ch)	(((ch)->desc) ? ((ch)->desc->wait > 1) : 0)
#define STATE(d)	((d)->connected)


/* object utils **********************************************************/


#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->obj_flags.extra_flags)
#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.timer)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(GET_OBJ_RNUM(obj) >= 0 ? \
				 obj_index[GET_OBJ_RNUM(obj)].virtual : -1)
#define GET_OBJ_COUNT(obj)	((obj)->count)

#define IS_OBJ_STAT(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags,stat))

#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
	(obj_index[(obj)->item_number].func) : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))


/* compound utilities and other macros **********************************/


#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!IS_AFFECTED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || IS_AFFECTED((sub), AFF_INFRAVISION)))

#define INVIS_OK(sub, obj) \
 ((!IS_AFFECTED((obj),AFF_INVISIBLE) || IS_AFFECTED(sub,AFF_DETECT_INVIS)) && \
 (!MOB_FLAGGED((obj), MOB_NOTTHERE)) && \
 (!IS_AFFECTED((obj), AFF_HIDE) || IS_AFFECTED(sub, AFF_SENSE_LIFE)))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || PRF_FLAGGED(sub, PRF_HOLYLIGHT))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= GET_INVIS_LEV(obj)) && IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || IS_AFFECTED((sub), AFF_DETECT_INVIS))

#define MORT_CAN_SEE_OBJ(sub, obj) (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || PRF_FLAGGED((sub), PRF_HOLYLIGHT) || \
   IS_OBJ_STAT(obj, ITEM_GLOW))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))


#define PERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_NAME(ch) : "someone")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])

#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))


#define RACE_ABBR(ch) ((GET_RACE(ch) == RACE_UNDEFINED) ? \
                             "--    " : race_abbrevs[(int)GET_RACE(ch)])

#define IS_HUMAN(ch)            (!IS_NPC(ch) && \
                                (GET_RACE(ch) == RACE_HUMAN))
#define IS_ELF(ch)              (!IS_NPC(ch) && \
                                (GET_RACE(ch) == RACE_ELF))
#define IS_HOBBIT(ch)           (!IS_NPC(ch) && \
                                (GET_RACE(ch) == RACE_HOBBIT))
#define IS_DWARF(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_DWARF))
#define IS_ORC(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_ORC))
#define IS_DROW(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_DROW))
#define IS_BUGBEAR(ch)          (!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_BUGBEAR))
#define IS_MINOTAUR(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_MINOTAUR))
#define IS_TROLL(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_TROLL))
#define IS_GIANT(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_GIANT))
#define IS_DRAGON(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_DRAGON))
#define IS_UNDEAD(ch)		(!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_UNDEAD))
#define IS_THRIKREEN(ch)        (!IS_NPC(ch) && \
				(GET_RACE(ch) == RACE_THRIKREEN))

#define CAN_CAST(ch)		(!ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC) && \
				!ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
				



/* HACKED to add immort abbreviations */
/*
#define CLASS_ABBR(ch) (IS_NPC(ch) ? "-- " : class_abbrevs[(int)GET_CLASS(ch)])
*/
/*
#define CLASS_ABBR(ch) (IS_NPC(ch) ? \
                            "-- " : ((GET_LEVEL(ch) >= LVL_IMMORT) ? \
                            immort_abbrevs[(int)GET_LEVEL(ch)-LVL_IMMORT] : \
                            class_abbrevs[(int)GET_CLASS(ch)]))
*/
#define CLASS_ABBR(ch) ((GET_CLASS(ch) == CLASS_UNDEFINED) ? \
                         "-- ":((GET_LEVEL(ch) >= LVL_IMMORT && !IS_NPC(ch)) ? \
                         immort_abbrevs[(int)GET_LEVEL(ch)-LVL_IMMORT] : \
                         class_abbrevs[(int)GET_CLASS(ch)]))
/* end of hack */


#define IS_MAGIC_USER(ch)	(!IS_NPC(ch) && \
                                 GET_CLASS(ch) == CLASS_MAGIC_USER)
#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_CLERIC)
#define IS_THIEF(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_THIEF)
#define IS_WARRIOR(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_WARRIOR)  
#define IS_BARD(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_BARD)
#define IS_DRUID(ch)		(!IS_NPC(ch) && \
                                 GET_CLASS(ch) == CLASS_DRUID)
#define IS_VAMPIRE(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_VAMPIRE)
#define IS_LICH(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_LICH)
#define IS_GHOUL(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_GHOUL)
#define IS_UNUSED(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_UNUSED)
#define IS_GHOST(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_GHOST)
#define IS_MAGE2(ch)            (!IS_NPC(ch) && \
                                 GET_CLASS(ch) == CLASS_MAGE2)
#define IS_CLERIC2(ch)          (!IS_NPC(ch) && \
                                 GET_CLASS(ch) == CLASS_CLERIC2)
#define IS_THIEF2(ch)           (!IS_NPC(ch) && \
                                 GET_CLASS(ch) == CLASS_THIEF2)
#define IS_WARRIOR2(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_WARRIOR2)
#define IS_BARD2(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_BARD2)
#define IS_CITY(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_CITY)
#define IS_DEATHKNIGHT(ch)	(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_DEATHKNIGHT)
#define IS_MUPPET(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_MUPPET)
#define IS_MONK(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_MONK)
#define IS_VALKYRIE(ch)		(!IS_NPC(ch) && \
				 GET_CLASS(ch) == CLASS_VALKYRIE)

#define IS_IMMORT(ch)		(!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)				 
#define IS_MORT(ch)		(!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT)

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))


/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * Some systems such as Sun's don't have prototyping in their header files.
 * Thus, we try to compensate for them.
 *
 * Much of this is from Merc 2.2, used with permission.
 */

#if defined(_AIX)
char	*crypt(const char *key, const char *salt);
#endif
/* This is a hack to see if I can make this compile under FreeBSD */
char	*crypt(const char *key, const char *salt);

#if defined(apollo)
int	atoi (const char *string);
void	*calloc( unsigned nelem, size_t size);
char	*crypt( const char *key, const char *salt);
#endif

#if defined(hpux)
char	*crypt(char *key, const char *salt);
#endif

#if defined(linux)
char	*crypt( const char *key, const char *salt);
#endif

#if defined(MIPS_OS)
char	*crypt(const char *key, const char *salt);
#endif

#if defined(NeXT)
char	*crypt(const char *key, const char *salt);
int	unlink(const char *path);
int	getpid(void);
#endif

/*
 * The proto for [NeXT's] getpid() is defined in the man pages are returning
 * pid_t but the compiler pukes on it (cc). Since pid_t is just
 * normally a typedef for int, I just use int instead.
 * So far I have had no other problems but if I find more I will pass
 * them along...
 * -reni
 */

#if defined(sequent)
char	*crypt(const char *key, const char *salt);
int	fclose(FILE *stream);
int	fprintf(FILE *stream, const char *format, ... );
int	fread(void *ptr, int size, int n, FILE *stream);
int	fseek(FILE *stream, long offset, int ptrname);
void	perror(const char *s);
int	ungetc(int c, FILE *stream);
#endif

#if defined(sun)
#include <memory.h>
void	bzero(char *b, int length);
char	*crypt(const char *key, const char *salt);
int	fclose(FILE *stream);
int	fflush(FILE *stream);
void	rewind(FILE *stream);
int	sscanf(const char *s, const char *format, ... );
int	fprintf(FILE *stream, const char *format, ... );
int	fscanf(FILE *stream, const char *format, ... );
int	fseek(FILE *stream, long offset, int ptrname);
size_t	fread(void *ptr, size_t size, size_t n, FILE *stream);
size_t	fwrite(const void *ptr, size_t size, size_t n, FILE *stream);
void	perror(const char *s);
int	ungetc(int c, FILE *stream);
time_t	time(time_t *tloc);
int	system(const char *string);
#endif

#if defined(ultrix)
char	*crypt(const char *key, const char *salt);
#endif

#if defined(DGUX_TARGET)
#ifndef NOCRYPT
#include <crypt.h>
#endif
#define bzero(a, b) memset((a), 0, (b))
#endif

#if defined(sgi)
#include <bstring.h>
#ifndef NOCRYPT
#include <crypt.h>
#endif
#endif


/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#ifdef NOCRYPT
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

/*
 * having problems with stock assert, replacing it
 */
#ifndef assert
#define assert(x)    if ((x) == 0) {fprintf(stderr, "Assertion failed: file %s, line %d\n", __FILE__, __LINE__); exit(1);};
#endif /* assert */
