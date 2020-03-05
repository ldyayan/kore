
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "comm.h"
#include "handler.h"
#include "screen.h"
#include "spells.h"

extern struct obj_data *obj_proto;
extern int top_of_objt;
extern char *objprognames[];
extern struct room_data *world;
extern struct index_data *obj_index;

void set_var(char *varname, char *newval);

#define CLEANER 3097

#define ERR_BAD_IFCHECK		2

#define OP_PTR_BAD	0
#define OP_PTR_CHAR	1
#define OP_PTR_OBJ	2

#define AREALOG arealog(buf, NRM, 58, TRUE, area);
#define LOGERR(x) \
    sprintf(buf, "OBJPROGS: Obj #%d: %s", vnum, x);		\
    AREALOG;								\
    
/* This stuff either goes global, or gets added to every function call. *
 * I chose global :-)                                                   */
bool objstopcommand;
int vnum, area;

char *op_ptr_names[] = {
  "character",
  "object"
};

/* This is a major function. All the $___ functions get expanded here
 *
 * Note : targ better be big enough or the whole thing's going down
 */
void expand_string(char *src, char *targ, void **ptrs, ubyte *ptrtypes) {
  char *p, *p2;
  char tmp[1000]; /* Can't use bufs, in case that's src or targ */
  int tmpint;
  struct char_data *tmpch;
  
  *targ = '\0';
  
  while (*src != '\0') {
    p = strchr(src, '$');
    if (!p) {
      strcat(targ, src);
      break;
    }
    *p = '\0';
    strcat(targ, src);
    src = p + 1;
    if (*src == '$') {
      strcat(targ, "$$"); /* Double dollar, for "act" */
      src++;
    } else {
      p = strchr(src, '(');
      if (!p) {
        LOGERR("No open bracket in $ expression.");
        return;
      }
      p2 = strchr(p, ')');
      if (!p2) {
        LOGERR("No close bracket in $ expression.");
        return;
      }
      *p = '\0';
      p++;
      if (!str_cmp(src, "name")) {
        if (*p != '%' || !p[1]) {
          LOGERR("$name requires a %? argument.");
          return;
        }
        tmpint = p[1] - '0';
        if (tmpint < 0 || tmpint > 9) {
          LOGERR("$name: %? argument is non-numeric.");
          return;
        }
        if (ptrtypes[tmpint] != OP_PTR_CHAR) {
          LOGERR("$name: %? is not a character!");
          return;
        }
        tmpch = ptrs[tmpint];
        strcat(targ, GET_NAME(tmpch));
      } else if (!str_cmp(src, "heshe")) {
        if (*p != '%' || !p[1]) {
          LOGERR("$heshe requires a %? argument.");
          return;
        }
        tmpint = p[1] - '0';
        if (tmpint < 0 || tmpint > 9) {
          LOGERR("$heshe: %? argument is non-numeric.");
          return;
        }
        if (ptrtypes[tmpint] != OP_PTR_CHAR) {
          LOGERR("$heshe: %? is not a character!");
          return;
        }
        tmpch = ptrs[tmpint];
        strcat(targ, HSSH(tmpch));
      } else if (!str_cmp(src, "himher")) {
        if (*p != '%' || !p[1]) {
          LOGERR("$himher requires a %? argument.");
          return;
        }
        tmpint = p[1] - '0';
        if (tmpint < 0 || tmpint > 9) {
          LOGERR("$himher: %? argument is non-numeric.");
          return;
        }
        if (ptrtypes[tmpint] != OP_PTR_CHAR) {
          LOGERR("$himher: %? is not a character!");
          return;
        }
        tmpch = ptrs[tmpint];
        strcat(targ, HMHR(tmpch));
      } else if (!str_cmp(src, "hisher")) {
        if (*p != '%' || !p[1]) {
          LOGERR("$hisher requires a %? argument.");
          return;
        }
        tmpint = p[1] - '0';
        if (tmpint < 0 || tmpint > 9) {
          LOGERR("$hisher: %? argument is non-numeric.");
          return;
        }
        if (ptrtypes[tmpint] != OP_PTR_CHAR) {
          LOGERR("$hisher: %? is not a character!");
          return;
        }
        tmpch = ptrs[tmpint];
        strcat(targ, HSHR(tmpch));
      } else {
        sprintf(tmp, "Unknown ifcheck $%s", src);
        LOGERR(tmp);
        return;
      }
      src = p2 + 1;
    }
  } 
} 
/* do $heshe...and try silent & mpsilent */
      
/* This is used in the flow control */
char *skip_until(char *targ, char *str) {
  char *next, oldch; /* , *spc */
  int depth = 0;
  
  if (!str) return NULL;
  
  for (;;) {
    for (next = str; *next; next++) {
      if (*next == '\n') break;
    }
    if (*next == '\0') return NULL;

    oldch = *next;
    *next = '\0';
    skip_spaces(&str);
    /* when we "skip_until", we want to ignore anything in nested "if"s */
    if (depth == 0) {
      if (!str_cmp(str, targ)) {
        *next = oldch;
        return next + 1;
      }
      if (!str_cmp(str, "if")) depth++;
    } else {
      if (!str_cmp(str, "endif")) depth--;
    }
    *next = oldch;
    next++;
    str = next;
  }
}

int eval_cond(char *str, const room_num room) {
  /* Evaluate the 'if' conditions for mobprogs
     (Eventually) Supports 'and', 'or', and 'not' operators, evaluated
     left-to-right.
     All conditions evaluate to boolean, and can be either boolean functions
     like isheld, or string/numeric conditions using "==", ">", etc.
   */
   char *b1, *b2, *tmp, *args;
   int tempint;
   room_num temproom;
/* This was unused */
/* struct char_data *tempchar; */
   
   skip_spaces(&str);
   strcpy(buf1, str);
   b1 = strchr(buf1, '(');
   if (b1) {
     *b1 = '\0';
     tmp = strchr(b1 + 1, '(');
     if (tmp) {
       sprintf(buf, "OBJPROGS: Obj #%d: double open bracket in ifcheck '%s'", vnum, buf1);
       AREALOG;
       return ERR_BAD_IFCHECK;
     }
     b2 = strchr(b1 + 1, ')');
     if (!b2) {
       sprintf(buf, "OBJPROGS: Obj #%d: no closing bracket in ifcheck '%s'", vnum, buf1);
       AREALOG;
       return ERR_BAD_IFCHECK;
     }
     tmp = strchr(b2 + 1, ')');
     if (tmp) {
       sprintf(buf, "OBJPROGS: Obj #%d: double close bracket in ifcheck '%s'", vnum, buf1);
       AREALOG;
       return ERR_BAD_IFCHECK;
     }
     *b2 = '\0';
     args = strdup(b1 + 1);
     *b2 = ')';
   } else {
     args = strdup("");
   }
   if (!str_cmp(buf1, "true")) {
     return TRUE;
   } else if (!str_cmp(buf1, "false")) {
     return FALSE;
   } else if (!str_cmp(buf1, "rand")) {
     if (!*args) {
       LOGERR("'rand' needs a value");
       return ERR_BAD_IFCHECK;
     }
     tempint = atoi(args);
     if (number(1, 100) <= tempint) return TRUE;
     return FALSE;
   } else if (!str_cmp(buf1, "isanypcat")) {
     if (!*args) {
       LOGERR("'isanyoneat' needs a room");
       return ERR_BAD_IFCHECK;
     }
     if (!isdigit(*args)) {
       if (!str_cmp(buf1, "here")) {
         temproom = room;
       } else {
         LOGERR("'isanyoneat' takes 'here' or a number!");
         return ERR_BAD_IFCHECK;
       }
     } else {
       temproom = atoi(args);
     }
     temproom = real_room(temproom);
     if (temproom == NOWHERE) {
       LOGERR("isanyoneat NOWHERE?!"); /* Beats me, is anyone nowhere? */
       return ERR_BAD_IFCHECK;
     }
     if (get_first_char(temproom)) return TRUE;
     return FALSE;
   }
   sprintf(buf, "OBJPROGS: Obj #%d: unknown ifcheck '%s'", vnum, buf1);
   arealog(buf, NRM, 58, TRUE, area);
   return ERR_BAD_IFCHECK;
 }

/* The main objprog function, this actually executes the objprogs. It *
 * handles the flow control and interprets the objcmds.               */
void exec_objprog(struct obj_data *obj, char *prog, struct char_data *ch,
                  struct obj_data *obj2) {

  struct obj_data *unequip_char(struct char_data * ch, int pos);


  char *start, *end, *full_cmd, *tmpchp;
  char full_cmd_buf[MAX_STRING_LENGTH], cmd[MAX_STRING_LENGTH];
  void *ptrs[10];
  ubyte ptrtypes[10];
  int i, j, pvar, spellnum;
  char *c;
  room_num room;
  struct char_data *tmpch, *tmpch2;
  struct obj_data *tmpobj;
  int ifdepth;
  bool cond;
  int number, sides, modifier;
  
  objstopcommand = FALSE;
  full_cmd = full_cmd_buf;
  
  if (prog == NULL) return;
  
  vnum = GET_OBJ_VNUM(obj);
  area = vnum / 100;
  
  for (i = 0; i < 10; i++) {
    ptrtypes[i] = 0;
    ptrs[i] = NULL;
  }
  
  ptrs[0] = obj;
  ptrs[1] = ch;
  ptrs[2] = obj2;
  ptrtypes[0] = OP_PTR_OBJ;
  ptrtypes[1] = OP_PTR_CHAR;
  ptrtypes[2] = OP_PTR_OBJ;
  
  start = prog;
  ifdepth = 0;
  
  /* Eeek I have to write flow control and ifchecks and stuff....da horror...*/
  for (;;) {
    for (end = start; *end != '\n'; end++) {
      if (*end == '\0') return;
    }
    *end = '\0';
    strcpy(full_cmd, start);
    *end = '\n';
    start = end + 1;
    if (*full_cmd == '\n') continue;
    if (*full_cmd == '~') break;
    
    full_cmd = any_one_arg(full_cmd, cmd);
    skip_spaces(&full_cmd);
    
    tmpobj = obj;
    tmpch = NULL;
    while (tmpobj->in_obj) tmpobj = tmpobj->in_obj;
    room = tmpobj->in_room;
    if (room == NOWHERE) {
      tmpch = tmpobj->carried_by;
      if (!tmpch) tmpch = tmpobj->worn_by;
      if (!tmpch) return; /* Object (or it's container) isn't on the floor, or
                             carried by anyone, or equipped. */
      room = tmpch->in_room;
      if (room == NOWHERE) return;
    }
    if (room == NOWHERE) return; /* how'd THAT happen? */
    
    /* over here, expand functions in full_cmd */
    
    /* -sigh- we're going to have to completely rewrite the interpreter :-) */
    
    /* First of all, is this a flow-control issue? */
    if (!str_cmp(cmd, "if")) {
      /* Evaluate and enter the 'if' clause */
      cond = eval_cond(full_cmd, room);
      if (cond == ERR_BAD_IFCHECK) {
        LOGERR("ifcheck error, halting objprog");
        return;
      }
      if (!cond) {
        tmpchp = start;
        start = skip_until("else", start);
        if (!start) {
          start = tmpchp;
          start = skip_until("endif", start);
          if (!start) {
            LOGERR("if without endif");
            return;
          }
        }
      }
      ifdepth++;
    } else if (!str_cmp(cmd, "else")) {
      /* Handle the else */
      start = skip_until("endif", start);
      if (!start) {
        if (ifdepth <= 0) {
          LOGERR("if & else without endif");
        } else {
          LOGERR("else without if & endif");
        }
        return;
      }
      if (ifdepth < 0) {
        LOGERR("else & endif without if.");
        return;
      }
    } else if (!str_cmp(cmd, "endif")) {
      /* Handle the endif */
      /* Actually, nothing to handle. Decrement the depth count though */
      ifdepth--;
      if (ifdepth < 0) {
        LOGERR("endif without if.");
        return;
      }
    } else {
    
    /* Handle the actual command, if it is one */
      if (*cmd == '\0') {
        // do nothing


      } else if (!str_cmp(cmd, "echo")) {
        /* Nice hack here - we can use act() if we find a char to do the echo */
        tmpch = world[room].people;
        if (!tmpch) break; /* No one here, so no point to echo anyway */
        expand_string(full_cmd, buf1, ptrs, ptrtypes);
        act(buf1, FALSE, tmpch, NULL, NULL, TO_ROOM);
        act(buf1, FALSE, tmpch, NULL, NULL, TO_CHAR);


      } else if (!str_cmp(cmd, "echoat")) {
        full_cmd = any_one_arg(full_cmd, buf1);
        skip_spaces(&full_cmd);
        if (*buf1 != '%') {
          LOGERR("echoat needs a %? target.");
          return;
        }
        i = buf1[1] - '0';
        if (i < 0 || i > 9) {
          LOGERR("echoat: %? target must be numeric!");
          return;
        }
        if (ptrtypes[i] != OP_PTR_CHAR) {
          sprintf(buf2, "echoat: %%%d is not a character!", i);
          LOGERR(buf2);
          return;
        }
        expand_string(full_cmd, buf1, ptrs, ptrtypes);
        act(buf1, FALSE, (struct char_data *)ptrs[i], NULL, NULL, TO_CHAR);


      } else if (!str_cmp(cmd, "echoaround")) {
        full_cmd = any_one_arg(full_cmd, buf1);
        skip_spaces(&full_cmd);
        if (*buf1 != '%') {
          LOGERR("echoaround needs a %? target.");
          return;
        }
        i = buf1[1] - '0';
        if (i < 0 || i > 9) {
          LOGERR("echoaround: %? target must be numeric!");
          return;
        }
        if (ptrtypes[i] != OP_PTR_CHAR) {
          sprintf(buf2, "echoaround: %%%d is not a character!", i);
          LOGERR(buf2);
          return;
        }
        expand_string(full_cmd, buf1, ptrs, ptrtypes);
        act(buf1, FALSE, (struct char_data *)ptrs[i], NULL, NULL, TO_ROOM);


      } else if (!str_cmp(cmd, "stopaction")) {
        objstopcommand = TRUE;
      

      } else if (!str_cmp(cmd, "poof")) {
        if (obj->worn_by) {
          j = IS_THRIKREEN(ch)? NUM_THRI_WEARS : NUM_WEARS;
          for (i = 0; i < j; i++) {
            if (obj->worn_by->equipment[i] == obj) break;
          }
          if (i == j) {
            LOGERR("poof: Extreme silliness: object worn, but in no slot!");
            return;
          }
          if (unequip_char(ch, i) != obj) {
            LOGERR("poof: Object unequipped is not the object that poofed!");
            return;
          }
        } else if (obj->carried_by) {
          obj_from_char(obj);
        } else if (obj->in_room != NOWHERE) {
          obj_from_room(obj);
        }
        obj_to_room(obj, real_room(CLEANER));


      } else if (!str_cmp(cmd, "load")) {
        full_cmd = any_one_arg(full_cmd, buf1);
        skip_spaces(&full_cmd);
        if (!*full_cmd) {
          LOGERR("not enough parameters to \"load\".");
          return;
        }
        full_cmd = any_one_arg(full_cmd, buf2);
        i = atoi(buf2);
        skip_spaces(&full_cmd);
        if (*full_cmd) {
          any_one_arg(full_cmd, buf3);
          if (*buf3 != '%') {
            LOGERR("third parameter to \"load\" must begin with '%'");
            return;
          }
          if (buf3[1] < '0' || buf3[1] > '9') {
            LOGERR("third parameter to \"load\" must be numeric");
            return;
          }
          pvar = buf3[1] - '0';
        } else {
          pvar = -1;
        }
        ptrs[pvar] = NULL;
        ptrtypes[pvar] = OP_PTR_BAD;
        if (!str_cmp(buf1, "obj")) {
          j = real_object(i);
          if (j == -1) {
            sprintf(buf, "OBJPROGS: Obj #%d: Invalid vnum \"%s\" passed to \"load obj\"", vnum, buf2);
            AREALOG;
            return;
          }
          tmpobj = read_object(j, REAL);
          if (!tmpobj) {
            LOGERR("load obj: chaos reigning supreme");
            return;
          }
          obj_to_room(tmpobj, room);
          if (pvar >= 0) {
            ptrtypes[pvar] = OP_PTR_OBJ;
            ptrs[pvar] = (void *)tmpobj;
          }
        } else if (!str_cmp(buf1, "mob")) {
          j = real_mobile(i);
          if (j == -1) {
            sprintf(buf, "OBJPROGS: Obj #%d: Invalid vnum \"%s\" passed to \"load mob\"", vnum, buf2);
            AREALOG;
            return;
          }
          tmpch = read_mobile(j, REAL);
          if (!tmpch) {
            LOGERR("load mob: chaos reigning supreme");
            return;
          }
          char_to_room(tmpch, room);
          if (pvar >= 0) {
            ptrtypes[pvar] = OP_PTR_CHAR;
            ptrs[pvar] = (void *)tmpch;
          }
        } else {
          LOGERR("first parameter to \"load\" must be \"obj\" or \"mob\".");
          return;
        }
        
        
      } else if (!str_cmp(cmd, "damage")) {
        if (!*full_cmd) {
          LOGERR("damage: No parameters!");
          return;
        }
        full_cmd = any_one_arg(full_cmd, buf2);
        skip_spaces(&full_cmd);
        if (!*full_cmd) {
          LOGERR("damage: Two parameters needed!");
          return;
        }
        if (sscanf(full_cmd, "%d d %d + %d", &number, &sides, &modifier) != 3) {
          if (sscanf(full_cmd, "%d d %d - %d", &number, &sides, &modifier) != 3) {
            if (sscanf(full_cmd, "%d d %d", &number, &sides) != 2) {
              if (sscanf(full_cmd, "%d", &number) != 1) {
                LOGERR("damage: bad damage string");
                return;
              }
            }
          } else /* it was of the form 'number d sides - modifier' */ {
            modifier *= -1;
          }
        }
        if (!str_cmp(buf2, "all")) {
          for (tmpch = world[room].people; tmpch; tmpch = tmpch->next) {
            i = dice(number, sides) + modifier;
            damage(tmpch, tmpch, i, SKILL_MPDAMAGE);
          }
        } else if (*buf2 == '%') {
          j = buf2[1] - '0';
          if (j < 0 || j > 10) {
            LOGERR("damage: non-numeric target");
            return;
          }
          if (ptrtypes[j] != OP_PTR_CHAR) {
            LOGERR("damage: target is not a character!");
            return;
          }
          tmpch = ptrs[j];
          i = dice(number, sides) + modifier;
          damage(tmpch, tmpch, i, SKILL_MPDAMAGE);
        } else {
          LOGERR("damage: bad target");
          return;
        }


      } else if (!str_cmp(cmd, "callmagic")) {
        if (!*full_cmd) {
          LOGERR("callmagic: no parameters!");
          return;
        }
        
        if (*full_cmd != '\'') {
          LOGERR("callmagic: paramters don't start with a quote!");
          return;
        }
        c = full_cmd + 1;
        while (*c) {
          if (*c == '\'') break;
          c++;
        }
        if (*c != '\'') {
          LOGERR("callmagic: no closing quote!");
          return;
        }
        *c = '\0';
        strcpy(buf, full_cmd + 1);
        *c = '\'';
        c++;
        spellnum = find_skill_num(buf);
        if (spellnum < 1 || spellnum > MAX_SPELLS) {
          sprintf(buf2, "callmagic: unknown spell '%s'", buf);
          LOGERR(buf2);
          return;
        }
        /* Find the caster (gotta have a caster, it's the rules) */
        c = any_one_arg(c, buf);
        if (!*buf) {
          LOGERR("callmagic: no caster");
          return;
        }
        if (*buf != '%') {
          sprintf(buf2, "callmagic: illegal caster '%s'", buf);
          LOGERR(buf2);
          return;
        }
        i = buf[1] - '0';
        if (i < 0 || i > 9) {
          sprintf(buf2, "callmagic: illegal caster '%s'", buf);
          LOGERR(buf2);
          return;
        }
        if (ptrtypes[i] != OP_PTR_CHAR) {
          LOGERR("callmagic: caster is not a character");
          return;
        }
        tmpch = ptrs[i];
        
        /* Now the fun part: targeting the spell. */
        /* Valid targets are none, or %n */
        any_one_arg(c, buf);
        if (!*buf) {
          /* no target */
          call_magic(tmpch, NULL, NULL, "", spellnum, 50, CAST_SPELL);
        } else if (*buf == '%') {
          /* a % target */
          if (ptrtypes[i] != OP_PTR_CHAR) {
            if (ptrtypes[i] != OP_PTR_OBJ) {
              LOGERR("callmagic: '%' target is not a character or object!");
              return;
            }
            tmpobj = ptrs[i];
            call_magic(tmpch, NULL, tmpobj, "", spellnum, 50, CAST_SPELL);
          } else {
            tmpch2 = ptrs[i];
            call_magic(tmpch, tmpch2, NULL, "", spellnum, 50, CAST_SPELL);
          }
        } else {
          sprintf(buf2, "callmagic: bad target '%s'", buf);
          LOGERR(buf2);
          return;
        }



      } else if (!str_cmp(cmd, "log")) {
        expand_string(full_cmd, buf1, ptrs, ptrtypes);
        LOGERR(buf1);



      } else if (!str_cmp(cmd, "set")) {
        if (!*full_cmd) {
          LOGERR("set: no parameters");
          return;
        }
        full_cmd = any_one_arg(full_cmd, buf2);
        skip_spaces(&full_cmd);
        if (!*full_cmd) {
          LOGERR("set: Two parameters needed!");
          return;
        }

        full_cmd = any_one_arg(full_cmd, buf3);
        
        set_var(buf2, buf3);
            

      
      } else {
        sprintf(buf, "OBJPROGS: Obj #%d: Undefined objcmd \"%s\".", vnum, cmd);
        AREALOG;
        return;
      }
    }
  }
}

void load_objprogs(struct obj_data *obj, char *fname, int vnum) {
  FILE *fl;
  char *b;
  ubyte ptype;
  struct obj_prog_data *nprog, *lastprog;
  lastprog = NULL;
  
  sprintf(buf, OBJPROG_DIR "/%s", fname);
  fl = fopen(buf, "r");
  if (!fl) {
    sprintf(buf, "Objprog file %s not found!", fname);
    mudlog(buf, NRM, 58, TRUE);
    return;
  }
  
  if (obj->progs) {
    sprintf(buf, "load_objprogs: Object #%d already has progs!", vnum);
    mudlog(buf, NRM, 58, TRUE);
    return;
  }
  
  for (;;) {
    fscanf(fl, "%s\n", buf);
    b = buf;
    skip_spaces(&b);
    if (*b == '~') {
      fclose(fl);
      return;
    }
    for (ptype = 0; *objprognames[ptype] != '\n'; ptype++) {
      if (!str_cmp(buf, objprognames[ptype])) break;
    }
    if (*objprognames[ptype] == '\n') {
      sprintf(buf1, "Unknown objprog type '%s' on object #%d", buf, vnum);
      mudlog(buf, NRM, 58, TRUE);
      return;
    }
    
    CREATE(nprog, struct obj_prog_data, 1);
    nprog->progtype = ptype;
    nprog->next = NULL;
    
    /* Ok time to read the prog into buf1 */
    *buf1 = '\0';
    for (;;) {
      get_line(fl, buf);
      b = buf;
      skip_spaces(&b);
      strcat(buf1, b);
      strcat(buf1, "\n");
      if (*b == '~') break;
    }
    nprog->prog = strdup(buf1);
    if (lastprog) {
      lastprog->next = nprog;
    } else {
      obj->progs = nprog;
    }
    lastprog = nprog;
    obj->progtypes |= 1 << ptype;
  }
}

void boot_objprogs() {
  FILE *index;
  int vnum, rnum;
  char fname[100];
  
  index = fopen(OBJPROG_DIR "/index", "r");
  if (!index) {
    mudlog("Objprog index file \"" OBJPROG_DIR "/index\" not found!", NRM, 58, TRUE);
    return;
  }
  
  for (;;) {
    get_line(index, buf);
    sscanf(buf, "%d %s\n", &vnum, fname);
    if (vnum == -1) break;
    rnum = real_object(vnum);
    if (rnum == -1) {
      sprintf(buf, "Invalid vnum #%d in objprog index linked to %s", vnum, fname);
      mudlog(buf, NRM, 58, TRUE);
      continue;
    }
    load_objprogs(&obj_proto[rnum], fname, vnum);
  }
  fclose(index);
}

char *find_objprog(struct obj_data *obj, ubyte ptype) {
  struct obj_prog_data *prog;
  
  for (prog = obj->progs;prog;prog = prog->next)
    if (prog->progtype == ptype)
      return prog->prog; /* with a prog prog here, and a prog prog there... */
  
  return NULL;
}

/* Returns true if the 'get' should be stopped (in theory) */
bool obj_get_prog(struct obj_data *obj, struct char_data *ch) {
  char *prog;
  
  if (obj->progtypes | BIT(OBJ_GET_PROG)) {
/*    mudlog("get_prog fired", BRF, 51, FALSE); */
    prog = find_objprog(obj, OBJ_GET_PROG);
    exec_objprog(obj, prog, ch, NULL);
    return objstopcommand;
  }
  return FALSE;
}

/* Misc. action prog */
/* - Entering portals */
/* Returns TRUE if the action should be stopped */
bool obj_use_prog(struct obj_data *obj, struct char_data *ch) {
  char *prog;
  
  if (obj->progtypes | BIT(OBJ_USE_PROG)) {
    prog = find_objprog(obj, OBJ_USE_PROG);
    exec_objprog(obj, prog, ch, NULL);
    return objstopcommand;
  }
  return FALSE;
}

/* Fires when an object rots. You can stop it from rotting. */
/* Returns TRUE if the object's normal rot code should be overridden */
bool obj_rot_prog(struct obj_data *obj) {
  char *prog;
  
  if (obj->progtypes | BIT(OBJ_ROT_PROG)) {
    prog = find_objprog(obj, OBJ_ROT_PROG);
    exec_objprog(obj, prog, NULL, NULL);
    return objstopcommand;
  }
  return FALSE;
}

/* Triggers on a 'drop'. Returns TRUE to stop the drop. */
bool obj_drop_prog(struct obj_data *obj, struct char_data *ch) {
  char *prog;
  
  if (obj->progtypes | BIT(OBJ_DROP_PROG)) {
    prog = find_objprog(obj, OBJ_DROP_PROG);
    exec_objprog(obj, prog, ch, NULL);
    return objstopcommand;
  }
  return FALSE;
}

/* Triggers on a 'identify'. Fires AFTER the normal identify text. */
void obj_ident_prog(struct obj_data *obj, struct char_data *ch) {
  char *prog;
  
  if (obj->progtypes | BIT(OBJ_IDENT_PROG)) {
    prog = find_objprog(obj, OBJ_IDENT_PROG);
    exec_objprog(obj, prog, ch, NULL);
  }
}

/* Triggers on a 'wear'. Returns TRUE to stop the drop. 
   This happens BEFORE the item is equipped. */
bool obj_wear_prog(struct obj_data *obj, struct char_data *ch) {
  char *prog;
  
  if (obj->progtypes | BIT(OBJ_WEAR_PROG)) {
    prog = find_objprog(obj, OBJ_WEAR_PROG);
    exec_objprog(obj, prog, ch, NULL);
    return objstopcommand;
  }
  return FALSE;
}

/* Triggers on a 'wear'. This happens AFTER the item is equipped. */
void obj_worn_prog(struct obj_data *obj, struct char_data *ch) {
  char *prog;
  
  if (obj->progtypes | BIT(OBJ_WORN_PROG)) {
    prog = find_objprog(obj, OBJ_WORN_PROG);
    exec_objprog(obj, prog, ch, NULL);
  }
}

ACMD(do_opstat) {
  struct obj_data *obj;
  struct obj_prog_data *prog;
  
  one_argument(argument, buf);
  obj = get_obj_vis(ch, buf);
  if (!obj) {
    sprintf(buf1, "What's a %s?\r\n", buf);
    send_to_char(buf1, ch);
    return;
  }
  
  if (!(prog = obj->progs)) {
    sprintf(buf, "%s has no objprogs.", obj->short_description);
    act(buf, FALSE, ch, NULL, NULL, TO_CHAR);
    return;
  }
  
  sprintf(buf, "Objprogs for object #%d, '%s'\r\n", GET_OBJ_VNUM(obj), obj->short_description);
  send_to_char(buf, ch);
  
  while (prog) {
    send_to_char(CCBCYN(ch), ch);
    send_to_char(objprognames[prog->progtype], ch);
    send_to_char(CCNRM(ch), ch);
    send_to_char("\r\n", ch);
    send_to_char(prog->prog, ch);
    prog = prog->next;
  }
  send_to_char("~\n", ch);
}

void reload_objprogs() {
  int rnum;
  struct obj_prog_data *prog, *tmp;
  extern struct obj_data *object_list;
  struct obj_data *obj;
  
  for (rnum = 0; rnum < top_of_objt; rnum++) {
    prog = obj_proto[rnum].progs;
    obj_proto[rnum].progtypes = 0;
    while (prog) {
      free(prog->prog);
      tmp = prog->next;
      free(prog);
      prog = tmp;
    }
    obj_proto[rnum].progs = NULL;
  }
  
  boot_objprogs();
  
  for (obj = object_list; obj; obj = obj->next) {
    obj->progs = obj_proto[obj->item_number].progs;
  }
}
