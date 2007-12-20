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

int cb_rotate(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
   if (strcmp(value, "none") == 0)
      *(int *)result = 0;
   else if (strcmp(value, "right") == 0)
      *(int *)result = 90;
   else if (strcmp(value, "left") == 0)
      *(int *)result = 270;
   else if (strcmp(value, "flip") == 0)
      *(int *)result = 180;
   else {
      cfg_error(cfg, "invalid value for option %s: %s", opt->name, value);
      return -1;
   }
   
   return 0;
}

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
      CFG_INT_CB(KEY_ROTATE, 0, CFGF_NONE, &cb_rotate),
      
      CFG_STR(KEY_SKIN_FILE, "", CFGF_NONE),
      CFG_INT(KEY_SNAPSHOT_DELAY, 500, CFGF_NONE),
      
      CFG_STR(KEY_MAME_PATH, "mame %r", CFGF_NONE),
      CFG_STR(KEY_MAME_SNAP_PATH, "", CFGF_NONE),
      
      CFG_INT(KEY_KEYCODE_EXIT, 27, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_UP, 273, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_DOWN, 274, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_PGUP, 276, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_PGDOWN, 275, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_RELOAD, 53, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_TOGGLE, 54, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_SELECT, 49, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_BACK, 50, CFGF_NONE),
      CFG_INT(KEY_KEYCODE_ALPHAMOD, 64, CFGF_NONE),
      CFG_END()
   };
   
   _cfg = cfg_init(opts, CFGF_NONE);
   
   // resolve config file
   string cfg_file("lemonlauncher.conf");
   resolve(cfg_file);
   
   int result = cfg_parse(_cfg, cfg_file.c_str());
   
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

void options::resolve(string& file) const
{
   // For now this just inserts the config dir blindly at the beginning of
   // the string.  A smarter version would check for existance of the file,
   // or try a relative path too.
   file.insert(0, 1, '/').insert(0, _conf_dir);
}
