/**
 * @file
 * @brief Tracks monsters that are in suspended animation between levels.
**/

#include "AppHdr.h"

#include <algorithm>

#include "mon-transit.h"

#include "artefact.h"
#include "coord.h"
#include "coordit.h"
#include "dungeon.h"
#include "env.h"
#include "items.h"
#include "mon-pathfind.h"
#include "mon-place.h"
#include "mon-util.h"
#include "random.h"
#include "religion.h"
#include "travel.h"

#define MAX_LOST 100

monsters_in_transit the_lost_ones;
monsters_to_remove the_doomed_ones;
items_in_transit    transiting_items;

void transit_lists_clear()
{
    the_lost_ones.clear();
    the_doomed_ones.clear();
    transiting_items.clear();
}

static void level_place_lost_monsters(m_transit_list &m, int time_taken);
static void level_place_followers(m_transit_list &m, int time_taken);

static void cull_lost_mons(m_transit_list &mlist, int how_many)
{
    // First pass, drop non-uniques.
    for (m_transit_list::iterator i = mlist.begin(); i != mlist.end();)
    {
        m_transit_list::iterator finger = i++;
        if (!mons_is_unique(finger->mons.type))
        {
            mlist.erase(finger);

            if (--how_many <= MAX_LOST)
                return;
        }
    }

    // If we're still over the limit (unlikely), just lose
    // the old ones.
    while (how_many-- > MAX_LOST && !mlist.empty())
        mlist.erase(mlist.begin());
}

static void cull_lost_items(i_transit_list &ilist, int how_many)
{
    // First pass, drop non-artefacts.
    for (i_transit_list::iterator i = ilist.begin(); i != ilist.end();)
    {
        i_transit_list::iterator finger = i++;
        if (!is_artefact(*finger))
        {
            ilist.erase(finger);

            if (--how_many <= MAX_LOST)
                return;
        }
    }

    // Second pass, drop randarts.
    for (i_transit_list::iterator i = ilist.begin(); i != ilist.end();)
    {
        i_transit_list::iterator finger = i++;
        if (is_random_artefact(*finger))
        {
            ilist.erase(finger);

            if (--how_many <= MAX_LOST)
                return;
        }
    }

    // Third pass, drop unrandarts.
    for (i_transit_list::iterator i = ilist.begin(); i != ilist.end();)
    {
        i_transit_list::iterator finger = i++;
        if (is_unrandom_artefact(*finger)
            && !is_special_unrandom_artefact(*finger))
        {
            ilist.erase(finger);

            if (--how_many <= MAX_LOST)
                return;
        }
    }

    // If we're still over the limit (unlikely), just lose
    // the old ones.
    while (how_many-- > MAX_LOST && !ilist.empty())
        ilist.erase(ilist.begin());
}

m_transit_list *get_transit_list(const level_id &lid)
{
    monsters_in_transit::iterator i = the_lost_ones.find(lid);
    return (i != the_lost_ones.end()? &i->second : NULL);
}

m_doomed_list *get_doomed_list(const level_id &lid)
{
    monsters_to_remove::iterator i = the_doomed_ones.find(lid);
    return (i != the_doomed_ones.end()? &i->second : NULL);
}

void add_monster_to_transit(const level_id &lid, level_id &origin, const monster& m)
{
    m_transit_list &mlist = the_lost_ones[lid];
    follower to_push = m;
    to_push.mons_original_index = m.mindex();
    
    //Pathfind to determine the shortest path to get to the staircase;
    //Can't just use coordinates, because there may be insurmountable
    //obstacles in between the monster and the staircase.
    monster_pathfind mp;
    if(!mp.init_pathfind(m.pos(), you.pos()))
    {
        //No path, bail.
        return;
    }
    
    //Now get the actual time needed to reach the staircase.
    const std::vector<coord_def> path = mp.backtrack();    
    int time_to_stairs = (path.size() - 1) * 100 / m.speed;
    to_push.aut_to_staircase = time_to_stairs;
    
    to_push.mons_original_lid = origin;
    
    //We'll never revisit non-dungeon areas anyway, so let's make sure that
    //a) We don't clobber monsters we shouldn't, and
    //b) We don't accumulate a monstrously huge the_doomed_ones list.
    if(origin.level_type == LEVEL_DUNGEON)
        to_push.add_to_doomed = true;
    else
        to_push.add_to_doomed = false;
    
    //Instead of just pushing to the back of the list, order by time to reach
    //staircase.  This'll make it cleaner to pop them in order.
    bool inserted = false;
    for(std::list<follower>::iterator iter = mlist.begin(); iter !=mlist.end(); iter++)
    {
        if(time_to_stairs < (*iter).aut_to_staircase)
        {
            mlist.insert(iter, to_push);
            inserted = true;
            break;
        }
    }
    
    if(!inserted)
        mlist.push_back(to_push);

#ifdef DEBUG_DIAGNOSTICS
    mprf(MSGCH_DIAGNOSTICS, "Monster in transit: %s",
         m.name(DESC_PLAIN).c_str());
#endif

    const int how_many = mlist.size();
    if (how_many > MAX_LOST)
        cull_lost_mons(mlist, how_many);
}

void add_monster_to_transit(const level_id &lid, const monster& m)
{
    m_transit_list &mlist = the_lost_ones[lid];
    follower to_push = m;
    to_push.mons_original_index = m.mindex();
        
    int time_to_stairs = (m.pos() - you.pos()).rdist() * 100 / m.speed;
    to_push.aut_to_staircase = time_to_stairs;
    
    to_push.add_to_doomed = false;
    
    //Instead of just pushing to the back of the list, order by time to reach
    //staircase.  This'll make it cleaner to pop them in order.
    bool inserted = false;
    for(std::list<follower>::iterator iter = mlist.begin(); iter !=mlist.end(); iter++)
   {
        if(time_to_stairs < (*iter).aut_to_staircase)
        {
            mlist.insert(iter, to_push);
            inserted = true;
            break;
        }
    }
    
    if(!inserted)
        mlist.push_back(to_push);

#ifdef DEBUG_DIAGNOSTICS
    mprf(MSGCH_DIAGNOSTICS, "Monster in transit: %s",
         m.name(DESC_PLAIN).c_str());
#endif

    const int how_many = mlist.size();
    if (how_many > MAX_LOST)
        cull_lost_mons(mlist, how_many);
}

void add_monster_to_doomed(const level_id &lid, const int index)
{
    m_doomed_list &doomed = the_doomed_ones[lid];
    doomed.push_back(index);
}

void place_lost_ones(void (*placefn)(m_transit_list &ml, int time_taken), int time_taken)
{
    level_id c = level_id::current();

    monsters_in_transit::iterator i = the_lost_ones.find(c);
    if (i == the_lost_ones.end())
        return;
    placefn(i->second, time_taken);
    if (i->second.empty())
        the_lost_ones.erase(i);
}

void remove_doomed_monsters()
{
    level_id current_level = level_id::current();
    
    monsters_to_remove::iterator i = the_doomed_ones.find(current_level);
    if (i == the_doomed_ones.end())
        return;
    
    m_doomed_list doomed = i->second;
    for(m_doomed_list::iterator doomed_iterator = doomed.begin();
        doomed_iterator != doomed.end(); doomed_iterator++)
    {
        menv[*doomed_iterator].destroy_inventory();
        monster_cleanup(&menv[*doomed_iterator]);
    }
    
    the_doomed_ones.erase(i);
}

void place_transiting_monsters(int time_taken)
{
    place_lost_ones(level_place_lost_monsters, time_taken);
}

void place_followers(int time_taken)
{
    place_lost_ones(level_place_followers, time_taken);
}

static bool place_lost_monster(follower &f)
{
#ifdef DEBUG_DIAGNOSTICS
    mprf(MSGCH_DIAGNOSTICS, "Placing lost one: %s",
         f.mons.name(DESC_PLAIN).c_str());
#endif
    return (f.place(true));
}

static void level_place_lost_monsters(m_transit_list &m, int time_taken)
{
    //First, decrement all times to staircase.
    for (m_transit_list::iterator i = m.begin();
         i != m.end(); i++)
    {
        (*i).aut_to_staircase -= time_taken;
    }

    for (m_transit_list::iterator i = m.begin();
         i != m.end();)
    {
        m_transit_list::iterator mon = i++;

        // Monsters transiting to the Abyss have a 50% chance of being
        // placed, otherwise a 100% chance.
        if (you.level_type == LEVEL_ABYSS && coinflip())
            continue;

        // Monster hasn't yet reached the staircase!            
        if ((*mon).aut_to_staircase > 0)
            break;

        if (place_lost_monster(*mon))
        {
            if((*mon).add_to_doomed)
                add_monster_to_doomed((*mon).mons_original_lid, (*mon).mons_original_index);
            m.erase(mon);
        }
    }
}

static void level_place_followers(m_transit_list &m, int time_taken)
{
    //First, decrement all times to staircase.
    for (m_transit_list::iterator i = m.begin();
         i != m.end(); i++)
    {
        (*i).aut_to_staircase -= time_taken;
    }

    for (m_transit_list::iterator i = m.begin(); i != m.end();)
    {
        m_transit_list::iterator mon = i++;
            
        if ((*mon).aut_to_staircase > 0)
            break;

        //Erase the monster both from the transit list and from the original level.
        if ((mon->mons.flags & MF_TAKING_STAIRS) && mon->place(true))
        {
            //flag monster to be removed
            if((*mon).add_to_doomed)
                add_monster_to_doomed((*mon).mons_original_lid, (*mon).mons_original_index);
            m.erase(mon);
        }
    }
}

void add_item_to_transit(const level_id &lid, const item_def &i)
{
    i_transit_list &ilist = transiting_items[lid];
    ilist.push_back(i);

#ifdef DEBUG_DIAGNOSTICS
    mprf(MSGCH_DIAGNOSTICS, "Item in transit: %s",
         i.name(DESC_PLAIN).c_str());
#endif

    const int how_many = ilist.size();
    if (how_many > MAX_LOST)
        cull_lost_items(ilist, how_many);
}

void place_transiting_items()
{
    level_id c = level_id::current();

    items_in_transit::iterator i = transiting_items.find(c);
    if (i == transiting_items.end())
        return;

    i_transit_list &ilist = i->second;
    i_transit_list keep;
    i_transit_list::iterator item;

    for (item = ilist.begin(); item != ilist.end(); ++item)
    {
        coord_def pos = item->pos;

        if (!in_bounds(pos))
            pos = random_in_bounds();

        const coord_def where_to_go =
            dgn_find_nearby_stair(DNGN_ESCAPE_HATCH_DOWN,
                                  pos, true);

        // List of items we couldn't place.
        if (!copy_item_to_grid(*item, where_to_go, 1, false, true))
            keep.push_back(*item);
    }

    // Only unplaceable items are kept in list.
    ilist = keep;
}

//////////////////////////////////////////////////////////////////////////
// follower

follower::follower(const monster& m) : mons(m), items()
{
    load_mons_items();
}

void follower::load_mons_items()
{
    for (int i = 0; i < NUM_MONSTER_SLOTS; ++i)
        if (mons.inv[i] != NON_ITEM)
            items[i] = mitm[ mons.inv[i] ];
        else
            items[i].clear();
}

bool follower::place(bool near_player)
{
    for (int i = 0; i < MAX_MONSTERS - 5; ++i)
    {
        // Find first empty slot in menv and copy monster into it.
        monster& m = menv[i];
        if (m.alive())
            continue;
        m = mons;

        bool placed = false;

        // In certain instances (currently, falling through a shaft)
        // try to place monster as close as possible to its previous
        // <x,y> coordinates.
        if (!near_player && you.level_type == LEVEL_DUNGEON
            && in_bounds(m.pos()))
        {
            const coord_def where_to_go =
                dgn_find_nearby_stair(DNGN_ESCAPE_HATCH_DOWN,
                                      m.pos(), true);

            if (where_to_go == you.pos())
                near_player = true;
            else if (m.find_home_near_place(where_to_go))
                placed = true;
        }

        if (!placed)
            placed = m.find_place_to_live(near_player);

        if (placed)
        {
#ifdef DEBUG_DIAGNOSTICS
            mprf(MSGCH_DIAGNOSTICS, "Placed follower: %s",
                 m.name(DESC_PLAIN).c_str());
#endif
            m.target.reset();

            m.flags &= ~MF_TAKING_STAIRS;

            restore_mons_items(m);
            return (true);
        }

        m.reset();
        break;
    }

    return (false);
}

void follower::restore_mons_items(monster& m)
{
    for (int i = 0; i < NUM_MONSTER_SLOTS; ++i)
    {
        if (items[i].base_type == OBJ_UNASSIGNED)
            m.inv[i] = NON_ITEM;
        else
        {
            const int islot = get_mitm_slot(0);
            m.inv[i] = islot;
            if (islot == NON_ITEM)
                continue;

            item_def &it = mitm[islot];
            it = items[i];
            it.pos.set(-2, -2);
            it.link = NON_ITEM + 1 + m.mindex();
        }
    }
}

static bool _is_religious_follower(const monster* mon)
{
    return ((you.religion == GOD_YREDELEMNUL || you.religion == GOD_BEOGH)
            && is_follower(mon));
}

static bool _tag_follower_at(const coord_def &pos, bool &real_follower)
{
    if (!in_bounds(pos) || pos == you.pos())
        return (false);

    monster* fmenv = monster_at(pos);
    if (fmenv == NULL)
        return (false);

    if (!fmenv->alive()
        || fmenv->speed_increment < 50
        || fmenv->incapacitated()
        || mons_is_stationary(fmenv))
    {
        return (false);
    }

    if (!monster_habitable_grid(fmenv, DNGN_FLOOR))
        return (false);

    // Only non-wandering friendly monsters or those actively
    // seeking the player will follow up/down stairs.
    if (!fmenv->friendly()
          && (!mons_is_seeking(fmenv) || fmenv->foe != MHITYOU)
        || fmenv->foe == MHITNOT)
    {
        return (false);
    }

    // Monsters that can't use stairs can still be marked as followers
    // (though they'll be ignored for transit), so any adjacent real
    // follower can follow through. (jpeg)
    if (!mons_can_use_stairs(fmenv))
    {
        if (_is_religious_follower(fmenv))
        {
            fmenv->flags |= MF_TAKING_STAIRS;
            return (true);
        }
        return (false);
    }

    real_follower = true;

    // Monster is chasing player through stairs.
    fmenv->flags |= MF_TAKING_STAIRS;

    // Clear patrolling/travel markers.
    fmenv->patrol_point.reset();
    fmenv->travel_path.clear();
    fmenv->travel_target = MTRAV_NONE;

    fmenv->clear_clinging();

    dprf("%s is marked for following.",
         fmenv->name(DESC_CAP_THE, true).c_str());

    return (true);
}

void tag_followers()
{
    // If we're doing this, we should first clear out the old follower list.
    m_transit_list *current_list = get_transit_list(level_id::current());
    if(current_list != NULL)
        (*current_list).clear();
    
    // Iterate over every square in LOS
    for (radius_iterator i(you.pos(), LOS_RADIUS); i; ++i)
    {
        bool real_follower = false;
        _tag_follower_at(*i, real_follower);
    }
}

void untag_followers()
{
    for (int m = 0; m < MAX_MONSTERS; ++m)
        menv[m].flags &= (~MF_TAKING_STAIRS);
}
