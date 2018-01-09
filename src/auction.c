#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "utils.h"
#include "screen.h"
#include "auction.h"

extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern char *color_codes[];
extern int auction_on;
ASPELL(spell_identify);


/* used here by AUC_OUT() macro .. AUC_OUT is in auction.h */
struct descriptor_data *d;


void auction_reset(void)
{
  auction.bidder = NULL;
  auction.seller = NULL;
  auction.obj = NULL;
  auction.ticks = AUC_NONE;
  auction.bid = 0;
}



void auction_cancel(struct char_data *ch)
{
  sprintf(buf2, "The auctioneer auctions, "
      "'Sorry, %s has cancelled the auction.'", GET_NAME(ch));
  AUC_OUT(buf2);
  if (auction.obj && auction.seller) {
    sprintf(buf2, "The auctioneer returns %s to you.\r\n",
        auction.obj->short_description);
    send_to_char(buf2, auction.seller);
    obj_from_room(auction.obj);
    obj_to_char(auction.obj, auction.seller);
  }
  if (auction.bidder) {
    sprintf(buf2, "Here's your %ld coin%s back.\r\n", auction.bid,
        auction.bid != 1 ? "s" : "");
    send_to_char(buf2, auction.bidder);
    GET_GOLD(auction.bidder) += auction.bid;
  }
  auction_reset();
}



void auction_update(void)
{
  if (auction.ticks == AUC_NONE) /* No auction */
    return;  

  if (auction.ticks >= AUC_BID && auction.ticks <= AUC_SOLD)
  {
    if ((auction.bidder) && (auction.ticks < AUC_SOLD))
    {
      switch (auction.ticks) {
        case AUC_BID:
            sprintf(buf2, "The auctioneer auctions, '%s going once...'",
                auction.obj->short_description);
            break;
        case AUC_ONCE:
            sprintf(buf2, "The auctioneer auctions, '%s going twice...'",
                auction.obj->short_description);
            break;
        case AUC_TWICE:
            sprintf(buf2, "The auctioneer auctions, 'last call: %s going to %s for %ld coin%s.'",
                auction.obj->short_description,
                GET_NAME(auction.bidder),
                auction.bid,
                auction.bid != 1 ? "s" : " ");
            break;
        default:
            return;
            break;
      }
      AUC_OUT(buf2);
      auction.ticks++;
      return;
    }

    if ((!auction.bidder) && (auction.ticks == AUC_SOLD))
    {
      sprintf(buf2, "The auctioneer auctions, '%s is unsold... sorry %s.'",
          auction.obj->short_description,
          GET_NAME(auction.seller));
      AUC_OUT(buf2);
      /* Give the poor fellow his unsold goods back */
      obj_from_room(auction.obj);
      obj_to_char(auction.obj, auction.seller);
      auction_reset();
/*      auction.ticks--; */
      return;
    }

    if((!auction.bidder) && (auction.ticks < AUC_SOLD))
    {
      sprintf(buf2, "The auctioneer auctions, '%s for %ld coin%s going %s%s%s.'",
          auction.obj->short_description,
          auction.bid,
          auction.bid != 1 ? "s" : "",
          auction.ticks == AUC_BID ? "once" : "",
          auction.ticks == AUC_ONCE ? "twice" : "",
          auction.ticks == AUC_TWICE ? "for the last call" : "");
      AUC_OUT(buf2);
      auction.ticks++;
      return;
    }

    if ((auction.bidder) && (auction.ticks >= AUC_SOLD))
    { /* Sold */
      sprintf(buf2, "The auctioneer auctions, 'SOLD! %s to %s for %ld coin%s!'",
          auction.obj->short_description,
          GET_NAME(auction.bidder),
          auction.bid,
          auction.bid != 1 ? "s" : "");
      AUC_OUT(buf2);
  
      GET_GOLD(auction.seller) += auction.bid;

      act("Congrats! You have sold $p!", FALSE, auction.seller,
          auction.obj, 0, TO_CHAR);
      obj_from_room(auction.obj);
      obj_to_char(auction.obj, auction.bidder);
      act("Congrats! You now have $p!", FALSE, auction.bidder,
          auction.obj, 0, TO_CHAR);
      auction_reset();
      return;
    }
  }
  return;
}



/*
 * these checks are to keep people from auctioning things that are either
 * worth lots of money (ie corpses) or are going to rot away while the 
 * auctioneer is auctioning them (rotting bags, seeds, portals) etc
 */
int invalid_auction_object(struct obj_data *obj, struct char_data *ch)
{
  /* no auctioning corpses */
  if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && GET_OBJ_VAL(obj, 3))
    return 1;

  /* no auctioning rotting objects */
  if (GET_OBJ_TIMER(obj) != 0)
    return 1;

  if (GET_REAL_LEVEL(ch) < LVL_IMMORT) {
    /* no auctioning food */
    if (GET_OBJ_TYPE(obj) == ITEM_FOOD)
      return 1;

    /* no auctioning recalls */
    if (isname("recall", obj->name))
      return 1;

    /* no auctioning trash */
    if (GET_OBJ_TYPE(obj) == ITEM_TRASH)
      return 1;
    
    if (IS_OBJ_STAT(obj, ITEM_NOSELL))
      return 1;

    if (IS_OBJ_STAT(obj, ITEM_NODROP))
      return 1;

    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE))
      return 1;
  }    
  return 0;
}



ACMD(do_bid)
{
  long bid;


  if (!auction_on) {
    send_to_char("Bid temporarily disabled.\r\n", ch);
    return;
  }

  if (PRF_FLAGGED(ch, PRF_NOAUCT)) {
    send_to_char("You aren't even on the channel!\r\n", ch);
    return;
  }

  if (auction.ticks == AUC_NONE) {
    send_to_char("No items on auction to bid on.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  one_argument(argument, buf);
  bid = atoi(buf);

  if (bid <= 0) {
    send_to_char("You must bid at least a single coin!\r\n", ch);
    return;
  }

  if (!*buf) {
    sprintf(buf2, "Current bid: %ld coin%s\r\n", auction.bid,
        auction.bid != 1 ? "s." : ".");
    send_to_char(buf2, ch);
  } else if (ch == auction.bidder)
    send_to_char("You're trying to outbid yourself.\r\n", ch);
  else if (ch == auction.seller)
    send_to_char("You can't bid on your own item.\r\n", ch);
  else if ((bid < auction.bid) && !auction.bidder) {
    sprintf(buf2, "The min bid is %ld coin%s\r\n", auction.bid,
        auction.bid != 1 ? "s." : ".");
    send_to_char(buf2, ch);
  } else if ((bid < (auction.bid * 1.10) && auction.bidder) || bid == 0) {
    sprintf(buf2, "Try bidding at least 10%% over the current bid of %ld. (%.0f coins).\r\n",
        auction.bid, auction.bid * 1.10 + 1);
    send_to_char(buf2, ch);
  } else if (GET_GOLD(ch) < bid) {
    sprintf(buf2, "You have only %d coin%s on hand.\r\n", GET_GOLD(ch),
        GET_GOLD(ch) != 1 ? "s" : "");
    send_to_char(buf2, ch);
  }else {
    /* Give last bidder money back! */
    if (auction.bidder)
      {GET_GOLD(auction.bidder) += auction.bid;}
    auction.bid = bid;
    auction.bidder = ch;
    auction.ticks = AUC_BID;
    /* Get money from new bidder. */
    GET_GOLD(auction.bidder) -= auction.bid;
    sprintf(buf2, "The auctioneer auctions, '%s bids %ld coin%s on %s.'", GET_NAME(auction.bidder), auction.bid, auction.bid!=1 ? "s" :"", auction.obj->short_description);
    AUC_OUT(buf2);
  }
}



#define AUCTION_IDENT_COST	500
#define AUCTION_CANCEL_COST	(GET_OBJ_COST(auction.obj) * 2)
ACMD(do_auction)
{
  struct obj_data *obj;
 
  if (!auction_on) {
    send_to_char("Auction temporarily disabled.\r\n", ch);
    return;
  }

  if (PRF_FLAGGED(ch, PRF_NOAUCT)) {
    send_to_char("You aren't even on the channel!\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  two_arguments(argument, buf1, buf2);
  argument = one_argument(argument, arg);
  skip_spaces(&argument);

  if (!*buf1) {
    send_to_char("Usage: auction <item> <starting bid>\r\n"
                 "       auction [STAT|IDENTIFY]\r\n"
                 "       auction [STOP|CANCEL]\r\n"
                 "       auctalk <message>\r\n"
                 "       bid <amount>\r\n", ch);
    return;
  }

  if (!strcasecmp(buf1, "stat") ||
      !strncasecmp(buf1, "identify", strlen(buf1))) {
    if (auction.ticks != AUC_NONE) {
      if (GET_GOLD(ch) < AUCTION_IDENT_COST) {
        sprintf(buf3, "Identifying an item on auction costs %d coins.\r\n",
            AUCTION_IDENT_COST);
        send_to_char(buf3, ch);
        return;
      } else {
        spell_identify(LVL_IMPL, ch, NULL, auction.obj, "");
        sprintf(buf3, "The auctioneer charges you %d coins for the service.\r\n", AUCTION_IDENT_COST);
        send_to_char(buf3, ch);
        GET_GOLD(ch) -= AUCTION_IDENT_COST;
      }
    } else
      send_to_char("No items on auction to stat.\r\n", ch);
    return;
  }

  if (!strcasecmp(buf1, "stop") ||
      !strcasecmp(buf1, "cancel")) {
    if (auction.ticks != AUC_NONE) {
      if (ch == auction.seller || GET_LEVEL(ch) >= LVL_IMMORT) {
        if (GET_LEVEL(ch) < LVL_IMMORT) {
          if (GET_GOLD(ch) < AUCTION_CANCEL_COST) {
            send_to_char("You don't have enough gold to cancel the auction!\r\n", ch);
            return;
          } else {
            sprintf(buf3, "The auctioneer charges you %d coins for the service.\r\n", AUCTION_CANCEL_COST);
            auction_cancel(ch);
            send_to_char(buf3, ch);
          }
        } else {
          auction_cancel(ch);
          send_to_char("Auction cancelled.\r\n", ch);
        }
      } else
        send_to_char("You can't stop other people from auctioning items.\r\n",
            ch);
    } else
      send_to_char("No auction to stop.\r\n", ch);
    return;
  }

  if (auction.ticks != AUC_NONE) {
    sprintf(buf2, "Sorry, %s is currently auctioning %s for %ld coins.\r\n",
        GET_NAME(auction.seller), auction.obj->short_description, 
        auction.bid);
    send_to_char(buf2, ch);
  } else if ((obj = get_obj_in_list_vis(ch, buf1, ch->carrying)) == NULL) {
    send_to_char("You don't seem to have that to sell.\r\n", ch);
  } else {
    if (invalid_auction_object(obj, ch)) {
      send_to_char("You can't auction that!\r\n", ch);
      return;
    }
    auction.ticks = AUC_BID;
    auction.seller = ch;
    auction.obj = obj;
    auction.bid = (atoi(buf2) > 0 ? atoi(buf2) : GET_OBJ_COST(auction.obj));
    /* Get the object from the character, so they cannot drop it! */
    obj_from_char(auction.obj);
    obj_to_room(auction.obj, real_room(2));  /* put it in storage */
    sprintf(buf2, "The auctioneer auctions, '%s puts %s up for sale, min bid %ld coin%s'", GET_NAME(auction.seller), auction.obj->short_description, auction.bid, auction.bid != 1 ? "s." : ".");
    AUC_OUT(buf2);
  }
}



ACMD(do_auctalk)
{
  struct descriptor_data *i;

  if (!auction_on) {
    send_to_char("Auction temporarily disabled.\r\n", ch);
    return;
  }

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char("You may not use channels.\n\r", ch);
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
    send_to_char("The walls seem to absorb your words.\n\r", ch);
    return;
  }

  if (PRF_FLAGGED(ch, PRF_NOAUCT)) {
    send_to_char("You aren't even on the channel!\r\n", ch);
    return;
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    skip_spaces(&argument);
    sprintf(buf1, "You auction, '%s'\n\r", argument);
    send_to_char(CCAUCTION(ch), ch);
    send_to_char(buf1, ch);
    send_to_char(CCNRM(ch), ch);                             \
  }
  
  skip_spaces(&argument);
  sprintf(buf1, "$n auctions, '%s'", argument);
  
  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i != ch->desc && i->character &&
	!PRF_FLAGGED(i->character, PRF_NOAUCT) &&
        !PLR_FLAGGED(i->character, PLR_WRITING) &&
        !ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

	send_to_char(CCAUCTION(i->character), i->character);
	act(buf1, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(i->character), i->character);
    }
  }
}



void auction_seller_gone(struct char_data * ch)
{
  sprintf(buf2, "The auctioneer auctions, 'Oh dear, %s is no longer with us...'", GET_NAME(auction.seller));
  AUC_OUT(buf2);
  if (auction.bidder != NULL) {
    sprintf(buf2, "The auctioneer auctions, 'SOLD! %s to %s for %ld coin%s!'",
            auction.obj->short_description,
            GET_NAME(auction.bidder),
            auction.bid,
            auction.bid != 1 ? "s" : "");
    AUC_OUT(buf2);
    GET_GOLD(auction.seller) += auction.bid;
    act("Congrats! You have sold $p!", FALSE, auction.seller,
        auction.obj, 0, TO_CHAR);
    obj_from_room(auction.obj);
    obj_to_char(auction.obj, auction.bidder);
    act("Congrats! You now have $p!", FALSE, auction.bidder,
        auction.obj, 0, TO_CHAR);
  } else {
    sprintf(buf2, "The auctioneer auctions, 'In fairness to all, the auction must begin again, sorry...'");
    AUC_OUT(buf2);
    sprintf(buf2, "The auctioneer returns %s to you.\r\n",
        auction.obj->short_description);
    send_to_char(buf2, auction.seller);
    obj_from_room(auction.obj);
    obj_to_char(auction.obj, auction.seller);
  }
  auction_reset(); 
}



void auction_bidder_gone(struct char_data * ch)
{
  sprintf(buf2, "The auctioneer auctions, 'Hrm, %s is no longer with us...'",
GET_NAME(auction.bidder));
  AUC_OUT(buf2);
  sprintf(buf2, "The auctioneer auctions, 'In fairness to all, the auction must begin again, sorry...'");
  AUC_OUT(buf2);
  sprintf(buf2, "The auctioneer returns %s to you.\r\n", 
      auction.obj->short_description);
  obj_from_room(auction.obj);
  obj_to_char(auction.obj, auction.seller);
  sprintf(buf2, "Here's your %ld coin%s back.\r\n", auction.bid, auction.bid !=
 1 ? "s" : "");
  send_to_char(buf2, auction.bidder);
  GET_GOLD(auction.bidder) += auction.bid;
  auction_reset();
}



/*
 * If you want to do object sanity checking at time of sale, here's what you
 * can do, or at least one way.  Make a function that returns an object
 * pointer when given someone and something about the object.  Loop through
 * his inventory, comparing the passed argument to the appropriate place
 * in every inventory object, this results in a more accurate match, then
 * return that object, otherwise null.  I removed this due to the fact that
 * A) some people would rather throw checks all over the place to simply
 * prevent the object from moving and B) it'd make it too easy ;)
 */
