/*
 * pc_game_id.h - the disc this build runs from (docs/PAL_DISC_PLAN.md).
 *
 * The game version is fixed at compile time (VERSION, native/cmake/GameConfig.cmake): the USA build
 * (COS_VERSION=2, the reference) runs only from GZLE01 revision 0, the European build
 * (COS_VERSION=3) only from GZLP01 revision 0. GameConfig.cmake defines COS_GAME_ID_STR for every
 * unit that links cos_game_headers; the Switch platform layer gets the same define
 * (switch/native/CMakeLists.txt). Plain C, so cos_switch.h can include it.
 */
#ifndef PC_GAME_ID_H
#define PC_GAME_ID_H

#ifndef COS_GAME_ID_STR
#define COS_GAME_ID_STR "GZLE01"
#endif

/* The other supported disc, for the message that names the right build. */
#define PC_GAME_ID_USA "GZLE01"
#define PC_GAME_ID_EUR "GZLP01"

/* The disc's region as the messages name it, the expected image's file name, the build's NRO. */
#define PC_GAME_IS_EUR (COS_GAME_ID_STR[3] == 'P')
#define PC_GAME_REGION_EN (PC_GAME_IS_EUR ? "Europe" : "USA")
#define PC_GAME_REGION_ES (PC_GAME_IS_EUR ? "Europa" : "EE. UU.")
#define PC_GAME_ISO_NAME COS_GAME_ID_STR ".iso"
#define PC_GAME_NRO_USA "switchwaker.nro"
#define PC_GAME_NRO_EUR "switchwaker-eur.nro"

/* What the player sees on the Switch (INSTALL.md): each build's Homebrew Menu entry (its NACP name,
 * switch/native/CMakeLists.txt) and the end of its release download's name
 * (scripts/release/package_switch.sh: SwitchWaker-<tag>-switch.zip, SwitchWaker-<tag>-switch-eur.zip).
 * Both downloads install the NRO as sdmc:/switch/switchwaker/switchwaker.nro (one region per SD card,
 * so the HOME-menu forwarder starts either); the European build is switchwaker-eur.nro only on the
 * build machine (PC_GAME_NRO_EUR, its .elf and log names). */
#define PC_GAME_ENTRY_USA "SwitchWaker"
#define PC_GAME_ENTRY_EUR "SwitchWaker EUR"
#define PC_GAME_ZIP_USA "-switch.zip"
#define PC_GAME_ZIP_EUR "-switch-eur.zip"

#endif /* PC_GAME_ID_H */
