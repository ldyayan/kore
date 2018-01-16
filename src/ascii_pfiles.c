/************************************************************************
 * Routines for saving and loading players to/from ASCII pfiles instead *
 * of the massive binary one.                                           *
 *                                        - Darryl Shpak / Culvan       *
 ************************************************************************/

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

void binary_to_ascii(char *buffer, char *data, int size) {
  /* This function converts binary data to ascii hex so it can go
     in the ascii pfile if there's no way to store it nicely. */
  
  int i;

  sprintf(buffer, "%x", (unsigned char)(* data));
  
  for (i = 1; i < size; i++)
    sprintf(buffer, "%s %x", buffer, (unsigned char)(*(data+i)));
}

void store_to_ascii_file(struct char_data *ch, struct char_file_u *st, FILE *fp) {
  /************************************************
   * Assumes that fp is opened for write.         *
   * Writes important data from st and ch to fp.  *
   ************************************************/
  
  char hex_buffer[200]; /* enough to store about 64 bytes */
  char *hex_data;
  struct affected_type af;
  int i;
  extern struct room_data *world;
  
/* HACKED because the loadroom code crashes */
return;

  fprintf(fp, "# -------------------- Ascii pfile for %s --------------------\n", GET_NAME(ch));
 /* Note: Lines preceded with a '#' are to be IGNORED by ascii_file_to_store */
  
  hex_data = (char *)&st->birth;
  binary_to_ascii(hex_buffer, hex_data, sizeof(time_t));
  fprintf(fp, "birth %s\n", hex_buffer);
  
/*  hex_data = (char *)&st->played;
  binary_to_ascii(hex_buffer, hex_data, sizeof(time_t)); */
  fprintf(fp, "time_played %d\n", st->played);
  
  hex_data = (char *)&st->last_logon;
  binary_to_ascii(hex_buffer, hex_data, sizeof(time_t));
  fprintf(fp, "last_logon %s\n", hex_buffer);
  
  fprintf(fp, "lasthost %s\n", st->host);  
  fprintf(fp, "hometown %d\n", st->hometown);
  fprintf(fp, "height %d\n", st->height);
  fprintf(fp, "weight %d\n", st->weight);
  fprintf(fp, "sex %d\n", st->sex);
  fprintf(fp, "class %d\n", st->class);
  fprintf(fp, "level %d\n", st->level);
  
  fprintf(fp, "# -------------------- real_abils --------------------\n");
  fprintf(fp, "str %d\nstr_add %d\n", st->abilities.str, st->abilities.str_add);
  fprintf(fp, "int %d\n", st->abilities.intel);
  fprintf(fp, "wis %d\n", st->abilities.wis);
  fprintf(fp, "dex %d\n", st->abilities.dex);
  fprintf(fp, "con %d\n", st->abilities.con);
  fprintf(fp, "cha %d\n", st->abilities.cha);
  
  fprintf(fp, "# -------------------- points --------------------\n");
  fprintf(fp, "mana %d %d\n", st->points.mana, st->points.max_mana);
  fprintf(fp, "hp %d %d\n", st->points.hit, st->points.max_hit);
  fprintf(fp, "move %d %d\n", st->points.move, st->points.max_move);
  fprintf(fp, "ac %d\n", st->points.armor);
  fprintf(fp, "gold %d\nbank %d\n", st->points.gold, st->points.bank_gold);
  fprintf(fp, "exp %d\n", st->points.exp);
  fprintf(fp, "hitroll %d\ndamroll %d\n", st->points.hitroll, st->points.damroll);
  
  fprintf(fp, "# --------------------- char_special ----------------\n");
  fprintf(fp, "alignment %d\n", st->char_specials_saved.alignment);
  fprintf(fp, "id %ld\n", st->char_specials_saved.idnum);
  fprintf(fp, "act %ld\n", st->char_specials_saved.act);
  
  fprintf(fp, "# --------------------- player_special ----------------\n");
  fprintf(fp, "skills");
  for (i = 0; i <= MAX_SKILLS; i++)
    fprintf(fp, " %d", st->player_specials_saved.skills[i]);
  fprintf(fp, "\n");
  fprintf(fp, "languages");
  for (i = 0; i <= MAX_TONGUE; i++)
    fprintf(fp, " %d", st->player_specials_saved.talks[i]);
  fprintf(fp, "\n");
  fprintf(fp, "wimpy %d\n", st->player_specials_saved.wimp_level);
  fprintf(fp, "frozen %d\n", st->player_specials_saved.freeze_level);
  fprintf(fp, "invis %d\n", st->player_specials_saved.invis_level);
  if (ch->in_room != NOWHERE) fprintf(fp, "loadroom %d\n", world[ch->in_room].number);
  fprintf(fp, "prefs %ld\n", st->player_specials_saved.pref);
  fprintf(fp, "badpwds %d\n", st->player_specials_saved.bad_pws);
  fprintf(fp, "drunk %d\n", st->player_specials_saved.conditions[0]);
  fprintf(fp, "hungry %d\n", st->player_specials_saved.conditions[1]);
  fprintf(fp, "thirsty %d\n", st->player_specials_saved.conditions[2]);
  fprintf(fp, "race %d\n", st->player_specials_saved.race);
  fprintf(fp, "clan %d\n", st->player_specials_saved.clan);
  fprintf(fp, "clanlevel %d\n", st->player_specials_saved.clan_level);
  fprintf(fp, "pref2 %ld\n", st->player_specials_saved.pref2);
  fprintf(fp, "olc %d\n", st->player_specials_saved.olc_zone);
  
  fprintf(fp, "# ----------------------- colors ---------------------\n");
  fprintf(fp, "colors");
  for (i = 0; i < 40; i++) {
    fprintf(fp, " %d", st->player_specials_saved.color_prefs[i]);
  }
  fprintf(fp, "\n");
  
  fprintf(fp, "# ------------------- affects --------------------\n");
  for (i = 0; i <= MAX_AFFECT; i++) {
    af = st->affected[i];
    if (af.type != 0) {
      fprintf(fp, "affect %d %d %d %d\n%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
        af.type, af.duration, af.modifier, af.location,
        af.bitvector, af.bitvector2, af.bitvector3, af.bitvector4,
        af.bitvector5, af.bitvector6, af.bitvector7, af.bitvector8,
        af.bitvector9, af.bitvector10
      );
    }
  }
  
  fprintf(fp, "# -------------------- text --------------------\n");
  fprintf(fp, "password %s\n", st->pwd);
  fprintf(fp, "title %s\n", st->title);
  fprintf(fp, "description\n%s@\n", st->description);
  
  return;
}

int save_ascii_pfile(struct char_file_u *st, struct char_data *ch) {
  FILE *fl;
  char fname[MAX_STRING_LENGTH];

  if (!get_filename(GET_NAME(ch), fname, ASCII_PLAYER_FILE))
    return 1;

  if (!(fl = fopen(fname, "w"))) {
    /* couldnt open the file for writing... the disk is probably full */
    sprintf("ERROR saving ascii pfile %s\n", fname);
    log(buf);
    return 1;
  }

  store_to_ascii_file(ch, st, fl);
  
  fclose(fl);
  
  return 0;
}

int save_ascii_data(struct char_data *ch) {
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  
  if (!get_filename(GET_NAME(ch), fname, ASCII_PLAYER_DATA))
    return 1;
    
  if (!(fl = fopen(fname, "w"))) {
    sprintf(buf, "ERROR saving ascii data %s\n", fname);
    log(buf);
    return 1;
  }
  
  if (ch->player_specials->email) {
    fprintf(fl, "email %s\n", ch->player_specials->email);
  }
  if (ch->player_specials->poofin) {
    fprintf(fl, "poofin %s\n", ch->player_specials->poofin);
  }
  if (ch->player_specials->poofout) {
    fprintf(fl, "poofout %s\n", ch->player_specials->poofout);
  }
  fprintf(fl, "title %s\n", GET_REAL_TITLE(ch));
  fprintf(fl, "end\n");
  fclose(fl);
  return 0;
}

int load_ascii_data(struct char_data *ch) {
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  char *p;
  
  if (!get_filename(GET_NAME(ch), fname, ASCII_PLAYER_DATA))
    return 1;
    
  if (!(fl = fopen(fname, "r"))) {
    return 0;
  }
  
  for (;;) {
    get_line(fl, buf);
    p = buf;
    p = any_one_arg(p, buf1);
    skip_spaces(&p);

    if (!str_cmp(buf1, "email")) {
      if (ch->player_specials->email) free(ch->player_specials->email);
      ch->player_specials->email = strdup(p);

    } else if (!str_cmp(buf1, "poofin")) {
      if (ch->player_specials->poofin) free(ch->player_specials->poofin);
      ch->player_specials->poofin = strdup(p);

    } else if (!str_cmp(buf1, "poofout")) {
      if (ch->player_specials->poofout) free(ch->player_specials->poofout);
      ch->player_specials->poofout = strdup(p);

    } else if (!str_cmp(buf1, "title")) {
      if (GET_REAL_TITLE(ch)) free(GET_REAL_TITLE(ch));
      GET_REAL_TITLE(ch) = strdup(p);

    } else if (!str_cmp(buf1, "end")) {
      break;

    } else {
      sprintf(buf, "ERROR: Unknown entry '%s' in '%s'", buf1, fname);
    }
  }
  
  fclose(fl);
  return 0;
}
