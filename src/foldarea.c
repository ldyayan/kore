/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - olc.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define _RV_OLC_


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "screen.h"
#include "handler.h"

extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern int thaco[NUM_CLASSES][LVL_IMPL + 1];

char *strip_cr( char *str )
{
    static char newstr[MAX_STRING_LENGTH];
    int i, j;
    
    for ( i=j=0; str[i] != '\0'; i++ )
        if ( str[i] != '\r' )
        {
          newstr[j++] = str[i];
        } 
    newstr[j] = '\0';
    return newstr;
}

void fold_area( int tarea, char filename[256])
{
    sh_int		*room;
    struct char_data	*pMobIndex;
    struct obj_data	*pObjIndex;
    int			*xit, counter2;
    struct extra_descr_data	*ed;
    struct affect_data		*paf;
    char		 buf[MAX_STRING_LENGTH];
    FILE		*fpout;
    int			 vnum;
    int			 val0, val1, val2, val3;
    bool		 complexmob;

    sprintf(buf, "Filename is: %s\n\r", filename);
    log(buf);

    if ( ( fpout = fopen( filename, "w" ) ) == NULL )
    {
	log( "fold_area: fopen" );
	return;
    }
    
    fprintf( fpout, "#AREA   Kore Area~\n\n\n\n");
    fprintf( fpout, "#VERSION 1\n");
    fprintf( fpout, "#AUTHOR Karns~\n\n");
    fprintf( fpout, "#RANGES\n");
    fprintf( fpout, "0 50 0 50\n");
    fprintf( fpout, "$\n\n");

    fprintf( fpout, "#FLAGS\n0\n\n");

    fprintf( fpout, "#ECONOMY 0 0\n\n");

    fprintf( fpout, "#CLIMATE 0 0 0\n\n");
    
    /* save mobiles */
    fprintf( fpout, "#MOBILES\n" );

    for ( vnum = zone_table[tarea].number * 100; vnum <= zone_table[tarea].top; vnum++ )
    {
	if ( ( pMobIndex = read_mobile( vnum, VIRTUAL )) == NULL )
	  continue;
	complexmob = FALSE;
	fprintf( fpout, "#%d\n",	GET_MOB_VNUM(pMobIndex));
	fprintf( fpout, "%s~\n",	pMobIndex->player.name);
	fprintf( fpout,	"%s~\n",	pMobIndex->player.short_descr);
	fprintf( fpout,	"%s~\n",	strip_cr(pMobIndex->player.long_descr));
	fprintf( fpout, "%s~\n",	strip_cr(pMobIndex->player.description));
	fprintf( fpout, "%ld ",		MOB_FLAGS(pMobIndex));
	fprintf( fpout, "%ld %d %c\n",	AFF_FLAGS(pMobIndex),
					GET_ALIGNMENT(pMobIndex),
					complexmob ? 'C' : 'S'		);
					
	fprintf( fpout, "%d %d %d ",	pMobIndex->player.level,
					thaco[(int) CLASS_WARRIOR][(int) GET_LEVEL(pMobIndex)],
					pMobIndex->points.armor		);
	fprintf( fpout, "1d%d+%d ",	GET_MAX_HIT(pMobIndex),
					hit_gain(pMobIndex)		);
	fprintf( fpout, "%dd%d+%d\n",	pMobIndex->mob_specials.damnodice,
					pMobIndex->mob_specials.damsizedice,
					GET_DAMROLL(pMobIndex)		);
	fprintf( fpout, "%d %d\n",	pMobIndex->points.gold,
					pMobIndex->points.exp			);
/* Need to convert to new positions correctly on loadup sigh -Shaddai */
	fprintf( fpout, "%d %d %d\n",	pMobIndex->char_specials.position+100,
					pMobIndex->mob_specials.default_pos+100,
					pMobIndex->player.sex		);
	char_to_room(pMobIndex, 0);
        extract_char(pMobIndex);
    }
    fprintf( fpout, "#0\n\n\n" );

    fprintf( fpout, "#OBJECTS\n" );
    for ( vnum = zone_table[tarea].number * 100; vnum <= zone_table[tarea].top; vnum++ )
    {
	if ( (pObjIndex = read_object( vnum, VIRTUAL )) == NULL )
	  continue;
	fprintf( fpout, "#%d\n",	GET_OBJ_VNUM(pObjIndex)		);
	fprintf( fpout, "%s~\n",	pObjIndex->name			);
	fprintf( fpout, "%s~\n",	pObjIndex->short_description	);
	fprintf( fpout, "%s~\n",	pObjIndex->description		);
	fprintf( fpout, "~\n"						);
	fprintf( fpout, "%d %d %d\n",	GET_OBJ_TYPE(pObjIndex),
					GET_OBJ_EXTRA(pObjIndex),
					GET_OBJ_WEAR(pObjIndex)		);

	val0 = GET_OBJ_VAL(pObjIndex, 0);
	val1 = GET_OBJ_VAL(pObjIndex, 1);
	val2 = GET_OBJ_VAL(pObjIndex, 2);
	val3 = GET_OBJ_VAL(pObjIndex, 3);
	fprintf( fpout, "%d %d %d %d\n",	val0, 
						val1,
						val2,
						val3 );

	fprintf( fpout, "%d %d %d\n",	GET_OBJ_WEIGHT(pObjIndex),
					pObjIndex->obj_flags.cost,
					pObjIndex->obj_flags.cost_per_day);


      if (pObjIndex->ex_description)
      { /*. Yep, save them too .*/
        for (ed = pObjIndex->ex_description; ed; ed = ed->next)
        {
          if (!*ed->keyword || !*ed->description)
          {
		mudlog("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!", BRF, LVL_BUILDER, TRUE);
		continue;
          } 
          fprintf(fpout, "E\n%s~\n%s~\n",
                        ed->keyword,
                        strip_cr( ed->description ));            
        } 

      for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
        if (pObjIndex->affected[counter2].modifier)
          fprintf(fpout,   "A\n"
                        "%d %d\n",
                        pObjIndex->affected[counter2].location,
                        pObjIndex->affected[counter2].modifier
          );


      }   
      extract_obj(pObjIndex);
    }
    fprintf( fpout, "#0\n\n\n" );

    fprintf( fpout, "#ROOMS\n" );
/*
    for ( vnum = tarea->low_r_vnum; vnum <= tarea->hi_r_vnum; vnum++ )
    {
	if ( (room = get_room_index( vnum )) == NULL )
	  continue;

	fprintf( fpout, "#%d\n",	vnum				);
	fprintf( fpout, "%s~\n",	room->name			);
	fprintf( fpout, "%s~\n",	strip_cr( room->description )	);
	fprintf( fpout, "0 %d %d\n",	room->room_flags,
					room->sector_type	);

	for ( xit = room->first_exit; xit; xit = xit->next )
	{
	   fprintf( fpout, "D%d\n",		xit->vdir );
	   fprintf( fpout, "%s~\n",		strip_cr( xit->description ) );
	   fprintf( fpout, "%s~\n",		strip_cr( xit->keyword ) );
	   if ( xit->distance > 1 || xit->pull )
	     fprintf( fpout, "%d %d %d %d %d %d\n",
	     					xit->exit_info & ~EX_BASHED,
	   					xit->key,
	   					xit->vnum,
	   					xit->distance,
	   					xit->pulltype,
	   					xit->pull );
	   else
	     fprintf( fpout, "%d %d %d\n",	xit->exit_info & ~EX_BASHED,
	   					xit->key,
	   					xit->vnum );
	}	
	for ( ed = room->first_extradesc; ed; ed = ed->next )
	   fprintf( fpout, "E\n%s~\n%s~\n",
			ed->keyword, strip_cr( ed->description ));

	fprintf( fpout, "S\n" );
    }
    fprintf( fpout, "#0\n\n\n" );

    fprintf( fpout, "#RESETS\n" );
    fprintf( fpout, "S\n\n\n" );

    fprintf( fpout, "#SHOPS\n" );
    fprintf( fpout, "0\n\n\n" );

    fprintf( fpout, "#REPAIRS\n" );
    fprintf( fpout, "0\n\n\n" );

    fprintf( fpout, "#SPECIALS\n" );
    fprintf( fpout, "S\n\n\n" );

*/
    fprintf( fpout, "#$\n" );
    fclose( fpout );
    return;
}

void do_foldarea( struct char_data *ch, char *argument )
{   
    int tarea, j;
    
    if ( !argument || argument[0] == '\0' )
    {
        send_to_char( "Fold what?\n\r", ch );
        return; 
    }   

    for (tarea = 0; tarea <= top_of_zone_table; tarea++)
    {     
	j = atoi(argument);
	sprintf(buf, "tarea number is: %d. tarea name should be: %s.  argument is %s.  j is: %d.\n\r", tarea, zone_table[tarea].name, argument, j);
	send_to_char(buf, ch);
        if ( zone_table[tarea].number == j )
        { 
          send_to_char( "Folding area...\n\r", ch );
	  sprintf(buf, "%s", zone_table[tarea].name);
          fold_area( tarea, buf );
          send_to_char( "Done.\n\r", ch );
          return; 
        }  
    }

    send_to_char( "No such area exists.\n\r", ch );
    return;
}
