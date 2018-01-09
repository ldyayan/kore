/*****************************************************************
 *    Code that allows mortals to create various items.          *
 *****************************************************************/
 
#include <stdio.h>
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

extern struct index_data *obj_index;
extern struct room_data *world;

const int components[][5] = {
  /* This is the master ingredient list for all created magical items.
     The three objects are purged from the player's inventory. The mob's
     corpse must be on the ground...it too is consumed.
     
    Use 0 for unneeded ingredients */
  /* Spell		object 1	object 2	object 3	mob */
  { SPELL_FLY,		26630,		0,		0,		183 },
  { SPELL_ARMOR,	10895,		10896,		10897,		0 },
  { -1, -1, -1, -1, -1 } /* Must be at the end! */
};

const char *potion_names[][3] = {
  { "A bubbly potion is floating in mid-air.", "a bubbly potion", "bubbly fly potion" },
  { "A metallic potion lies on the floor.", "a metallic potion", "metallic armor potion" }
};

#define COMP(x) components[index][x]
#define CORPSE COMP(4)
#define SPELL COMP(0)

struct obj_data *find_component( struct char_data *ch, int vnum ) {
  struct obj_data *o;
  
  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == vnum) return o;
  
  return NULL;
}

struct obj_data *find_corpse( struct char_data *ch, int vnum ) {
  struct obj_data *o;
  
  for (o = world[ch->in_room].contents; o; o = o->next_content)
    if (GET_OBJ_TYPE(o) == ITEM_CONTAINER && GET_OBJ_VAL(o, 3)
        && GET_OBJ_RENT(o) == vnum)
      return o;
  
  return NULL;
}

ACMD(do_brew) {

  struct obj_data *potion, *c[3], *corpse=NULL;
  int i, spellnum, index;
  bool ok;
  
  skip_spaces(&argument);
  strcpy(buf, argument);
  
  if (!*buf) {
    send_to_char("Sure, brew something. Anything specific?\r\n", ch);
    return;
  }
  
  spellnum = find_skill_num(buf);
  if (spellnum < 0) {
    send_to_char("Err....WHAT exactly?\r\n", ch);
    return;
  }
  
  for (index = 0; SPELL != spellnum && SPELL != -1; index++);
  
  if (SPELL == -1) {
    send_to_char("That brew has not yet been discovered!\r\n", ch);
    return;
  }
  
  ok = TRUE;
  for (i = 1; i <= 3; i++) {
    c[i-1] = COMP(i) ? find_component( ch, COMP(i) ) : NULL;
    if (COMP(i) && !c[i-1]) ok = FALSE;
  }
  
  if (CORPSE) {
    corpse = find_corpse( ch, CORPSE );
    if (!corpse) ok = FALSE;
  }
  
  if (!ok) {
    send_to_char( "Your incantation sputters out and dies.\r\n", ch );
    return;
  }
    
  potion = read_object( PROTO_POTION, VIRTUAL );
  
  if (!potion) {
    send_to_char( "Ack! Fatal error in brew, couldn't make the potion!\r\n", ch );
    return;
  }
  
  potion->in_room = NOWHERE;
  potion->name = str_dup(potion_names[index][2]);

  potion->description = str_dup(potion_names[index][0]);

  potion->short_description = str_dup(potion_names[index][1]);

  GET_OBJ_VAL(potion, 0) = GET_LEVEL(ch);
  GET_OBJ_VAL(potion, 1) = spellnum;
  
  obj_to_char(potion, ch);
  
  for (i = 0; i <= 2; i++)
    if (c[i]) {
      act ("$p disappears in a swirl of smoke and light!", FALSE, ch, c[i], NULL, TO_ROOM);
      act ("You consume $p in your spellcasting.", FALSE, ch, c[i], NULL, TO_CHAR);
      extract_obj(c[i]);
    }
  
  if (corpse) {
    act ("$n lights $p on fire and it burns within seconds in a shower of sparks.", FALSE, ch, corpse, NULL, TO_ROOM);
    act ("You burn $p, absorbing the life energy into your spell.", FALSE, ch, corpse, NULL, TO_CHAR);
    extract_obj(corpse);
  }

  act("$n carefully brews $p.", FALSE, ch, potion, NULL, TO_ROOM);
  act("You carefully brew $p.", FALSE, ch, potion, NULL, TO_CHAR);
  
  return;
}
