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
#include <config.h>
#include <string>
#include <stdlib.h>

#include "error.h"
#include "options.h"
#include "log.h"
#include "lemonmenu.h"
#include "lemonui.h"

using namespace ll;
using namespace std;

int main(int argc, char** argv)
{
#ifdef HAVE_CONF_DIR
   string dir(HAVE_CONF_DIR);
#else
   string dir(getenv("HOME"));
   dir.append("/.lemonlauncher");
#endif
   
   g_opts.load(dir.c_str());
   
   int level = g_opts.get_int(KEY_LOGLEVEL);
   log.level((log_level)level);
   log << info << "main: setting log level " << level << endl;
   log << info << "main: " << PACKAGE_STRING << endl;
   
   lemon_menu* menu = NULL;
   lemonui* ui = NULL;
   
   try {
      ui = new lemonui(g_opts.get_string(KEY_SKIN_FILE));
      ui->setup_screen();
      
      menu = new lemon_menu(ui);
      menu->main_loop();
   } catch (bad_lemon& e) {
      // error was already logged in bad_lemon constructor
      // TODO be a good boy damn it and handle your exceptions!
   }
   
   if (menu) delete menu;
   if (ui) delete ui;
   
   return 0;
}
