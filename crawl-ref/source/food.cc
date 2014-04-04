/**
 * @file
 * @brief Functions for eating and butchering.
**/

#include "AppHdr.h"

#include "food.h"

#include <sstream>

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "externs.h"
#include "options.h"

#include "artefact.h"
#include "cio.h"
#include "clua.h"
#include "command.h"
#include "debug.h"
#include "delay.h"
#include "effects.h"
#include "env.h"
#include "hints.h"
#include "invent.h"
#include "items.h"
#include "itemname.h"
#include "itemprop.h"
#include "item_use.h"
#include "macro.h"
#include "mgen_data.h"
#include "message.h"
#include "misc.h"
#include "mon-place.h"
#include "mon-util.h"
#include "mutation.h"
#include "output.h"
#include "player.h"
#include "player-equip.h"
#include "player-stats.h"
#include "potion.h"
#include "random.h"
#include "religion.h"
#include "godconduct.h"
#include "skills2.h"
#include "state.h"
#include "stuff.h"
#include "transform.h"
#include "travel.h"
#include "xom.h"

/*
 *  BEGIN PUBLIC FUNCTIONS
 */

// More of a "weapon_switch back from butchering" function, switching
// to a weapon is done using the wield_weapon code.
// special cases like staves of power or other special weps are taken
// care of by calling wield_effects().    {gdl}
void weapon_switch(int targ)
{
    // Give the player an option to abort.
    if (you.weapon() && !check_old_item_warning(*you.weapon(), OPER_WIELD))
        return;

    if (targ == -1) // Unarmed Combat.
    {
        // Already unarmed?
        if (!you.weapon())
            return;

        mpr("You switch back to your bare hands.");
    }
    else
    {
        // Possibly not valid anymore (dropped, etc.).
        if (!you.inv[targ].defined())
            return;

        // Already wielding this weapon?
        if (you.equip[EQ_WEAPON] == you.inv[targ].link)
            return;

        if (!can_wield(&you.inv[targ]))
        {
            mprf("Not switching back to %s.",
                 you.inv[targ].name(DESC_INVENTORY).c_str());
            return;
        }
    }

    // Unwield the old weapon and wield the new.
    // XXX This is a pretty dangerous hack;  I don't like it.--GDL
    //
    // Well yeah, but that's because interacting with the wielding
    // code is a mess... this whole function's purpose was to
    // isolate this hack until there's a proper way to do things. -- bwr
    if (you.weapon())
        unwield_item(false);

    if (targ != -1)
        equip_item(EQ_WEAPON, targ, false);

    if (Options.chunks_autopickup || you.species == SP_VAMPIRE)
        autopickup();

    // Same amount of time as normal wielding.
    // FIXME: this duplicated code is begging for a bug.
    if (you.weapon())
        you.time_taken /= 2;
    else                        // Swapping to empty hands is faster.
    {
        you.time_taken *= 3;
        you.time_taken /= 10;
    }

    you.wield_change = true;
    you.turn_is_over = true;
}

static bool _prepare_butchery(bool can_butcher, bool removed_gloves,
                              bool wpn_switch)
{
    // No preparation necessary.
    if (can_butcher)
        return (true);

    // At least one of these has to be true, else what are we doing
    // here?
    ASSERT(removed_gloves || wpn_switch);

    // If you can butcher by taking off your gloves, don't prompt.
    if (removed_gloves)
    {
        // Actually take off the gloves; this creates a delay.  We
        // assume later on that gloves have a 1-turn takeoff delay!
        if (!takeoff_armour(you.equip[EQ_GLOVES]))
            return (false);

        // Ensure that the gloves are taken off by now by finishing the
        // DELAY_ARMOUR_OFF delay started by takeoff_armour() above.
        // FIXME: get rid of this hack!
        finish_last_delay();
    }

    if (wpn_switch
        && !wield_weapon(true, SLOT_BARE_HANDS, false, true, false, false))
    {
        return (false);
    }

    // Switched to a good butchering tool.
    return (true);
}

static bool _butcher_corpse(int corpse_id, int butcher_tool,
                            bool first_corpse = true,
                            bool bottle_blood = false)
{
    ASSERT(corpse_id != -1);

    const bool rotten = food_is_rotten(mitm[corpse_id]);

    if (is_forbidden_food(mitm[corpse_id])
        && !yesno("Desecrating this corpse would be a sin. Continue anyway?",
                  false, 'n'))
    {
        return false;
    }

    // Start work on the first corpse we butcher.
    if (first_corpse)
        mitm[corpse_id].plus2++;

    int work_req = std::max(0, 4 - mitm[corpse_id].plus2);

    delay_type dtype = DELAY_BUTCHER;
    // Sanity checks.
    if (bottle_blood && !rotten
        && can_bottle_blood_from_corpse(mitm[corpse_id].plus))
    {
        dtype = DELAY_BOTTLE_BLOOD;
    }

    start_delay(dtype, work_req, corpse_id, mitm[corpse_id].special,
                butcher_tool);

    you.turn_is_over = true;
    return (true);
}

static void _terminate_butchery(bool wpn_switch, bool removed_gloves,
                                int old_weapon, int old_gloves)
{
    // Switch weapon back.
    if (wpn_switch && you.equip[EQ_WEAPON] != old_weapon
        && (!you.weapon() || !you.weapon()->cursed()))
    {
        start_delay(DELAY_WEAPON_SWAP, 1, old_weapon);
    }

    // Put on the removed gloves.
    if (removed_gloves && you.equip[EQ_GLOVES] != old_gloves)
        start_delay(DELAY_ARMOUR_ON, 1, old_gloves, 1);
}

int count_corpses_in_pack(bool blood_only)
{
    int num = 0;
    for (int i = 0; i < ENDOFPACK; ++i)
    {
        item_def &obj(you.inv[i]);

        if (!obj.defined())
            continue;

        // Only actually count corpses, not skeletons.
        if (obj.base_type != OBJ_CORPSES || obj.sub_type != CORPSE_BODY)
            continue;

        // Only saprovorous characters care about rotten food.
        if (food_is_rotten(obj) && (blood_only
                                    || !player_mutation_level(MUT_SAPROVOROUS)))
        {
            continue;
        }

        if (!blood_only || mons_has_blood(obj.plus))
            num++;
    }

    return num;
}

static bool _have_corpses_in_pack(bool remind, bool bottle_blood = false)
{
    const int num = count_corpses_in_pack(bottle_blood);

    if (num == 0)
        return (false);

    std::string verb = (bottle_blood ? "bottle" : "butcher");

    std::string noun, pronoun;
    if (num == 1)
    {
        noun    = "corpse";
        pronoun = "it";
    }
    else
    {
        noun    = "corpses";
        pronoun = "them";
    }

    if (remind)
    {
        mprf("You might want to also %s the %s in your pack.", verb.c_str(),
             noun.c_str());
    }
    else
    {
        mprf("If you dropped the %s in your pack you could %s %s.",
             noun.c_str(), verb.c_str(), pronoun.c_str());
    }

    return (true);
}

bool butchery(int which_corpse, bool bottle_blood)
{
    if (you.visible_igrd(you.pos()) == NON_ITEM)
    {
        if (!_have_corpses_in_pack(false))
            mpr("There isn't anything here!");
        return (false);
    }

    if (you.flight_mode() == FL_LEVITATE)
    {
        mpr("You can't reach the floor from up here.");
        learned_something_new(HINT_LEVITATING);
        return (false);
    }

    // Vampires' fangs are optimised for biting, not for tearing flesh.
    // (Not that they really need to.) Other species with this mutation
    // might still benefit from it.
    bool teeth_butcher    = (you.has_usable_fangs() == 3
                             && you.species != SP_VAMPIRE);

    bool birdie_butcher   = (player_mutation_level(MUT_BEAK)
                             && player_mutation_level(MUT_TALONS));

    bool barehand_butcher = (form_can_butcher_barehanded(you.form)
                                 || you.has_claws())
                             && !player_wearing_slot(EQ_GLOVES);

    bool gloved_butcher   = (you.has_claws() && player_wearing_slot(EQ_GLOVES)
                             && !you.inv[you.equip[EQ_GLOVES]].cursed());

    bool knife_butcher    = !barehand_butcher && !you.weapon();

    bool can_butcher      = (teeth_butcher || barehand_butcher
                             || birdie_butcher || knife_butcher
                             || you.weapon() && can_cut_meat(*you.weapon()));

    // It makes more sense that you first find out if there's anything
    // to butcher, *then* decide to actually butcher it.
    // The old code did it the other way.
    if (!can_butcher && you.berserk())
    {
        mpr("You are too berserk to search for a butchering tool!");
        return (false);
    }

    // First determine how many things there are to butcher.
    int num_corpses = 0;
    int corpse_id   = -1;
    bool prechosen  = (which_corpse != -1);
    for (stack_iterator si(you.pos(), true); si; ++si)
    {
        if (si->base_type == OBJ_CORPSES && si->sub_type == CORPSE_BODY)
        {
            if (bottle_blood && (food_is_rotten(*si)
                                 || !can_bottle_blood_from_corpse(si->plus)))
            {
                continue;
            }

            corpse_id = si->index();
            num_corpses++;

            // Return pre-chosen corpse if it exists.
            if (prechosen && corpse_id == which_corpse)
                break;
        }
    }

    if (num_corpses == 0)
    {
        if (!_have_corpses_in_pack(false, bottle_blood))
        {
            mprf("There isn't anything to %s here.",
                 bottle_blood ? "bottle" : "butcher");
        }
        return (false);
    }

    int old_weapon      = you.equip[EQ_WEAPON];
    int old_gloves      = you.equip[EQ_GLOVES];

    bool wpn_switch     = false;
    bool removed_gloves = false;

    if (!can_butcher)
    {
        if (you.weapon()->cursed() && gloved_butcher)
            removed_gloves = true;
        else
            wpn_switch = true;
    }

    int butcher_tool;

    if (barehand_butcher || removed_gloves)
        butcher_tool = SLOT_CLAWS;
    else if (teeth_butcher)
        butcher_tool = SLOT_TEETH;
    else if (birdie_butcher)
        butcher_tool = SLOT_BIRDIE;
    else if (wpn_switch || knife_butcher)
        butcher_tool = SLOT_BUTCHERING_KNIFE;
    else
        butcher_tool = you.weapon()->link;

    // Butcher pre-chosen corpse, if found, or if there is only one corpse.
    bool success = false;
    if (prechosen && corpse_id == which_corpse
        || num_corpses == 1 && !Options.always_confirm_butcher)
    {
        if (!_prepare_butchery(can_butcher, removed_gloves, wpn_switch))
            return (false);

        success = _butcher_corpse(corpse_id, butcher_tool, true, bottle_blood);
        _terminate_butchery(wpn_switch, removed_gloves, old_weapon, old_gloves);

        // Remind player of corpses in pack that could be butchered or
        // bottled.
        _have_corpses_in_pack(true, bottle_blood);
        return (success);
    }

    // Now pick what you want to butcher. This is only a problem
    // if there are several corpses on the square.
    bool butcher_all   = false;
    bool repeat_prompt = false;
    bool did_weap_swap = false;
    bool first_corpse  = true;
    int keyin;
    for (stack_iterator si(you.pos(), true); si; ++si)
    {
        if (si->base_type != OBJ_CORPSES || si->sub_type != CORPSE_BODY)
            continue;

        if (bottle_blood && (food_is_rotten(*si)
                             || !can_bottle_blood_from_corpse(si->plus)))
        {
            continue;
        }

        if (butcher_all)
            corpse_id = si->index();
        else
        {
            corpse_id = -1;

            std::string corpse_name = si->name(DESC_NOCAP_A);

            // We don't need to check for undead because
            // * Mummies can't eat.
            // * Ghouls relish the bad things.
            // * Vampires won't bottle bad corpses.
            if (!you.is_undead)
                corpse_name = get_menu_colour_prefix_tags(*si, DESC_NOCAP_A);

            // Shall we butcher this corpse?
            do
            {
                mprf(MSGCH_PROMPT, "%s %s? [(y)es/(c)hop/(n)o/(a)ll/(q)uit/?]",
                     bottle_blood ? "Bottle" : "Butcher",
                     corpse_name.c_str());
                repeat_prompt = false;

                keyin = tolower(getchm(KMC_CONFIRM));
                switch (keyin)
                {
                case 'y':
                case 'c':
                case 'd':
                case 'a':
                    if (!did_weap_swap)
                    {
                        if (_prepare_butchery(can_butcher, removed_gloves,
                                              wpn_switch))
                        {
                            did_weap_swap = true;
                        }
                        else
                            return (false);
                    }
                    corpse_id = si->index();

                    if (keyin == 'a')
                        butcher_all = true;
                    break;

                case 'q':
                CASE_ESCAPE
                    canned_msg(MSG_OK);
                    _terminate_butchery(wpn_switch, removed_gloves, old_weapon,
                                        old_gloves);
                    return (false);

                case '?':
                    show_butchering_help();
                    mesclr();
                    redraw_screen();
                    repeat_prompt = true;
                    break;

                default:
                    break;
                }
            }
            while (repeat_prompt);
        }

        if (corpse_id != -1)
        {
            if (_butcher_corpse(corpse_id, butcher_tool, first_corpse,
                                bottle_blood))
            {
                success = true;
                first_corpse = false;
            }
        }
    }

    if (!butcher_all && corpse_id == -1)
    {
        mprf("There isn't anything else to %s here.",
             bottle_blood ? "bottle" : "butcher");
    }
    _terminate_butchery(wpn_switch, removed_gloves, old_weapon, old_gloves);

    if (success)
    {
        // Remind player of corpses in pack that could be butchered or
        // bottled.
        _have_corpses_in_pack(true, bottle_blood);
    }

    return (success);
}

//     END PUBLIC FUNCTIONS

// Returns which of two food items is older (true for first, else false).
class compare_by_freshness
{
public:
    bool operator()(item_def *food1, item_def *food2)
    {
        ASSERT(food1->base_type == OBJ_CORPSES || food1->base_type == OBJ_FOOD);
        ASSERT(food2->base_type == OBJ_CORPSES || food2->base_type == OBJ_FOOD);
        ASSERT(food1->base_type == food2->base_type);

        if (food1->base_type == OBJ_FOOD)
        {
            // Prefer chunks to non-chunks. (Herbivores handled above.)
            if (food1->sub_type != FOOD_CHUNK && food2->sub_type == FOOD_CHUNK)
                return (false);
            if (food2->sub_type != FOOD_CHUNK && food1->sub_type == FOOD_CHUNK)
                return (true);
        }

        // Both food types are edible (not rotten, or player is Saprovore).
        if (food1->base_type == OBJ_CORPSES
            || food1->sub_type == FOOD_CHUNK && food2->sub_type == FOOD_CHUNK)
        {
            if (Options.prefer_safe_chunks && !you.is_undead)
            {
                // Offer contaminated chunks last.
                if (is_contaminated(*food1) && !is_contaminated(*food2))
                    return (false);
                if (is_contaminated(*food2) && !is_contaminated(*food1))
                    return (true);
            }
        }

        return (food1->special < food2->special);
    }
};

// Returns true if a food item (also corpses) is poisonous AND the player
// is not (known to be) poison resistant.
bool is_poisonous(const item_def &food)
{
    if (food.base_type != OBJ_FOOD && food.base_type != OBJ_CORPSES)
        return (false);

    if (player_res_poison(false))
        return (false);

    return (chunk_is_poisonous(mons_corpse_effect(food.plus)));
}

// Returns true if a food item (also corpses) is mutagenic.
bool is_mutagenic(const item_def &food)
{
    if (food.base_type != OBJ_FOOD && food.base_type != OBJ_CORPSES)
        return (false);

    // Has no effect on ghouls.
    if (you.species == SP_GHOUL)
        return (false);

    return (mons_corpse_effect(food.plus) == CE_MUTAGEN_RANDOM);
}

// Returns true if a food item (also corpses) may cause sickness.
bool is_contaminated(const item_def &food)
{
    if ((food.base_type != OBJ_FOOD || food.sub_type != FOOD_CHUNK)
            && food.base_type != OBJ_CORPSES)
        return (false);

    const corpse_effect_type chunk_type = mons_corpse_effect(food.plus);
    return (chunk_type == CE_CONTAMINATED
            || (player_res_poison(false) && chunk_type == CE_POISON_CONTAM)
            || food_is_rotten(food)
               && player_mutation_level(MUT_SAPROVOROUS) < 3);
}

// Returns true if a food item (also corpses) will cause rotting.
bool causes_rot(const item_def &food)
{
    if (food.base_type != OBJ_FOOD && food.base_type != OBJ_CORPSES)
        return (false);

    // Has no effect on ghouls.
    if (you.species == SP_GHOUL)
        return (false);

    return (mons_corpse_effect(food.plus) == CE_ROT);
}

bool is_forbidden_food(const item_def &food)
{
    if (food.base_type != OBJ_CORPSES
        && (food.base_type != OBJ_FOOD || food.sub_type != FOOD_CHUNK))
    {
        return (false);
    }

    // Some gods frown upon cannibalistic behaviour.
    if (god_hates_cannibalism(you.religion)
        && is_player_same_species(food.plus))
    {
        return (true);
    }

    // Holy gods do not like it if you are eating holy creatures
    if (is_good_god(you.religion)
        && mons_class_holiness(food.plus) == MH_HOLY)
    {
        return (true);
    }

    // Zin doesn't like it if you eat beings with a soul.
    if (you.religion == GOD_ZIN && mons_class_intel(food.plus) >= I_NORMAL)
        return (true);

    // Everything else is allowed.
    return (false);
}

bool chunk_is_poisonous(int chunktype)
{
    return (chunktype == CE_POISONOUS || chunktype == CE_POISON_CONTAM);
}
