/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	1			/* was 1 */
/* HACKED to make cryo cheaper */
#define CRYO_FACTOR 	2			/* was 4 */
/* end of hack */

/* HACKED to add in RENT_ADJUST, which divides the rent to make objects
  cheaper without having to fix the rent on every object in the game 
  set it to 1 to make the rent cost stock, and NEVER set it to 0!!! */
#define RENT_ADJUST	100
/* end of hack */

extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int min_rent_cost;
/* HACKED to add containers in rent proper */
static struct obj_data *last_loaded_obj;

/* Extern functions */
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
void clanlog(char *str, struct char_data * ch);



struct obj_data *Obj_from_store(struct obj_file_elem object)
{
  struct obj_data *obj;
  int j;

  if (real_object(object.item_number) > -1) {
    obj = read_object(object.item_number, VIRTUAL);
    GET_OBJ_VAL(obj, 0) = object.value[0];
    GET_OBJ_VAL(obj, 1) = object.value[1];
    GET_OBJ_VAL(obj, 2) = object.value[2];
    GET_OBJ_VAL(obj, 3) = object.value[3];
    GET_OBJ_EXTRA(obj) = object.extra_flags;
    GET_OBJ_WEIGHT(obj) = object.weight;
    GET_OBJ_TIMER(obj) = object.timer;

    obj->obj_flags.bitvector = object.bitvector;
    obj->obj_flags.bitvector2 = object.bitvector2;

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      obj->affected[j] = object.affected[j];

    return obj;
  } else
    return NULL;
}



int Obj_to_store(struct obj_data * obj, FILE * fl)
{
/* HACKED to write ascii object file 
   needs to save modified versions that have been affected by spells */
  int obj_match;
  int j;
/* old code
  struct obj_file_elem object;

  object.item_number = GET_OBJ_VNUM(obj);
  object.value[0] = GET_OBJ_VAL(obj, 0);
  object.value[1] = GET_OBJ_VAL(obj, 1);
  object.value[2] = GET_OBJ_VAL(obj, 2);
  object.value[3] = GET_OBJ_VAL(obj, 3);
  object.extra_flags = GET_OBJ_EXTRA(obj);
  object.weight = GET_OBJ_WEIGHT(obj);
  object.timer = GET_OBJ_TIMER(obj);

  object.bitvector = obj->obj_flags.bitvector;
  object.bitvector2 = obj->obj_flags.bitvector2;

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    object.affected[j] = obj->affected[j];

  if (fwrite(&object, sizeof(struct obj_file_elem), 1, fl) < 1) {
    perror("Error writing object in Obj_to_store");
    return 0;
  }
*/
  extern char *equipment_types[];
  struct obj_data *generic_obj;

  /* create a generic object to compare what we have to it */
/* HACKED to get rid of the mail crash bug (negative vnums) */
  if (obj->item_number > -1)
    generic_obj = read_object(obj->item_number, REAL);
  else
    return 1;	/* trying to save mail, a corpse, or a limb or somesuch */
/* end of hack */

  obj_match = 1;
  if (generic_obj->obj_flags.value[0] != obj->obj_flags.value[0]) obj_match = 0;
  if (generic_obj->obj_flags.value[1] != obj->obj_flags.value[1]) obj_match = 0;
  if (generic_obj->obj_flags.value[2] != obj->obj_flags.value[2]) obj_match = 0;
  if (generic_obj->obj_flags.value[3] != obj->obj_flags.value[3]) obj_match = 0;
  if (generic_obj->obj_flags.type_flag !=
      obj->obj_flags.type_flag) obj_match = 0;
  if (generic_obj->obj_flags.wear_flags !=
      obj->obj_flags.wear_flags) obj_match = 0;
  if (generic_obj->obj_flags.extra_flags !=
      obj->obj_flags.extra_flags) obj_match = 0;
      
  /* Hacked to ignore changed weight by Culvan! Works for containers now *
  if (generic_obj->obj_flags.weight != obj->obj_flags.weight) obj_match = 0;
  * End of hack */

  if (generic_obj->obj_flags.cost != obj->obj_flags.cost) obj_match = 0;
  if (generic_obj->obj_flags.cost_per_day !=
      obj->obj_flags.cost_per_day) obj_match = 0;
  if (generic_obj->obj_flags.timer != obj->obj_flags.timer) obj_match = 0;

  if (generic_obj->obj_flags.bitvector != 
      obj->obj_flags.bitvector) obj_match = 0;
  if (generic_obj->obj_flags.bitvector2 !=
      obj->obj_flags.bitvector2) obj_match = 0;

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    if (generic_obj->affected[j].location != obj->affected[j].location)
      obj_match = 0;
    if (generic_obj->affected[j].modifier != obj->affected[j].modifier)
      obj_match = 0;
  }
  extract_obj(generic_obj);

  /* lights are never put back... worn_on 0 */
  /* HACKED by Culvan...special case for lights! */
  if (obj_match) {
    if ((obj->worn_on == 0) || (obj->worn_on == -1)) {
      if (!obj->worn_by) {
        fprintf(fl, "G %5d\t\t\tgive %s\n", GET_OBJ_VNUM(obj), 
          obj->short_description);
      } else {
        fprintf(fl, "L %5d\t\t\tlight %s\n", GET_OBJ_VNUM(obj), 
          obj->short_description);        
      }
    } else {
      fprintf(fl, "E %5d %2d\t\t%s, %s\n", GET_OBJ_VNUM(obj), obj->worn_on,
        obj->short_description, equipment_types[obj->worn_on]);
    }
  } else /* the object is different from the generic */ {
    fprintf(fl, "A %5d %2d\t\t", GET_OBJ_VNUM(obj), obj->worn_on > 0 ?
      obj->worn_on : (obj->worn_by ? -2 : -1) );
    if ((obj->worn_on == 0) || (obj->worn_on == -1)) {
      fprintf(fl, "give %s (affected)...\n", obj->short_description);
    } else {
      fprintf(fl, "%s (affected), %s...\n", obj->short_description,
        equipment_types[obj->worn_on]);
    }
/* HACKED to save AC correctly */
    fprintf(fl, "%d ", obj->obj_flags.value[0]);
    if (obj->obj_flags.value[0] == APPLY_AC)
      fprintf(fl, "%d ", obj->obj_flags.value[1] * 10);
    else
      fprintf(fl, "%d ", obj->obj_flags.value[1]);
    fprintf(fl, "%d ", obj->obj_flags.value[2]);
    if (obj->obj_flags.value[2] == APPLY_AC)
      fprintf(fl, "%d ", obj->obj_flags.value[3] * 10);
    else
      fprintf(fl, "%d ", obj->obj_flags.value[3]);
/* end of hack */
    fprintf(fl, "%d %d %d ",
      obj->obj_flags.extra_flags, obj->obj_flags.weight,
      obj->obj_flags.timer
    );

    fprintf(fl, "%ld ", obj->obj_flags.bitvector);

    /* bitvector2 and additional bitvectors go after the affects */
    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      fprintf(fl, "%u %u ", obj->affected[j].location,
        obj->affected[j].modifier);
    }

    /* all bitvector2's and more have to go onto the end of the affects line */
    fprintf(fl, "%ld ", obj->obj_flags.bitvector2);

    fprintf(fl, "\n");
  }
  /* HACKED to save renamed objects! */
  if (strcmp(obj->description, generic_obj->description))
    fprintf(fl, "D %s\n", obj->description);
  if (strcmp(obj->name, generic_obj->name))
    fprintf(fl, "N %s\n", obj->name);
  if (strcmp(obj->short_description, generic_obj->short_description))
    fprintf(fl, "S %s\n", obj->short_description);
  /* end of rename hack */
/* end of hack */
  return 1;
}



int Crash_delete_file(char *name)
{
  char filename[50];
  FILE *fl;

  if (!get_filename(name, filename, CRASH_FILE))
    return 0;
  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT) {	/* if it fails but NOT because of no file */
      sprintf(buf1, "SYSERR: deleting crash file %s (1)", filename);
      perror(buf1);
    }
    return 0;
  }
  fclose(fl);

  if (unlink(filename) < 0) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: deleting crash file %s (2)", filename);
      perror(buf1);
    }
  }
  return (1);
}


int Crash_delete_crashfile(struct char_data * ch)
{
  char fname[MAX_INPUT_LENGTH];
  char line[256];
  struct rent_info rent;
  FILE *fl;

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
    return 0;
  if (!(fl = fopen(fname, "rb"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: checking for crash file %s (3)", fname);
      perror(buf1);
    }
    return 0;
  }
  if (!feof(fl))
/* HACKED to read from an ascii object file */
/* old code 
    fread(&rent, sizeof(struct rent_info), 1, fl);
*/
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d", &rent.time, &rent.rentcode,
        &rent.net_cost_per_diem, &rent.gold, &rent.account, &rent.nitems);
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &rent.spare0, &rent.spare1,
        &rent.spare2, &rent.spare3, &rent.spare4, &rent.spare5, &rent.spare6,
        &rent.spare7);
  }
/* end of hack */
  fclose(fl);

  if (rent.rentcode == RENT_CRASH)
    Crash_delete_file(GET_NAME(ch));

  return 1;
}


int Crash_clean_file(char *name)
{
  char fname[MAX_STRING_LENGTH], filetype[20];
  char line[256];
  struct rent_info rent;
  extern int rent_file_timeout, crash_file_timeout;
  FILE *fl;

  if (!get_filename(name, fname, CRASH_FILE))
    return 0;
  /*
   * open for write so that permission problems will be flagged now, at boot
   * time.
   */
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: OPENING OBJECT FILE %s (4)", fname);
      perror(buf1);
    }
    return 0;
  }
  if (!feof(fl))
/* HACKED to read from an ascii object file */
/* old code
    fread(&rent, sizeof(struct rent_info), 1, fl);
*/
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d", &rent.time, &rent.rentcode,
        &rent.net_cost_per_diem, &rent.gold, &rent.account, &rent.nitems);
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &rent.spare0, &rent.spare1,
        &rent.spare2, &rent.spare3, &rent.spare4, &rent.spare5, &rent.spare6,
        &rent.spare7);
  }
/* end of hack */                                                              
  fclose(fl);

  if ((rent.rentcode == RENT_CRASH) ||
      (rent.rentcode == RENT_FORCED) || (rent.rentcode == RENT_TIMEDOUT)) {
    if (rent.time < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      switch (rent.rentcode) {
      case RENT_CRASH:
	strcpy(filetype, "crash");
	break;
      case RENT_FORCED:
	strcpy(filetype, "forced rent");
	break;
      case RENT_TIMEDOUT:
	strcpy(filetype, "idlesave");
	break;
      default:
	strcpy(filetype, "UNKNOWN!");
	break;
      }
      sprintf(buf, "   Deleting %s's %s file.", name, filetype);
      log(buf);
      return 1;
    }
    /* Must retrieve rented items w/in 30 days */
  } else if (rent.rentcode == RENT_RENTED)
    if (rent.time < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      sprintf(buf, "   Deleting %s's rent file.", name);
      log(buf);
      return 1;
    }
  return (0);
}



void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    Crash_clean_file((player_table + i)->name);
  return;
}



void Crash_listrent(struct char_data * ch, char *name)
{
  FILE *fl;
  char fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  char line[256];
  char equip_command;
  int tmp;
  struct obj_file_elem object;
  struct obj_data *obj;
  struct rent_info rent;

  if (!get_filename(name, fname, CRASH_FILE))
    return;
  if (!(fl = fopen(fname, "rb"))) {
    sprintf(buf, "%s has no rent file.\r\n", name);
    send_to_char(buf, ch);
    return;
  }
  sprintf(buf, "%s\r\n", fname);
  if (!feof(fl))
/* HACKED to read from an ascii object file */
/* old code
    fread(&rent, sizeof(struct rent_info), 1, fl);                             
*/
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d", &rent.time, &rent.rentcode,
        &rent.net_cost_per_diem, &rent.gold, &rent.account, &rent.nitems);
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &rent.spare0, &rent.spare1,
        &rent.spare2, &rent.spare3, &rent.spare4, &rent.spare5, &rent.spare6,
        &rent.spare7);
  }
/* end of hack */

  switch (rent.rentcode) {
  case RENT_RENTED:
    strcat(buf, "Rent\r\n");
    break;
  case RENT_CRASH:
    strcat(buf, "Crash\r\n");
    break;
  case RENT_CRYO:
    strcat(buf, "Cryo\r\n");
    break;
  case RENT_TIMEDOUT:
  case RENT_FORCED:
    strcat(buf, "TimedOut\r\n");
    break;
  default:
    strcat(buf, "Undef\r\n");
    break;
  }
/* HACKED to read objects from an ascii object file,
   it does not however yet save affects put onto objects */
/* old code
  while (!feof(fl)) {
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
    if (ferror(fl)) {
      fclose(fl);
      return;
    }
    if (!feof(fl))
      if (real_object(object.item_number) > -1) {
	obj = read_object(object.item_number, VIRTUAL);
	sprintf(buf, "%s [%5d] (%5dau) %-20s\r\n", buf,
		object.item_number, GET_OBJ_RENT(obj),
		obj->short_description);
	extract_obj(obj);
      }
  }
*/
/* new code to read in the objects */
  while (!feof(fl)) {
    get_line(fl, line);
    if (feof(fl))
      break;
    sscanf(line, "%c", &equip_command);
    switch (equip_command) {
      case 'G':
            sscanf(line, "G %d", &tmp);
            break;
      case 'E':
            sscanf(line, "E %d", &tmp);
            break;
      case 'L':
            sscanf(line, "L %d", &tmp);
            break;
      case 'A':
            sscanf(line, "A %d", &tmp);
            get_line(fl, line);
            break;
      default:
            tmp = -1;
            break;
    }
    object.item_number = tmp;
    if (real_object(object.item_number) > -1) {
      obj = read_object(object.item_number, VIRTUAL);
      sprintf(buf, "%s [%5d] (%5dau) %-20s\r\n", buf,
            object.item_number, GET_OBJ_RENT(obj),
            obj->short_description);
      extract_obj(obj);
    }
  }
/* end of hack */
  send_to_char(buf, ch);
  fclose(fl);
}



int Crash_write_rentcode(struct char_data * ch, FILE * fl, struct rent_info * rent)
{
/* HACKED to write ascii object files */
/* old code 
  if (fwrite(rent, sizeof(struct rent_info), 1, fl) < 1) {
    perror("Writing rent code.");
    return 0;
  }
*/
  fprintf(fl, "* %s object file\n", GET_NAME(ch));
  fprintf(fl, "* Rent Info\n");
  fprintf(fl, "%d %d %d %d %d 0\n", rent->time, rent->rentcode,
        rent->net_cost_per_diem, rent->gold, rent->account);
        /* rent->nitems would normally be the last value on the line */
  /* spare values all zeroed out */
  /*
  fprintf(fl, "%d %d %d %d %d %d %d %d\n", rent->spare0, rent->spare1,
        rent->spare2, rent->spare3, rent->spare4, rent->spare5, rent->spare6,
        rent->spare7);
  */
  fprintf(fl, "0 0 0 0 0 0 0 0\n");
  fprintf(fl, "* Objects\n");
/* end of hack */
  return 1;
}



/***************************************************************************\
*  In a fit of madness I decided it would be a good idea to hack this code  *
*  so that objects would save INSIDE of containers. This code is butt-ugly. *
*  I ripped out all the commented sections in a new copy, and tweaked it.   *
*                                   - Culvan                                *
\***************************************************************************/
#if (0)
int Crash_load(struct char_data * ch)
/* return values:
	0 - successful load, keep char in rent room.
	1 - load failure or load of crash items -- put char in temple.
	2 - rented equipment lost (no $)
*/
{
  void Crash_crashsave(struct char_data * ch);

  FILE *fl;

  char fname[MAX_STRING_LENGTH];
  char line[256];
  char equip_command;
  int item_number, where;
  struct obj_data *obj;
  int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  long tmp8, tmp21;
  int tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17, tmp18,
        tmp19, tmp20;
  struct obj_file_elem object;
  struct rent_info rent;
  int orig_rent_code;
  int n;
  
  extern const char *potion_names[][3];


/* HACKED to make rent really free */
/*
  int cost;
  float num_of_days;
*/
/* end of hack */

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
    return 1;
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: READING OBJECT FILE %s (5)", fname);
      perror(buf1);
      send_to_char("\r\n********************* NOTICE *********************\r\n"
		   "There was a problem loading your objects from disk.\r\n"
		   "Contact a God for assistance.\r\n", ch);
    }
    sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    return 1;
  }
  if (!feof(fl))
/* HACKED to read from an ascii object file */
/* old code
    fread(&rent, sizeof(struct rent_info), 1, fl);
*/
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d", &rent.time, &rent.rentcode,
        &rent.net_cost_per_diem, &rent.gold, &rent.account, &rent.nitems);
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &rent.spare0, &rent.spare1,
        &rent.spare2, &rent.spare3, &rent.spare4, &rent.spare5, &rent.spare6,
        &rent.spare7);
  }
/* end of hack */
/* hacked to not charge rent - Scott */
/*
  if (rent.rentcode == RENT_RENTED || rent.rentcode == RENT_TIMEDOUT) {
    num_of_days = (float) (time(0) - rent.time) / SECS_PER_REAL_DAY;
    cost = (int) (rent.net_cost_per_diem * num_of_days);
    if (cost / 100 > (GET_GOLD(ch) + GET_BANK_GOLD(ch) )) {
      fclose(fl);
      sprintf(buf, "%s entering game, rented equipment lost (no $).",
	      GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      Crash_crashsave(ch);
      return 2;
    } else {
      GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
      GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
      save_char(ch, NOWHERE);
    }
  }
*/
  switch (orig_rent_code = rent.rentcode) {
  case RENT_RENTED:
    sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRASH:
    sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRYO:
    sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  default:
    sprintf(buf, "WARNING: %s entering game with undefined rent code.", GET_NAME(ch));
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  }
  sprintf(buf, "%s entering game.", GET_NAME(ch)); 
  clanlog(buf, ch);

  while (!feof(fl)) {
/* HACKED to read ascii object files */
/* old code 
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
*/
    get_line(fl, line);
    if (feof(fl))
      break;
    sscanf(line, "%c", &equip_command);
    switch (equip_command) {
      case 'G':
           sscanf(line, "G %d", &item_number);
           where = -1;
           break;
      case 'E':
           sscanf(line, "E %d %d", &item_number, &where);
           break;
      case 'L':
           sscanf(line, "L %d", &item_number);
           where = 0;
           break;
      case 'A': /* each affected item uses two lines */
           sscanf(line, "A %d %d", &item_number, &where);
           /* get_line(fl, line); is done a few lines later... */
           if (where == -2) where = 0;
           break;
      case 'C':
           /* Begin a container here! */
           break;
      case 'c':
           /* End a container */
           break;
      case 'D':
           sscanf(line, "D %s", buf);
           free(
      default:
           /* some new load type? */
           break;
    }
    object.item_number = item_number;
/* end of hack */
    if (ferror(fl)) {
      perror("Reading crash file: Crash_load.");
      fclose(fl);
      return 1;
    }
/* HACKED to not just read in the objects with all the affects, this
   needs to be fixed so that objects keep affects in special cases */
/*
    if (!feof(fl))
      obj_to_char(Obj_from_store(object), ch);
*/
    obj = read_object(real_object(object.item_number), REAL);

    /* HACKED to skip loading eq that does not exist */
    /* and read the next line of an 'A' load */
    if (obj == NULL) {
      sprintf(buf, "SYSERR: obj #%d does not exist", object.item_number);
      log(buf);
      if (equip_command == 'A')
        get_line(fl, line);
      continue;
    }
    /* end of hack */
    
    /* HACKED for potions, to set aliasas, names, and descs correctly */
#if(0)
  ************** DEAL WITH THIS LATER ***********
  - check vnum for PROTO_POTION, then look up index, then set these.
  - memory leak here...just a little one. Make the defualt potion
    text really small?
    
    obj->name = str_dup(potion_names[index][2]);
    obj->description = str_dup(potion_names[index][0]);
    obj->short_description = str_dup(potion_names[index][1]);
#endif    
    /* END of hack */

    if (equip_command == 'A') {
      get_line(fl, line);
      /* watch out for MAX_OBJ_AFFECTS!! :( */
    n = sscanf(line, "%d %d %d %d %d %d %d %ld %d %d %d %d %d %d %d %d %d %d %d %d %ld",
        &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7, &tmp8,
        &tmp9, &tmp10, &tmp11, &tmp12, &tmp13, &tmp14, 
        &tmp15, &tmp16, &tmp17, &tmp18, &tmp19, &tmp20, &tmp21);
      obj->obj_flags.value[0] = tmp1;
      obj->obj_flags.value[1] = tmp2;
      obj->obj_flags.value[2] = tmp3;
      obj->obj_flags.value[3] = tmp4;
      obj->obj_flags.extra_flags = tmp5;
      obj->obj_flags.weight = tmp6;
      obj->obj_flags.timer = tmp7;

      if (n > 7)
          obj->obj_flags.bitvector = tmp8;
      else
          obj->obj_flags.bitvector = 0;
      if (n > 20)
          obj->obj_flags.bitvector2 = tmp21;
      else
          obj->obj_flags.bitvector2 = 0;

      obj->affected[0].location = tmp9;
      obj->affected[0].modifier = tmp10;
      obj->affected[1].location = tmp11;
      obj->affected[1].modifier = tmp12;
      obj->affected[2].location = tmp13;
      obj->affected[2].modifier = tmp14;
      obj->affected[3].location = tmp15;
      obj->affected[3].modifier = tmp16;
      obj->affected[4].location = tmp17;
      obj->affected[4].modifier = tmp18;
      obj->affected[5].location = tmp19;
      obj->affected[5].modifier = tmp20;
    }
    if (where == -1) {
      obj_to_char(obj, ch);
    } else {
      equip_char(ch, obj, where);
    }
  }

  /* turn this into a crash file by re-writing the control block */
  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
  rewind(fl);
  Crash_write_rentcode(ch, fl, &rent);

  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return 0;
  else
    return 1;
}
#endif










int Crash_load_objects(FILE * fl, struct char_data * ch, struct obj_data * container) {
  /* return values:
     0 - All is well, rejoice and be glad
     1 - OH NO, didn't work (sucker)
     2 - EOF
     3 - Object does not exist
  */
  char equip_command;
  char line[256];
  int item_number, where;
  struct obj_data *obj;
  struct obj_file_elem object;
  int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  long tmp8, tmp21;
  int tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17, tmp18,
        tmp19, tmp20;
  int load_results;
  int n;

while (!feof(fl)) { 

  get_line(fl, line);
  if (feof(fl))
    return 2;
  sscanf(line, "%c", &equip_command);
  switch (equip_command) {
    case 'G':
         sscanf(line, "G %d", &item_number);
         where = -1;
         break;
    case 'E':
         sscanf(line, "E %d %d", &item_number, &where);
         break;
    case 'L':
         sscanf(line, "L %d", &item_number);
         where = 0;
         break;
    case 'A': /* each affected item uses two lines */
         sscanf(line, "A %d %d", &item_number, &where);
         /* get_line(fl, line); is done a few lines later... */
         if (where == -2) where = 0;
         break;
    case 'C':
         /* Begin a container here! */
         if (!last_loaded_obj) {
           log ("Container contents started in rent file with nothing to fill!");
           item_number = -1;
           break;
         }
         if (GET_OBJ_TYPE(last_loaded_obj) != ITEM_CONTAINER) {
           log ("Container contents started in rent file after a non-container!");
           item_number = -1;
           break;
         }
         load_results = Crash_load_objects(fl, ch, last_loaded_obj);
         if (load_results == 2 || load_results == 1) return load_results;
         if (load_results == 3) continue;
         item_number = -1;
         break;
    case 'c':
         if (!container) {
           log ("End of container found OUTSIDE container in rent file!");
           item_number = -1;
           break;  /* DON'T bail out here, or we lose the rest of the rent */
         }
         return 0;
         /* End a container */
         break;
    case 'D':
    	if (!last_loaded_obj) {
    	  log ("Changing obj->description with no last_loaded_obj!");
    	} else {
    	  obj->description = strdup(line+2);
    	}
    	item_number = -1;
        break;
    case 'N':
         if (!last_loaded_obj) {
           log ("Changing obj->name with no last_loaded_obj!");
         } else {
           obj->name = strdup(line+2);
         }
         item_number = -1;
         break;
    case 'S':
         if (!last_loaded_obj) {
           log ("Changing obj->short_description with no last_loaded_obj!");
         } else {
           obj->short_description = strdup(line+2);
         }
         item_number = -1;
         break;
    default:
         /* some new load type? */
         sprintf(buf, "Unkown rent load type '%c' in %s.objs!", equip_command,
                 GET_NAME(ch));
         log(buf);
         break;
  }
  if (item_number > -1) {
    object.item_number = item_number;
    if (ferror(fl)) {
      perror("Reading crash file: Crash_load.");
      fclose(fl);
      return 1;
    }
    obj = read_object(real_object(object.item_number), REAL);
    /* HACKED to skip loading eq that does not exist */
    /* and read the next line of an 'A' load */
    if (obj == NULL) {
      sprintf(buf, "SYSERR: obj #%d does not exist", object.item_number);
      log(buf);
      if (equip_command == 'A')
        get_line(fl, line);
      /* Actually, this SHOULDN'T exit...keep trying! */
      /* return 3; */
      continue;
    }
    /* end of hack */
    
/*    last_loaded_obj = (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) ? obj : NULL; */
    last_loaded_obj = obj;

    if (equip_command == 'A') {
      get_line(fl, line);
      /* watch out for MAX_OBJ_AFFECTS!! :( */
      n = sscanf(line, "%d %d %d %d %d %d %d %ld %d %d %d %d %d %d %d %d %d %d %d %d %ld",
          &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7, &tmp8,
          &tmp9, &tmp10, &tmp11, &tmp12, &tmp13, &tmp14, 
          &tmp15, &tmp16, &tmp17, &tmp18, &tmp19, &tmp20, &tmp21);
      obj->obj_flags.value[0] = tmp1;
      obj->obj_flags.value[1] = tmp2;
      obj->obj_flags.value[2] = tmp3;
      obj->obj_flags.value[3] = tmp4;
      obj->obj_flags.extra_flags = tmp5;
/* HACKED by Culvan...weight won't change (will it?) and this screws up
   containers */
/*      obj->obj_flags.weight = tmp6; */
      obj->obj_flags.timer = tmp7;

      if (n > 7)
          obj->obj_flags.bitvector = tmp8;
      else
          obj->obj_flags.bitvector = 0;
      if (n > 20)
          obj->obj_flags.bitvector2 = tmp21;
      else
          obj->obj_flags.bitvector2 = 0;

      obj->affected[0].location = tmp9;
      obj->affected[0].modifier = tmp10;
      obj->affected[1].location = tmp11;
      obj->affected[1].modifier = tmp12;
      obj->affected[2].location = tmp13;
      obj->affected[2].modifier = tmp14;
      obj->affected[3].location = tmp15;
      obj->affected[3].modifier = tmp16;
      obj->affected[4].location = tmp17;
      obj->affected[4].modifier = tmp18;
      obj->affected[5].location = tmp19;
      obj->affected[5].modifier = tmp20;
    }
    if (where == -1) {
      if (container)
        obj_to_obj(obj, container);
      else
        obj_to_char(obj, ch);
    } else {
      equip_char(ch, obj, where);
    }
  }
}
  return 0;
};  



int Crash_load(struct char_data * ch)
/* return values:
	0 - successful load, keep char in rent room.
	1 - load failure or load of crash items -- put char in temple.
	2 - rented equipment lost (no $)
*/
{
  void Crash_crashsave(struct char_data * ch);

  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  char line[256];
  struct rent_info rent;
  int orig_rent_code;
  int load_one;
  
  last_loaded_obj = NULL;

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
    return 1;
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: READING OBJECT FILE %s (5)", fname);
      perror(buf1);
      send_to_char("\r\n********************* NOTICE *********************\r\n"
		   "There was a problem loading your objects from disk.\r\n"
		   "Contact a God for assistance.\r\n", ch);
    }
    sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    return 1;
  }
  if (!feof(fl))
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d", &rent.time, &rent.rentcode,
        &rent.net_cost_per_diem, &rent.gold, &rent.account, &rent.nitems);
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &rent.spare0, &rent.spare1,
        &rent.spare2, &rent.spare3, &rent.spare4, &rent.spare5, &rent.spare6,
        &rent.spare7);
  }

  switch (orig_rent_code = rent.rentcode) {
  case RENT_RENTED:
    sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRASH:
    sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRYO:
    sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  default:
    sprintf(buf, "WARNING: %s entering game with undefined rent code.", GET_NAME(ch));
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  }
  sprintf(buf, "%s entering game.", GET_NAME(ch)); 
  clanlog(buf, ch);

  load_one = Crash_load_objects(fl, ch, NULL);
/*  if (load_one == 2) return 1; */
  if (load_one == 1) return 1;

  /* turn this into a crash file by re-writing the control block */
  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
  rewind(fl);
  Crash_write_rentcode(ch, fl, &rent);

  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return 0;
  else
    return 1;
}










/*int Crash_save(struct obj_data * obj, FILE * fp)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->contains, fp);
    Crash_save(obj->next_content, fp);
    result = Obj_to_store(obj, fp);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return 0;
  }
  return TRUE;
}*/


/* New hacked version by Culvan to save objects in containers */
int Crash_save(struct obj_data * obj, FILE * fp)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    result = Obj_to_store(obj, fp);
    if (obj->contains) {
      fprintf(fp, "C\t\t\tPut in above container:\n");
      Crash_save(obj->contains, fp);
      fprintf(fp, "c\t\t\tEnd container contents.\n");
    }
    Crash_save(obj->next_content, fp);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return 0;
  }
  return TRUE;
}


void Crash_restore_weight(struct obj_data * obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}



void Crash_extract_objs(struct obj_data * obj)
{
  if (obj) {
    Crash_extract_objs(obj->contains);
    Crash_extract_objs(obj->next_content);
    extract_obj(obj);
  }
}


int Crash_is_unrentable(struct obj_data * obj)
{
  if (!obj)
    return 0;

  if (IS_OBJ_STAT(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0 ||
      GET_OBJ_RNUM(obj) <= NOTHING || GET_OBJ_TYPE(obj) == ITEM_KEY)
    return 1;

  return 0;
}


void Crash_extract_norents(struct obj_data * obj)
{
  if (obj) {
    Crash_extract_norents(obj->contains);
    Crash_extract_norents(obj->next_content);
    if (Crash_is_unrentable(obj))
      extract_obj(obj);
  }
}



/* HACKED to cut the rent costs */
void Crash_extract_expensive(struct obj_data * obj)
{
  struct obj_data *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
    if (GET_OBJ_RENT(tobj) / RENT_ADJUST > GET_OBJ_RENT(max))
      max = tobj;
  extract_obj(max);
}



/* HACKED to cut the rent costs */
void Crash_calculate_rent(struct obj_data * obj, int *cost)
{
  if (obj) {
    *cost += MAX(0, GET_OBJ_RENT(obj) / RENT_ADJUST);
    Crash_calculate_rent(obj->contains, cost);
    Crash_calculate_rent(obj->next_content, cost);
  }
}


void Crash_crashsave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
    return;
  if (!(fp = fopen(buf, "wb")))
    return;

  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }
  Crash_restore_weight(ch->carrying);
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if (ch->equipment[j]) {
      if (!Crash_save(ch->equipment[j], fp)) {
	fclose(fp);
	return;
      }
      Crash_restore_weight(ch->equipment[j]);
    }

  fclose(fp);
  REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}


void Crash_idlesave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  int cost;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
    return;
  if (!(fp = fopen(buf, "wb")))
    return;

  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if (ch->equipment[j])
      obj_to_char(unequip_char(ch, j), ch);
  Crash_extract_norents(ch->carrying);

  cost = 0;
  Crash_calculate_rent(ch->carrying, &cost);
  cost <<= 1;			/* forcerent cost is 2x normal rent */
  while ((cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) && ch->carrying) {
    Crash_extract_expensive(ch->carrying);
    cost = 0;
    Crash_calculate_rent(ch->carrying, &cost);
    cost <<= 1;
  }

  if (!ch->carrying) {
    fclose(fp);
    Crash_delete_file(GET_NAME(ch));
    return;
  }
  rent.net_cost_per_diem = cost;

  rent.rentcode = RENT_TIMEDOUT;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


void Crash_rentsave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
    return;
  if (!(fp = fopen(buf, "wb")))
    return;

/* HACKED to adjust the order these checks are done for ascii save file */
/* old code 
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if (ch->equipment[j])
      obj_to_char(unequip_char(ch, j), ch);
*/
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if ((ch->equipment[j]) && (Crash_is_unrentable(ch->equipment[j])))
      obj_to_char(unequip_char(ch, j), ch);
/* end of hack */
  Crash_extract_norents(ch->carrying);

  rent.net_cost_per_diem = cost;
  rent.rentcode = RENT_RENTED;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }
  for (j = 0; j < GET_NUM_WEARS(ch); j++)                                              
    if (ch->equipment[j]) {
      if (!Crash_save(ch->equipment[j], fp)) {
        fclose(fp);
        return;
      }
    }
  fclose(fp);

/* HACKED to unequip really fast */
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if (ch->equipment[j])
      obj_to_char(unequip_char(ch, j), ch);
/* end of hack */
  Crash_extract_objs(ch->carrying);
}


void Crash_cryosave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
    return;
  if (!(fp = fopen(buf, "wb")))
    return;

/* HACKED to change the order a little for ascii object files */
/* old code 
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if (ch->equipment[j])
      obj_to_char(unequip_char(ch, j), ch);
*/
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if ((ch->equipment[j]) && (Crash_is_unrentable(ch->equipment[j])))
      obj_to_char(unequip_char(ch, j), ch);
/* end of hack */
  Crash_extract_norents(ch->carrying);

  GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

  rent.rentcode = RENT_CRYO;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  rent.net_cost_per_diem = 0;
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if (ch->equipment[j]) {
      if (!Crash_save(ch->equipment[j], fp)) {
        fclose(fp);
        return;
      }
    }                                                                          
  fclose(fp);

/* HACKED to unequip really fast */
  for (j = 0; j < GET_NUM_WEARS(ch); j++)
    if (ch->equipment[j])
      obj_to_char(unequip_char(ch, j), ch);
/* end of hack */
  Crash_extract_objs(ch->carrying);
  SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
}


/* ************************************************************************
* Routines used for the receptionist					  *
************************************************************************* */

void Crash_rent_deadline(struct char_data * ch, struct char_data * recep,
			      long cost)
{
/* HACKED to make rent free */
/*
  long rent_deadline;
*/

  if (!cost)
    return;

/* HACKED to put in free rent */
/*
  rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
  sprintf(buf,
      "$n tells you, 'You can rent for %ld day%s with the gold you have\r\n"
	  "on hand and in the bank.'\r\n",
	  rent_deadline, (rent_deadline > 1) ? "s" : "");
*/
  sprintf(buf, "$n tells you, 'You can rent forever in Kore.\r\n");
  act(buf, FALSE, recep, 0, ch, TO_VICT);
}



int Crash_report_unrentables(struct char_data * ch, struct char_data * recep,
			         struct obj_data * obj)
{
  char buf[128];
  int has_norents = 0;

  if (obj) {
    if (Crash_is_unrentable(obj)) {
      has_norents = 1;
      sprintf(buf, "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
      act(buf, FALSE, recep, 0, ch, TO_VICT);
    }
    has_norents += Crash_report_unrentables(ch, recep, obj->contains);
    has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
  }
  return (has_norents);
}



/* HACKED to make rent free but check unrentables */
void Crash_report_rent(struct char_data * ch, struct char_data * recep,
		            struct obj_data * obj, long *cost, long *nitems, int display, int factor)
{
/* HACKED to make rent only 100 gold a day */
/* this hack disabled */
/*
  static char buf[256];
*/

  if (obj) {
    if (!Crash_is_unrentable(obj)) {
      (*nitems)++;
/* old code disabled */
/*
      *cost += MAX(0, (GET_OBJ_RENT(obj) / RENT_ADJUST * factor));
      if (display) {
	sprintf(buf, "$n tells you, '%5d coins for %s..'",
		(GET_OBJ_RENT(obj) / RENT_ADJUST * factor), OBJS(obj, ch));
	act(buf, FALSE, recep, 0, ch, TO_VICT);
      }
*/
/* end of hack */
/* new code */
      *cost = 0;
/* end of hack */
    }
    Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor);
    Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor);
  }
}



int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist,
		         int display, int factor)
{
  extern int max_obj_save;	/* change in config.c */
  char buf[MAX_INPUT_LENGTH];
  int i;
  long totalcost = 0, numitems = 0, norent = 0;

  norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
  for (i = 0; i < GET_NUM_WEARS(ch); i++)
    norent += Crash_report_unrentables(ch, receptionist, ch->equipment[i]);

  if (norent)
    return 0;

/* HACKED to make rent free */
/*
  totalcost = min_rent_cost * factor;
*/

  Crash_report_rent(ch, receptionist, ch->carrying, &totalcost, &numitems, display, factor);

  for (i = 0; i < GET_NUM_WEARS(ch); i++)
    Crash_report_rent(ch, receptionist, ch->equipment[i], &totalcost, &numitems, display, factor);

/*
  if (!numitems) {
    act("$n tells you, 'But you are not carrying anything!  Just quit!'",
	FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
*/

  if (numitems > max_obj_save) {
    sprintf(buf, "$n tells you, 'Sorry, but I cannot store more than %d items.'",
	    max_obj_save);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (display) {
/* HACKED to make rent free */
/* old code disabled */
/*
    sprintf(buf, "$n tells you, 'Plus, my %d coin fee..'",
	    min_rent_cost * factor);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    sprintf(buf, "$n tells you, 'For a total of %ld coins%s.'",
	    totalcost, (factor == RENT_FACTOR ? " per day" : ""));
*/
/* newer code also disabled */
/*
    sprintf(buf, "$n tells you, 'The price of a room is only %d coins,'",
            min_rent_cost * factor);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    if (totalcost > GET_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, receptionist, 0, ch, TO_VICT);
      return (0);
    } else */
/* end of hack */
    if (factor == RENT_FACTOR)
      Crash_rent_deadline(ch, receptionist, totalcost);
  }
/* HACKED to return, instead of totalcost, a 1 */
/*
  return (totalcost);
*/
  return 1;
/* end of hack */
}



int gen_receptionist(struct char_data * ch, struct char_data * recep,
		         int cmd, char *arg, int mode)
{
  struct descriptor_data *d, *next_d;
  int cost = 0;
/* HACKED to put in free rent -- oddly this variable doesnt need checking */
/*
  extern int free_rent;
*/
/* end of hack */
  sh_int save_room;
  char *action_table[] = {"smile", "dance", "sigh", "blush", "burp",
  "cough", "fart", "twiddle", "yawn"};

  ACMD(do_action);

  if (!ch->desc || IS_NPC(ch))
    return FALSE;

  if (!cmd && !number(0, 5)) {
    do_action(recep, "", find_command(action_table[number(0, 8)]), 0);
    return FALSE;
  }
  if (!CMD_IS("offer") && !CMD_IS("rent"))
    return FALSE;

  if (!AWAKE(recep)) {
    send_to_char("She is unable to talk to you...\r\n", ch);
    return TRUE;
  }
  if (!CAN_SEE(recep, ch)) {
    act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
    return TRUE;
  }
/* HACKED to put in free rent */
/*
  if (free_rent) {
    act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
	FALSE, recep, 0, ch, TO_VICT);
    return 1;
  }
*/
  if (CMD_IS("rent")) {
    if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
      return TRUE;
/*
    if (mode == RENT_FACTOR)
      sprintf(buf, "$n tells you, 'Rent will cost you %d gold coins per day.'", cost);
    else if (mode == CRYO_FACTOR)
      sprintf(buf, "$n tells you, 'It will cost you %d gold coins to be frozen.'", cost);
*/
    sprintf(buf, "$n waves to you.");
    act(buf, FALSE, recep, 0, ch, TO_VICT);
/*
    if (cost > GET_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, recep, 0, ch, TO_VICT);
      return TRUE;
    }
*/
    cost = 0;
    if (cost && (mode == RENT_FACTOR))
      Crash_rent_deadline(ch, recep, cost);

/* Dupekill */

    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc)
        continue;
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch))) {
        close_socket(d);
	mudlog("Possible dupe attempt.", NRM, MAX(LVL_IMPL, GET_INVIS_LEV(ch)), TRUE);
      }
    }   

    if (mode == RENT_FACTOR) {
      act("$n stores your belongings and helps you into your private chamber.",
	  FALSE, recep, 0, ch, TO_VICT);
      Crash_rentsave(ch, cost);
      sprintf(buf, "%s has left the game.", GET_NAME(ch));
      clanlog(buf, ch);
    } else {			/* cryo */
      act("$n stores your belongings and helps you into your private chamber.\r\n"
	  "A white mist appears in the room, chilling you to the bone...\r\n"
	  "You begin to lose consciousness...",
	  FALSE, recep, 0, ch, TO_VICT);
      Crash_cryosave(ch, cost);
      sprintf(buf, "%s has left the game.", GET_NAME(ch));
      clanlog(buf, ch);
      SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
    }

    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);
    save_room = ch->in_room;
    extract_char(ch);
    ch->in_room = world[save_room].number;
    save_char(ch, ch->in_room);
  } else {
    Crash_offer_rent(ch, recep, TRUE, mode);
    act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
  }
  return TRUE;
}



SPECIAL(receptionist)
{
  return (gen_receptionist(ch, me, cmd, argument, RENT_FACTOR));
}


 
SPECIAL(cryogenicist)
{
  return (gen_receptionist(ch, me, cmd, argument, CRYO_FACTOR));
}



void Crash_save_all(void)
{
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if ((d->connected == CON_PLAYING) && !IS_NPC(d->character)) {
      if (PLR_FLAGGED(d->character, PLR_CRASH)) {
	Crash_crashsave(d->character);
	save_char(d->character, NOWHERE);
	REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
      }
    }
  }
}
