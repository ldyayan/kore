typedef struct  clan_data               CLAN_DATA;
#define MAX_CLAN                   50   
struct  clan_data
{
    CLAN_DATA * next;           /* next clan in list                    */
    CLAN_DATA * prev;           /* previous clan in list                */
    char *      filename;       /* Clan filename                        */
    char *      name;           /* Clan name                            */
    char *      motto;          /* Clan motto                           */
    char *      description;    /* A brief description of the clan      */
    char *      deity;          /* Clan's deity                         */
    char *      leader;         /* Head clan leader                     */
    char *      number1;        /* First officer                        */
    char *      number2;        /* Second officer                       */
    char *      badge;          /* Clan badge on who/where/to_room      */
    char *      leadrank;       /* Leader's rank                        */
    char *      onerank;        /* Number One's rank                    */
    char *      tworank;        /* Number Two's rank                    */
    int         pkills[7];      /* Number of pkills on behalf of clan   */
    int         pdeaths[7];     /* Number of pkills against clan        */
    int         mkills;         /* Number of mkills on behalf of clan   */
    int         mdeaths;        /* Number of clan deaths due to mobs    */
    int         illegal_pk;     /* Number of illegal pk's by clan       */
    int         score;          /* Overall score                        */
    sh_int      clan_type;      /* See clan type defines                */
    sh_int      favour;         /* Deities favour upon the clan         */
    sh_int      strikes;        /* Number of strikes against the clan   */
    sh_int      members;        /* Number of clan members               */
    sh_int      mem_limit;      /* Number of clan members allowed       */
    sh_int      alignment;      /* Clan's general alignment             */
    int         board;          /* Vnum of clan board                   */
    int         clanobj1;       /* Vnum of first clan obj               */
    int         clanobj2;       /* Vnum of second clan obj              */
    int         clanobj3;       /* Vnum of third clan obj               */
    int         clanobj4;       /* Vnum of fourth clan obj              */ 
    int         recall;         /* Vnum of clan's recall room           */
    int         storeroom;      /* Vnum of clan's store room            */
    int         guard1;         /* Vnum of clan guard type 1            */
    int         guard2;         /* Vnum of clan guard type 2            */
    int         class;          /* For guilds                           */
};
