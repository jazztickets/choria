/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
const  int          DEFAULT_CONFIG_VERSION          =  3;
const  int          DEFAULT_SAVE_VERSION            =  1;
const  glm::ivec2   DEFAULT_WINDOW_SIZE             =  glm::ivec2(1024,768);
const  int          DEFAULT_FULLSCREEN              =  0;
const  int          DEFAULT_AUDIOENABLED            =  1;
const  int          DEFAULT_VSYNC                   =  1;
const  int          DEFAULT_ANISOTROPY              =  0;
const  double       DEFAULT_MAXFPS                  =  180.0;
const  size_t       DEFAULT_MAXCLIENTS              =  100;
const  double       DEFAULT_NETWORKRATE             =  1.0/20.0;
const  uint16_t     DEFAULT_NETWORKPORT             =  31234;
const  uint16_t     DEFAULT_NETWORKPORT_ALT         =  31235;
const  double       DEFAULT_TIMESTEP                =  1/100.0;
const  double       DEFAULT_AUTOSAVE_PERIOD         =  60.0;
//     Graphics
const  int          GRAPHICS_CIRCLE_VERTICES        =  32;
//     Camera
const  float        CAMERA_DISTANCE                 =  8.4375f;
const  float        CAMERA_DIVISOR                  =  30.0f;
const  float        CAMERA_EDITOR_DIVISOR           =  5.0f;
const  float        CAMERA_FOVY                     =  90.0f;
const  float        CAMERA_NEAR                     =  0.1f;
const  float        CAMERA_FAR                      =  100.0f;
//     Account
const  size_t       ACCOUNT_MAX_USERNAME_SIZE       =  20;
const  size_t       ACCOUNT_MAX_PASSWORD_SIZE       =  20;
const  int          ACCOUNT_MAX_CHARACTER_SLOTS     =  8;
//     Map
const  int          MAP_VERSION                     =  1;
const  int          MAP_TILE_WIDTH                  =  64;
const  int          MAP_TILE_HEIGHT                 =  64;
const  double       MAP_CLOCK_START                 =  8.0*60.0;
const  double       MAP_DAY_LENGTH                  =  24.0*60.0;
const  double       MAP_CLOCK_SPEED                 =  1.0;
const  glm::vec4    MAP_AMBIENT_LIGHT               =  glm::vec4(0.3,0.3,0.3,1);
const  std::string  MAP_DEFAULT_TILESET             =  "atlas0.png";
//     HUD
const  int          HUD_CHAT_MESSAGES               =  15;
const  int          HUD_CHAT_TIMEOUT                =  10;
const  double       HUD_CHAT_FADETIME               =  1.0;
const  int          HUD_CHAT_SIZE                   =  100;
const  int          HUD_KEYNAME_LENGTH              =  3;
const  double       HUD_RECENTITEM_TIMEOUT          =  10.0;
const  double       HUD_RECENTITEM_FADETIME         =  2.0;
const  double       HUD_MESSAGE_TIMEOUT             =  10.0;
const  double       HUD_MESSAGE_FADETIME            =  2.0;
const  double       HUD_ACTIONRESULT_TIMEOUT        =  2.5;
const  double       HUD_ACTIONRESULT_SPEED          =  3.0;
const  double       HUD_ACTIONRESULT_TIMEOUT_SHORT  =  1.0;
const  double       HUD_ACTIONRESULT_SPEED_SHORT    =  0.25;
const  double       HUD_ACTIONRESULT_FADETIME       =  1.0;
const  double       HUD_STATCHANGE_TIMEOUT          =  1.0;
const  double       HUD_STATCHANGE_TIMEOUT_LONG     =  3.0;
const  double       HUD_STATCHANGE_FADETIME         =  0.5;
const  float        HUD_STATCHANGE_DISTANCE         =  20.0f;
//     Battle
const  int          BATTLE_MINSTEPS                 =  9;
const  int          BATTLE_MAXSTEPS                 =  30;
const  int          BATTLE_MAXFIGHTERS_SIDE         =  8;
const  int          BATTLE_MULTI_TARGET_COUNT       =  3;
const  int          BATTLE_ROWS_PER_SIDE            =  4;
const  int          BATTLE_COLUMN_SPACING           =  230;
const  double       BATTLE_DEFAULTSPEED             =  (1.0/2.0);
const  double       BATTLE_WAITDEADTIME             =  0.75;
const  double       BATTLE_MAX_START_TURNTIMER      =  0.35;
const  float        BATTLE_HEALTHBAR_WIDTH          =  90;
const  float        BATTLE_HEALTHBAR_HEIGHT         =  22;
const  int          BATTLE_MIN_SPEED                =  5;
//     Player
const  double       PLAYER_TELEPORT_TIME            =  3.0;
const  double       PLAYER_MOVETIME                 =  0.15;
const  int          PLAYER_MIN_MOVESPEED            =  10;
const  double       PLAYER_ATTACKTIME               =  1.0;
const  int          PLAYER_NAME_SIZE                =  10;
const  float        PLAYER_INVIS_ALPHA              =  0.27f;
const  int          PLAYER_MAX_GOLD                 =  1000000000;
const  float        PLAYER_DEATH_GOLD_PENALTY       =  0.2f;
//     Game
const  int          GAME_MAX_RESISTANCE             =  75;
const  double       GAME_DIFFICULTY_PER_PLAYER      =  0.1;
const  float        GAME_UPGRADE_COST_MULTIPLIER    =  0.25f;
const  int          GAME_BASE_UPGRADE_COST          =  5;
const  float        GAME_UPGRADE_AMOUNT             =  0.25f;
//     Actionbar
const  int          ACTIONBAR_MAX_SIZE              =  8;
//     Inventory
const  int          INVENTORY_TOOLTIP_OFFSET        =  20;
const  int          INVENTORY_TOOLTIP_PADDING       =  10;
const  float        INVENTORY_TOOLTIP_WIDTH         =  250;
const  float        INVENTORY_TOOLTIP_HEIGHT        =  340;
const  int          INVENTORY_MAX_STACK             =  255;
//     Menu
const  float        MENU_ACCEPTINPUT_FADE           =  0.7f;
const  float        MENU_PAUSE_FADE                 =  0.7f;
const  double       MENU_DOUBLECLICK_TIME           =  0.250;
//     Scripting
const  std::string  SCRIPTS_PATH                    =  "scripts/";
const  std::string  SCRIPTS_GAME                    =  "game.lua";
//     Textures
const  std::string  TEXTURES_PATH                   =  "textures/";
const  std::string  TEXTURES_BATTLE                 =  "battle/";
const  std::string  TEXTURES_BUFFS                  =  "buffs/";
const  std::string  TEXTURES_BUILDS                 =  "builds/";
const  std::string  TEXTURES_EDITOR                 =  "editor/";
const  std::string  TEXTURES_HUD                    =  "hud/";
const  std::string  TEXTURES_HUD_REPEAT             =  "hud_repeat/";
const  std::string  TEXTURES_INTERFACE              =  "interface/";
const  std::string  TEXTURES_ITEMS                  =  "items/";
const  std::string  TEXTURES_MAP                    =  "map/";
const  std::string  TEXTURES_MENU                   =  "menu/";
const  std::string  TEXTURES_MONSTERS               =  "monsters/";
const  std::string  TEXTURES_PORTRAITS              =  "portraits/";
const  std::string  TEXTURES_MODELS                 =  "models/";
const  std::string  TEXTURES_SKILLS                 =  "skills/";
const  std::string  TEXTURES_STATUS                 =  "status/";
//     Assets
const  std::string  ASSETS_FONTS_PATH               =  "fonts/";
const  std::string  ASSETS_MAPS_PATH                =  "maps/";
const  std::string  ASSETS_SOUND_PATH               =  "sounds/";
const  std::string  ASSETS_MUSIC_PATH               =  "music/";
const  std::string  ASSETS_PROGRAMS                 =  "tables/programs.tsv";
const  std::string  ASSETS_COLORS                   =  "tables/colors.tsv";
const  std::string  ASSETS_FONTS                    =  "tables/fonts.tsv";
const  std::string  ASSETS_LAYERS                   =  "tables/layers.tsv";
const  std::string  ASSETS_UI_BUTTONS               =  "tables/ui/buttons.tsv";
const  std::string  ASSETS_UI_ELEMENTS              =  "tables/ui/elements.tsv";
const  std::string  ASSETS_UI_IMAGES                =  "tables/ui/images.tsv";
const  std::string  ASSETS_UI_LABELS                =  "tables/ui/labels.tsv";
const  std::string  ASSETS_UI_STYLES                =  "tables/ui/styles.tsv";
const  std::string  ASSETS_UI_TEXTBOXES             =  "tables/ui/textboxes.tsv";
//     Colors
const  glm::vec4    COLOR_WHITE                     =  {1.0f,1.0f,1.0f,1.0f};
const  glm::vec4    COLOR_DARK                      =  {0.3f,0.3f,0.3f,1.0f};
const  glm::vec4    COLOR_TGRAY                     =  {1.0f,1.0f,1.0f,0.2f};
const  glm::vec4    COLOR_RED                       =  {1.0f,0.0f,0.0f,1.0f};
const  glm::vec4    COLOR_GREEN                     =  {0.0f,1.0f,0.0f,1.0f};
const  glm::vec4    COLOR_BLUE                      =  {0.0f,0.0f,1.0f,1.0f};
const  glm::vec4    COLOR_YELLOW                    =  {1.0f,1.0f,0.0f,1.0f};
const  glm::vec4    COLOR_MAGENTA                   =  {1.0f,0.0f,1.0f,1.0f};
const  glm::vec4    COLOR_CYAN                      =  {0.0f,1.0f,1.0f,1.0f};
const  glm::vec4    COLOR_GOLD                      =  {0.76f,0.73f,0.173f,1.0f};
const  glm::vec4    COLOR_LIGHTBLUE                 =  {0.0f,0.45f,1.0f,1.0f};
const  glm::vec4    COLOR_LIGHTGOLD                 =  {0.88f,0.85f,0.33f,1.0f};
const  glm::vec4    COLOR_GRAY                      =  {0.6f,0.6f,0.6f,1.0f};
const  glm::vec4    COLOR_LIGHTGRAY                 =  {0.78f,0.78f,0.78f,1.0f};
const  glm::vec4    COLOR_ITEMFADE                  =  {1.0f,1.0f,1.0f,0.5f};
