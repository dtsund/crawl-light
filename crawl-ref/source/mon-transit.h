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
    level_id mons_original_lid; //The level id of the monster's original floor.
    bool add_to_doomed; //Whether to add to the doomed list at all (sometimes we don't).
    
    int aut_to_staircase;

    follower() : mons(), items() { }
    follower(const monster& m);
    bool place(bool near_player = false);
    void load_mons_items();
    void restore_mons_items(monster& m);
};

typedef std::list<follower> m_transit_list;
typedef std::map<level_id, m_transit_list> monsters_in_transit;

typedef std::list<int> m_doomed_list;
typedef std::map<level_id, m_doomed_list> monsters_to_remove;

typedef std::list<item_def> i_transit_list;
typedef std::map<level_id, i_transit_list> items_in_transit;

extern monsters_in_transit the_lost_ones; //Monsters to place when arriving on a level
extern monsters_to_remove the_doomed_ones; //Monsters to remove when arriving
extern items_in_transit    transiting_items;

void transit_lists_clear();

m_transit_list *get_transit_list(const level_id &where);
m_doomed_list *get_doomed_list(const level_id &lid);
void add_monster_to_transit(const level_id &dest, level_id &origin, const monster& m);
void add_monster_to_transit(const level_id &dest, const monster& m);
void add_monster_to_doomed(const level_id &lid, const int index);
void remove_doomed_monsters();
void add_item_to_transit(const level_id &dest, const item_def &i);

// Places (some of the) monsters eligible to be placed on this level.
void place_transiting_monsters(int time_taken);
void place_followers(int time_taken);

void place_transiting_items();

void tag_followers();
void untag_followers();

#endif
