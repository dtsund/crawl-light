#include "AppHdr.h"

#include "ng-setup.h"

#include "abl-show.h"
#include "dungeon.h"
#include "files.h"
#include "food.h"
#include "hints.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "item_use.h"
#include "jobs.h"
#include "maps.h"
#include "misc.h"
#include "mutation.h"
#include "newgame.h"
#include "ng-init.h"
#include "ng-wanderer.h"
#include "options.h"
#include "player.h"
#include "skills.h"
#include "skills2.h"
#include "spl-book.h"
#include "spl-util.h"
#include "sprint.h"
#include "state.h"
#include "tutorial.h"

#define MIN_START_STAT       3

static void _init_player(void)
{
    you.init();
    dlua.callfn("dgn_clear_data", "");
}

// Recall that demonspawn & demigods get more later on. {dlb}
static void _species_stat_init(species_type which_species)
{
    int sb = 0; // strength base
    int ib = 0; // intelligence base
    int db = 0; // dexterity base

    // Note: The stats in in this list aren't intended to sum the same
    // for all races.  The fact that Mummies and Ghouls are really low
    // is considered acceptable (Mummies don't have to eat, and Ghouls
    // are supposed to be a really hard race). - bwr
    switch (which_species)
    {
    default:                    sb =  6; ib =  6; db =  6;      break;  // 18
    case SP_HUMAN:              sb =  6; ib =  6; db =  6;      break;  // 18
    case SP_DEMIGOD:            sb =  9; ib = 10; db =  9;      break;  // 28
    case SP_DEMONSPAWN:         sb =  6; ib =  7; db =  6;      break;  // 19

    case SP_HIGH_ELF:           sb =  5; ib =  9; db =  8;      break;  // 22
    case SP_DEEP_ELF:           sb =  3; ib = 10; db =  8;      break;  // 21
    case SP_SLUDGE_ELF:         sb =  6; ib =  7; db =  7;      break;  // 20

    case SP_MOUNTAIN_DWARF:     sb =  9; ib =  4; db =  5;      break;  // 18
    case SP_DEEP_DWARF:         sb =  9; ib =  6; db =  6;      break;  // 21

    case SP_TROLL:              sb = 13; ib =  2; db =  3;      break;  // 18
    case SP_OGRE:               sb = 10; ib =  5; db =  3;      break;  // 18

    case SP_MINOTAUR:           sb = 10; ib =  3; db =  3;      break;  // 16
    case SP_HILL_ORC:           sb =  8; ib =  6; db =  4;      break;  // 16
    case SP_CENTAUR:            sb =  8; ib =  5; db =  2;      break;  // 15
    case SP_NAGA:               sb =  8; ib =  6; db =  4;      break;  // 18

    case SP_MERFOLK:            sb =  6; ib =  5; db =  7;      break;  // 18
    case SP_KENKU:              sb =  6; ib =  6; db =  7;      break;  // 19

    case SP_KOBOLD:             sb =  5; ib =  4; db =  8;      break;  // 17
    case SP_HALFLING:           sb =  3; ib =  6; db =  9;      break;  // 18
    case SP_SPRIGGAN:           sb =  2; ib =  7; db =  9;      break;  // 18

    case SP_MUMMY:              sb =  9; ib =  5; db =  5;      break;  // 19
    case SP_GHOUL:              sb =  9; ib =  1; db =  2;      break;  // 12
    case SP_VAMPIRE:            sb =  5; ib =  8; db =  7;      break;  // 20

    case SP_RED_DRACONIAN:
    case SP_WHITE_DRACONIAN:
    case SP_GREEN_DRACONIAN:
    case SP_YELLOW_DRACONIAN:
    case SP_GREY_DRACONIAN:
    case SP_BLACK_DRACONIAN:
    case SP_PURPLE_DRACONIAN:
    case SP_MOTTLED_DRACONIAN:
    case SP_PALE_DRACONIAN:
    case SP_BASE_DRACONIAN:     sb =  8; ib =  6; db =  4;      break;  // 18

    case SP_CAT:                sb =  2; ib =  7; db =  9;      break;  // 18
    }

    you.base_stats[STAT_STR] = sb + 2;
    you.base_stats[STAT_INT] = ib + 2;
    you.base_stats[STAT_DEX] = db + 2;
}

static void _give_last_paycheck(job_type which_job)
{
    switch (which_job)
    {
    case JOB_HEALER:
        you.gold = 100;
        break;

    case JOB_WANDERER:
    case JOB_WARPER:
    case JOB_ARCANE_MARKSMAN:
    case JOB_ASSASSIN:
        you.gold = 50;
        break;

    default:
        you.gold = 20;
        break;

    case JOB_MONK:
        you.gold = 0;
        break;
    }
}

// Randomly boost stats a number of times.
static void _wanderer_assign_remaining_stats(int points_left)
{
    while (points_left > 0)
    {
        // Stats that are already high will be chosen half as often.
        stat_type stat = static_cast<stat_type>(random2(NUM_STATS));
        if (you.base_stats[stat] > 17 && coinflip())
            continue;

        you.base_stats[stat]++;
        points_left--;
    }
}

static void _jobs_stat_init(job_type which_job)
{
    int s = 0;   // strength mod
    int i = 0;   // intelligence mod
    int d = 0;   // dexterity mod
    int hp = 0;  // HP base
    int mp = 0;  // MP base

    // Note: Wanderers are correct, they've got a challenging background. - bwr
    switch (which_job)
    {
    case JOB_FIGHTER:           s =  8; i =  0; d =  4; hp = 18; mp = 0; break;
    case JOB_BERSERKER:         s =  9; i = -1; d =  4; hp = 18; mp = 0; break;
    case JOB_GLADIATOR:         s =  7; i =  0; d =  5; hp = 17; mp = 0; break;

    case JOB_CRUSADER:          s =  4; i =  4; d =  4; hp = 16; mp = 1; break;
    case JOB_REAVER:            s =  4; i =  4; d =  4; hp = 16; mp = 1; break;
    case JOB_CHAOS_KNIGHT:      s =  4; i =  4; d =  4; hp = 16; mp = 1; break;
    case JOB_DEATH_KNIGHT:      s =  5; i =  3; d =  4; hp = 16; mp = 2; break;
    case JOB_ABYSSAL_KNIGHT:    s =  4; i =  4; d =  4; hp = 16; mp = 1; break;

    case JOB_HEALER:            s =  5; i =  5; d =  2; hp = 16; mp = 2; break;
    case JOB_PRIEST:            s =  5; i =  4; d =  3; hp = 16; mp = 2; break;

    case JOB_ASSASSIN:          s =  3; i =  3; d =  6; hp = 15; mp = 0; break;
    case JOB_STALKER:           s =  2; i =  4; d =  6; hp = 15; mp = 1; break;

    case JOB_HUNTER:            s =  4; i =  3; d =  5; hp = 16; mp = 0; break;
    case JOB_WARPER:            s =  3; i =  5; d =  4; hp = 15; mp = 1; break;
    case JOB_ARCANE_MARKSMAN:   s =  3; i =  5; d =  4; hp = 15; mp = 1; break;

    case JOB_MONK:              s =  3; i =  2; d =  7; hp = 16; mp = 0; break;
    case JOB_TRANSMUTER:        s =  2; i =  5; d =  5; hp = 15; mp = 1; break;

    case JOB_WIZARD:            s = -1; i = 10; d =  3; hp = 11; mp = 5; break;
    case JOB_CONJURER:          s =  0; i =  7; d =  5; hp = 13; mp = 3; break;
    case JOB_ENCHANTER:         s =  0; i =  7; d =  5; hp = 13; mp = 3; break;
    case JOB_FIRE_ELEMENTALIST: s =  0; i =  7; d =  5; hp = 13; mp = 3; break;
    case JOB_ICE_ELEMENTALIST:  s =  0; i =  7; d =  5; hp = 13; mp = 3; break;
    case JOB_AIR_ELEMENTALIST:  s =  0; i =  7; d =  5; hp = 13; mp = 3; break;
    case JOB_EARTH_ELEMENTALIST:s =  0; i =  7; d =  5; hp = 13; mp = 3; break;
    case JOB_SUMMONER:          s =  0; i =  7; d =  5; hp = 13; mp = 3; break;
    case JOB_VENOM_MAGE:        s =  0; i =  7; d =  5; hp = 13; mp = 3; break;
    case JOB_NECROMANCER:       s =  0; i =  7; d =  5; hp = 13; mp = 3; break;

    case JOB_WANDERER:
    {
        // Wanderers get their stats randomly distributed.
        _wanderer_assign_remaining_stats(12);
                                                        hp = 14; mp = 1; break;
    }

    case JOB_ARTIFICER:         s =  3; i =  4; d =  5; hp = 16; mp = 1; break;
    default:                    s =  0; i =  0; d =  0; hp = 13; mp = 0; break;
    }

    you.base_stats[STAT_STR] += s;
    you.base_stats[STAT_INT] += i;
    you.base_stats[STAT_DEX] += d;

    // Used for Jiyva's stat swapping if the player has not reached
    // experience level 3.
    you.last_chosen = (stat_type) random2(NUM_STATS);

    set_hp(hp, true);
    set_mp(mp, true);
}

// Make sure no stats are unacceptably low
// (currently possible only for GhBe - 1KB)
void unfocus_stats()
{
    int needed;

    for (int i = 0; i < NUM_STATS; ++i)
    {
        int j = (i + 1) % NUM_STATS;
        int k = (i + 2) % NUM_STATS;
        if ((needed = MIN_START_STAT - you.base_stats[i]) > 0)
        {
            if (you.base_stats[j] > you.base_stats[k])
                you.base_stats[j] -= needed;
            else
                you.base_stats[k] -= needed;
            you.base_stats[i] = MIN_START_STAT;
        }
    }
}

// Some consumables to make the starts of Sprint and Zotdef a little easier.
void _give_bonus_items()
{
    newgame_give_item(OBJ_POTIONS, POT_CURING);
    newgame_give_item(OBJ_POTIONS, POT_HEAL_WOUNDS);
    newgame_give_item(OBJ_POTIONS, POT_SPEED);
    newgame_give_item(OBJ_POTIONS, POT_MAGIC, 2);
    newgame_give_item(OBJ_POTIONS, POT_BERSERK_RAGE);
    newgame_give_item(OBJ_SCROLLS, SCR_BLINKING);
}

void give_basic_mutations(species_type speci)
{
    switch (speci)
    {
    case SP_OGRE:
        you.mutation[MUT_TOUGH_SKIN]      = 1;
        break;
    case SP_HALFLING:
        you.mutation[MUT_MUTATION_RESISTANCE] = 1;
        you.mutation[MUT_GLOW_TOLERANCE]      = 1;
        break;
    case SP_MINOTAUR:
        you.mutation[MUT_HORNS] = 2;
        break;
    case SP_SPRIGGAN:
        you.mutation[MUT_ACUTE_VISION]      = 1;
        you.mutation[MUT_FAST]              = 3;
        you.mutation[MUT_GLOW_INTOLERANCE]  = 3;
        break;
    case SP_CENTAUR:
        you.mutation[MUT_TOUGH_SKIN]      = 3;
        you.mutation[MUT_FAST]            = 2;
        you.mutation[MUT_DEFORMED]        = 1;
        you.mutation[MUT_HOOVES]          = 3;
        break;
    case SP_NAGA:
        you.mutation[MUT_ACUTE_VISION]      = 1;
        you.mutation[MUT_POISON_RESISTANCE] = 1;
        you.mutation[MUT_DEFORMED]          = 1;
        you.mutation[MUT_SLOW]              = 2;
        break;
    case SP_MUMMY:
        you.mutation[MUT_TORMENT_RESISTANCE]         = 1;
        you.mutation[MUT_POISON_RESISTANCE]          = 1;
        you.mutation[MUT_COLD_RESISTANCE]            = 1;
        you.mutation[MUT_NEGATIVE_ENERGY_RESISTANCE] = 3;
        you.mutation[MUT_UNBREATHING]                = 1;
        you.mutation[MUT_GLOW_TOLERANCE]             = 3;
        break;
    case SP_DEEP_DWARF:
        you.mutation[MUT_SLOW_HEALING]    = 3;
        you.mutation[MUT_PASSIVE_MAPPING] = 1;
        break;
    case SP_GHOUL:
        you.mutation[MUT_TORMENT_RESISTANCE]         = 1;
        you.mutation[MUT_POISON_RESISTANCE]          = 1;
        you.mutation[MUT_COLD_RESISTANCE]            = 1;
        you.mutation[MUT_NEGATIVE_ENERGY_RESISTANCE] = 3;
        you.mutation[MUT_SLOW_HEALING]               = 1;
        you.mutation[MUT_UNBREATHING]                = 1;
        break;
    case SP_KENKU:
        you.mutation[MUT_BEAK]   = 1;
        you.mutation[MUT_TALONS] = 3;
        break;
    case SP_TROLL:
        you.mutation[MUT_TOUGH_SKIN]       = 2;
        you.mutation[MUT_REGENERATION]     = 2;
        you.mutation[MUT_GLOW_INTOLERANCE] = 3;
        you.mutation[MUT_SHAGGY_FUR]       = 1;
        break;
    case SP_VAMPIRE:
        you.mutation[MUT_FANGS]        = 3;
        you.mutation[MUT_ACUTE_VISION] = 1;
        you.mutation[MUT_UNBREATHING]  = 1;
        break;
    case SP_CAT:
        you.mutation[MUT_FANGS]           = 3;
        you.mutation[MUT_SHAGGY_FUR]      = 1;
        you.mutation[MUT_ACUTE_VISION]    = 1;
        you.mutation[MUT_FAST]            = 1;
        break;
    default:
        break;
    }

    // Zot def games come with teleport control
    if (crawl_state.game_is_zotdef())
        you.mutation[MUT_TELEPORT_CONTROL] = 1;

    // Some mutations out-sourced because they're
    // relevant during character choice.
    you.mutation[MUT_CLAWS] = species_has_claws(speci, true);

    // Starting mutations are unremovable.
    for (int i = 0; i < NUM_MUTATIONS; ++i)
        you.innate_mutations[i] = you.mutation[i];
}

static void _newgame_make_item_tutorial(int slot, equipment_type eqslot,
                                 object_class_type base,
                                 int sub_type, int replacement = -1,
                                 int qty = 1, int plus = 0, int plus2 = 0)
{
    newgame_make_item(slot, eqslot, base, sub_type, replacement, qty, plus,
                      plus2, true);
}

// Creates an item of a given base and sub type.
// replacement is used when handing out armour that is not wearable for
// some species; otherwise use -1.
void newgame_make_item(int slot, equipment_type eqslot,
                       object_class_type base,
                       int sub_type, int replacement,
                       int qty, int plus, int plus2, bool force_tutorial)
{
    // Don't set normal equipment in the tutorial.
    if (!force_tutorial && crawl_state.game_is_tutorial())
        return;

    if (slot == -1)
    {
        // If another of the item type is already there, add to the
        // stack instead.
        for (int i = 0; i < ENDOFPACK; ++i)
        {
            item_def& item = you.inv[i];
            if (item.base_type == base && item.sub_type == sub_type
                && is_stackable_item(item))
            {
                item.quantity += qty;
                return;
            }
        }

        for (int i = 0; i < ENDOFPACK; ++i)
        {
            if (!you.inv[i].defined())
            {
                slot = i;
                break;
            }
        }
        ASSERT(slot != -1);
    }

    item_def &item(you.inv[slot]);
    item.base_type = base;
    item.sub_type  = sub_type;
    item.quantity  = qty;
    item.plus      = plus;
    item.plus2     = plus2;
    item.special   = 0;

    // If the character is restricted in wearing armour of equipment
    // slot eqslot, hand out replacement instead.
    if (item.base_type == OBJ_ARMOUR && replacement != -1
        && !can_wear_armour(item, false, true))
    {
        item.sub_type = replacement;
    }

    if (eqslot != EQ_NONE && you.equip[eqslot] == -1)
        you.equip[eqslot] = slot;

    //Set deck rarity; always plain for now.
    if (is_deck(item))
        item.special = DECK_RARITY_COMMON;
}

void newgame_give_item(object_class_type base, int sub_type,
                       int qty, int plus, int plus2)
{
    newgame_make_item(-1, EQ_NONE, base, sub_type, -1, qty, plus, plus2);
}

static void _newgame_clear_item(int slot)
{
    you.inv[slot] = item_def();

    for (int i = EQ_WEAPON; i < NUM_EQUIP; ++i)
        if (you.equip[i] == slot)
            you.equip[i] = -1;
}

static void _give_wand(const newgame_def& ng)
{
    bool is_rod;
    int wand = start_to_wand(ng.wand, is_rod);
    ASSERT(wand != -1);

    if (is_rod)
        make_rod(you.inv[2], STAFF_STRIKING, 8);
    else
    {
        // 1 wand of random effects and one chosen lesser wand
        const wand_type choice = static_cast<wand_type>(wand);
        const int ncharges = 15;
        newgame_make_item(2, EQ_NONE, OBJ_WANDS, WAND_RANDOM_EFFECTS,
                           -1, 1, ncharges, 0);
        newgame_make_item(3, EQ_NONE, OBJ_WANDS, choice,
                           -1, 1, ncharges, 0);
    }
}

static void _update_weapon(const newgame_def& ng)
{
    ASSERT(ng.weapon != NUM_WEAPONS);

    if (ng.weapon == WPN_UNARMED)
        _newgame_clear_item(0);
    else if (ng.weapon != WPN_UNKNOWN)
        you.inv[0].sub_type = ng.weapon;
}

static void _enchant_weapon(int slot, int plus, int plus2)
{
    you.inv[slot].plus = plus;
    you.inv[slot].plus2 = plus2;
}

static void _give_items_skills(const newgame_def& ng)
{
    int weap_skill = 0;
    int curr;

    switch (you.char_class)
    {
    case JOB_FIGHTER:
        // Equipment.
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD);
        _update_weapon(ng);

        if (player_genus(GENPC_DRACONIAN))
        {
            newgame_make_item(1, EQ_GLOVES, OBJ_ARMOUR, ARM_GLOVES, -1, 1, 0,
                              TGLOV_DESC_GAUNTLETS);
            newgame_make_item(3, EQ_BOOTS, OBJ_ARMOUR, ARM_BOOTS);
        }
        else
        {
            newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_SCALE_MAIL,
                              ARM_ROBE);
        }
        newgame_make_item(2, EQ_SHIELD, OBJ_ARMOUR, ARM_SHIELD, ARM_BUCKLER);
        
        //Give Fighters the next weapon up in whatever type they chose,
        //to give the class a draw compared to, say, Berserkers.
        switch(ng.weapon)
        {
        case WPN_SHORT_SWORD:
            //Yes, this isn't a huge upgrade.
            //No, you may not start with a quick blade.
            newgame_make_item(4, EQ_NONE, OBJ_WEAPONS, WPN_SABRE);
            break;
        case WPN_HAND_AXE:
            newgame_make_item(4, EQ_NONE, OBJ_WEAPONS, WPN_WAR_AXE);
            break;
        case WPN_MACE:
            //Flails aren't a big enough upgrade from maces to make this
            //interesting, so jump to morningstars.
            newgame_make_item(4, EQ_NONE, OBJ_WEAPONS, WPN_MORNINGSTAR);
            break;
        case WPN_TRIDENT:
            //Halberds can't be used with shields, which would kind of make
            //the class a bit schizophrenic.  Demon tridents are out of
            //the question.  Therefore, just enchant the starting weapon
            //a bit instead.
            _enchant_weapon(0, 2, 2);
            break;
        case WPN_FALCHION:
            newgame_make_item(4, EQ_NONE, OBJ_WEAPONS, WPN_LONG_SWORD);
            break;
        default:
            break;
        }

        // Skills.
        you.skills[SK_FIGHTING] = 3;
        you.skills[SK_SHIELDS]  = 2;

        weap_skill = (you.species == SP_CAT) ? 4 : 2;

        you.skills[(player_effectively_in_light_armour()
                   ? SK_DODGING : SK_ARMOUR)] = 3;

        break;

    case JOB_GLADIATOR:
    {
        // Equipment.
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD);
        _update_weapon(ng);

        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_LEATHER_ARMOUR,
                           ARM_ANIMAL_SKIN);
        
        //No buckler anymore.  Instead, gladiators get stronger ranged options.

        curr = 3;
        if (you_can_wear(EQ_HELMET))
        {
            newgame_make_item(3, EQ_HELMET, OBJ_ARMOUR, ARM_HELMET);
            curr++;
        }

        // Small species get darts (some exploding), the others nets and javelins.
        if (you.body_size(PSIZE_BODY) < SIZE_MEDIUM)
        {
            newgame_make_item(curr++, EQ_NONE, OBJ_MISSILES, MI_DART, -1, 15, 1);
            //Make some exploding darts.  They won't last long, but will be
            //fun and useful while they're there.
            newgame_make_item(curr, EQ_NONE, OBJ_MISSILES, MI_DART, -1, 30, 1);
            set_item_ego_type(you.inv[curr++], OBJ_MISSILES, SPMSL_EXPLODING);
        }
        else
        {
            //Four throwing nets and six +1 javelins.
            newgame_make_item(curr++, EQ_NONE, OBJ_MISSILES, MI_THROWING_NET, -1,
                               4);
            newgame_make_item(curr, EQ_NONE, OBJ_MISSILES, MI_JAVELIN, -1, 25, 1);
            
        }

        // Skills.
        you.skills[SK_FIGHTING] = 2;
        you.skills[SK_THROWING] = 3;
        you.skills[SK_DODGING]  = 3;
        you.skills[SK_UNARMED_COMBAT] = 2;
        weap_skill = 3;
        break;
    }

    case JOB_MONK:
        // Equipment.
        curr = 0;
        if (ng.weapon == WPN_QUARTERSTAFF)
        {
            newgame_make_item(curr++, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD);
            _update_weapon(ng);
        }
        else
            you.equip[EQ_WEAPON] = -1;

        newgame_make_item(curr++, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);

        you.skills[SK_FIGHTING]       = 3;

        if (ng.weapon == WPN_QUARTERSTAFF)
        {
            you.skills[SK_STAVES] = 3;
            you.skills[SK_UNARMED_COMBAT] = 1;
        }
        else
            you.skills[SK_UNARMED_COMBAT] = 4;

        you.skills[SK_DODGING]        = 3;
        you.skills[SK_STEALTH]        = 2;
        break;

    case JOB_BERSERKER:
        you.religion = GOD_TROG;
        you.piety = 35;

        // WEAPONS
        if (you.has_claws())
            you.equip[EQ_WEAPON] = -1; // Trolls/Ghouls/Felids fight unarmed.
        else
        {
            weapon_type startwep = WPN_HAND_AXE;
            if (species_apt(SK_MACES_FLAILS) > species_apt(SK_AXES))
                startwep = WPN_MACE;
            else if (species_apt(SK_POLEARMS) > species_apt(SK_AXES))
                startwep = WPN_SPEAR;
            else if (species_apt(SK_SHORT_BLADES) > species_apt(SK_AXES))
                startwep = WPN_SHORT_SWORD;

            newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, startwep);
        }

        // ARMOUR
        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ANIMAL_SKIN);

        // SKILLS
        you.skills[SK_FIGHTING] = 3;
        you.skills[SK_DODGING]  = 2;
        weap_skill = 3;

        if (you_can_wear(EQ_BODY_ARMOUR))
            you.skills[SK_ARMOUR] = 2;
        else
        {
            you.skills[SK_DODGING]++;
            you.skills[SK_ARMOUR] = 1; // for the eventual dragon scale mail :)
        }
        break;

    case JOB_PRIEST:
        you.religion = ng.religion;
        you.piety = 45;

        if (you.religion == GOD_BEOGH)
            newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_HAND_AXE);
        else
            newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_QUARTERSTAFF);

        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);

        you.skills[SK_FIGHTING]    = 2;
        you.skills[SK_INVOCATIONS] = 5;
        you.skills[SK_DODGING]     = 1;
        weap_skill = 3;
        break;

    case JOB_CHAOS_KNIGHT:
        you.religion = GOD_XOM;
        you.piety = 100;
        you.gift_timeout = std::max(5, random2(40) + random2(40));

        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD, -1, 1,
                           2, 2);
        _update_weapon(ng);

        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_LEATHER_ARMOUR,
                           ARM_ROBE, 1, 2);

        you.skills[SK_FIGHTING] = 4;
        you.skills[SK_ARMOUR]   = 1;
        you.skills[SK_DODGING]  = 1;
        if (species_apt(SK_ARMOUR) < species_apt(SK_DODGING))
            you.skills[SK_DODGING]++;
        else
            you.skills[SK_ARMOUR]++;
        weap_skill = 2;
        break;

    case JOB_DEATH_KNIGHT:
        you.religion = GOD_YREDELEMNUL;
        you.piety = 35;

        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD, -1, 1,
                          1, 1);
        _update_weapon(ng);

        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_LEATHER_ARMOUR,
                              ARM_ROBE);

        you.skills[SK_FIGHTING]    = 2;
        you.skills[SK_ARMOUR]      = 1;
        you.skills[SK_DODGING]     = 1;
        you.skills[SK_INVOCATIONS] = 3;
        weap_skill = 2;
        break;

    case JOB_ABYSSAL_KNIGHT:
        you.religion = GOD_LUGONU;
        if (!crawl_state.game_is_zotdef())
            you.char_direction = GDT_GAME_START;
        you.piety = 38;

        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD, -1, 1,
                          2, 2);
        _update_weapon(ng);

        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_LEATHER_ARMOUR,
                              ARM_ROBE);

        you.skills[SK_FIGHTING]    = 3;
        you.skills[SK_ARMOUR]      = 1;
        you.skills[SK_DODGING]     = 1;
        if (species_apt(SK_ARMOUR) < species_apt(SK_DODGING))
            you.skills[SK_DODGING]++;
        else
            you.skills[SK_ARMOUR]++;

        you.skills[SK_INVOCATIONS] = 2;
        weap_skill = 2;
        break;

    case JOB_HEALER:
        you.religion = GOD_ELYVILON;
        you.piety = 55;

        you.equip[EQ_WEAPON] = -1;

        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE, -1, 1, 1);
        newgame_make_item(1, EQ_NONE, OBJ_POTIONS, POT_CURING);
        newgame_make_item(2, EQ_NONE, OBJ_POTIONS, POT_HEAL_WOUNDS);

        you.skills[SK_FIGHTING]       = 2;
        you.skills[SK_DODGING]        = 2;
        you.skills[SK_INVOCATIONS]    = 4;
        break;

    case JOB_CRUSADER:
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD);
        _update_weapon(ng);

        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_LEATHER_ARMOUR,
                           ARM_ROBE);
        newgame_make_item(2, EQ_NONE, OBJ_BOOKS, BOOK_WAR_CHANTS);
        newgame_make_item(3, EQ_NONE, OBJ_POTIONS, POT_RESTORE_ABILITIES);

        you.skills[SK_FIGHTING]     = 2;
        you.skills[SK_ARMOUR]       = 1;
        you.skills[SK_DODGING]      = 1;
        you.skills[SK_SPELLCASTING] = 2;
        you.skills[SK_ENCHANTMENTS] = 2;
        weap_skill = 2;
        break;
    
    case JOB_REAVER:
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD);
        _update_weapon(ng);

        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_LEATHER_ARMOUR,
                           ARM_ROBE);
        newgame_make_item(2, EQ_NONE, OBJ_BOOKS, BOOK_RUIN);

        you.skills[SK_FIGHTING]     = 2;
        you.skills[SK_ARMOUR]       = 1;
        you.skills[SK_DODGING]      = 1;
        you.skills[SK_SPELLCASTING] = 2;
        you.skills[SK_CONJURATIONS] = 2;
        weap_skill = 2;
        break;

    case JOB_WARPER:
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_SHORT_SWORD);
        _update_weapon(ng);

        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_LEATHER_ARMOUR,
                           ARM_ROBE);
        newgame_make_item(2, EQ_NONE, OBJ_BOOKS, BOOK_SPATIAL_TRANSLOCATIONS);

        // One free escape.
        newgame_make_item(3, EQ_NONE, OBJ_SCROLLS, SCR_BLINKING);
        newgame_make_item(4, EQ_NONE, OBJ_MISSILES, MI_DART, -1, 40, 1);

        newgame_make_item(5, EQ_NONE, OBJ_MISSILES, MI_DART, -1, 5);
        set_item_ego_type(you.inv[5], OBJ_MISSILES, SPMSL_DISPERSAL);

        you.skills[SK_FIGHTING]       = 1;
        you.skills[SK_ARMOUR]         = 1;
        you.skills[SK_DODGING]        = 2;
        you.skills[SK_SPELLCASTING]   = 2;
        you.skills[SK_TRANSLOCATIONS] = 3;
        you.skills[SK_THROWING]       = 1;
        weap_skill = 3;
    break;

    case JOB_ARCANE_MARKSMAN:
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);

        switch (you.species)
        {
        case SP_HALFLING:
            newgame_make_item(1, EQ_NONE, OBJ_WEAPONS, WPN_SLING);

            // Wield the sling instead.
            you.equip[EQ_WEAPON] = 1;
            break;

        case SP_MOUNTAIN_DWARF:
        case SP_DEEP_DWARF:
        case SP_KOBOLD:
            newgame_make_item(1, EQ_NONE, OBJ_WEAPONS, WPN_CROSSBOW);

            // Wield the crossbow instead.
            you.equip[EQ_WEAPON] = 1;
            break;

        default:
            newgame_make_item(1, EQ_NONE, OBJ_WEAPONS, WPN_BOW);

            // Wield the bow instead.
            you.equip[EQ_WEAPON] = 1;
            break;
        }

        // And give them a book
        newgame_make_item(2, EQ_NONE, OBJ_BOOKS, BOOK_BRANDS);

        you.skills[range_skill(you.inv[1])] = 2;
        you.skills[SK_DODGING]              = 1;
        you.skills[SK_SPELLCASTING]         = 2;
        you.skills[SK_SORCERY]         = 2;
        break;

    case JOB_WIZARD:
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(1, EQ_HELMET, OBJ_ARMOUR, ARM_WIZARD_HAT);

        newgame_make_item(2, EQ_NONE, OBJ_BOOKS, BOOK_MINOR_MAGIC);

        you.skills[SK_DODGING]        = 2;
        you.skills[SK_STEALTH]        = 2;
        you.skills[SK_SPELLCASTING]   = 3;
        you.skills[SK_TRANSLOCATIONS] = 1;
        you.skills[SK_CONJURATIONS]   = 1;
        you.skills[SK_SUMMONINGS]     = 1;
        break;

    case JOB_CONJURER:
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);

        newgame_make_item(2, EQ_NONE, OBJ_BOOKS,
                           start_to_book(BOOK_CONJURATIONS_I, ng.book));

        you.skills[SK_CONJURATIONS] = 4;
        you.skills[SK_SPELLCASTING] = 1;
        you.skills[SK_DODGING]      = 2;
        you.skills[SK_STEALTH]      = 2;
        break;

    case JOB_ENCHANTER:
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_DAGGER, -1, 1, 1,
                           1);
        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE, -1, 1, 1);
        newgame_make_item(2, EQ_NONE, OBJ_BOOKS, BOOK_MALEDICT);

        // Gets some darts - this job is difficult to start off with.
        newgame_make_item(3, EQ_NONE, OBJ_MISSILES, MI_DART, -1, 16, 1);

        // Spriggans used to get a rod of striking, but now that anyone
        // can get one when playing an Artificer, this is no longer
        // necessary. (jpeg)

        if (player_genus(GENPC_OGREISH) || you.species == SP_TROLL)
            you.inv[0].sub_type = WPN_CLUB;

        weap_skill = 1;
        you.skills[SK_THROWING]     = 1;
        you.skills[SK_SORCERY] = 3;
        you.skills[SK_SPELLCASTING] = 1;
        you.skills[SK_DODGING]      = 2;
        you.skills[SK_STEALTH]      = 2;
        you.skills[SK_STABBING]     = 1;
        break;

    case JOB_SUMMONER:
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(1, EQ_NONE, OBJ_BOOKS, BOOK_CALLINGS);

        you.skills[SK_SUMMONINGS]   = 4;
        you.skills[SK_SPELLCASTING] = 1;
        you.skills[SK_DODGING]      = 2;
        you.skills[SK_STEALTH]      = 2;
        break;

    case JOB_NECROMANCER:
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(1, EQ_NONE, OBJ_BOOKS, BOOK_NECROMANCY);

        you.skills[SK_SPELLCASTING] = 1;
        you.skills[SK_NECROMANCY]   = 4;
        you.skills[SK_DODGING]      = 2;
        you.skills[SK_STEALTH]      = 2;
        break;

    case JOB_TRANSMUTER:
        you.equip[EQ_WEAPON] = -1; // Transmuters fight unarmed.

        // Some sticks for sticks to snakes.
        // A lot more now that mundane arrow generation is suppressed...
        // Note that starting a transmuter is now the only way to find mundane
        // arrows now.
        newgame_make_item(1, EQ_NONE, OBJ_MISSILES, MI_ARROW, -1, 40);
        newgame_make_item(2, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(3, EQ_NONE, OBJ_BOOKS, BOOK_CHANGES);

        // A little bit of starting ammo for evaporate... don't need too
        // much now that the character can make their own. - bwr
        newgame_make_item(4, EQ_NONE, OBJ_POTIONS, POT_CONFUSION, -1, 2);
        newgame_make_item(5, EQ_NONE, OBJ_POTIONS, POT_POISON);

        // Spriggans used to get a rod of striking, but now that anyone
        // can get one when playing an Artificer, this is no longer
        // necessary. (jpeg)

        you.skills[SK_FIGHTING]       = 1;
        you.skills[SK_UNARMED_COMBAT] = 3;
        you.skills[SK_DODGING]        = 2;
        you.skills[SK_SPELLCASTING]   = 2;
        you.skills[SK_TRANSMUTATIONS] = 2;
        break;

    case JOB_FIRE_ELEMENTALIST:
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(1, EQ_NONE, OBJ_BOOKS, BOOK_FLAMES);

        you.skills[SK_CONJURATIONS] = 1;
        you.skills[SK_FIRE_MAGIC]   = 3;
        you.skills[SK_SPELLCASTING] = 1;
        you.skills[SK_DODGING]      = 2;
        you.skills[SK_STEALTH]      = 2;
        break;

    case JOB_ICE_ELEMENTALIST:
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(1, EQ_NONE, OBJ_BOOKS, BOOK_FROST);

        you.skills[SK_CONJURATIONS] = 1;
        you.skills[SK_ICE_MAGIC]    = 3;
        you.skills[SK_SPELLCASTING] = 1;
        you.skills[SK_DODGING]      = 2;
        you.skills[SK_STEALTH]      = 2;
        break;

    case JOB_AIR_ELEMENTALIST:
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(1, EQ_NONE, OBJ_BOOKS, BOOK_AIR);

        you.skills[SK_CONJURATIONS] = 1;
        you.skills[SK_AIR_MAGIC]    = 3;
        you.skills[SK_SPELLCASTING] = 1;
        you.skills[SK_DODGING]      = 2;
        you.skills[SK_STEALTH]      = 2;
        break;

    case JOB_EARTH_ELEMENTALIST:
        // stones in switch slot (b)
        newgame_make_item(1, EQ_NONE, OBJ_MISSILES, MI_STONE, -1, 20);
        newgame_make_item(2, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(3, EQ_NONE, OBJ_BOOKS, BOOK_GEOMANCY);

        you.skills[SK_TRANSMUTATIONS] = 1;
        you.skills[SK_EARTH_MAGIC]    = 3;
        you.skills[SK_SPELLCASTING]   = 1;
        you.skills[SK_DODGING]        = 2;
        you.skills[SK_STEALTH]        = 2;
        break;

    case JOB_VENOM_MAGE:
        // Venom Mages don't need a starting weapon since acquiring a weapon
        // to poison should be easy, and Sting is *powerful*.
        newgame_make_item(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(1, EQ_NONE, OBJ_BOOKS, BOOK_YOUNG_POISONERS);

        you.skills[SK_SORCERY] = 4;
        you.skills[SK_SPELLCASTING] = 1;
        you.skills[SK_DODGING]      = 2;
        you.skills[SK_STEALTH]      = 2;
        break;

    case JOB_STALKER:
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_DAGGER, -1, 1, 2, 2);
        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(2, EQ_CLOAK, OBJ_ARMOUR, ARM_CLOAK);
        newgame_make_item(3, EQ_NONE, OBJ_BOOKS, BOOK_STALKING);

        newgame_make_item(4, EQ_NONE, OBJ_POTIONS, POT_CONFUSION, -1, 2);

        if (player_genus(GENPC_OGREISH) || you.species == SP_TROLL)
            you.inv[0].sub_type = WPN_CLUB;

        weap_skill = 1;
        you.skills[SK_FIGHTING]       = 1;
        you.skills[SK_DODGING]        = 2;
        you.skills[SK_STEALTH]        = 2;
        you.skills[SK_SPELLCASTING]   = 2;
        you.skills[SK_TRANSMUTATIONS] = 1;
        you.skills[SK_TRANSLOCATIONS] = 1;
        you.skills[SK_SORCERY]        = 1;
        break;

    case JOB_ASSASSIN:
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_DAGGER, -1, 1, 2, 2);
        newgame_make_item(1, EQ_NONE, OBJ_WEAPONS, WPN_BLOWGUN);

        newgame_make_item(2, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);
        newgame_make_item(3, EQ_CLOAK, OBJ_ARMOUR, ARM_CLOAK);

        newgame_make_item(4, EQ_NONE, OBJ_MISSILES, MI_NEEDLE, -1, 25);
        set_item_ego_type(you.inv[4], OBJ_MISSILES, SPMSL_POISONED);
        newgame_make_item(5, EQ_NONE, OBJ_MISSILES, MI_NEEDLE, -1, 7);
        set_item_ego_type(you.inv[5], OBJ_MISSILES, SPMSL_CURARE);
        newgame_make_item(6, EQ_NONE, OBJ_MISSILES, MI_NEEDLE, -1, 10);
        set_item_ego_type(you.inv[6], OBJ_MISSILES, SPMSL_SLEEP);
        newgame_make_item(7, EQ_NONE, OBJ_MISSILES, MI_NEEDLE, -1, 10);
        set_item_ego_type(you.inv[7], OBJ_MISSILES, SPMSL_SLOW);
        newgame_make_item(8, EQ_NONE, OBJ_MISSILES, MI_NEEDLE, -1, 10);
        set_item_ego_type(you.inv[8], OBJ_MISSILES, SPMSL_CONFUSION);        

        if (you.species == SP_OGRE || you.species == SP_TROLL)
            you.inv[0].sub_type = WPN_CLUB;

        weap_skill = 2;
        you.skills[SK_FIGHTING]     = 2;
        you.skills[SK_DODGING]      = 1;
        you.skills[SK_STEALTH]      = 3;
        you.skills[SK_STABBING]     = 2;
        you.skills[SK_THROWING]     = 2;
        break;

    case JOB_HUNTER:
        // Equipment.
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_DAGGER);

        switch (you.species)
        {
        case SP_MOUNTAIN_DWARF:
        case SP_DEEP_DWARF:
        case SP_HILL_ORC:
        case SP_CENTAUR:
            you.inv[0].sub_type = WPN_HAND_AXE;
            break;
        case SP_OGRE:
            you.inv[0].sub_type = WPN_CLUB;
            break;
        case SP_GHOUL:
        case SP_TROLL:
            _newgame_clear_item(0);
            break;
        default:
            break;
        }

        //A butchering implement and enchanted ammo doesn't exactly compensate
        //for the absence of the book of Distance.  Therefore, Hunters also start
        //with an enchanted weapon, in the applicable cases.
        switch (you.species)
        {
        //Sludge elves, hill orcs, and merfolk who want nets and javelins can
        //just play Gladiator.
        /*
        case SP_SLUDGE_ELF:
        case SP_HILL_ORC:
        case SP_MERFOLK:
            newgame_make_item(1, EQ_NONE, OBJ_MISSILES, MI_JAVELIN, -1, 6, 1);
            newgame_make_item(2, EQ_NONE, OBJ_MISSILES, MI_THROWING_NET, -1,
                               2);
            break;
        */

        case SP_TROLL:
            newgame_make_item(1, EQ_NONE, OBJ_MISSILES, MI_LARGE_ROCK, -1, 5,
                               1);
            newgame_make_item(2, EQ_NONE, OBJ_MISSILES, MI_THROWING_NET, -1,
                               3);
            break;

        case SP_HALFLING:
        case SP_SPRIGGAN:
            newgame_make_item(1, EQ_NONE, OBJ_WEAPONS, WPN_SLING);

            // Wield the sling instead.
            you.equip[EQ_WEAPON] = 1;
            _enchant_weapon(1, 1, 1);
            break;

        case SP_MOUNTAIN_DWARF:
        case SP_DEEP_DWARF:
        case SP_KOBOLD:
            newgame_make_item(1, EQ_NONE, OBJ_WEAPONS, WPN_CROSSBOW);

            // Wield the crossbow instead.
            you.equip[EQ_WEAPON] = 1;
            _enchant_weapon(1, 1, 1);
            break;

        default:
            newgame_make_item(1, EQ_NONE, OBJ_WEAPONS, WPN_BOW);
            _enchant_weapon(0, 1, 1);

            // Wield the bow instead.
            you.equip[EQ_WEAPON] = 1;
            break;
        }

        newgame_make_item(3, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_LEATHER_ARMOUR,
                           ARM_ANIMAL_SKIN);

        // Skills.
        you.skills[SK_FIGHTING] = 2;
        you.skills[SK_DODGING]  = 2;
        you.skills[SK_STEALTH]  = 1;
        weap_skill = 1;

        you.skills[range_skill(you.inv[1])] = 4;
        break;
        
    case JOB_WANDERER:
        create_wanderer();
        break;

    case JOB_ARTIFICER:
        // Equipment. Staff, and armour or robe.
        newgame_make_item(0, EQ_WEAPON, OBJ_WEAPONS, WPN_STAFF);
        newgame_make_item(1, EQ_BODY_ARMOUR, OBJ_ARMOUR,
                           ARM_LEATHER_ARMOUR, ARM_ROBE);

        // Choice of lesser wands, 15 charges plus wand of random
        // effects: confusion, enslavement, slowing, magic dart, frost,
        // flame; OR a rod of striking, 8 charges and no random effects.
        _give_wand(ng);

        // If an offensive wand or the rod of striking was chosen,
        // don't hand out a weapon.
        if (item_is_rod(you.inv[2]))
        {
            // If the rod of striking was chosen, put it in the first
            // slot and wield it.
            you.inv[0] = you.inv[2];
            you.equip[EQ_WEAPON] = 0;
            _newgame_clear_item(2);
        }
        else if (you.inv[3].base_type != OBJ_WANDS
                 || you.inv[3].sub_type != WAND_CONFUSION
                    && you.inv[3].sub_type != WAND_ENSLAVEMENT)
        {
            _newgame_clear_item(0);
        }

        // Skills
        you.skills[SK_EVOCATIONS]  = 4;
        you.skills[SK_TRAPS_DOORS] = 3;
        you.skills[SK_DODGING]     = 2;
        you.skills[SK_FIGHTING]    = 1;
        you.skills[SK_STEALTH]     = 1;
        break;

    default:
        break;
    }
    
    // Wrath of Nemelex characters worship Nemelex regardless of class.
    // They also start with infinite plain decks of Summoning, Destruction,
    // and Escape.
    // Maybe I'll give them Dungeons later too (probably not).  Wonders is out
    // of the question, though.
    if (you.challenge == CHALLENGE_NEMELEX)
    {
        you.religion = GOD_NEMELEX_XOBEH;
        you.piety = 35;
        newgame_make_item(-1, EQ_NONE, OBJ_MISCELLANY,
                          MISC_DECK_OF_DESTRUCTION, -1, 1, -1);
        newgame_make_item(-1, EQ_NONE, OBJ_MISCELLANY,
                          MISC_DECK_OF_DUNGEONS, -1, 1, -1);
        newgame_make_item(-1, EQ_NONE, OBJ_MISCELLANY,
                          MISC_DECK_OF_ESCAPE, -1, 1, -1);
    }

    // Deep Dwarves get a wand of healing (5).
    if (you.species == SP_DEEP_DWARF)
        newgame_make_item(-1, EQ_NONE, OBJ_WANDS, WAND_HEAL_WOUNDS, -1, 1, 5);

    // Zotdef: everyone gets a bonus two potions of curing
    if (crawl_state.game_is_zotdef())
        newgame_make_item(-1, EQ_NONE, OBJ_POTIONS, POT_CURING, -1, 2);

    if (weap_skill)
    {
        if (!you.weapon())
            you.skills[SK_UNARMED_COMBAT] = weap_skill;
        else
            you.skills[weapon_skill(*you.weapon())] = weap_skill;
    }

    if (you.species == SP_CAT)
    {
        for (int i = SK_SHORT_BLADES; i <= SK_CROSSBOWS; i++)
        {
            you.skills[SK_UNARMED_COMBAT] += you.skills[i];
            you.skills[i] = 0;
        }
        you.skills[SK_DODGING] += you.skills[SK_ARMOUR];
        you.skills[SK_ARMOUR] = 0;
        you.skills[SK_THROWING] = 0;
        you.skills[SK_SHIELDS] = 0;
    }

    if (you.species == SP_BASE_DRACONIAN)
    {
        you.skills[SK_DODGING] += you.skills[SK_ARMOUR];
        you.skills[SK_ARMOUR] = 0;
    }

    if (you.religion != GOD_NO_GOD)
    {
        you.worshipped[you.religion] = 1;
        set_god_ability_slots();
        if (you.religion != GOD_XOM)
            you.piety_max[you.religion] = you.piety;
    }
}

static void _give_species_bonus_hp()
{
    if (player_genus(GENPC_DRACONIAN) || player_genus(GENPC_DWARVEN))
        inc_max_hp(1);
    else
    {
        switch (you.species)
        {
        case SP_CENTAUR:
        case SP_OGRE:
        case SP_TROLL:
            inc_max_hp(3);
            break;

        case SP_GHOUL:
        case SP_MINOTAUR:
        case SP_NAGA:
        case SP_DEMIGOD:
            inc_max_hp(2);
            break;

        case SP_HILL_ORC:
        case SP_MUMMY:
        case SP_MERFOLK:
            inc_max_hp(1);
            break;

        case SP_HIGH_ELF:
        case SP_VAMPIRE:
            dec_max_hp(1);
            break;

        case SP_DEEP_ELF:
        case SP_HALFLING:
        case SP_KENKU:
        case SP_KOBOLD:
        case SP_SPRIGGAN:
            dec_max_hp(2);
            break;

        case SP_CAT:
            dec_max_hp(3);
            break;

        default:
            break;
        }
    }
}

// Adjust max_magic_points by species. {dlb}
static void _give_species_bonus_mp()
{
    switch (you.species)
    {
    case SP_VAMPIRE:
    case SP_SPRIGGAN:
    case SP_DEMIGOD:
    case SP_DEEP_ELF:
    case SP_KENKU:
        inc_max_mp(1);
        break;

    default:
        break;
    }
}

//Now that you can't just take a staircase to avoid trouble, let's give the player
//a few tools to be able to avoid dying early.
static void _give_starting_escape_items()
{
    //One potion of agility, for an evasion and swiftness boost...
    item_def agility;
    agility.quantity = 1;
    agility.base_type = OBJ_POTIONS;
    agility.sub_type = POT_AGILITY;
    const int agility_slot = find_free_slot(agility);
    you.inv[agility_slot] = agility;
    
    //...and one scroll of teleportation.
    item_def teleport;
    teleport.quantity = 1;
    teleport.base_type = OBJ_SCROLLS;
    teleport.sub_type = SCR_TELEPORTATION;
    const int teleport_slot = find_free_slot(teleport);
    you.inv[teleport_slot] = teleport;
}

static void _setup_tutorial_miscs()
{
    // Allow for a few specific hint mode messages.
    // A few more will be initialised by the tutorial map.
    tutorial_init_hints();

    // No gold to begin with.
    you.gold = 0;

    // Give him some mana to play around with.
    inc_max_mp(2);

    _newgame_make_item_tutorial(0, EQ_BODY_ARMOUR, OBJ_ARMOUR, ARM_ROBE);

    // No need for Shields skill without shield.
    you.skills[SK_SHIELDS] = 0;

    // Some spellcasting for the magic tutorial.
    if (crawl_state.map.find("tutorial_lesson4") != std::string::npos)
        you.skills[SK_SPELLCASTING] = 1;

    // Set Str low enough for the burdened tutorial.
    you.base_stats[STAT_STR] = 12;
}

static void _mark_starting_books()
{
    for (int i = 0; i < ENDOFPACK; ++i)
        if (you.inv[i].defined() && you.inv[i].base_type == OBJ_BOOKS)
            mark_had_book(you.inv[i]);
}

static void _racialise_starting_equipment()
{
    for (int i = 0; i < ENDOFPACK; ++i)
    {
        if (you.inv[i].defined())
        {
            if (is_useless_item(you.inv[i]))
                _newgame_clear_item(i);
            // Don't change object type modifier unless it starts plain.
            else if ((you.inv[i].base_type == OBJ_ARMOUR
                    || you.inv[i].base_type == OBJ_WEAPONS)
                && get_equip_race(you.inv[i]) == ISFLAG_NO_RACE)
            {
                // Now add appropriate species type mod.
                if (player_genus(GENPC_ELVEN))
                    set_equip_race(you.inv[i], ISFLAG_ELVEN);
                else if (player_genus(GENPC_DWARVEN))
                    set_equip_race(you.inv[i], ISFLAG_DWARVEN);
                else if (you.species == SP_HILL_ORC)
                    set_equip_race(you.inv[i], ISFLAG_ORCISH);
            }
        }
    }
}

static void _give_basic_spells(job_type which_job)
{
    // Wanderers may or may not already have a spell. - bwr
    if (which_job == JOB_WANDERER)
        return;

    spell_type which_spell = SPELL_NO_SPELL;

    switch (which_job)
    {
    case JOB_WIZARD:
    case JOB_CONJURER:
    case JOB_REAVER:
        which_spell = SPELL_MAGIC_DART;
        break;
    case JOB_VENOM_MAGE:
        which_spell = SPELL_STING;
        break;
    case JOB_SUMMONER:
        which_spell = SPELL_SUMMON_SMALL_MAMMALS;
        break;
    case JOB_NECROMANCER:
        which_spell = SPELL_PAIN;
        break;
    case JOB_ENCHANTER:
        which_spell = SPELL_CORONA;
        break;
    case JOB_FIRE_ELEMENTALIST:
        which_spell = SPELL_FLAME_TONGUE;
        break;
    case JOB_ICE_ELEMENTALIST:
        which_spell = SPELL_FREEZE;
        break;
    case JOB_AIR_ELEMENTALIST:
        which_spell = SPELL_SHOCK;
        break;
    case JOB_EARTH_ELEMENTALIST:
        which_spell = SPELL_SANDBLAST;
        break;
    case JOB_STALKER:
        which_spell = SPELL_CORONA;
        break;
    case JOB_WARPER:
        which_spell = SPELL_APPORTATION;

    default:
        break;
    }

    if (which_spell != SPELL_NO_SPELL)
        add_spell_to_memory(which_spell);

    return;
}

static void _setup_normal_game();
static void _setup_tutorial(const newgame_def& ng);
static void _setup_sprint(const newgame_def& ng);
static void _setup_zotdef(const newgame_def& ng);
static void _setup_hints();
static void _setup_generic(const newgame_def& ng);

// Initialise a game based on the choice stored in ng.
void setup_game(const newgame_def& ng)
{
    crawl_state.type = ng.type;
    crawl_state.map  = ng.map;

    switch (crawl_state.type)
    {
    case GAME_TYPE_CHALLENGE:
    case GAME_TYPE_NORMAL:
        _setup_normal_game();
        break;
    case GAME_TYPE_TUTORIAL:
        _setup_tutorial(ng);
        break;
    case GAME_TYPE_SPRINT:
        _setup_sprint(ng);
        break;
    case GAME_TYPE_ZOTDEF:
        _setup_zotdef(ng);
        break;
    case GAME_TYPE_HINTS:
        _setup_hints();
        break;
    case GAME_TYPE_ARENA:
    default:
        die("Bad game type");
        end(-1);
    }

    _setup_generic(ng);
}

/**
 * Special steps that normal game needs;
 */
static void _setup_normal_game()
{
    //Not really sure why this function needed to be there.
    //make_hungry(0, true);
}

/**
 * Special steps that tutorial game needs;
 */
static void _setup_tutorial(const newgame_def& ng)
{
    //Nothing at the moment...
}

/**
 * Special steps that sprint needs;
 */
static void _setup_sprint(const newgame_def& ng)
{
    //Nothing at the moment...
}

/**
 * Special steps that zotdef needs;
 */
static void _setup_zotdef(const newgame_def& ng)
{
    // nothing currently
}

/**
 * Special steps that hints mode needs;
 */
static void _setup_hints()
{
    init_hints();
}

static void _setup_generic(const newgame_def& ng)
{
    _init_player();

    you.your_name  = ng.name;
    you.species    = ng.species;
    you.char_class = ng.job;

    you.class_name = get_job_name(you.char_class);

    _species_stat_init(you.species);     // must be down here {dlb}

    you.is_undead = get_undead_state(you.species);

    // Before we get into the inventory init, set light radius based
    // on species vision. Currently, all species see out to 8 squares.
    update_vision_range();

    _jobs_stat_init(you.char_class);
    _give_last_paycheck(you.char_class);

    unfocus_stats();

    // Needs to be done before handing out food.
    give_basic_mutations(you.species);

    // This function depends on stats and mutations being finalised.
    _give_items_skills(ng);

    _give_species_bonus_hp();
    _give_species_bonus_mp();

    if (you.species == SP_DEMONSPAWN)
        roll_demonspawn_mutations();

    _give_starting_escape_items();

    if (crawl_state.game_is_sprint() || crawl_state.game_is_zotdef())
        _give_bonus_items();

    // Give tutorial skills etc
    if (crawl_state.game_is_tutorial())
        _setup_tutorial_miscs();

    _mark_starting_books();

    _give_basic_spells(you.char_class);

    _racialise_starting_equipment();
    initialise_item_descriptions();

    reassess_starting_skills();
    calc_total_skill_points();
    init_skill_order();
    if(you.challenge == CHALLENGE_SIF_MUNA)
        you.exp_available = 0;
    else if (crawl_state.game_is_zotdef())
        you.exp_available = 80;
    else
        you.exp_available = 25;

    for (int i = 0; i < ENDOFPACK; ++i)
        if (you.inv[i].defined())
        {
            // XXX: Why is this here? Elsewhere it's only ever used for runes.
            you.inv[i].flags |= ISFLAG_BEEN_IN_INV;

            // Identify all items in pack.
            set_ident_type(you.inv[i], ID_KNOWN_TYPE);
            set_ident_flags(you.inv[i], ISFLAG_IDENT_MASK);

            // link properly
            you.inv[i].pos.set(-1, -1);
            you.inv[i].link = i;
            you.inv[i].slot = index_to_letter(you.inv[i].link);
            item_colour(you.inv[i]);  // set correct special and colour
        }

    // If the item in slot 'a' is a throwable weapon like a dagger,
    // inscribe it with {=f} to prevent it being autoquivered.
    // (It's no fun to discover you've just thrown your +2 dagger
    // because you ran out of needles for your blowgun!)
    // FIXME: It ought to be possible to override this with autoinscribe rules.
    if (you.inv[0].base_type == OBJ_WEAPONS
        && is_throwable(&you, you.inv[0]))
    {
        you.inv[0].inscription = "=f";
    }

    // Apply autoinscribe rules to inventory.
    request_autoinscribe();
    autoinscribe();

    // Brand items as original equipment.
    origin_set_inventory(origin_set_startequip);

    // We calculate hp and mp here; all relevant factors should be
    // finalised by now. (GDL)
    calc_hp();
    calc_mp();

    // Make sure the starting player is fully charged up.
    set_hp(you.hp_max, false);
    set_mp(you.max_magic_points, false);
    
    // Initialize difficulty to 0.
    you.difficulty_level = 0;
    
    // Set max glow according to race.
    you.max_magic_contamination = get_max_magic_contamination();

    initialise_branch_depths();
    initialise_temples();
    init_level_connectivity();

    // Generate the second name of Jiyva
    fix_up_jiyva_name();

    for (int i = 0; i < NUM_NEMELEX_GIFT_TYPES; ++i)
        you.nemelex_sacrificing = true;

    // Create the save file.
    you.save = new package((get_savedir_filename(you.your_name, "", "")
                            + SAVE_SUFFIX).c_str(), true, true);

    // Pretend that a savefile was just loaded, in order to
    // get things setup properly.
    SavefileCallback::post_restore();
}
