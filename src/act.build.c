/*!
 * \file act.build.h
 *
 * \par CirlcleMUD copyright
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 *
 * \par DikuMUD copyright
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991
 *
 * \par License
 * All rights reserved. See license.doc for complete information.
 */

#define _CIRCLE_ACT_BUILD_C_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "shop.h"

/* external variables */
extern char *action_bits[];
extern char *apply_types[];
extern char *affected_bits[];
extern char *extra_bits[];
extern char *item_types[];
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern char *reset_modes[];
extern char *room_bits[];
extern char *sector_types[];
extern char *shop_bits[];
extern struct shop_data *shop_index;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_shop;
extern int top_of_world;
extern int top_of_zone_table;
extern char *trade_letters[];
extern char *wear_bits[];
extern struct room_data *world;
extern char *zone_bits[];
extern struct zone_data *zone_table;

/* external functions */
sh_int real_zone(const sh_int vnum);
sh_int real_zone_by_thing(const sh_int vnum);

/* local functions */
ACMD(do_mlist);
ACMD(do_olist);
ACMD(do_rlist);
ACMD(do_slist);
ACMD(do_zlist);


#define MLIST_USAGE \
  "Usage: mlist -keyword <keyword> -level <level | minimum-maximum>\r\n" \
  "             -mobbits <mobbits> -permbits <permbits> -zone <zone-vnum>\r\n"

ACMD(do_mlist) {
  /* mlist criteria */
  static char *mlist_criteria[] = {
    /* 00 */ "keyword",
    /* 01 */ "level",
    /* 02 */ "mobbits",
    /* 03 */ "permbits",
    /* 04 */ "zone",
             "\n"
  };

  /* skip leading whitespace */
  skip_spaces(&argument);

  if (*argument == '\0') {
    send_to_char(MLIST_USAGE, ch);
  } else {
    /* Search critera */
    char keyword[MAX_INPUT_LENGTH] = {'\0'};
    uint16_t level_bottom = 1;
    uint16_t level_top = LVL_IMMORT - 1;
    long mobBits = 0;
    long permBits = 0;
    sh_int zone = NOWHERE;
    sh_int zoneV = NOWHERE;

    /* Process command line */
    register int opt = -1;
    while (argument && *argument != '\0') {
      /* Read one name */
      char name[MAX_INPUT_LENGTH] = {'\0'};
      argument = any_one_arg(argument, name);

      /* A leading dash introduces an option */
      if (*name == '-') {
	if ((opt = search_block(name + 1, mlist_criteria, FALSE)) < 0) {
	  snprintf(buf1, sizeof(buf1), "Unknown mlist option %s.\r\n", name);
	  send_to_char(buf1, ch), argument = NULL;
	}
      } else if (*name != '\0') {
	switch (opt) {
	case /* keyword */ 0:
	  {
	    register size_t keywordlen = strlen(keyword);
	    BPrintf(keyword, sizeof(keyword), keywordlen, "%s%s", *keyword != '\0' ? " " : "", name);
	  }
	  break;
	case /* level */ 1:
	  {
	    const ssize_t R = sscanf(name, "%hu-%hu", &level_bottom, &level_top);
	    if (R == 1) {
	      level_top = level_bottom;
	    } else if (R == 2) {
	      if (level_top < level_bottom) {
		const uint16_t temp = level_top;
		level_top = level_bottom;
		level_bottom = temp;
	      }
	    } else {
	      argument = NULL;
	    }
	    opt = -1;
	  }
	  break;
	case /* mobbits */ 2:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, action_bits, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown mobbit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(mobBits, (1 << R));
	    }
	  }
	  break;
	case /* permbits */ 3:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, affected_bits, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown permbit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(permBits, (1 << R));
	    }
	  }
	  break;
	case /* zone */ 4:
	  if (!is_number(name) || (zoneV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified zone vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((zone = real_zone(zoneV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no zone with vnum #%hd.\r\n", zoneV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	default:
	  send_to_char(MLIST_USAGE, ch);
	  argument = NULL;
	  break;
	}
      }
    }
    /* Successfully processed command line */
    if (argument && *argument == '\0') {
      /* An output buffer */
      char out[MAX_STRING_LENGTH] = {'\0'};
      register size_t outlen = 0;

      /* How many mobiles? */
      register size_t howMany = 0;

      /* Loop over mobile prototypes */
      register sh_int rMobile = 0;
      for (; rMobile <= top_of_mobt; ++rMobile) {
	/* So we can abort when the buffer overflows */
	const size_t outlen_saved = outlen;

	/* Shortcuts */
	const struct char_data *mobile = mob_proto + rMobile;
	const sh_int vMobile = GET_MOB_VNUM(mobile);
	const sh_int rMobileZone = real_zone_by_thing(vMobile);
	const sh_int vMobileZone = zone_table[rMobileZone].number;

	/* Keyword criteria */
	if (*keyword != '\0') {
	  register bool found = FALSE;
	  register char *curtok = keyword;
	  while (!found && curtok && *curtok != '\0') {
	    char name[MAX_INPUT_LENGTH];
	    curtok = any_one_arg(curtok, name);
	    if (mobile->player.name && *mobile->player.name != '\0')
	      if (isname(name, mobile->player.name))
		found = true;
	    if (mobile->player.description && *mobile->player.description != '\0')
	      if (isname(name, mobile->player.description))
		found = true;
	    if (mobile->player.short_descr && *mobile->player.short_descr != '\0')
	      if (isname(name, mobile->player.short_descr))
		found = true;
	    if (mobile->player.long_descr && *mobile->player.long_descr != '\0')
	      if (isname(name, mobile->player.long_descr))
		found = true;
	  }
	  if (!found)
	    continue;
	}

	/* Level critera */
	if (GET_LEVEL(mobile) &&
	   (GET_LEVEL(mobile) < level_bottom || GET_LEVEL(mobile) > level_top))
	  continue;

	/* Mobbits criteria */
	if (mobBits && !MOB_FLAGGED(mobile, mobBits))
	  continue;

	/* Permbits criteria */
	if (permBits && !AFF_FLAGGED(mobile, permBits))
	  continue;

	/* Zone criteria */
	if (zone != NOWHERE && rMobileZone != zone)
	  continue;

	if (!outlen) {
	  BPrintf(out, sizeof(out) - 32, outlen,
		" %sNum   %sVnum %sZone %sLvl %sName%s\r\n"
		"%s---- ------ ---- --- -----------------------------------------------------------%s\r\n",
		CCWHT(ch), CCCYN(ch), CCGRN(ch), CCRED(ch), CCWHT(ch), CCNRM(ch),
		CCWHT(ch), CCNRM(ch));
	}

	/* Format mobile data */
	BPrintf(out, sizeof(out) - 32, outlen,
		"%s%4zu %s%6d %s%4d %s%3d %s%-58.58s%s\r\n",
		CCWHT(ch), howMany + 1,
		CCCYN(ch), vMobile,
		CCGRN(ch), vMobileZone,
		CCRED(ch), GET_LEVEL(mobile),
		CCWHT(ch), BLANK(mobile->player.short_descr),
		CCNRM(ch));

	/* Check buffer overflow */
	if (outlen == outlen_saved) {
	  BPrintf(out, sizeof(out), outlen, "%s*OVERFLOW*%s\r\n", CCRED(ch), CCNRM(ch));
	  goto mlist_overflow;
	} else
	  howMany++;
      }

mlist_overflow:

      if (!outlen) {
	send_to_char("No mobiles were found.\r\n", ch);
      } else {
	page_string(ch->desc, out, true);
      }
    }
  }
}


#define OLIST_USAGE \
  "Usage: olist -apply <apply> -extrabits <extrabits> -keyword <keyword>\r\n" \
  "             -permbits <permbits> -type <type> -wearbits <wearbits>\r\n" \
  "             -zone <zone-vnum>\r\n"

ACMD(do_olist) {
  /* olist criteria */
  static char *olist_criteria[] = {
    /* 00 */ "apply",
    /* 01 */ "extrabits",
    /* 02 */ "keyword",
    /* 03 */ "permbits",
    /* 04 */ "type",
    /* 05 */ "wearbits",
    /* 06 */ "zone",
             "\n"
  };

  /* skip leading whitespace */
  skip_spaces(&argument);

  if (*argument == '\0') {
    send_to_char(OLIST_USAGE, ch);
  } else {
    /* Search critera */
    int apply = APPLY_NONE;
    long extraBits = 0;
    char keyword[MAX_INPUT_LENGTH] = {'\0'};
    long permBits = 0;
    int type = -1;
    long wearBits = 0;
    sh_int zone = NOWHERE;
    sh_int zoneV = NOWHERE;

    /* Process command line */
    register int opt = -1;
    while (argument && *argument != '\0') {
      /* Read one name */
      char name[MAX_INPUT_LENGTH] = {'\0'};
      argument = any_one_arg(argument, name);

      /* A leading dash introduces an option */
      if (*name == '-') {
	if ((opt = search_block(name + 1, olist_criteria, FALSE)) < 0) {
	  snprintf(buf1, sizeof(buf1), "Unknown olist option %s.\r\n", name);
	  send_to_char(buf1, ch), argument = NULL;
	}
      } else if (*name != '\0') {
	switch (opt) {
	case /* apply */ 0:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, apply_types, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown apply type %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else
	      apply = R;
	  }
	  break;
	case /* extrabits */ 1:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, extra_bits, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown extrabit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(extraBits, (1 << R));
	    }
	  }
	  break;
	case /* keyword */ 2:
	  {
	    register size_t keywordlen = strlen(keyword);
	    BPrintf(keyword, sizeof(keyword), keywordlen, "%s%s", *keyword != '\0' ? " " : "", name);
	  }
	  break;
	case /* permbits */ 3:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, affected_bits, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown permbit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(permBits, (1 << R));
	    }
	  }
	  break;
	case /* type */ 4:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, item_types, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown item type %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else
	      type = R;
	  }
	  break;
	case /* wearbits */ 5:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, wear_bits, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown wearbit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(wearBits, (1 << R));
	    }
	  }
	  break;
	case /* zone */ 6:
	  if (!is_number(name) || (zoneV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified zone vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((zone = real_zone(zoneV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no zone with vnum #%hd.\r\n", zoneV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	default:
	  send_to_char(OLIST_USAGE, ch);
	  argument = NULL;
	  break;
	}
      }
    }
    /* Successfully processed command line */
    if (argument && *argument == '\0') {
      /* An output buffer */
      char out[MAX_STRING_LENGTH] = {'\0'};
      register size_t outlen = 0;

      /* How many objects? */
      register size_t howMany = 0;

      /* Loop over object prototypes */
      register sh_int rObject = 0;
      for (; rObject <= top_of_objt; ++rObject) {
	/* So we can abort when the buffer overflows */
	const size_t outlen_saved = outlen;

	/* Shortcuts */
	const struct obj_data *object = obj_proto + rObject;
	const sh_int vObject = GET_OBJ_VNUM(object);
	const sh_int rObjectZone = real_zone_by_thing(vObject);
	const sh_int vObjectZone = zone_table[rObjectZone].number;

	/* Apply criteria */
	if (apply != APPLY_NONE) {
	  register size_t affectedN = 0;
	  for (; affectedN < MAX_OBJ_AFFECT; ++affectedN) {
	    if (object->affected[affectedN].location == apply)
	      break;
	  }
	  if (affectedN == MAX_OBJ_AFFECT)
	    continue;
	}

	/* Extrabits criteria */
	if (extraBits && !IS_SET(GET_OBJ_EXTRA(object), extraBits))
	  continue;

	/* Keyword criteria */
	if (*keyword != '\0') {
	  register bool found = FALSE;
	  register char *curtok = keyword;
	  while (!found && curtok && *curtok != '\0') {
	    char name[MAX_INPUT_LENGTH];
	    curtok = any_one_arg(curtok, name);
	    if (object->name && *object->name != '\0')
	      if (isname(name, object->name))
		found = true;
	    if (object->description && *object->description != '\0')
	      if (isname(name, object->description))
		found = true;
	    if (object->short_description && *object->short_description != '\0')
	      if (isname(name, object->short_description))
		found = true;
	    if (object->action_description && *object->action_description != '\0')
	      if (isname(name, object->action_description))
		found = true;
	  }
	  if (!found)
	    continue;
	}

	/* Permbits criteria */
	if (permBits && !IS_SET(GET_OBJ_AFFECT(object), permBits))
	  continue;

	/* Item type criteria */
	if (type != -1 && GET_OBJ_TYPE(object) != type)
	  continue;

	/* Wearbits criteria */
	if (wearBits && !IS_SET(GET_OBJ_WEAR(object), wearBits))
	  continue;

	/* Zone criteria */
	if (zone != NOWHERE && rObjectZone != zone)
	  continue;

	if (!outlen) {
	  BPrintf(out, sizeof(out) - 32, outlen,
		" %sNum   %sVnum %sZone          %sType %sName%s\r\n"
		"%s---- ------ ---- ------------- -------------------------------------------------%s\r\n",
		CCWHT(ch), CCCYN(ch), CCGRN(ch), CCMAG(ch), CCWHT(ch), CCNRM(ch),
		CCWHT(ch), CCNRM(ch));
	}

	/* Format the item name */
	char objname[MAX_INPUT_LENGTH] = {'\0'};
	register size_t objnamelen = 0;
	BPrintf(objname, sizeof(objname), objnamelen, "%s", BLANK(object->short_description));

	/* Basic object information */
	switch (GET_OBJ_TYPE(object)) {
	case ITEM_ARMOR:
	  BPrintf(objname, sizeof(objname), objnamelen, " (%dac)", GET_OBJ_VAL(object, 0));
	  break;
	case ITEM_FIREWEAPON:
	case ITEM_WEAPON:
	  BPrintf(objname, sizeof(objname), objnamelen, " (%dd%d)",
		GET_OBJ_VAL(object, 1),
		GET_OBJ_VAL(object, 2));
	  break;
	}

	/* Item effects */
	register size_t affectedN = 0;
	for (; affectedN < MAX_OBJ_AFFECT; ++affectedN) {
	  if (!object->affected[affectedN].location)
	    continue;
	  if (!object->affected[affectedN].modifier)
	    continue;

	  char applyname[MAX_INPUT_LENGTH] = {'\0'};
	  sprinttype(object->affected[affectedN].location, apply_types, applyname);
	  BPrintf(objname, sizeof(objname), objnamelen, " %+d %s",
		object->affected[affectedN].modifier,
		applyname);
	}

	/* Format the item type */
	char type_name[MAX_INPUT_LENGTH] = {'\0'};
	sprinttype(GET_OBJ_TYPE(object), item_types, type_name);

	/* Format mobile data */
	BPrintf(out, sizeof(out) - 32, outlen,
		"%s%4zu %s%6d %s%4d %s%13.13s %s%-49.49s%s\r\n",
		CCWHT(ch), howMany + 1,
		CCCYN(ch), vObject,
		CCGRN(ch), vObjectZone,
		CCMAG(ch), type_name,
		CCWHT(ch), *objname != '\0' ? objname : "<Blank>",
		CCNRM(ch));

	/* Check buffer overflow */
	if (outlen == outlen_saved) {
	  BPrintf(out, sizeof(out), outlen, "%s*OVERFLOW*%s\r\n", CCRED(ch), CCNRM(ch));
	  goto olist_overflow;
	} else
	  howMany++;
      }

olist_overflow:

      if (!outlen) {
	send_to_char("No objects were found.\r\n", ch);
      } else {
	page_string(ch->desc, out, true);
      }
    }
  }
}


#define RLIST_USAGE \
  "Usage: rlist -key <item-vnum> -keyword <keyword> -roombits <roobits>\r\n" \
  "             -sector <sector> -toroom <room-vnum> -tozone <zone-vnum>\r\n" \
  "             -zone <zone-vnum>\r\n"

ACMD(do_rlist) {
  /* rlist criteria */
  static char *rlist_criteria[] = {
    /* 00 */ "key",
    /* 01 */ "keyword",
    /* 02 */ "mobile",
    /* 03 */ "object",
    /* 04 */ "roombits",
    /* 05 */ "sector",
    /* 06 */ "toroom",
    /* 07 */ "tozone",
    /* 08 */ "zone",
             "\n"
  };

  /* skip leading whitespace */
  skip_spaces(&argument);

  if (*argument == '\0') {
    send_to_char(RLIST_USAGE, ch);
  } else {
    /* Search critera */
    sh_int key = NOTHING;
    sh_int keyV = NOTHING;
    char keyword[MAX_INPUT_LENGTH] = {'\0'};
    long roomBits = 0;
    int sector = -1;
    sh_int mobile = NOWHERE;
    sh_int mobileV = NOWHERE;
    sh_int object = NOWHERE;
    sh_int objectV = NOWHERE;
    sh_int toRoom = NOWHERE;
    sh_int toRoomV = NOWHERE;
    sh_int toZone = NOWHERE;
    sh_int toZoneV = NOWHERE;
    sh_int zone = NOWHERE;
    sh_int zoneV = NOWHERE;

    /* Process command line */
    register int opt = -1;
    while (argument && *argument != '\0') {
      /* Read one name */
      char name[MAX_INPUT_LENGTH] = {'\0'};
      argument = any_one_arg(argument, name);

      /* A leading dash introduces an option */
      if (*name == '-') {
	if ((opt = search_block(name + 1, rlist_criteria, FALSE)) < 0) {
	  snprintf(buf1, sizeof(buf1), "Unknown rlist option %s.\r\n", name);
	  send_to_char(buf1, ch), argument = NULL;
	}
      } else if (*name != '\0') {
	switch (opt) {
	case /* key */ 0:
	  if (!is_number(name) || (keyV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified item vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((key = real_room(keyV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no object with vnum #%hd.\r\n", keyV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* keyword */ 1:
	  {
	    register size_t keywordlen = strlen(keyword);
	    BPrintf(keyword, sizeof(keyword), keywordlen, "%s%s", *keyword != '\0' ? " " : "", name);
	  }
	  break;
	case /* mobile */ 2:
	  if (!is_number(name) || (mobileV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified mobile vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((mobile = real_mobile(mobileV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no mobile with vnum #%hd.\r\n", mobileV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* object */ 3:
	  if (!is_number(name) || (objectV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified object vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((object = real_object(objectV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no object with vnum #%hd.\r\n", objectV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* roombits */ 4:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, room_bits, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown roombit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(roomBits, (1 << R));
	    }
	  }
	  break;
	case /* sector */ 5:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, sector_types, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown sector type %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else
	      sector = R;
	  }
	  break;
	case /* toroom */ 6:
	  if (!is_number(name) || (toRoomV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified room vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((toRoom = real_room(toRoomV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no room with vnum #%hd.\r\n", toRoomV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* tozone */ 7:
	  if (!is_number(name) || (toZoneV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified zone vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((toZone = real_zone(toZoneV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no zone with vnum #%hd.\r\n", toZoneV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* zone */ 8:
	  if (!is_number(name) || (zoneV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified zone vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((zone = real_zone(zoneV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no zone with vnum #%hd.\r\n", zoneV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	default:
	  send_to_char(RLIST_USAGE, ch);
	  argument = NULL;
	  break;
	}
      }
    }
    /* Successfully processed command line */
    if (argument && *argument == '\0') {
      /* An output buffer */
      char out[MAX_STRING_LENGTH] = {'\0'};
      register size_t outlen = 0;

      /* How many rooms? */
      register size_t howMany = 0;

      /* Loop over rooms */
      register sh_int rRoom = 0;
      for (; rRoom <= top_of_world; ++rRoom) {
	/* So we can abort when the buffer overflows */
	const size_t outlen_saved = outlen;

	/* Shortcuts */
	const struct room_data *room = world + rRoom;
	const sh_int vRoom = room->number;
	const sh_int rRoomZone = room->zone;
	const sh_int vRoomZone = zone_table[rRoomZone].number;

	/* Key criteria */
	if (key != NOWHERE) {
	  register size_t door = 0;
	  for (door = 0; door < NUM_OF_DIRS; ++door) {
	    if (room->dir_option[door] &&
		room->dir_option[door]->key == keyV)
	      break;
	  }
	  if (door == NUM_OF_DIRS)
	    continue;
	}

	/* Keyword criteria */
	if (*keyword != '\0') {
	  register bool found = FALSE;
	  register char *curtok = keyword;
	  while (!found && curtok && *curtok != '\0') {
	    char name[MAX_INPUT_LENGTH];
	    curtok = any_one_arg(curtok, name);
	    if (room->name && *room->name != '\0')
	      if (isname(name, room->name))
		found = true;
	    if (room->description && *room->description != '\0')
	      if (isname(name, room->description))
		found = true;
	  }
	  if (!found)
	    continue;
	}

	/* Mobile criteria */
	if (mobile != NOWHERE) {
	  register bool found = false;
	  register size_t cmdN = 0;
	  for (; !found && zone_table[rRoomZone].cmd[cmdN].command != 'S'; ++cmdN) {
	    if (zone_table[rRoomZone].cmd[cmdN].command == 'M') {
	      if (zone_table[rRoomZone].cmd[cmdN].arg1 == mobile &&
		  zone_table[rRoomZone].cmd[cmdN].arg3 == rRoom)
		found = true;
	    }
	  }
	  if (!found)
	    continue;
	}

	/* Object criteria */
	if (object != NOWHERE) {
	  register bool found = false;
	  register size_t cmdN = 0;
	  for (; !found && zone_table[rRoomZone].cmd[cmdN].command != 'S'; ++cmdN) {
	    switch (zone_table[rRoomZone].cmd[cmdN].command) {
	    case 'E':
	    case 'G':
	      if (zone_table[rRoomZone].cmd[cmdN].arg1 == object)
		found = true;
	      break;
	    case 'O':
	      if (zone_table[rRoomZone].cmd[cmdN].arg1 == object &&
		  zone_table[rRoomZone].cmd[cmdN].arg3 == rRoom)
		found = true;
	      break;
	    case 'R':
	      if (zone_table[rRoomZone].cmd[cmdN].arg2 == object &&
		  zone_table[rRoomZone].cmd[cmdN].arg3 == rRoom)
		found = true;
	      break;
	    case 'P':
	      if (zone_table[rRoomZone].cmd[cmdN].arg1 == object)
		found = true;
	      if (zone_table[rRoomZone].cmd[cmdN].arg3 == object)
		found = true;
	      break;
	    }
	  }
	  if (!found)
	    continue;
	}

	/* Roombits criteria */
	if (roomBits && !IS_SET(ROOM_FLAGS(rRoom), roomBits))
	  continue;

	/* Sector type criteria */
	if (sector != -1 && room->sector_type != sector)
	  continue;

	/* To-room criteria */
	if (toRoom != NOWHERE) {
	  register size_t door = 0;
	  for (door = 0; door < NUM_OF_DIRS; ++door) {
	    if (room->dir_option[door] &&
		room->dir_option[door]->to_room == toRoom)
	      break;
	  }
	  if (door == NUM_OF_DIRS)
	    continue;
	}

	/* To-zone criteria */
	if (toZone != NOWHERE) {
	  register size_t door = 0;
	  for (door = 0; door < NUM_OF_DIRS; ++door) {
	    if (room->dir_option[door] &&
		room->dir_option[door]->to_room != NOWHERE &&
		room->zone != toZone &&
		world[room->dir_option[door]->to_room].zone == toZone)
	      break;
	  }
	  if (door == NUM_OF_DIRS)
	    continue;
	}

	/* Zone criteria */
	if (zone != NOWHERE && rRoomZone != zone)
	  continue;

	if (!outlen) {
	  BPrintf(out, sizeof(out) - 32, outlen,
		" %sNum   %sVnum %sZone      %sSector %sName%s\r\n"
		"%s---- ------ ---- ----------- ---------------------------------------------------%s\r\n",
		CCWHT(ch), CCCYN(ch), CCGRN(ch), CCMAG(ch), CCWHT(ch), CCNRM(ch),
		CCWHT(ch), CCNRM(ch));
	}

	/* Format the sector type */
	char sector_name[MAX_INPUT_LENGTH] = {'\0'};
	sprinttype(room->sector_type, sector_types, sector_name);

	/* Format room data */
	BPrintf(out, sizeof(out) - 32, outlen,
		"%s%4zu %s%6d %s%4d %s%11.11s %s%-51.51s%s\r\n",
		CCWHT(ch), howMany + 1,
		CCCYN(ch), vRoom,
		CCGRN(ch), vRoomZone,
		CCMAG(ch), sector_name,
		CCWHT(ch), BLANK(room->name),
		CCNRM(ch));

	/* Check buffer overflow */
	if (outlen == outlen_saved) {
	  BPrintf(out, sizeof(out), outlen, "%s*OVERFLOW*%s\r\n", CCRED(ch), CCNRM(ch));
	  goto rlist_overflow;
	} else
	  howMany++;
      }

rlist_overflow:

      if (!outlen) {
	send_to_char("No rooms were found.\r\n", ch);
      } else {
	page_string(ch->desc, out, true);
      }
    }
  }
}


#define SLIST_USAGE \
  "Usage: slist -buytype <item-type> -keeper <keeper-vnum>\r\n" \
  "             -product <item-vnum> -room <room-vnum\r\n" \
  "             -shopbits <shopbits> -tradebits <tradebits>\r\n" \
  "             -zone <zone-vnum>\r\n"

ACMD(do_slist) {
  /* slist criteria */
  static char *slist_criteria[] = {
    /* 00 */ "buytype",
    /* 01 */ "keeper",
    /* 02 */ "product",
    /* 03 */ "room",
    /* 04 */ "shopbits",
    /* 05 */ "tradebits",
    /* 06 */ "zone",
             "\n"
  };

  /* skip leading whitespace */
  skip_spaces(&argument);

  if (*argument == '\0') {
    send_to_char(RLIST_USAGE, ch);
  } else {
    /* Search critera */
    int buyType = -1;
    sh_int keeper = NOBODY;
    sh_int keeperV = NOBODY;
    sh_int product = NOWHERE;
    sh_int productV = NOWHERE;
    sh_int room = NOWHERE;
    sh_int roomV = NOWHERE;
    long shopBits = 0;
    long tradeBits = 0;
    sh_int zone = NOWHERE;
    sh_int zoneV = NOWHERE;

    /* Process command line */
    register int opt = -1;
    while (argument && *argument != '\0') {
      /* Read one name */
      char name[MAX_INPUT_LENGTH] = {'\0'};
      argument = any_one_arg(argument, name);

      /* A leading dash introduces an option */
      if (*name == '-') {
	if ((opt = search_block(name + 1, slist_criteria, FALSE)) < 0) {
	  snprintf(buf1, sizeof(buf1), "Unknown slist option %s.\r\n", name);
	  send_to_char(buf1, ch), argument = NULL;
	}
      } else if (*name != '\0') {
	switch (opt) {
	case /* buytype */ 0:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, item_types, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown item type %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else
	      buyType = R;
	  }
	  break;
	case /* keeper */ 1:
	  if (!is_number(name) || (keeperV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified mobile vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((keeper = real_mobile(keeperV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no mobile with vnum #%hd.\r\n", keeperV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* product */ 2:
	  if (!is_number(name) || (productV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified item vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((product = real_object(productV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no item with vnum #%hd.\r\n", productV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* room */ 3:
	  if (!is_number(name) || (roomV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified room vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((room = real_room(roomV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no room with vnum #%hd.\r\n", roomV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* shopbits */ 4:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, shop_bits, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown shopbit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(shopBits, (1 << R));
	    }
	  }
	  break;
	case /* tradebits */ 5:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, trade_letters, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown tradebit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(tradeBits, (1 << R));
	    }
	  }
	  break;
	case /* zone */ 6:
	  if (!is_number(name) || (zoneV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified zone vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((zone = real_zone(zoneV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no zone with vnum #%hd.\r\n", zoneV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	default:
	  send_to_char(SLIST_USAGE, ch);
	  argument = NULL;
	  break;
	}
      }
    }
    /* Successfully processed command line */
    if (argument && *argument == '\0') {
      /* An output buffer */
      char out[MAX_STRING_LENGTH] = {'\0'};
      register size_t outlen = 0;

      /* How many shop? */
      register size_t howMany = 0;

      /* Loop over shops */
      register sh_int rShop = 0;
      for (; rShop <= top_shop; ++rShop) {
	/* So we can abort when the buffer overflows */
	const size_t outlen_saved = outlen;

	/* Shortcuts */
	const struct shop_data *shop = shop_index + rShop;
	const sh_int vShop = shop->virtual;
	const sh_int rShopZone = real_zone_by_thing(vShop);
	const sh_int vShopZone = zone_table[rShopZone].number;

	/* Buy type criteria */
	if (buyType != -1) {
	  register size_t buyN = 0;
	  for (buyN = 0; SHOP_BUYTYPE(rShop, buyN) != -1; ++buyN) { 
	    if (SHOP_BUYTYPE(rShop, buyN) == buyType)
	      break;
	  }
	  if (SHOP_BUYTYPE(rShop, buyN) == -1)
	    continue;
	}

	/* Skop keeper criteria */
	if (keeper != -1 && shop->keeper != keeper)
	  continue;

	/* Product criteria */
	if (product != NOWHERE) {
	  register size_t productN = 0;
	  for (productN = 0; SHOP_PRODUCT(rShop, productN) != -1; ++productN) {
	    if (SHOP_PRODUCT(rShop, productN) == product)
	      break;
	  }
	  if (SHOP_PRODUCT(rShop, productN) == -1)
	    continue;
	}

	/* Room criteria */
	if (room != NOWHERE) {
	  register size_t roomN = 0;
	  for (roomN = 0; SHOP_ROOM(rShop, roomN) != -1; ++roomN) {
	    if (SHOP_ROOM(rShop, roomN) == roomV)
	      break;
	  }
	  if (SHOP_ROOM(rShop, roomN) == -1)
	    continue;
	}

	/* Shopbits criteria */
	if (shopBits && !IS_SET(SHOP_BITVECTOR(rShop), shopBits))
	  continue;

	/* Tradebits criteria */
	if (tradeBits && !IS_SET(SHOP_TRADE_WITH(rShop), tradeBits))
	  continue;

	/* Zone criteria */
	if (zone != NOWHERE && rShopZone != zone)
	  continue;

	if (!outlen) {
	  BPrintf(out, sizeof(out) - 32, outlen,
		" %sNum   %sVnum %sZone   %sRoom  %sBuy %sSell %sKeeper %sName%s\r\n"
		"%s---- ------ ---- ------ ---- ---- ------ ---------------------------------------%s\r\n",
		CCWHT(ch), CCCYN(ch), CCGRN(ch), CCYEL(ch), CCRED(ch), CCGRN(ch), CCMAG(ch), CCWHT(ch), CCNRM(ch),
		CCWHT(ch), CCNRM(ch));
	}

	/* Format room data */
	BPrintf(out, sizeof(out) - 32, outlen,
		"%s%4zu %s%6d %s%4d %s%6d %s%3.2f %s%3.2f %s%6d %s%-39.39s%s\r\n",
		CCWHT(ch), howMany + 1,
		CCCYN(ch), vShop,
		CCGRN(ch), vShopZone,
		CCYEL(ch), SHOP_ROOM(rShop, 0),
		CCRED(ch), shop->profit_sell,
		CCGRN(ch), shop->profit_buy,
		CCMAG(ch), shop->keeper != NOBODY ? mob_index[shop->keeper].virtual : -1,
		CCWHT(ch), shop->keeper != NOBODY ? BLANK(mob_proto[shop->keeper].player.short_descr) : "<None>",
		CCNRM(ch));

	/* Check buffer overflow */
	if (outlen == outlen_saved) {
	  BPrintf(out, sizeof(out), outlen, "%s*OVERFLOW*%s\r\n", CCRED(ch), CCNRM(ch));
	  goto slist_overflow;
	} else
	  howMany++;
      }

slist_overflow:

      if (!outlen) {
	send_to_char("No shops were found.\r\n", ch);
      } else {
	page_string(ch->desc, out, true);
      }
    }
  }
}


#define ZLIST_USAGE \
  "Usage: rlist -keyword <keyword> -mobile <mobile-vnum>\r\n" \
  "             -object <item-vnum> -toroom <room-vnum> -tozone <zone-vnum>\r\n" \
  "             -zonebits <zonebits>\r\n"

ACMD(do_zlist) {
  /* zlist criteria */
  static char *zlist_criteria[] = {
    /* 00 */ "keyword",
    /* 01 */ "mobile",
    /* 02 */ "object",
    /* 03 */ "toroom",
    /* 04 */ "tozone",
    /* 05 */ "zonebits",
             "\n"
  };

  /* skip leading whitespace */
  skip_spaces(&argument);

  if (*argument == '\0') {
    send_to_char(ZLIST_USAGE, ch);
  } else {
    /* Search critera */
    char keyword[MAX_INPUT_LENGTH] = {'\0'};
    sh_int mobile = NOBODY;
    sh_int mobileV = NOBODY;
    sh_int object = NOBODY;
    sh_int objectV = NOBODY;
    sh_int toRoom = NOWHERE;
    sh_int toRoomV = NOWHERE;
    sh_int toZone = NOWHERE;
    sh_int toZoneV = NOWHERE;
    long zoneBits = 0;

    /* Process command line */
    register int opt = -1;
    while (argument && *argument != '\0') {
      /* Read one name */
      char name[MAX_INPUT_LENGTH] = {'\0'};
      argument = any_one_arg(argument, name);

      /* A leading dash introduces an option */
      if (*name == '-') {
	if ((opt = search_block(name + 1, zlist_criteria, FALSE)) < 0) {
	  snprintf(buf1, sizeof(buf1), "Unknown zlist option %s.\r\n", name);
	  send_to_char(buf1, ch), argument = NULL;
	}
      } else if (*name != '\0') {
	switch (opt) {
	case /* keyword */ 0:
	  {
	    register size_t keywordlen = strlen(keyword);
	    BPrintf(keyword, sizeof(keyword), keywordlen, "%s%s", *keyword != '\0' ? " " : "", name);
	  }
	  break;
	case /* mobile */ 1:
	  if (!is_number(name) || (mobileV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified mobile vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((mobile = real_mobile(mobileV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no mobile with vnum #%hd.\r\n", mobileV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* object */ 2:
	  if (!is_number(name) || (objectV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified object vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((object = real_object(objectV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no object with vnum #%hd.\r\n", objectV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* toroom */ 3:
	  if (!is_number(name) || (toRoomV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified room vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((toRoom = real_room(toRoomV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no room with vnum #%hd.\r\n", toRoomV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* tozone */ 4:
	  if (!is_number(name) || (toZoneV = atoi(name)) < 0) {
	    snprintf(buf1, sizeof(buf1), "The specified zone vnum, %s, is not valid.\r\n", name);
	    send_to_char(buf1, ch), argument = NULL;
	  } else if ((toZone = real_zone(toZoneV)) < 0) {
	    snprintf(buf1, sizeof(buf1), "There is no zone with vnum #%hd.\r\n", toZoneV);
	    send_to_char(buf1, ch), argument = NULL;
	  }
	  break;
	case /* zonebits */ 5:
	  {
	    register ssize_t R;
	    if ((R = search_block(name, zone_bits, FALSE)) < 0) {
	      snprintf(buf1, sizeof(buf1), "Unknown zonebit %s.\r\n", name);
	      send_to_char(buf1, ch), argument = NULL;
	    } else {
	      SET_BIT(zoneBits, (1 << R));
	    }
	  }
	  break;
	default:
	  send_to_char(ZLIST_USAGE, ch);
	  argument = NULL;
	  break;
	}
      }
    }
    /* Successfully processed command line */
    if (argument && *argument == '\0') {
      /* An output buffer */
      char out[MAX_STRING_LENGTH] = {'\0'};
      register size_t outlen = 0;

      /* How many zones? */
      register size_t howMany = 0;

      /* Loop over zones */
      register sh_int rZone = 0;
      for (; rZone <= top_of_zone_table; ++rZone) {
	/* So we can abort when the buffer overflows */
	const size_t outlen_saved = outlen;

	/* Shortcuts */
	const struct zone_data *zone = zone_table + rZone;
	const sh_int vZone = zone->number;

	/* Keyword criteria */
	if (*keyword != '\0') {
	  register bool found = FALSE;
	  register char *curtok = keyword;
	  while (!found && curtok && *curtok != '\0') {
	    char name[MAX_INPUT_LENGTH];
	    curtok = any_one_arg(curtok, name);
	    if (zone->name && *zone->name != '\0')
	      if (isname(name, zone->name))
		found = true;
	  }
	  if (!found)
	    continue;
	}

	/* Mobile criteria */
	if (mobile != NOWHERE) {
	  register bool found = false;
	  register size_t cmdN = 0;
	  for (; !found && zone->cmd[cmdN].command != 'S'; ++cmdN) {
	    if (zone->cmd[cmdN].command == 'M')
	      if (zone->cmd[cmdN].arg1 == mobile)
		found = true;
	  }
	  if (!found)
	    continue;
	}

	/* Object criteria */
	if (object != NOWHERE) {
	  register bool found = false;
	  register size_t cmdN = 0;
	  for (; !found && zone->cmd[cmdN].command != 'S'; ++cmdN) {
	    switch (zone->cmd[cmdN].command) {
	    case 'E':
	    case 'G':
	    case 'O':
	      if (zone->cmd[cmdN].arg1 == object)
		found = true;
	      break;
	    case 'R':
	      if (zone->cmd[cmdN].arg3 == object)
		found = true;
	      break;
	    case 'P':
	      if (zone->cmd[cmdN].arg1 == object)
		found = true;
	      if (zone->cmd[cmdN].arg3 == object)
		found = true;
	      break;
	    }
	  }
	  if (!found)
	    continue;
	}

	/* To-room criteria */
	if (toRoom != NOWHERE) {
	  register sh_int rRoom = 0;
	  for (; rRoom <= top_of_world; ++rRoom) {
	    register size_t door = 0;
	    for (door = 0; door < NUM_OF_DIRS; ++door) {
	      if (world[rRoom].dir_option[door] &&
		  world[rRoom].zone == rZone &&
		  world[world[rRoom].dir_option[door]->to_room].zone != world[rRoom].zone &&
		  world[rRoom].dir_option[door]->to_room == toRoom)
		break;
	    }
	    if (door != NUM_OF_DIRS)
	      break;
	  }
	  if (rRoom > top_of_world)
	    continue;
	}

	/* To-zone criteria */
	if (toZone != NOWHERE) {
	  register sh_int rRoom = 0;
	  for (; rRoom <= top_of_world; ++rRoom) {
	    register size_t door = 0;
	    for (door = 0; door < NUM_OF_DIRS; ++door) {
	      if (world[rRoom].dir_option[door] &&
		  world[rRoom].dir_option[door]->to_room != NOWHERE &&
		  world[rRoom].zone == rZone &&
		  world[world[rRoom].dir_option[door]->to_room].zone != world[rRoom].zone &&
		  world[world[rRoom].dir_option[door]->to_room].zone == toZone)
		break;
	    }
	    if (door != NUM_OF_DIRS)
	      break;
	  }
	  if (rRoom > top_of_world)
	    continue;
	}

	/* Zonebits criteria */
	if (zoneBits && !IS_SET(zone->zone_flags, zoneBits))
	  continue;

	if (!outlen) {
	  BPrintf(out, sizeof(out) - 32, outlen,
		" %sNum   %sVnum %sBottom    %sTop %sLife  %sAge  %sReset %sName%s\r\n"
		"%s---- ------ ------ ------ ---- ---- ------ -------------------------------------%s\r\n",
		CCWHT(ch), CCCYN(ch), CCYEL(ch), CCYEL(ch), CCGRN(ch), CCGRN(ch), CCMAG(ch), CCWHT(ch), CCNRM(ch),
		CCWHT(ch), CCNRM(ch));
	}

	/* Format the reset mode */
	char reset_mode[MAX_INPUT_LENGTH] = {'\0'};
	sprinttype(zone->reset_mode, reset_modes, reset_mode);

	/* Format zone data */
	BPrintf(out, sizeof(out) - 32, outlen,
		"%s%4zu %s%6d %s%6d %s%6d %s%4d %s%4d %s%6.6s %s%-37.37s%s\r\n",
		CCWHT(ch), howMany + 1,
		CCCYN(ch), zone->number,
		CCYEL(ch), zone->number * 100,
		CCYEL(ch), zone->top,
		CCGRN(ch), zone->lifespan,
		CCGRN(ch), zone->age,
		CCMAG(ch), reset_mode,
		CCWHT(ch), *BLANK(zone->name) != '\0' ? zone->name : "<None>",
		CCNRM(ch));

	/* Check buffer overflow */
	if (outlen == outlen_saved) {
	  BPrintf(out, sizeof(out), outlen, "%s*OVERFLOW*%s\r\n", CCRED(ch), CCNRM(ch));
	  goto zlist_overflow;
	} else
	  howMany++;
      }

zlist_overflow:

      if (!outlen) {
	send_to_char("No zones were found.\r\n", ch);
      } else {
	page_string(ch->desc, out, true);
      }
    }
  }
}
