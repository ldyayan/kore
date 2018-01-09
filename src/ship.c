/* ************************************************************************
*   File: ship.c                                                          *
*                                                                         *
*   Usage: ship code.                                                     *
*                                                                         *
*   Coded by Tom Youderian (Aule of Heroes of Kore)                       *
************************************************************************ */

/*
  Installation notes:

  Make an area that represents a ship. Flag all of the deck rooms OUTDOORS.
  Make a ship be the first object in the zone. Make the ship's tiller be
  the second object. Assign the deck special procedure to every room in the
  ships world file. Assign the tiller special to the tiller object. Put the
  procedure call to boot ships into db.c; ships needed to be loaded special,
  not like fountains, because the ships move away and the zone would
  otherwise continuously pop new ships. Put code in so that DTs dont erase
  ships, or make it so that DTs are not dumps (set dts_are_dumps to NO).
  Hope I didnt forget anything. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"


/*   external vars  */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct command_info cmd_info[];
extern struct obj_data *object_list;
extern char *dirs[];
extern char *dir_abbrevs[];

/* extern functions */
ACMD(do_look);

/* function declarations */
SPECIAL(ship_tiller);



SPECIAL(ship_deck)
{
  register struct obj_data *tiller;
  register struct obj_data *ship;
  int was_in;
  bool found = FALSE;


  if (CMD_IS("disembark") || CMD_IS("leave") ||
      CMD_IS("lookout")) {

    /* first find the number of the ship we are on 
      by taking the room number and dividing by 100 and then
      multiplying by 100
      and looking for a match in the object list */

    for (tiller = object_list; tiller; tiller = tiller->next) {
      if (GET_OBJ_SPEC(tiller) == ship_tiller) {
        if (world[ch->in_room].number >= GET_OBJ_VAL(tiller, 1) && 
            world[ch->in_room].number <= GET_OBJ_VAL(tiller, 2)) {
          for (ship = object_list; ship; ship = ship->next) {
            if (GET_OBJ_VNUM(ship) == GET_OBJ_VAL(tiller, 0)) {
              found = TRUE;
              break;
            }
          }
        }
      }
    }
    if (!found) 	/* ouch! couldnt find the ship we're on! */
      return FALSE;

    if (CMD_IS("disembark") || CMD_IS("leave")) {
      act("You disembark $p.", FALSE, ch, ship, 0, TO_CHAR);
      act("$n disembarks $p.", TRUE, ch, ship, 0, TO_ROOM);
      char_from_room(ch);
      char_to_room(ch, ship->in_room);
      act("$n disembarks from $p.", TRUE, ch, ship, 0, TO_ROOM);
      look_at_room(ch, 0);
      return TRUE;
    }
    if (CMD_IS("lookout")) {
      act("You looks out around $p.", FALSE, ch, ship, 0, TO_CHAR);
      act("$n looks out around $p.", TRUE, ch, ship, 0, TO_ROOM);
      was_in = ch->in_room;
      char_from_room(ch);
      char_to_room(ch, ship->in_room);
      do_look(ch, "", 0, SCMD_LOOK);
      char_from_room(ch);
      char_to_room(ch, was_in);
      return TRUE; 
    }
  }

  return FALSE;
}



/* tiller of the ship stores in value[0] the vnum of the ship object (a portal)
                             in value[1] the first room of the ship zone
                             in value[2] the last room of the ship zone
*/
SPECIAL(ship_tiller)
{
  struct obj_data *tiller = (struct obj_data *) me;
  struct obj_data *ship;
  struct descriptor_data *i;
  int dir;
  int was_in;


  if (!CMD_IS("steer"))
    return FALSE;

  for (ship = object_list; ship; ship = ship->next) {
    if (GET_OBJ_VNUM(ship) == GET_OBJ_VAL(tiller, 0))
      break;
  }
  if (ship == NULL) {
    send_to_char("Your ship is sunk!\r\n", ch);
    return TRUE;
  }

  skip_spaces(&argument);

  dir = search_block(argument, dirs, FALSE);
  if (dir == -1)
    dir = search_block(argument, dir_abbrevs, FALSE);
  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, ship->in_room);

  if ((EXIT(ch, dir)) &&
      (EXIT(ch, dir)->to_room != NOWHERE) &&
      (!IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED))) {
    if ((world[EXIT(ch, dir)->to_room].sector_type != SECT_WATER_SWIM) &&
        (world[EXIT(ch, dir)->to_room].sector_type != SECT_WATER_NOSWIM) &&
        (world[EXIT(ch, dir)->to_room].sector_type != SECT_OCEAN)) {
      send_to_char("You would run aground if you went that way!\r\n", ch);
    } else {
      sprintf(buf, "You steer $p %s to %s.", dirs[dir],
          world[EXIT(ch, dir)->to_room].name);
      act(buf, FALSE, ch, ship, 0, TO_CHAR);
      sprintf(buf, "$p heads %s to %s.", dirs[dir],
          world[EXIT(ch, dir)->to_room].name);
      sprintf(buf1, "$p heads %s.", dirs[dir]);
      char_from_room(ch);
      char_to_room(ch, was_in);
      for (i = descriptor_list; i; i = i->next) {
        if (!i->connected && i != ch->desc && i->character &&
            !PLR_FLAGGED(i->character, PLR_WRITING) &&
            !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF) &&
            (world[ch->in_room].zone == world[i->character->in_room].zone) &&
            (world[i->character->in_room].number >= GET_OBJ_VAL(tiller, 1)) &&
            (world[i->character->in_room].number <= GET_OBJ_VAL(tiller, 2)) ) {
          if (!ROOM_FLAGGED(i->character->in_room, ROOM_INDOORS))
            act(buf, TRUE, ch, ship, i->character, TO_VICT | TO_SLEEP);
          else
            act(buf1, TRUE, ch, ship, i->character, TO_VICT | TO_SLEEP);
        }
      }      
      char_from_room(ch);
      char_to_room(ch, ship->in_room);
      obj_from_room(ship);
      obj_to_room(ship, EXIT(ch, dir)->to_room);
      char_from_room(ch);
      char_to_room(ch, ship->in_room);
    }
  } else {
    send_to_char("You cannot sail that way!\r\n", ch);
  }

  char_from_room(ch);
  char_to_room(ch, was_in);

  return TRUE;
}



void load_ship(int number, int room)
{  
  struct obj_data *obj;
  int r_num, r_room;
   
  if ((r_num = real_object(number)) < 0) {
    sprintf(buf, "SYSERR:    Ship Warning: There is no ship vnum %d.\r\n",
        number);
    log(buf);
    return;
  }
  if ((r_room = real_room(room)) < 0) {
    sprintf(buf, "SYSERR:    Ship Warning: There is no room vnum %d.\r\n",
        room);
    log(buf);
    return;
  }
  obj = read_object(r_num, REAL);
  obj_to_room(obj, r_room);
}



void Ship_boot(void)
{
  load_ship(2092, 2099);	/* Load Charon's ferry upon a creaking dock */

  load_ship(5800, 16181);
  load_ship(5700, 16181);

  load_ship(22023, 22021);	/* Load the packet steamer at the dock */
}
