/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#pragma once

#include <string>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

//     Config
const  int          DEFAULT_CONFIG_VERSION             =  8;
const  int          DEFAULT_SAVE_VERSION               =  10;
const  glm::ivec2   DEFAULT_WINDOW_SIZE                =  glm::ivec2(1024,768);
const  bool         DEFAULT_FULLSCREEN                 =  false;
const  bool         DEFAULT_AUDIOENABLED               =  true;
const  bool         DEFAULT_VSYNC                      =  true;
const  double       DEFAULT_MAXFPS                     =  180.0;
const  std::size_t  DEFAULT_MAXCLIENTS                 =  999;
const  double       DEFAULT_NETWORKRATE                =  1.0/20.0;
const  uint16_t     DEFAULT_NETWORKPORT                =  31234;
const  uint16_t     DEFAULT_NETWORKPINGPORT            =  31235;
const  double       DEFAULT_TIMESTEP                   =  1/100.0;
const  double       DEFAULT_AUTOSAVE_PERIOD            =  60.0;
//     Debug
const  double       DEBUG_STALL_THRESHOLD              =  1.0;
//     Camera
const  float        CAMERA_DISTANCE                    =  8.4375f;
const  float        CAMERA_DIVISOR                     =  30.0f;
const  float        CAMERA_EDITOR_DIVISOR              =  5.0f;
const  float        CAMERA_FOVY                        =  90.0f;
const  float        CAMERA_NEAR                        =  0.1f;
const  float        CAMERA_FAR                         =  100.0f;
//     Account
const  uint32_t     ACCOUNT_SINGLEPLAYER_ID            =  1;
const  uint32_t     ACCOUNT_BOTS_ID                    =  2;
const  std::size_t  ACCOUNT_MAX_USERNAME_SIZE          =  20;
const  std::size_t  ACCOUNT_MAX_PASSWORD_SIZE          =  20;
const  int          ACCOUNT_MAX_CHARACTER_SLOTS        =  10;
//     Map
const  int          MAP_VERSION                        =  1;
const  int          MAP_TILE_WIDTH                     =  128;
const  int          MAP_TILE_HEIGHT                    =  128;
const  double       MAP_CLOCK_START                    =  8.0*60.0;
const  double       MAP_DAY_LENGTH                     =  24.0*60.0;
const  double       MAP_CLOCK_SPEED                    =  1.0;
const  double       MAP_EDITOR_CLOCK_SPEED             =  200.0;
const  glm::vec4    MAP_AMBIENT_LIGHT                  =  glm::vec4(0.3,0.3,0.3,1);
const  char*const   MAP_DEFAULT_TILESET                =  "textures/atlas/main.png";
//     UI
const  glm::vec2    UI_PORTRAIT_SIZE                   =  glm::vec2(100,100);
const  glm::vec2    UI_SLOT_SIZE                       =  glm::vec2(64,64);
const  glm::vec2    UI_BUFF_SIZE                       =  glm::vec2(48,48);
const  glm::vec2    UI_TILE_SIZE                       =  glm::vec2(64,64);
const  glm::vec2    UI_GRID_SIZE                       =  glm::vec2(64,64);
//     HUD
const  int          HUD_CHAT_MESSAGES                  =  15;
const  int          HUD_CHAT_TIMEOUT                   =  10;
const  double       HUD_CHAT_FADETIME                  =  1.0;
const  int          HUD_CHAT_SIZE                      =  100;
const  int          HUD_KEYNAME_LENGTH                 =  3;
const  double       HUD_RECENTITEM_TIMEOUT             =  10.0;
const  double       HUD_RECENTITEM_FADETIME            =  2.0;
const  double       HUD_MESSAGE_TIMEOUT                =  10.0;
const  double       HUD_MESSAGE_FADETIME               =  2.0;
const  double       HUD_ACTIONRESULT_TIMEOUT           =  2.5;
const  double       HUD_ACTIONRESULT_SPEED             =  3.0;
const  double       HUD_ACTIONRESULT_TIMEOUT_SHORT     =  1.0;
const  double       HUD_ACTIONRESULT_SPEED_SHORT       =  0.25;
const  double       HUD_ACTIONRESULT_FADETIME          =  1.0;
const  double       HUD_STATCHANGE_TIMEOUT             =  1.0;
const  double       HUD_STATCHANGE_TIMEOUT_LONG        =  3.0;
const  double       HUD_STATCHANGE_FADETIME            =  0.5;
const  float        HUD_STATCHANGE_DISTANCE            =  20.0f;
const  double       HUD_GPM_TIME                       =  60.0;
//     Battle
const  int          BATTLE_MINSTEPS                    =  12;
const  int          BATTLE_MAXSTEPS                    =  30;
const  int          BATTLE_MAX_OBJECTS_PER_SIDE        =  8;
const  int          BATTLE_ROWS_PER_SIDE               =  4;
const  int          BATTLE_COLUMN_SPACING              =  324;
const  double       BATTLE_DEFAULTATTACKPERIOD         =  2.0;
const  double       BATTLE_STUNNED_BATTLESPEED         =  5.0;
const  double       BATTLE_WAITDEADTIME                =  0.75;
const  double       BATTLE_MAX_START_TURNTIMER         =  0.35;
const  float        BATTLE_HEALTHBAR_WIDTH             =  126;
const  float        BATTLE_HEALTHBAR_HEIGHT            =  30;
const  int          BATTLE_MIN_SPEED                   =  5;
const  int          BATTLE_PVP_VICTIM_SIDE             =  0;
const  int          BATTLE_PVP_ATTACKER_SIDE           =  1;
const  float        BATTLE_PVP_DISTANCE                =  1.42f*1.42f;
const  float        BATTLE_JOIN_DISTANCE               =  1.42f*1.42f;
const  float        BATTLE_COOP_DISTANCE               =  7.0f*7.0f;
const  int          BATTLE_LEVEL_RANGE                 =  10;
const  int          BATTLE_BOSS_DIFFICULTY_PER_KILL    =  50;
const  int          BATTLE_DIFFICULTY_DAMAGE_START     =  0;
const  float        BATTLE_DIFFICULTY_DAMAGE           =  0.2f;
const  int          BATTLE_DIFFICULTY_PER_PLAYER       =  20;
const  int          BATTLE_DIFFICULTY_PER_PLAYER_BOSS  =  50;
//     Player
const  double       PLAYER_TELEPORT_TIME               =  3.0;
const  double       PLAYER_MOVETIME                    =  0.15;
const  int          PLAYER_MIN_MOVESPEED               =  5;
const  double       PLAYER_ATTACKTIME                  =  1.0;
const  int          PLAYER_NAME_SIZE                   =  15;
const  float        PLAYER_INVIS_ALPHA                 =  0.27f;
const  int64_t      PLAYER_MAX_GOLD                    =  1000000000000000;
const  float        PLAYER_DEATH_GOLD_PENALTY          =  0.2f;
const  double       PLAYER_IDLE_TIME                   =  120.0;
//     Game
const  int          GAME_MAX_RESISTANCE                =  75;
const  int          GAME_MIN_RESISTANCE                =  -200;
const  int          GAME_MAX_EVASION                   =  99;
const  int64_t      GAME_ENCHANT_COST_BASE             =  5;
const  int64_t      GAME_ENCHANT_COST_POWER            =  3;
const  int64_t      GAME_ENCHANT_COST_RATE             =  5;
const  double       GAME_UPGRADE_COST_MULTIPLIER       =  0.2;
const  int64_t      GAME_UPGRADE_BASE_COST             =  0;
const  double       GAME_UPGRADE_AMOUNT                =  0.2;
const  double       GAME_NEGATIVE_UPGRADE_SCALE        =  0.25;
const  int          GAME_TRADING_LEVEL                 =  3;
const  int          GAME_TRADING_LEVEL_RANGE           =  10;
const  int          GAME_DEFAULT_MAX_SKILL_LEVEL       =  5;
const  int          GAME_MAX_SKILL_UNLOCKS             =  10;
const  int          GAME_MAX_SKILL_LEVEL               =  50;
const  int          GAME_MAX_VENDOR_DISCOUNT           =  99;
const  double       GAME_REBIRTH_WEALTH_MULTIPLIER     =  0.1;
//     Levels
const  int          LEVELS_MAX                         =  9999;
const  int          LEVELS_HEALTH_BASE                 =  150;
const  int          LEVELS_HEALTH_RATE                 =  20;
const  int          LEVELS_MANA_BASE                   =  0;
const  int          LEVELS_MANA_RATE                   =  0;
const  int          LEVELS_SKILL_POINTS                =  3;
//     Actionbar
const  int          ACTIONBAR_BELT_STARTS              =  10;
const  int          ACTIONBAR_DEFAULT_BELTSIZE         =  1;
const  int          ACTIONBAR_DEFAULT_SKILLBARSIZE     =  4;
const  int          ACTIONBAR_MAX_SKILLBARSIZE         =  8;
const  int          ACTIONBAR_MAX_BELTSIZE             =  4;
const  int          ACTIONBAR_MAX_SIZE                 =  ACTIONBAR_BELT_STARTS+ACTIONBAR_DEFAULT_SKILLBARSIZE;
//     Inventory
const  int          INVENTORY_TOOLTIP_OFFSET           =  50;
const  int          INVENTORY_TOOLTIP_PADDING          =  14;
const  float        INVENTORY_TOOLTIP_WIDTH            =  400;
const  float        INVENTORY_TOOLTIP_HEIGHT           =  280;
const  float        INVENTORY_TOOLTIP_MAP_HEIGHT       =  550;
const  float        INVENTORY_TOOLTIP_TEXT_SPACING     =  26;
const  int          INVENTORY_MAX_TRADE_ITEMS          =  10;
const  int          INVENTORY_SIZE                     =  36;
const  int          INVENTORY_MAX_STACK                =  65535;
const  int          INVENTORY_INCREMENT_MODIFIER       =  5;
const  int          INVENTORY_SPLIT_MODIFIER           =  5;
//     Trader
const  int          TRADER_MAXITEMS                    =  8;
//     Menu
const  float        MENU_ACCEPTINPUT_FADE              =  0.7f;
const  float        MENU_PAUSE_FADE                    =  0.7f;
const  double       MENU_DOUBLECLICK_TIME              =  0.250;
const  float        MENU_MAP_SCROLL_SPEED              =  0.005f;
const  double       MENU_CONNECT_COOLDOWN              =  5.0;
//     Scripting
const  char*const   SCRIPTS_GAME                       =  "scripts/game.lua";
const  char*const   SCRIPTS_DATA                       =  "scripts/data.lua";
