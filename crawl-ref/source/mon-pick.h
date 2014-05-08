/**
 * @file
 * @brief Functions used to help determine which monsters should appear.
**/


#ifndef MONPICK_H
#define MONPICK_H

#include "externs.h"

int mons_rarity(int mcls, const level_id &place = level_id::current(), 
                bool force_normal = false);

int mons_level(int mcls, const level_id &place = level_id::current());

//bool mons_abyss(int mcls);

//int mons_rare_abyss(int mcls, bool force_normal = false);

int mons_null_level(int mcls);
int mons_null_rare(int mcls, bool force_normal = false);

int mons_abyss_level(int mcls);
int mons_abyss_rare(int mcls, bool force_normal = false);
int mons_pan_level(int mcls);
int mons_pan_rare(int mcls, bool force_normal = false);

int mons_cocytus_level(int mcls);
int mons_cocytus_rare(int mcls, bool force_normal = false);
int mons_crypt_level(int mcls);
int mons_crypt_rare(int mcls, bool force_normal = false);
int mons_dis_level(int mcls);
int mons_dis_rare(int mcls, bool force_normal = false);
int mons_gehenna_level(int mcls);
int mons_gehenna_rare(int mcls, bool force_normal = false);
int mons_hallblade_level(int mcls);
int mons_hallblade_rare(int mcls, bool force_normal = false);
int mons_hallelf_level(int mcls);
int mons_hallelf_rare(int mcls, bool force_normal = false);
int mons_hallzot_level(int mcls);
int mons_hallzot_rare(int mcls, bool force_normal = false);
int mons_hive_level(int mcls);
int mons_hive_rare(int mcls, bool force_normal = false);
int mons_lair_level(int mcls);
int mons_lair_rare(int mcls, bool force_normal = false);
int mons_dwarf_level(int mcls);
int mons_dwarf_rare(int mcls, bool force_normal = false);
int mons_mineorc_level(int mcls);
int mons_mineorc_rare(int mcls, bool force_normal = false);
int mons_pitslime_level(int mcls);
int mons_pitslime_rare(int mcls, bool force_normal = false);
int mons_pitsnake_level(int mcls);
int mons_pitsnake_rare(int mcls, bool force_normal = false);
int mons_standard_level(int mcls);
int mons_standard_rare(int mcls, bool force_normal = false);
int mons_swamp_level(int mcls);
int mons_swamp_rare(int mcls, bool force_normal = false);
int mons_shoals_level(int mcls);
int mons_shoals_rare(int mcls, bool force_normal = false);
int mons_spidernest_level(int mcls);
int mons_spidernest_rare(int mcls, bool force_normal = false);
int mons_forest_level(int mcls);
int mons_forest_rare(int mcls, bool force_normal = false);
int mons_tartarus_level(int mcls);
int mons_tartarus_rare(int mcls, bool force_normal = false);
int mons_tomb_level(int mcls);
int mons_tomb_rare(int mcls, bool force_normal = false);
int mons_vestibule_level(int mcls);
int mons_vestibule_rare(int mcls, bool force_normal = false);
int mons_sewer_level(int mcls);
int mons_sewer_rare(int mcls, bool force_normal = false);

#endif
