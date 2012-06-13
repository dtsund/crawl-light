/**
 * @file
 * @brief Tracking monsters in transit between levels.
**/

#ifndef MON_TRANSIT_H
#define MON_TRANSIT_H

#include "monster.h"
#include <map>
#include <list>

struct follower
{
    monster mons;
    FixedVector<item_def, NUM_MONSTER_SLOTS> items;
    
    //Information regarding the monster's original position in the data.  Used
    //for off-level deletion.
    int mons_original_index; //The index of the monster in the floor it came from.
    short items_original_index[NUM_MONSTER_SLOTS]; //Indices of the monster's gear.
    std::string mons_original_lid; //The level id of the monster's original floor.
    
    int aut_to_staircase;

    follower() : mons(), items() { }
    follower(const monster& m);
    bool place(bool near_player = false);
    void load_mons_items();
    void restore_mons_items(monster& m);
};

typedef std::list<follower> m_transit_list;
typedef std::map<level_id, m_transit_list> monsters_in_transit;

typedef std::list<item_def> i_transit_list;
typedef std::map<level_id, i_transit_list> items_in_transit;

extern monsters_in_transit the_lost_ones;
extern items_in_transit    transiting_items;

void transit_lists_clear();

m_transit_list *get_transit_list(const level_id &where);
void add_monster_to_transit(const level_id &dest, const monster& m);
void add_item_to_transit(const level_id &dest, const item_def &i);

// Places (some of the) monsters eligible to be placed on this level.
void place_transiting_monsters(int time_taken);
void place_followers(int time_taken);

void place_transiting_items();

void tag_followers();
void untag_followers();

#endif
