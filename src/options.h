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

#include <confuse.h>

namespace ll {

/* log level: 0 = off, 1 = error, 2 = info, 3 = warning, 4 = debug */
#define KEY_LOGLEVEL "loglevel"

/* Screen settings */
#define KEY_SCREEN_WIDTH   "width"     /* width of video mode  (int) */
#define KEY_SCREEN_HEIGHT  "height"    /* height of video mode (int) */
#define KEY_SCREEN_BPP     "bitdepth"  /* pits per pixel */
#define KEY_FULLSCREEN     "fullscreen"

/* Ui settings */
#define KEY_SKIN_FILE       "theme"
#define KEY_SNAPSHOT_DELAY  "snapshot_delay"

/* MAME settings */
#define KEY_MAME_PATH       "mame"
#define KEY_MAME_SNAP_PATH  "snap"

/* Key mapping */
#define KEY_KEYCODE_EXIT      "exit"
#define KEY_KEYCODE_UP        "up"
#define KEY_KEYCODE_DOWN      "down"
#define KEY_KEYCODE_PGUP      "pgup"
#define KEY_KEYCODE_PGDOWN    "pgdown"
#define KEY_KEYCODE_RELOAD    "reload"
#define KEY_KEYCODE_TOGGLE    "showhide"
#define KEY_KEYCODE_SELECT    "select"
#define KEY_KEYCODE_BACK      "back"
#define KEY_KEYCODE_ALPHAMOD  "alphamod"

/**
 * Class for reading configuration file.  Settings are accessed by passing
 * a key to one of the get_* methods.
 */
class options
{
private:
   char* _conf_dir;
   cfg_t *_cfg;
   
public:
   /**
    * Creates options class.  The load method must be called before calling
    * any of the get_ methods.
    */
   options();
   
   /** Cleanup */
   ~options();
   
   /** Parses conf conf files from the conf file directory */
   void load(const char* conf_dir);
   
   /** Returns an option as a boolean */
   bool get_bool(const char* key) const;
   
   /** Returns an option as an integer */
   int get_int(const char* key) const;
   
   /** Returns an option as a string */
   const char* get_string(const char* key) const;
   
   /** Returns full path to the given file relative to conf dir */
   const char* locate(const char* file) const;
};

extern options g_opts;

} // end namespace declaration 

#endif /*OPTIONS_H_*/
