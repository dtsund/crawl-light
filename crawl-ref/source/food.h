/**
 * @file
 * @brief Functions for eating and butchering.  Mostly butchering, currently.
**/


#ifndef FOOD_H
#define FOOD_H

enum food_type
{
    FOOD_PEAR,                         //   0
    FOOD_APPLE,
    FOOD_CHOKO,
    FOOD_SNOZZCUMBER,
    FOOD_APRICOT,
    FOOD_ORANGE,                       //   5
    FOOD_BANANA,
    FOOD_STRAWBERRY,
    FOOD_RAMBUTAN,
    FOOD_LEMON,
    FOOD_GRAPE,                        //   10
    FOOD_SULTANA,
    FOOD_LYCHEE,
    FOOD_CHUNK,
    NUM_FOODS                          //   14
};

int count_corpses_in_pack(bool blood_only = false);
bool butchery(int which_corpse = -1, bool bottle_blood = false);

void weapon_switch(int targ);

bool is_poisonous(const item_def &food);
bool is_mutagenic(const item_def &food);
bool is_contaminated(const item_def &food);
bool causes_rot(const item_def &food);

bool is_forbidden_food(const item_def &food);

bool chunk_is_poisonous(int chunktype);

#endif
