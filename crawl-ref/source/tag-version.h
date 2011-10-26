#ifndef TAG_VERSION_H
#define TAG_VERSION_H

// Let CDO updaters know if the syntax changes.
#define TAG_MAJOR_VERSION  32

// Minor version will be reset to zero when major version changes.
enum tag_minor_version
{
    TAG_MINOR_INVALID         = -1,
    TAG_MINOR_RESET           = 0, // Minor tags were reset
    TAG_MINOR_DIRECTED_ABYSS,      // Add the directed Abyss portal.
    TAG_MINOR_LUA_COLOUR_ENUM,     // Require enums for Lua colours.
    TAG_MINOR_GOLDIFIED_RUNES,     // Runes are undroppable and don't take space.
    TAG_MINOR_MONS_THREAT_LEVEL,   // Save threat level in monster_info.
    TAG_MINOR_STAIR_FOLLOW,        // Monsters follow you more dynamically, Brogue-style.
    TAG_MINOR_TAR_DRAINING,        // Tartarus drains your experience.
    TAG_MINOR_UNIQUE_NOTES,        // Automatic annotations for uniques.
    TAG_MINOR_MAP_MASK,            // Wider map mask.
    TAG_MINOR_POLEARMS_REACH,      // Purge reaching from polearms (built-in now).
    TAG_MINOR_HELL_KEY,            // Add the key to Hell.
    TAG_MINOR_NUM_LEVEL_CONN,      // Fix level connectivity data hard-coding NUM_BRANCHES.
    TAG_MINOR_CHERUB_ATTACKS,      // Give Cherubs a new attack flavour.
    TAG_MINOR_SAVE_MAP,            // Save the name of the loaded tutorial map.
    TAG_MINOR_CHALLENGE_GAMES,     // Add challenge games.
	TAG_MINOR_MANUAL_CONTROL,	   // Less randomness in Wrath of Sif Muna
	TAG_MINOR_MOVE_DIFFICULTY,     // Move difficulty level to tag_read/write_char,
	                               // so that it can be displayed in file select
    TAG_MINOR_ZIN_REFORM,          // Zin overhaul with edicts and commandments
    NUM_TAG_MINORS,
    TAG_MINOR_VERSION = NUM_TAG_MINORS - 1
};

#endif
