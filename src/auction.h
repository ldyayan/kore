struct auction_data {
  struct char_data *seller;
  struct char_data *bidder;
  struct obj_data *obj;
  long bid;
  int ticks;
};

struct auction_data auction;

#define AUC_NONE	-1
#define AUC_NEW		0
#define AUC_BID		1
#define AUC_ONCE	2
#define AUC_TWICE       3
#define AUC_SOLD        4

#define AUC_OUT(txt)                                             \
  for (d = descriptor_list; d; d = d->next)			\
   if (!d->connected && d->character &&				\
       !PLR_FLAGGED(d->character, PLR_WRITING) &&		\
       !PRF_FLAGGED(d->character, PRF_NOAUCT) &&		\
       !ROOM_FLAGGED(d->character->in_room, ROOM_SOUNDPROOF))	\
     {sprintf(buf, "%s%s%s\r\n",CCAUCTION(d->character), \
         txt, CCNRM(d->character));                             \
     send_to_char(buf, d->character);}
