/**************************************************************************
 *  Pets.h                                     Written by Darryl Shpak    *
 *  Heroes of Kore: chaos.exic.net 6000                                   *
 *                                                                        *
 *  Combined with Pets.c, functions and definitions for the handling of   *
 *  individual pets which stay with their owner across reboots and        *
 *  renting, can advance in levels and abilities, and assist their owner  *
 *  in various ways, based on their loyalty                               *
 **************************************************************************/

void pet_load(struct char_data *ch);
void pet_from_char(struct char_data *pet, struct char_data *owner);
void pet_leave(struct char_data *pet);
void pet_lose(struct char_data *owner);
void new_pet_to_char(struct char_data *pet, struct char_data *ch);
struct char_data *pet_from_disk(FILE *fp);
bool pet_sanity_check(struct char_data *pet);
void extract_pet(struct char_data *pet);
void boot_pets();
void save_pet(struct char_data *ch);
bool pet_to_disk(struct char_data *pet, FILE *fp);

#define MOUNT_NONE 	0
#define MOUNT_SMALL 	1
#define MOUNT_NORMAL 	2
#define MOUNT_LARGE 	3

struct pet_type_data {
  int mob_vnum;				/* which mobnum this is */
  int mountsize;			/* How big a pet this is */
};
