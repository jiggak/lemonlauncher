/*
 * Copyright 2007 Josh Kropf
 * 
 * This file is part of Lemon Launcher.
 * 
 * Lemon Launcher is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Lemon Launcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Lemon Launcher; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <string>
#include <confuse.h>

namespace ll {

/* log level: 0 = off, 1 = error, 2 = info, 3 = warning, 4 = debug */
#define KEY_LOGLEVEL "loglevel"

/* Screen settings */
#define KEY_SCREEN_WIDTH  "width"     /* width of video mode  (int) */
#define KEY_SCREEN_HEIGHT "height"    /* height of video mode (int) */
#define KEY_SCREEN_BPP    "bitdepth"  /* pits per pixel */
#define KEY_FULLSCREEN    "fullscreen"
#define KEY_FONT_FILE     "font"
#define KEY_TITLE_HEIGHT  "title_height"
#define KEY_LIST_HEIGHT   "list_height"
#define KEY_PAGE_SIZE     "pagesize"

/* MAME settings */
#define KEY_MAME_PATH       "path"
#define KEY_MAME_PARAMS     "params"
#define KEY_MAME_SNAPS_PATH "snaps"
#define KEY_MAME_ROM_PATH   "rompath"

/* Key mapping */
#define KEY_KEYCODE_EXIT      "exit"
#define KEY_KEYCODE_SNAP      "snap"
#define KEY_KEYCODE_RELOAD    "reload"
#define KEY_KEYCODE_TOGGLE    "showhide"
#define KEY_KEYCODE_P1_UP     "p1up"
#define KEY_KEYCODE_P1_DOWN   "p1down"
#define KEY_KEYCODE_P1_PGUP   "p1pgup"
#define KEY_KEYCODE_P1_PGDOWN "p1pgdown"
#define KEY_KEYCODE_P1_BTN1   "p1button1"
#define KEY_KEYCODE_P1_BTN2   "p1button2"
#define KEY_KEYCODE_P2_UP     "p2up"
#define KEY_KEYCODE_P2_DOWN   "p2down"
#define KEY_KEYCODE_P2_PGUP   "p2pgup"
#define KEY_KEYCODE_P2_PGDOWN "p2pgdown"
#define KEY_KEYCODE_P2_BTN1   "p2button1"
#define KEY_KEYCODE_P2_BTN2   "p2button2"

/**
 * Class for reading configuration file.  Settings are accessed by passing
 * a key to one of the get_* methods.
 */
class options
{
private:
   cfg_t *_cfg;
   
public:
   /**
    * Creates options instance by parsing config file at the given path
    * @param path fully qualified path to config file
    */
   options(const char* path);
   
   /**
    * Cleanup
    */
   ~options();
   
   /**
    * 
    */
   bool get_bool(const char* key) const;
   
   /**
    * 
    */
   int get_int(const char* key) const;
   
   /**
    *
    */
   const char* get_string(const char* key) const;
};

} // end namespace declaration 

#endif /*OPTIONS_H_*/
