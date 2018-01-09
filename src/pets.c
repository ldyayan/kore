/*

Use both descriptor_data and char_data

Watch out for:
**  logging in
**  death
**  dt
**  "quit"
**  rent
**  lost link
**  reconnect after lost link
**  usurped
**  quit from menu  - Note: Pet is PERMAMENTLY d/c'd from char - FIXED
**  void (nothing needed here)
**  enter from menu
**  pet death : appears in temple, still with master
**  switching
**  returning from switching
**  magic jar
**  disconnect (close_socket)
**  saving (check desc AND char) (save_char)
  giving eq to pets (bad! bad!)
  
Todo:
  - ispet ifcheck
  - mounts
  - skills
  * goto
  - pets that track you
  * pet location in stat of owner
  - pet levelling
*/

/**************************************************************************
 *  Pets.c                                     Written by Darryl Shpak    *
 *  Heroes of Kore: chaos.exic.net 6000                                   *
 *                                                                        *
 *  Combined with Pets.h, functions and definitions for the handling of   *
 *  individual pets which stay with their owner across reboots and        *
 *  renting, can advance in levels and abilities, and assist their owner  *
 *  in various ways, based on their loyalty                               *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "pets.h"

const struct pet_type_data pet_db[] = {
  {180, MOUNT_NONE},	/* toad */
  {181, MOUNT_NONE},	/* raven */
  {182, MOUNT_NONE},	/* cat */
  {183, MOUNT_NONE},	/* owl */
  {184, MOUNT_NONE},	/* rat */
  {185, MOUNT_NONE},	/* snake */
  {1250, MOUNT_SMALL},	/* dragon toad */
  {5812, MOUNT_NORMAL}, /* bull mastiff */
  {1254, MOUNT_NONE},   /* dog */
  {1251, MOUNT_SMALL}, /* baby tiamat */
  {1253, MOUNT_NONE},  /* cabbit */
  {1252, MOUNT_LARGE}, /* zombie dragon */
  {-1, MOUNT_NONE}
};

const int mount_size[] = {
  MOUNT_NORMAL, /* Human */
  MOUNT_NORMAL, /* Elf */
  MOUNT_SMALL,  /* Hobbit */
  MOUNT_SMALL,  /* Dwarf */
  MOUNT_NORMAL, /* Orc */
  MOUNT_NORMAL, /* Drow */
  MOUNT_LARGE,  /* Insect */
  MOUNT_LARGE,  /* Minotaur */
  MOUNT_LARGE,  /* Troll */
  MOUNT_LARGE,  /* Giant */
  MOUNT_NONE,   /* Dragon */
  MOUNT_NORMAL, /* Undead */
  MOUNT_NORMAL, /* Half-elf */
  MOUNT_SMALL,  /* Gnome */
  MOUNT_NONE,   /* Elemental */
  MOUNT_SMALL,  /* Duergar */
  MOUNT_LARGE   /* Thri-kreen */
};

int num_valid_pets;

extern struct char_data *mob_proto;

void boot_pets() {
  int i;
  for (i = 0; pet_db[i].mob_vnum != -1; i++) {
    if (real_mobile(pet_db[i].mob_vnum) == -1) {
      sprintf(buf, "ERROR: Pet number %d has invalid vnum!", i);
      log(buf);
    }
  }
  
  num_valid_pets = i;
}

void pet_load(struct char_data *ch) {
  /* Load a player's pet from disk */
  FILE *fp;
  struct char_data *pet;
  struct descriptor_data *d;
  extern sh_int r_mortal_start_room;
  
  if (!ch) return;
  if (IS_NPC(ch)) return;
  d = ch->desc;
  if (!d) return;  /* Won't load a pet if you've got no desc */
  if (d->original) return;  /* Or if you're switched (!?) */
  
  get_filename(GET_NAME(ch), buf, PET_FILE);
  
  if (!(fp = fopen(buf, "r"))) return; /* no pet */
  
  if (!(pet = pet_from_disk(fp))) {
    sprintf(buf, "Fatal error loading pet from disk for %s", GET_NAME(ch));
    mudlog(buf, NRM, 58, TRUE);
    send_to_char("Ooops! Couldn't load your pet...tell an immortal!\r\n", ch);
    fclose(fp);
    return;
  }
  /* Link them */
  GET_OWNER_DESC(pet) = d;
  GET_OWNER(pet) = ch;
  GET_PET(d) = pet;
  GET_PET(ch) = pet;
  
  if (ch->in_room != NOWHERE) {
    char_to_room(pet, ch->in_room);
    act("$n runs up to you and wags $s tail!", FALSE, pet, NULL, ch, TO_VICT);
    act("$n runs up to $N and wags $s tail!", TRUE, pet, NULL, ch, TO_NOTVICT);
  } else {
    char_to_room(pet, r_mortal_start_room);
    act("$n comes running in, but looks confused.", TRUE, ch, NULL, NULL, TO_ROOM);
    sprintf(buf, "Pet for %s loaded, but owner is NOWHERE.", GET_NAME(ch));
    mudlog(buf, NRM, 58, TRUE);
  }
  
  fclose(fp);
}

void pet_from_char(struct char_data *pet, struct char_data *owner) {
  /* Disconnects pet from owner. */
  if (!pet || !owner) return;
  GET_OWNER(pet) = NULL;
  GET_OWNER_DESC(pet) = NULL;
  GET_PET(owner) = NULL;
  if (owner->desc) GET_PET(owner->desc) = NULL;
  GET_LOYALTY(pet) = 0;
  
  sprintf(buf, "Your beloved pet, %s, leaves you!\r\n", GET_NAME(pet));
  send_to_char(buf, owner);
}

void pet_leave(struct char_data *pet) {
  /* Make a pet leave his/her master */
  struct char_data *owner;
  
  if (!pet) return;
  
  if (!IS_PET(pet)) return;
  owner = GET_OWNER(pet);
  pet_from_char(pet, owner);
}

void pet_lose(struct char_data *owner) {
  /* Your pet leaves you */
  struct char_data *pet;
  
  if (!owner) return;
 
  if (!HAS_PET(owner)) return;
  pet = GET_PET(owner);
  pet_from_char(pet, owner);
}

void new_pet_to_char(struct char_data *pet, struct char_data *ch) {
  /* You get a new pet! */

  if (!pet || !ch) return;
  if (IS_NPC(ch) || !IS_NPC(pet)) return;
  if ((GET_OWNER(pet) == ch) && (GET_PET(ch) == pet)) return;
  if (!ch->desc) return;
  if (ch->desc->original) return;
  
  if (IS_PET(pet)) pet_leave(pet);
  if (HAS_PET(ch)) pet_lose(ch);
  if (pet->master) stop_follower(pet);
  
  GET_PET(ch) = pet;
  GET_PET(ch->desc) = pet;
  GET_OWNER(pet) = ch;
  GET_OWNER_DESC(pet) = ch->desc;
  
  act("$N runs up to you and wags $S tail!", TRUE, ch, NULL, pet, TO_CHAR);
  act("$N runs up to $n, tail wagging!", TRUE, ch, NULL, pet, TO_ROOM);
  if (!pet_sanity_check(pet)) /* Just in case */
    mudlog("   -- in pets.c:new_pet_to_char()", NRM, 58, TRUE);
}

bool pet_sanity_check(struct char_data *pet) {
  /**** Call this to make sure the given pet is stable. Returns TRUE
   **** if all is well, FALSE otherwise. Logs what it finds to file & imms.
   ****/
  struct char_data *ch, *ch_pet, *d_pet;
  struct descriptor_data *pet_d, *ch_d;
  bool Ok;

  if (!pet) {
    mudlog("Sanity check for pet failed : NULL pet!", NRM, 58, TRUE);
    return FALSE;
  }

  if (!IS_PET(pet)) {
    sprintf(buf, "Sanity check for pet %s failed: Not a pet!", GET_NAME(pet));
    mudlog(buf, NRM, 58, TRUE);
    return FALSE;
  }
  
  /* Checks to make sure the pet is OK */
  /* Init some variables */
  ch = GET_OWNER(pet);
  pet_d = GET_OWNER_DESC(pet);
  if (ch) {
    ch_d = ch->desc;
    ch_pet = GET_PET(ch);
  } else {
    ch_d = NULL;
    ch_pet = NULL;
  }
  if (ch_d) d_pet = GET_PET(ch_d);
  
  /* Do the actual check */
  Ok = TRUE;
  if (ch) if (ch_pet != pet) Ok = FALSE;
  if (ch_d) if (d_pet != pet) Ok = FALSE;
  if (ch) if (ch_d != pet_d) Ok = FALSE;
  if (!ch_d && pet_d) if (pet_d->pet != pet) Ok = FALSE;
  if (!pet->petdata) Ok = FALSE;
  
  if (!Ok) {
    /* If something's wrong, log it! */
    sprintf(buf, "Sanity check for pet %s failed!", GET_NAME(pet));
    mudlog(buf, NRM, 58, TRUE);

    if (ch)
      sprintf(buf, "   Apparant owner is %s", GET_NAME(ch));
    else
      sprintf(buf, "   Owner unknown!");

    mudlog(buf, NRM, 58, TRUE);
    
    sprintf(buf, "   ch->pet == pet: %s   ch->desc = GET_OWNER_DESC(pet): %s",
            YESNO(ch_pet == pet), YESNO(ch_d == pet_d));
    mudlog(buf, NRM, 58, TRUE);
    if (ch_d) {
      sprintf(buf, "   ch->desc->pet == pet: %s", YESNO(d_pet == pet));
      mudlog(buf, NRM, 58, TRUE);
    }
    if (!ch_d && pet_d) {
      sprintf(buf, "   pet->ownerd->pet == pet: %s", YESNO(pet_d->pet == pet));
      mudlog(buf, NRM, 58, TRUE);
    }
    sprintf(buf, "   ch->petdata exists : %s", YESNO(pet->petdata));
    mudlog(buf, NRM, 58, TRUE);
    sprintf(buf, "   GET_OWNER_DESC(pet) exists: %s", YESNO(pet->petdata));
    mudlog(buf, NRM, 58, TRUE);
  }
  
  return Ok;
}

ACMD(do_petcontrol) {
  /* This command handles all the pet-based commands. These include:
       grant <player> <pet>		: Give pet to player
       check <pet>			: Performs sanity check on pet
  */
  struct char_data *owner, *pet;
  int petnum, vnum, i;
  
  if (IS_NPC(ch)) return;    /* Don't goof around */
  
  argument = one_argument(argument, arg);
  
  if (!strcmp(arg, "grant")) {
    /****** petcontrol grant ******/
    
    argument = one_argument(argument, buf2);
    argument = one_argument(argument, buf);
    if (!*buf2 || !*buf) {
      send_to_char("Syntax: petcontrol grant <player> <pet>\r\n", ch);
      return;
    }
    if (!(owner = get_char_room_vis(ch, buf2))) {
      send_to_char("No one by that name here!\r\n", ch);
      return;
    }
    petnum = atoi(buf) - 1;
    if (petnum < 0 || petnum >= num_valid_pets) {
      send_to_char("Invalid pet number!\r\n", ch);
      return;
    }
    if (IS_NPC(owner)) {
      send_to_char("The owner has to be a player!\r\n", ch);
      return;
    }
    if (!ch->desc) {
      send_to_char("The owner must be connected to grant a pet!\r\n", ch);
      return;
    }
/*    if (!IS_NPC(pet)) {
      send_to_char("Only mobs can be pets, not other players!\r\n", ch);
      return;
    }*/
    if (ch->desc->original) {
      send_to_char("You can't give a pet to someone that's switched!\r\n", ch);
      return;
    }
    /* pant pant...ok looks good now, I hope I didn't miss any checks */
    
    /* Make the pet */
    pet = read_mobile(pet_db[petnum].mob_vnum, VIRTUAL);
    CREATE(pet->petdata, struct pet_specials, 1);
    GET_LOYALTY(pet) = 50;
    GET_PETNUM(pet) = petnum;
    for (i = 0; i < MAX_PET_SKILLS; i++) {
      GET_PET_SKILL(pet, i) = 0;
    }
    
    char_to_room(pet, owner->in_room);
    new_pet_to_char(pet, owner);

  } else if (!strcmp(arg, "remove")) {
    /****** petcontrol remove ******/
    
    argument = one_argument(argument, buf2);
    argument = one_argument(argument, buf);
    if (!*buf2 || !*buf) {
      send_to_char("Syntax: petcontrol remove <player> <pet>\r\n", ch);
      return;
    }
    if (!(owner = get_char_room_vis(ch, buf2))) {
      send_to_char("No one by that name here!\r\n", ch);
      return;
    }
    if (!ch->desc) {
      send_to_char("The owner must be connected to grant a pet!\r\n", ch);
      return;
    }
    if (ch->desc->original) {
      send_to_char("You can't remove a pet from someone that's switched!\r\n", ch);
      return;
    }

    if (HAS_PET(ch)) pet_lose(ch);

  } else if (!strcmp(arg, "check")) {
    /****** petcontrol check *******/
    argument = one_argument(argument, buf);
    if (!*buf) {
      send_to_char("Check who?\r\n", ch);
      return;
    }

    pet = get_char_room_vis(ch, buf);

    if (!pet) {
      sprintf(buf2, "Can't find anyone named %s!\r\n", buf);
      send_to_char(buf2, ch);
      return;
    }
    
    if (!IS_PET(pet)) {
      sprintf(buf2, "Sorry, %s isn't a pet!\r\n", GET_NAME(pet));
      send_to_char(buf2, ch);
      return;
    }
    
    if (pet_sanity_check(pet)) {
      send_to_char("All is well.\r\n", ch);
    } else {
      sprintf(buf2, "   -- check requested by %s", GET_NAME(ch));
      mudlog(buf2, NRM, 58, TRUE);
      sprintf(buf2, "Sanity check on %s failed!\r\n", GET_NAME(pet));
      send_to_char(buf2, ch);
    }

  } else if (!strcmp(arg, "list")) {
    /******* petcontrol list ********/
    *buf = '\0';
    for (i = 0; i < num_valid_pets; i++) {
      vnum = pet_db[i].mob_vnum;
      sprintf(buf, "%s#%-2d : %-5d  %s\r\n", buf, i+1, vnum,
              GET_NAME(&(mob_proto[real_mobile(vnum)])));
    }
    send_to_char(buf, ch);
  } else {
    send_to_char("Valid commands:\r\n"
                 "  grant <player> <pet>\r\n"
                 "  check <pet>\r\n", ch);
  }
}

void extract_pet(struct char_data *pet) {
  int i;
  
  if (!pet) {
    log("Attempt to extract a null pet!");
    return;
  }
  if (!IS_PET(pet)) {
    sprintf(buf, "Attempt to extract non-pet %s via extract_pet()!", GET_NAME(pet));
    log(buf);
    return;
  }
  
  /* Purge inventory */
  while (pet->carrying) extract_obj (pet->carrying);
  for (i = 0; i < NUM_WEARS; i++)
    if (pet->equipment[i]) extract_obj(pet->equipment[i]);
  
  /* Fix the pointers */
  if (GET_OWNER(pet)) GET_PET(GET_OWNER(pet)) = NULL;
  GET_OWNER(pet) = NULL;
  if (GET_OWNER_DESC(pet)) GET_PET(GET_OWNER_DESC(pet)) = NULL;
  GET_OWNER_DESC(pet) = NULL;
  
  /* Get rid of the special pet data! */
  if (pet->petdata) free(pet->petdata);
  
  /* It's no longer a pet, so extract_char will work fine */
  /* Remove the pet from the world */
  extract_char(pet);
}

void save_pet(struct char_data *ch) {
  struct char_data *pet;
  FILE *fp;
  char fname[100];

  *fname = '\0';
  get_filename(GET_NAME(ch), fname, PET_FILE);

  pet = GET_PET(ch);
  if (!pet && ch->desc) pet = GET_PET(ch->desc);
  if (!pet) { /* no pet! */
/*    remove(fname); */
    return;
  }
  
  fp = fopen(fname, "w");
  
  if (!pet_to_disk(pet, fp)) {
    sprintf(buf, "Fatal error saving pet to disk for %s", GET_NAME(ch));
    mudlog(buf, NRM, 58, TRUE);
    send_to_char("Ooops! Couldn't save your pet...tell an immortal!\r\n", ch);
  }
  
  fclose(fp);
}

struct char_data *pet_from_disk(FILE *fp) {
  int petnum, vnum, rnum, i;
  struct char_data *pet;
  int a, b, c, d, e, f, g;
  
  /* First, get the petnum, vnum, and rnum, and make that mob */
  fscanf(fp, "%d\n", &petnum);
  if (petnum >= num_valid_pets || petnum < 0) {
    sprintf(buf, "Invalid pet number #%d in pet file!", petnum);
    mudlog(buf, NRM, 58, TRUE);
    return NULL;
  }
  vnum = pet_db[petnum].mob_vnum;
  if ((rnum = real_mobile(vnum)) == -1) {
    sprintf(buf, "Vnum #%d for pet #%d does not exist!", vnum, petnum);
    mudlog(buf, NRM, 58, TRUE);
    return NULL;
  }
  if (!(pet = read_mobile(rnum, REAL))) {
    sprintf(buf, "Fatal error loading pet #%d (mob #%d)!", petnum, vnum);
    mudlog(buf, NRM, 58, TRUE);
    return NULL;
  }
  
  /* Ok, that went well. The pet's been loaded - create the pet data
     structure and start loading his info */
  CREATE(pet->petdata, struct pet_specials, 1);
  GET_PETNUM(pet) = petnum;

  /* Loyalty and skills */
  fscanf(fp, "%d\n", &GET_LOYALTY(pet));
  for (i = 0; i < MAX_PET_SKILLS; i++) {
    fscanf(fp, "%d ", &GET_PET_SKILL(pet, i));
  }
  fscanf(fp, "\n");

  /* Level, attribs, points, AC/hit/dam, damage dice */
  fscanf(fp, "%d\n", &a);
  GET_LEVEL(pet) = a;
/*  fscanf(fp, "%d %d %d %d %d %d %d\n", &(pet->real_abils.str),
          &(pet->real_abils.str_add), &(pet->real_abils.intel),
          &(pet->real_abils.wis), &(pet->real_abils.dex),
          &(pet->real_abils.con), &(pet->real_abils.cha));*/
  fscanf(fp, "%d %d %d %d %d %d %d\n", &a, &b, &c, &d, &e, &f, &g);
  GET_STR(pet) = a;
  GET_ADD(pet) = b;
  GET_INT(pet) = c;
  GET_WIS(pet) = d;
  GET_DEX(pet) = e;
  GET_CON(pet) = f;
  GET_CHA(pet) = g;
/*  fscanf(fp, "%d %d %d %d %d %d\n", &GET_HIT(pet), &GET_MAX_HIT(pet),
          &GET_MANA(pet), &GET_MAX_MANA(pet), &GET_MOVE(pet),
          &GET_MAX_MOVE(pet));*/
  fscanf(fp, "%d %d %d %d %d %d\n", &a, &b, &c, &d, &e, &f);
  GET_HIT(pet) = a;
  GET_MAX_HIT(pet) = b;
  GET_MANA(pet) = c;
  GET_MAX_MANA(pet) = d;
  GET_MOVE(pet) = e;
  GET_MAX_MOVE(pet) = f;
/*  fscanf(fp, "%d %d %d\n", &GET_AC(pet), &GET_HITROLL(pet), &GET_DAMROLL(pet));
  fscanf(fp, "%d %d\n", &pet->mob_specials.damnodice, &pet->mob_specials.damsizedice);*/
  fscanf(fp, "%d %d %d\n", &a, &b, &c);
  GET_AC(pet) = a; GET_HITROLL(pet) = b; GET_DAMROLL(pet) = c;
  fscanf(fp, "%d %d\n", &a, &b);
  pet->mob_specials.damnodice = a;
  pet->mob_specials.damsizedice = b;

  /* money, exp, align */
  fscanf(fp, "%d\n", &GET_GOLD(pet));
  fscanf(fp, "%d\n", &GET_EXP(pet));
  fscanf(fp, "%d\n", &GET_ALIGNMENT(pet));
  
  return pet;
}

bool pet_to_disk(struct char_data *pet, FILE *fp) {
  /* Assumes the pet is stable! */
  int i;
  
  /* Ok we can deduce most of the info from the mobs vnum. Start
     with the petnum here so we have a vnum to work with */
  fprintf(fp, "%d\n", GET_PETNUM(pet));
  
  /* Loyalty and skills */
  fprintf(fp, "%d\n", GET_LOYALTY(pet));
  for (i = 0; i < MAX_PET_SKILLS; i++) {
    fprintf(fp, "%d ", GET_PET_SKILL(pet, i));
  }
  fprintf(fp, "\n");
  
  /* Level, attribs, points, AC/hit/dam, damage dice */
  fprintf(fp, "%d\n", GET_LEVEL(pet));
  fprintf(fp, "%d %d %d %d %d %d %d\n", GET_STR(pet), GET_ADD(pet),
        GET_INT(pet), GET_WIS(pet), GET_DEX(pet), GET_CON(pet), GET_CHA(pet));
  fprintf(fp, "%d %d %d %d %d %d\n", GET_HIT(pet), GET_MAX_HIT(pet),
        GET_MANA(pet), GET_MAX_MANA(pet), GET_MOVE(pet), GET_MAX_MOVE(pet));
  fprintf(fp, "%d %d %d\n", GET_AC(pet), GET_HITROLL(pet), GET_DAMROLL(pet));
  fprintf(fp, "%d %d\n", pet->mob_specials.damnodice, pet->mob_specials.damsizedice);

  /* non-level related stuff */
  /* money, exp, align */
  fprintf(fp, "%d\n", GET_GOLD(pet));
  fprintf(fp, "%d\n", GET_EXP(pet));
  fprintf(fp, "%d\n", GET_ALIGNMENT(pet));
  
  return TRUE;
}

void stat_pet(struct char_data *ch, struct char_data *k) {
  extern struct room_data *world;
  if (IS_PET(k)) {
    if (GET_OWNER(k)) {
      sprintf(buf, "Is a pet. Owner is %s. Pet #%d. Loyalty: %d.\r\n"
                   "Mounted: %s\r\n",
              GET_NAME(GET_OWNER(k)), GET_PETNUM(k) + 1, GET_LOYALTY(k),
              YESNO(IS_MOUNTED(k)));
    } else {
      if (GET_OWNER_DESC(k)) {
        sprintf(buf, "Is a pet. Owned by connection %d. Pet #%d. Loyalty: %d.\r\n"
              "Mounted: %s\r\n",
              GET_OWNER_DESC(k)->desc_num, GET_PETNUM(k) + 1, GET_LOYALTY(k),
              YESNO(IS_MOUNTED(k)));
      } else {
        /* This can't happen now, but maybe later... */
        sprintf(buf, "Is a pet. OWNER MISSING! Pet #%d. Loyalty: %d.\r\n"
                "Mounted: %s\r\n",
                GET_PETNUM(k) + 1, GET_LOYALTY(k), YESNO(IS_MOUNTED(k)));
        if (!pet_sanity_check(k)) /* Might get useful debug info */
          mudlog("   -- in pets.c:stat_pet()", NRM, 58, TRUE);
      }
    }
    send_to_char(buf, ch);
  } else if (HAS_PET(k)) {
    sprintf(buf, "Has pet #%d: %s. Pet location: %d. Mounted: %s\r\n",
        GET_PETNUM(GET_PET(k)), GET_NAME(GET_PET(k)),
        world[GET_PET(k)->in_room].number, YESNO(IS_MOUNTED(GET_PET(k))));
    send_to_char(buf, ch);
  }
}

bool pet_loyalty_check(struct char_data *pet, int mod) {
  if (!IS_PET(pet)) return FALSE;
  
  /* A positive mod means your chances are better. Loyalty 100 always
     succeeds, loyalty 0 always fails */
  
  return (number(0, 99) - mod < GET_LOYALTY(pet));
}

ACMD(do_mount) {
  struct char_data *pet;
  int petnum;
  
  if (IS_NPC(ch)) {
    send_to_char("Forget it, you can walk!\r\n", ch);
    return;
  }
  if (!HAS_PET(ch)) {
    send_to_char("You don't have anything to ride!\r\n", ch);
    return;
  }
  pet = GET_PET(ch);
  if (IS_MOUNTED(pet)) {
    send_to_char("You're already riding your pet!\r\n", ch);
    return;
  }
  if (pet->in_room != ch->in_room) {
    send_to_char("You need to find your pet first!\r\n", ch);
    return;
  }
  
  petnum = GET_PETNUM(pet);
  if (mount_size[GET_RACE(ch)] == MOUNT_NONE) {
    send_to_char("You can't ride other creatures!\r\n", ch);
    return;
  }
  if (pet_db[petnum].mountsize == MOUNT_NONE) {
    sprintf(buf, "Don't be silly, you can't ride %s!\r\n", GET_NAME(pet));
    send_to_char(buf, ch);
    return;
  }
  if (mount_size[GET_RACE(ch)] > pet_db[petnum].mountsize) {
    act("You can't ride $s, $e's far too large!", FALSE, pet, NULL, ch, TO_VICT);
    return;
  }
  if (mount_size[GET_RACE(ch)] < pet_db[petnum].mountsize) {
    act("You can't ride $s, you'd crush $m!", FALSE, pet, NULL, ch, TO_VICT);
    return;
  }

  act("You jump up on $N.", FALSE, ch, NULL, pet, TO_CHAR);
  act("$n jumps up on $N.", FALSE, ch, NULL, pet, TO_NOTVICT);
  
  IS_MOUNTED(pet) = TRUE;
}

void perform_unmount(struct char_data *ch) {
  struct char_data *pet;
  
  if (!(pet = GET_PET(ch))) return;
  
  if (!IS_MOUNTED(pet)) return;
  
  IS_MOUNTED(pet) = FALSE;
  act("You get off of $N.", FALSE, ch, NULL, pet, TO_CHAR);
  act("$n gets off of $N.", TRUE, ch, NULL, pet, TO_NOTVICT);
  GET_POS(ch) = POS_STANDING;
}
