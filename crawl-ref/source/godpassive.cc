#include "AppHdr.h"

#include "godpassive.h"

#include "artefact.h"
#include "branch.h"
#include "coord.h"
#include "coordit.h"
#include "defines.h"
#include "describe.h"
#include "env.h"
#include "files.h"
#include "food.h"
#include "fprop.h"
#include "godprayer.h"
#include "items.h"
#include "itemname.h"
#include "itemprop.h"
#include "math.h"
#include "mon-stuff.h"
#include "options.h"
#include "player.h"
#include "player-stats.h"
#include "religion.h"
#include "skills2.h"
#include "spl-book.h"
#include "state.h"

int che_boost_level()
{
    if (you.religion != GOD_CHEIBRIADOS || you.penance[GOD_CHEIBRIADOS])
        return (0);

    return (std::min(player_ponderousness(), 2*(piety_rank() - 1)));
}

int che_boost(che_boost_type bt, int level)
{
    if (level == 0)
        return (0);

    switch (bt)
    {
    case CB_RNEG:
        return (level > 1 ? 1 : 0);
    case CB_RCOLD:
        return (level > 3 ? 1 : 0);
    case CB_RFIRE:
        return (level > 5 ? 1 : 0);
    case CB_STATS:
        return ((level * 3)/2);
    default:
        return (0);
    }
}

void che_handle_change(che_change_type ct, int diff)
{
    if (you.religion != GOD_CHEIBRIADOS)
        return;

    const std::string typestr = (ct == CB_PIETY ? "piety" :
                (ct == CB_PONDEROUSNESS ? "ponderousness" :
            (ct == CB_PONDEROUS_COUNT ? "ponderous count" : "slots")));

    // Values after the change.
    const int ponder = player_ponderousness();
    const int prank = piety_rank() - 1;
    const int newlev = std::min(ponder, 2 * prank);

    // Reconstruct values before the change.
    int oldponder = ponder;
    int oldprank = prank;
    int slots = player_armour_slots();
    if (ct == CB_PIETY)
        oldprank -= diff;
    else if (ct == CB_PONDEROUSNESS)
        oldponder -= diff;
    else if (ct == CB_PONDEROUS_COUNT)
    {
        if (slots != 0)
            oldponder = ((player_ponderous_count() - diff) * 10)/slots;
    }
    else // ct == CB_SLOTS
    {
        if (slots - diff == 0)
            oldponder = 0;
        else
            oldponder = (player_ponderous_count() * 10)/(slots - diff);
    }
    const int oldlev = std::min(oldponder, 2 * oldprank);
    dprf("Che %s %+d: %d/%d -> %d/%d", typestr.c_str(), diff,
         oldponder, oldprank,
         ponder, prank);

    for (int i = 0; i < NUM_BOOSTS; ++i)
    {
        const che_boost_type bt = static_cast<che_boost_type>(i);
        const int boostdiff = che_boost(bt, newlev) - che_boost(bt, oldlev);
        if (boostdiff == 0)
            continue;

        const std::string elemmsg = god_name(you.religion)
                                    + (boostdiff > 0 ? " " : " no longer ")
                                    + "protects you from %s.";
        switch (bt)
        {
        case CB_RNEG:
            mprf(MSGCH_GOD, elemmsg.c_str(), "negative energy");
            break;
        case CB_RCOLD:
            mprf(MSGCH_GOD, elemmsg.c_str(), "cold");
            break;
        case CB_RFIRE:
            mprf(MSGCH_GOD, elemmsg.c_str(), "fire");
            break;
        case CB_STATS:
        {
            mprf(MSGCH_GOD, "%s %s the support of your attributes.",
                 god_name(you.religion).c_str(),
                 boostdiff > 0 ? "raises" : "reduces");
            const std::string reason = "Cheibriados " + typestr + " change";
            notify_stat_change(reason.c_str());
            break;
        }
        case NUM_BOOSTS:
            break;
        }
    }
}

monster_type che_monster_tier(const monster *mon)
{
    double factor = sqrt(exp_needed(you.experience_level) / 30.0);
    int tension = exper_value(mon) / (1 + factor);

    if (mon->friendly())
        return MONS_SENSED_FRIENDLY;
    else if (tension <= 0)
        // Conjurators use melee to conserve mana, MDFis switch plates...
        return MONS_SENSED_TRIVIAL;
    else if (tension <= 5)
        // An easy fight but not ignorable.
        return MONS_SENSED_EASY;
    else if (tension <= 32)
        // Hard but reasonable.
        return MONS_SENSED_TOUGH;
    else
        // Check all wands/jewels several times, wear brown pants...
        return MONS_SENSED_NASTY;
}

// Eat from one random off-level item stack.
void jiyva_eat_offlevel_items()
{
    // For wizard mode 'J' command
    if (you.religion != GOD_JIYVA)
        return;

    if (crawl_state.game_is_sprint())
        return;

    while (true)
    {
        if (one_chance_in(200))
            break;

        const int branch = random2(NUM_BRANCHES);
        int js = JS_NONE;

        // Choose level based on main dungeon depth so that levels short branches
        // aren't picked more often.
        ASSERT(branches[branch].depth <= branches[BRANCH_MAIN_DUNGEON].depth);
        const int level  = random2(branches[BRANCH_MAIN_DUNGEON].depth) + 1;

        const level_id lid(static_cast<branch_type>(branch), level);

        if (lid == level_id::current() || !is_existing_level(lid))
            continue;

        dprf("Checking %s", lid.describe().c_str());

        level_excursion le;
        le.go_to(lid);
        while (true)
        {
            if (one_chance_in(200))
                break;

            const coord_def p = random_in_bounds();

            if (igrd(p) == NON_ITEM || testbits(env.pgrid(p), FPROP_NO_JIYVA))
                continue;

            for (stack_iterator si(p); si; ++si)
            {
                if (!is_item_jelly_edible(*si) || one_chance_in(4))
                    continue;

                if (one_chance_in(4))
                    break;

                dprf("Eating %s on %s",
                     si->name(DESC_PLAIN).c_str(), lid.describe().c_str());

                // Needs a message now to explain possible hp or mp
                // gain from jiyva_slurp_bonus()
                mpr("You hear a distant slurping noise.");
                sacrifice_item_stack(*si, &js);
                destroy_item(si.link());
                jiyva_slurp_message(js);
            }
            return;
        }
    }
}

void jiyva_slurp_bonus(int item_value, int *js)
{
    if (you.penance[GOD_JIYVA])
        return;

    if (you.piety >= piety_breakpoint(1)
        && x_chance_in_y(you.piety, MAX_PIETY)
        && you.is_undead != US_UNDEAD)
    {
        //same as a sultana
        lessen_hunger(70, true);
        *js |= JS_FOOD;
    }

    if (you.piety >= piety_breakpoint(3)
        && x_chance_in_y(you.piety, MAX_PIETY)
        && you.magic_points < you.max_magic_points)
    {
         inc_mp(std::max(random2(item_value), 1), false);
         *js |= JS_MP;
     }

    if (you.piety >= piety_breakpoint(4)
        && x_chance_in_y(you.piety, MAX_PIETY)
        && you.hp < you.hp_max)
    {
         inc_hp(std::max(random2(item_value), 1), false);
         *js |= JS_HP;
     }
}

void jiyva_slurp_message(int js)
{
    if (js != JS_NONE)
    {
        if (js & JS_FOOD)
            mpr("You feel a little less hungry.");
        if (js & JS_MP)
            mpr("You feel your power returning.");
        if (js & JS_HP)
            mpr("You feel a little better.");
    }
}

enum eq_type
{
    ET_WEAPON,
    ET_ARMOUR,
    ET_JEWELS,
    NUM_ET
};

int ash_bondage_level(int type_only)
{
    if (you.religion != GOD_ASHENZARI)
        return (0);

    int cursed[NUM_ET] = {0}, slots[NUM_ET] = {0};

    for (int i = EQ_WEAPON; i < NUM_EQUIP; i++)
    {
        eq_type s;
        if (i == EQ_WEAPON)
            s = ET_WEAPON;
        else if (i <= EQ_MAX_ARMOUR)
            s = ET_ARMOUR;
        else
            s = ET_JEWELS;

        // kittehs don't obey hoomie rules!
        if (you.species == SP_CAT)
        {
            if (i >= EQ_LEFT_RING)
                s = (eq_type)(i - EQ_LEFT_RING);
            else
                ASSERT(!you_can_wear(i, true));
        }

        // transformed away slots are still considered to be possibly bound
        if (you_can_wear(i, true))
        {
            slots[s]++;
            if (you.equip[i] != -1)
            {
                const item_def& item = you.inv[you.equip[i]];
                if (item.cursed()
                    && (i != EQ_WEAPON
                        || item.base_type == OBJ_WEAPONS
                        || item.base_type == OBJ_STAVES))
                {
                    cursed[s]++;
                }
            }
        }
    }

    int bonus = 0;
    for (int s = ET_WEAPON; s < NUM_ET; s++)
    {
        if (type_only && s+1 != type_only)
            continue;
        if (cursed[s] > slots[s] / 2)
            bonus++;
    }
    return bonus;
}

void ash_check_bondage()
{
    int new_level = ash_bondage_level();

    if (new_level == you.bondage_level)
        return;

    if (new_level > you.bondage_level)
        mprf(MSGCH_GOD, "You feel %s bound.",
             (new_level == 1) ? "slightly" :
             (new_level == 2) ? "seriously" :
             (new_level == 3) ? "completely" :
                                "buggily");
    else
        mprf(MSGCH_GOD, "You feel less bound.");
    you.bondage_level = new_level;
}

static bool is_ash_portal(dungeon_feature_type feat)
{
    switch (feat)
    {
    case DNGN_ENTER_HELL:
    case DNGN_ENTER_LABYRINTH:
    case DNGN_ENTER_ABYSS: // for completeness/Pan
    case DNGN_EXIT_ABYSS:
    case DNGN_ENTER_PANDEMONIUM:
    case DNGN_EXIT_PANDEMONIUM:
    // DNGN_TRANSIT_PANDEMONIUM is too mundane
    case DNGN_ENTER_PORTAL_VAULT:
        return true;
    default:
        return false;
    }
}

// Yay for rectangle_iterator and radius_iterator not sharing a base type
static bool _check_portal(coord_def where)
{
    const dungeon_feature_type feat = grd(where);
    if (feat != env.map_knowledge(where).feat() && is_ash_portal(feat))
    {
        env.map_knowledge(where).set_feature(feat);
        env.map_knowledge(where).flags |= MAP_MAGIC_MAPPED_FLAG;

        if (!testbits(env.pgrid(where), FPROP_SEEN_OR_NOEXP))
        {
            env.pgrid(where) |= FPROP_SEEN_OR_NOEXP;
            if (!you.see_cell(where))
                return true;
        }
    }
    return false;
}

int ash_detect_portals(bool all)
{
    if (you.religion != GOD_ASHENZARI)
        return 0;

    int portals_found = 0;
    const int map_radius = LOS_RADIUS + 1;

    if (all)
    {
        for (rectangle_iterator ri(0); ri; ++ri)
        {
            if (_check_portal(*ri))
                portals_found++;
        }
    }
    else
    {
        for (radius_iterator ri(you.pos(), map_radius, C_ROUND); ri; ++ri)
        {
            if (_check_portal(*ri))
                portals_found++;
        }
    }

    you.seen_portals += portals_found;
    return (portals_found);
}

#if 0
int ash_skill_boost(skill_type sk)
{
    int level = you.skills[sk];
    std::set<skill_type> boosted_skills;

    bool bondage_types[NUM_ET];
    for (int i = 0; i < NUM_ET; i++)
        bondage_types[i] = ash_bondage_level(i+1);

    const item_def* wpn = you.weapon();
    if (wpn && bondage_types[ET_WEAPON])
    {
        // Boost your weapon skill.
        if(wpn->base_type == OBJ_WEAPONS)
        {
            boosted_skills.insert(is_range_weapon(*wpn) ? range_skill(*wpn)
                                                        : weapon_skill(*wpn));
        }
        // Those staves don't benefit from evocation.
        //Boost spellcasting instead.
        else if (item_is_staff(*wpn)
                 && (wpn->sub_type == STAFF_POWER
                     || wpn->sub_type == STAFF_CONJURATION
                     || wpn->sub_type == STAFF_ENCHANTMENT
                     || wpn->sub_type == STAFF_ENERGY
                     || wpn->sub_type == STAFF_WIZARDRY))
        {
            boosted_skills.insert(SK_SPELLCASTING);
        }
        // Those staves and rods use evocation. Boost it.
        else if (item_is_staff(*wpn) || item_is_rod(*wpn))
        {
            boosted_skills.insert(SK_EVOCATIONS);
        }
    }

    if (bondage_types[ET_ARMOUR])
    {
        // Boost armour or dodging, whichever is higher.
        boosted_skills.insert(compare_skills(SK_ARMOUR, SK_DODGING)
                              ? SK_ARMOUR
                              : SK_DODGING);
    }

    if (bondage_types[ET_JEWELS])
    {
        // Boost your highest magical skill.
        skill_type highest = SK_NONE;
        for (int i = SK_FIRST_MAGIC_SCHOOL; i <= SK_LAST_MAGIC_SCHOOL; ++i)
            if (compare_skills(skill_type(i), highest))
                highest = skill_type(i);

        boosted_skills.insert(highest);
    }

    // Apply the skill boost.
    if (boosted_skills.find(sk) != boosted_skills.end())
        level += std::max(0, piety_rank() - 3);

    // If not wearing uncursed items, boost low skills.
    if (!you.wear_uncursed && you.skills[sk])
        level = std::max(level, piety_rank() - 1);

    return std::min(level, 27);
}
#endif
