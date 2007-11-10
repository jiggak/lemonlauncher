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
#include "options.h"
#include "error.h"

#include <string>
#include <iostream>

namespace ll { options g_opts; }

using namespace ll;
using namespace std;

options::options() : _cfg(NULL) { }

void options::load(const char* conf_dir)
{
   _conf_dir = (char*)conf_dir;
   
   cfg_opt_t opts[] = {
      CFG_INT(KEY_LOGLEVEL, 2, CFGF_NONE),
      
      CFG_INT(KEY_SCREEN_WIDTH, 640, CFGF_NONE),
      CFG_INT(KEY_SCREEN_HEIGHT, 480, CFGF_NONE),
      CFG_INT(KEY_SCREEN_BPP, 24, CFGF_NONE),
      CFG_BOOL(KEY_FULLSCREEN, cfg_false, CFGF_NONE),
      
      CFG_STR(KEY_SKIN_FILE, "", CFGF_NONE),
      CFG_INT(KEY_SNAPSHOT_DELAY, 500, CFGF_NONE),
      
      CFG_STR(KEY_MAME_PATH, "mame %r", CFGF_NONE),
      CFG_STR(KEY_MAME_SNAP_PATH, "", CFGF_NONE),
      
      CFG_INT(KEY_KEYCODE_EXIT, 27, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_RELOAD, 49, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_TOGGLE, 50, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P1_UP, 273, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P1_DOWN, 274, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P1_PGUP, 275, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P1_PGDOWN, 276, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P1_BTN1, 306, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P1_BTN2, 308, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P2_UP, 114, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P2_DOWN, 102, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P2_PGUP, 100, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P2_PGDOWN, 103, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P2_BTN1, 97, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_P2_BTN2, 115, CFGF_NONE),
      CFG_END()
   };
   
   _cfg = cfg_init(opts, CFGF_NONE);
   int result = cfg_parse(_cfg, locate("lemonlauncher.conf"));
   
   if (result == CFG_FILE_ERROR) {
      log << warn << "options: file error, using defaults" << endl;
      cfg_parse_buf(_cfg, "");
   } else if (result == CFG_PARSE_ERROR) {
      throw bad_lemon("options: parse error");
   }
}

options::~options()
{ cfg_free(_cfg); }

bool options::get_bool(const char* key) const
{ return cfg_getbool(_cfg, key) == cfg_true; }

int options::get_int(const char* key) const
{ return cfg_getint(_cfg, key); }

const char* options::get_string(const char* key) const
{ return cfg_getstr(_cfg, key); }

const char* options::locate(const char* file) const
{
   string path(_conf_dir);
   path.append("/").append(file);
   return path.c_str();
}
