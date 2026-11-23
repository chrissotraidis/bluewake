// SPDX-License-Identifier: GPL-3.0-or-later
// The game's toon tables: the textures its shading reads as data (light and
// shadow thresholds, specular and metal ramps), not as pictures. An HD pack's
// versions of them change how characters and items are shaded rather than how
// sharp they look: Hypatia's pack replaces 15 of them, among them the shadow
// threshold table (toonEX) whose redrawn edges shrink the shadow in Link's hair
// from 26 % to 21 % of it and make its edge jagged. With "Keep the game's
// shading tables" on (BLUEWAKE_TEXTURE_PACK_KEEP_TABLES=1, main.c), the host
// hands these names to Aurora (DOL_AURORA_TEXTURE_PACK_SKIP), which leaves a
// pack's files for them out, so the game's own tables stay.
// Every J3D/BTI texture on the GZLE01 disc whose name has "toon" in it, named
// as Dolphin names a replacement (the importer's reader, index_gc in
// scripts/import_wwhd_textures.py, and its filename()); the comment is the
// archive and texture name. Names only, no artwork.
#pragma once

#define BLUEWAKE_TOON_TABLES     "tex1_128x128_688d250af56391bc_5," /* Bwd.arc toonAM */ \
    "tex1_128x128_8be9535b8b90b1e4_14," /* Btd.arc toonGOMA */ \
    "tex1_128x128_95e99b35c0b15dd0_871fc244c10c9b7e_8," /* Demo07.arc toonGN */ \
    "tex1_128x128_b0409d585a6fb1f0_cbdb6661a0b8bd5e_8," /* Am.arc toonAM2 */ \
    "tex1_128x128_c02cbe44ec949385_c1d516e425fb9ea5_8," /* Demo18.arc toonJB2 */ \
    "tex1_128x128_d39c29d2875175e6_96932f402d9baa1f_8," /* Bwds.arc toonJB */ \
    "tex1_128x32_acb0eea2ba61e548_2," /* Always.arc toonICE */ \
    "tex1_16x16_6c22104500a8ff19_0," /* Doguu.arc toon_spc */ \
    "tex1_256x256_22e85c65595223d7_14," /* Cloth.arc clothtoon.bti */ \
    "tex1_256x256_2e07614311d95b09_14," /* Bm.arc toonEX */ \
    "tex1_256x256_c2c9175a311d7325_e23b5512ccbf2215_9," /* Bwd.arc toonbwd */ \
    "tex1_256x8_05491310a772fe76_0," /* Doguu.arc toon_glass */ \
    "tex1_256x8_5e7a68079a8c1d89_0," /* Ji.arc toon */ \
    "tex1_256x8_5e9a844c867970ba_0," /* Link.arc AK_uchiwaToon */ \
    "tex1_256x8_6b58360929adb54e_0," /* Always.arc toon */ \
    "tex1_256x8_8e161af8128bda8d_0," /* Always.arc toonEFF */ \
    "tex1_256x8_ab4d8fce1be88deb_0," /* Demo34.arc AK_kekkaiToon00 */ \
    "tex1_256x8_dde56356981b2f3c_0," /* Co.arc toon_pearl */ \
    "tex1_256x8_fc242dacd52c764f_0," /* Vmc.arc V_toon_mc */ \
    "tex1_32x32_492972587493da1e_0," /* Always.arc toon_ball_spc */ \
    "tex1_64x32_3156b2fa13b2fff3_2," /* Link.arc toonBottle */ \
    "tex1_64x4_dea8961e1bd0da68_1," /* Vteng.arc B_toon_g */ \
    "tex1_64x64_42a929fbe8adf2df_eb2aee8777b654e2_8," /* Sh.arc toonSH */ \
    "tex1_64x64_4a6611f7f4310dc4_5cd1cd1e2c2069b1_8," /* Bdk.arc toonMETAL */ \
    "tex1_64x64_5b3efbc1868fab77_14," /* Cc.arc toonCCmini */ \
    "tex1_64x64_9554dfc06f519623_14," /* VzouK.arc V_toonMETAL_EX */ \
    "tex1_64x64_c793e8f30a470401_69e4ee9751b2f527_9," /* Pz.arc toonPZ */ \
    "tex1_64x64_f2c39ce8cca81a4a_14," /* Cc.arc toonCCmini */ \
    "tex1_64x8_089c52a7131cb1ef_0," /* Always.arc toonMETAL02 */ \
    "tex1_64x8_1e2af5ccd6e3432c_0," /* Ship.arc V_toonCANON */ \
    "tex1_64x8_349d677372a9a040_0," /* Always.arc bb_toon */ \
    "tex1_64x8_4a98bdbc02cf1857_0," /* Always.arc V_toonCHU */ \
    "tex1_64x8_bdc505ea303ae698_0," /* Always.arc V_toonMETAL */ \
    "tex1_64x8_ce07e52f85693435_2," /* Bin.arc toon_bin */ \
    "tex1_64x8_e8b769144fdfebc9_2," /* Figure6.arc V_toon_ZIC */ \
    "tex1_64x8_f79677e80d404c3b_2," /* VshiM.arc V_toonICE */ \
    "tex1_8x8_af9319179ef24461_0" /* Ac.arc ZBtoonEX */
