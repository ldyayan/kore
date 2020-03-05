/* ************************************************************************
*   File: weather.c                                     Part of CircleMUD *
*  Usage: functions handling time and the weather                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"

/* external vars */
extern struct time_info_data time_info;
extern struct room_data *world;
extern int top_of_world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;

/* funct prototypes */
void weather_and_time(int mode);
void daynight_shift(int time);
void another_hour(int mode);
void weather_change(void);
void mprog_time_trigger(struct char_data * ch);



/*
const struct {
  int zone;
  int day;
  int night;
} daynight_zones[] = {
  {30, 30, 82},
  {82, 30, 82},
  {-1, -1, -1}
};

void daynight_shift(int time)
{
  struct descriptor_data *d;
  struct char_data *ch;
  struct room_data *rm;
  sh_int location;
  int current_zone, to_zone = -1;
  int i;

  for (d = descriptor_list; d; d = d->next) {
>>> HACKED because it might need this check ... <<<
    if (d->connected || !d->character)
      continue;
>>> end of hack <<<
    ch = d->character;
    rm = &world[ch->in_room];

    current_zone = rm->number / 100;

    for (i = 0; daynight_zones[i].zone != -1; i++) {
      if (daynight_zones[i].zone == current_zone) {
        if (time == 5) {
          to_zone = daynight_zones[i].day;
        } else if (time == 22) {
          to_zone = daynight_zones[i].night;
        } else {
          return;
        }
      }
    }

    if (to_zone == -1)
      return;

    if ((location = real_room(rm->number 
                            - (current_zone * 100)
                            + (to_zone * 100))) < 0) {
      return;
    } else {
      char_from_room(ch);
      char_to_room(ch, location);
      look_at_room(ch, 0);
    }
  }

  return;
}
*/



void another_hour(int mode)
{
  time_info.hours++;

  if (mode) {
    switch (time_info.hours) {
    case 5:
      weather_info.sunlight = SUN_RISE;
      send_to_outdoor("The sun rises in the east.\r\n");
/*    daynight_shift(time_info.hours); */
      break;
    case 6:
      weather_info.sunlight = SUN_LIGHT;
      send_to_outdoor("The day has begun.\r\n");
      break;
    case 21:
      weather_info.sunlight = SUN_SET;
      send_to_outdoor("The sun slowly disappears in the west.\r\n");
      break;
    case 22:
      weather_info.sunlight = SUN_DARK;
      send_to_outdoor("The night has begun.\r\n");
/*    daynight_shift(time_info.hours); */
      break;
    default:
      break;
    }
  }
  if (time_info.hours > 23) {	/* Changed by HHS due to bug ??? */
    time_info.hours -= 24;
    time_info.day++;

    if (time_info.day > 34) {
      time_info.day = 0;
      time_info.month++;

      if (time_info.month > 16) {
	time_info.month = 0;
	time_info.year++;
      }
    }
  }
}



void weather_change(void)
{
  int diff, change;
  if ((time_info.month >= 9) && (time_info.month <= 16))
    diff = (weather_info.pressure > 985 ? -2 : 2);
  else
    diff = (weather_info.pressure > 1015 ? -2 : 2);

  weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

  weather_info.change = MIN(weather_info.change, 12);
  weather_info.change = MAX(weather_info.change, -12);

  weather_info.pressure += weather_info.change;

  weather_info.pressure = MIN(weather_info.pressure, 1040);
  weather_info.pressure = MAX(weather_info.pressure, 960);

  change = 0;

  switch (weather_info.sky) {
  case SKY_CLOUDLESS:
    if (weather_info.pressure < 990)
      change = 1;
    else if (weather_info.pressure < 1010)
      if (dice(1, 4) == 1)
	change = 1;
    break;
  case SKY_CLOUDY:
    if (weather_info.pressure < 970)
      change = 2;
    else if (weather_info.pressure < 990)
      if (dice(1, 4) == 1)
	change = 2;
      else
	change = 0;
    else if (weather_info.pressure > 1030)
      if (dice(1, 4) == 1)
	change = 3;

    break;
  case SKY_RAINING:
    if (weather_info.pressure < 970)
      if (dice(1, 4) == 1)
	change = 4;
      else
	change = 0;
    else if (weather_info.pressure > 1030)
      change = 5;
    else if (weather_info.pressure > 1010)
      if (dice(1, 4) == 1)
	change = 5;

    break;
  case SKY_LIGHTNING:
    if (weather_info.pressure > 1010)
      change = 6;
    else if (weather_info.pressure > 990)
      if (dice(1, 4) == 1)
	change = 6;

    break;
  default:
    change = 0;
    weather_info.sky = SKY_CLOUDLESS;
    break;
  }

  switch (change) {
  case 0:
    break;
  case 1:
    send_to_outdoor("The sky starts to get cloudy.\r\n");
    weather_info.sky = SKY_CLOUDY;
    break;
  case 2:
    send_to_outdoor("It starts to rain.\r\n");
    weather_info.sky = SKY_RAINING;
    break;
  case 3:
    send_to_outdoor("The clouds disappear.\r\n");
    weather_info.sky = SKY_CLOUDLESS;
    break;
  case 4:
    send_to_outdoor("A wind blows in from the north.\r\n");
    weather_info.sky = SKY_LIGHTNING;
    break;
  case 5:
    send_to_outdoor("The rain stops.\r\n");
    weather_info.sky = SKY_CLOUDY;
    break;
  case 6:
    send_to_outdoor("The wind stops.\r\n");
    weather_info.sky = SKY_RAINING;
    break;
  default:
    break;
  }
}



void weather_and_time(int mode)
{
  struct char_data *ch;
  struct char_data *next_ch;


  another_hour(mode);
/* daynight shift here */
  if (mode)
    weather_change();

/* HACKED to add in time_prog triggers */
  if (mode)
    for (ch = character_list; ch; ch = next_ch) {
      next_ch = ch->next;
      mprog_time_trigger(ch);
    }
/* end of hack */
}
