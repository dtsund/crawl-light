/**
 * @file
 * @brief Initializing non-player-related parts of a new game.
**/
/* TODO: 'you' shouldn't occur here.
 *       Some of these might fit better elsewhere.
 */

#include "AppHdr.h"

#include "branch.h"
#include "describe.h"
#include "dungeon.h"
#include "itemname.h"
#include "libutil.h"
#include "maps.h"
#include "player.h"
#include "random.h"
#include "religion.h"
#include "state.h"
#include "store.h"

#ifdef DEBUG_DIAGNOSTICS
#define DEBUG_TEMPLES
#endif

static uint8_t _random_potion_description()
{
    int desc, nature, colour;

    do
    {
        desc = random2(PDQ_NQUALS * PDC_NCOLOURS);

        if (coinflip())
            desc %= PDC_NCOLOURS;

        nature = PQUAL(desc);
        colour = PCOLOUR(desc);

        // nature and colour correspond to primary and secondary in
        // itemname.cc.  This check ensures clear potions don't get odd
        // qualifiers.
    }
    while (colour == PDC_CLEAR && nature > PDQ_VISCOUS
           || desc == PDESCS(PDC_CLEAR));

    return desc;
}

void initialise_branches_for_game_type()
{
    if (crawl_state.game_is_sprint())
    {
        brdepth.init(-1);
        brdepth[BRANCH_MAIN_DUNGEON] = 1;
        brdepth[BRANCH_ABYSS] = 1;
        return;
    }

    for (int i = 0; i < NUM_BRANCHES; i++)
        brdepth[i] = branches[i].numlevels;
}

// Determine starting depths of branches.
void initialise_branch_depths()
{
    for (int branch = BRANCH_ECUMENICAL_TEMPLE; branch < NUM_BRANCHES; ++branch)
    {
        const Branch *b = &branches[branch];
        if (!is_connected_branch(b->id))
           // hopefully unused, but let's have a reasonable estimate just in case
            startdepth[branch] = (b->mindepth + b->maxdepth) / 2;
        else if (crawl_state.game_is_sprint())
            startdepth[branch] = -1;
        else if (branch < BRANCH_VESTIBULE_OF_HELL || branch > BRANCH_LAST_HELL)
            startdepth[branch] = random_range(b->mindepth, b->maxdepth);
    }

/*  Always enable Shoals for now, for testing.

    // Disable one of the Swamp/Shoals/Snake Pit.
    const branch_type disabled_branch =
        static_cast<branch_type>(
            random_choose(BRANCH_SWAMP, BRANCH_SHOALS, BRANCH_SNAKE_PIT, -1));
*/
    // Disable one of the Swamp/Snake Pit.
    const branch_type disabled_branch =
        static_cast<branch_type>(
            random_choose(BRANCH_SWAMP, BRANCH_SNAKE_PIT, -1));


    dprf("Disabling branch: %s", branches[disabled_branch].shortname);
    startdepth[disabled_branch] = -1;


    startdepth[BRANCH_SPIDER_NEST] = -1;
    startdepth[BRANCH_FOREST] = -1;
    startdepth[BRANCH_DWARVEN_HALL] = -1;

    initialise_branches_for_game_type();
}

#define MAX_OVERFLOW_LEVEL 4

// Determine which altars go into the Ecumenical Temple, which go into
// overflow temples, and on what level the overflow temples are.
void initialise_temples()
{
    //////////////////////////////////////////
    // First determine main temple map to use.
    level_id ecumenical(BRANCH_ECUMENICAL_TEMPLE, 1);

    map_def *main_temple = NULL;
    for (int i = 0; i < 10; i++)
    {
        main_temple
            = const_cast<map_def*>(random_map_for_place(ecumenical, false));

        if (main_temple == NULL)
            end (1, false, "No temples?!");

        // Without all this find_glyph() returns 0.
        std::string err;
              main_temple->load();
              main_temple->reinit();
        err = main_temple->run_lua(true);

        if (!err.empty())
        {
            mprf(MSGCH_ERROR, "Temple %s: %s", main_temple->name.c_str(),
                 err.c_str());
            main_temple = NULL;
            continue;
        }

              main_temple->fixup();
        err = main_temple->resolve();

        if (!err.empty())
        {
            mprf(MSGCH_ERROR, "Temple %s: %s", main_temple->name.c_str(),
                 err.c_str());
            main_temple = NULL;
            continue;
        }
        break;
    }

    if (main_temple == NULL)
        end(1, false, "No valid temples.");

    you.props[TEMPLE_MAP_KEY] = main_temple->name;

    const std::vector<coord_def> altar_coords
        = main_temple->find_glyph('B');
    const unsigned int main_temple_size = altar_coords.size();

    if (main_temple_size == 0)
    {
        end(1, false, "Main temple '%s' has no altars",
            main_temple->name.c_str());
    }

#ifdef DEBUG_TEMPLES
    mprf(MSGCH_DIAGNOSTICS, "Chose main temple %s, size %u",
         main_temple->name.c_str(), main_temple_size);
#endif

    ///////////////////////////////////
    // Now set up the overflow temples.

    std::vector<god_type> god_list = temple_god_list();

    std::random_shuffle(god_list.begin(), god_list.end());

    std::vector<god_type> overflow_gods;

    while (god_list.size() > main_temple_size)
    {
        overflow_gods.push_back(god_list.back());
        god_list.pop_back();
    }

#ifdef DEBUG_TEMPLES
    mprf(MSGCH_DIAGNOSTICS, "%u overflow altars", (unsigned int)overflow_gods.size());
#endif

    CrawlVector &temple_gods
        = you.props[TEMPLE_GODS_KEY].new_vector(SV_BYTE);

    for (unsigned int i = 0; i < god_list.size(); i++)
        temple_gods.push_back((char) god_list[i]);

    CrawlVector &overflow_temples
        = you.props[OVERFLOW_TEMPLES_KEY].new_vector(SV_VEC);
    overflow_temples.resize(MAX_OVERFLOW_LEVEL);

    // NOTE: The overflow temples don't have to contain only one
    // altar; they can contain any number of altars, so long as there's
    // at least one vault definition with the tag "overflow_temple_num"
    // (where "num" is the number of altars).
    for (unsigned int i = 0; i < overflow_gods.size(); i++)
    {
        const unsigned int level = random_range(2, MAX_OVERFLOW_LEVEL);

        // List of overflow temples on this level.
        CrawlVector &level_temples
            = overflow_temples[level - 1].get_vector();

        CrawlHashTable temple;

        CrawlVector &gods
            = temple[TEMPLE_GODS_KEY].new_vector(SV_BYTE);

        // Only single-altar overflow temples for now.
        gods.push_back((char) overflow_gods[i]);

        level_temples.push_back(temple);
    }
}

void initialise_item_descriptions()
{
    // Must remember to check for already existing colours/combinations.
    you.item_description.init(255);

    you.item_description[IDESC_POTIONS][POT_WATER] = PDESCS(PDC_CLEAR);

    // The order here must match that of IDESC in describe.h
    const int max_item_number[6] = { NUM_WANDS,
                                     NUM_POTIONS,
                                     NUM_SCROLLS,
                                     NUM_JEWELLERY,
                                     NUM_SCROLLS,
                                     NUM_STAVES };

    for (int i = 0; i < NUM_IDESC; i++)
    {
        // Only loop until NUM_WANDS etc.
        for (int j = 0; j < max_item_number[i]; j++)
        {
            // Don't override predefines
            if (you.item_description[i][j] != 255)
                continue;

            // Pick a new description until it's good.
            while (true)
            {
                switch (i)
                {
                case IDESC_WANDS: // wands
                    you.item_description[i][j] = random2(NDSC_WAND_PRI
                                                         * NDSC_WAND_SEC);
                    if (coinflip())
                        you.item_description[i][j] %= NDSC_WAND_PRI;
                    break;

                case IDESC_POTIONS: // potions
                    you.item_description[i][j] = _random_potion_description();
                    break;

                case IDESC_SCROLLS: // scrolls: random seed for the name
                case IDESC_SCROLLS_II:
                    you.item_description[i][j] = random2(151);
                    break;

                case IDESC_RINGS: // rings and amulets
                    you.item_description[i][j] = random2(NDSC_JEWEL_PRI
                                                         * NDSC_JEWEL_SEC);
                    if (coinflip())
                        you.item_description[i][j] %= NDSC_JEWEL_PRI;
                    break;

                case IDESC_STAVES: // staves and rods
                    you.item_description[i][j] = random2(NDSC_STAVE_PRI
                                                         * NDSC_STAVE_SEC);
                    break;
                }

                bool is_ok = true;

                // Test whether we've used this description before.
                // Don't have p < j because some are preassigned.
                for (int p = 0; p < max_item_number[i]; p++)
                {
                    if (p == j)
                        continue;

                    if (you.item_description[i][p] == you.item_description[i][j])
                    {
                        is_ok = false;
                        break;
                    }
                }
                if (is_ok)
                    break;
            }
        }
    }
}

void fix_up_jiyva_name()
{
    do
        you.jiyva_second_name = make_name(random_int(), false, 8, 'J');
    while (strncmp(you.jiyva_second_name.c_str(), "J", 1) != 0);

    you.jiyva_second_name = replace_all(you.jiyva_second_name, " ", "");
}
