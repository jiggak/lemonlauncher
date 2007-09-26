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
#include "lemonmenu.h"

#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#include <confuse.h>
#include <sstream>

#define UPDATE_SNAP_EVENT 1
#define RGB(r,g,b) (((Uint32)b << 16) | ((Uint32)g << 8) | ((Uint32)r))
#define RGBA(r,g,b,a) (((Uint32)a << 32) | ((Uint32)b << 16) | ((Uint32)g << 8) | ((Uint32)r))
#define SDL_RGB(r,g,b) ((SDL_Color){r, g, b})

using namespace ll;
using namespace std;

/**
 * Timer callback function
 */
Uint32 snap_timer_callback(Uint32 interval, void *param);

lemon_menu::lemon_menu(SDL_Surface* screen, options* opts) :
   _screen(screen), _opts(opts), _page_size(_opts->get_int(KEY_PAGE_SIZE)),
   _show_hidden(false), _snap(NULL), _snap_timer(0)
{
   load_menus();

   const char* font_path = _opts->get_string(KEY_FONT_FILE);
   int title_height = _opts->get_int(KEY_TITLE_HEIGHT);
   int list_height = _opts->get_int(KEY_LIST_HEIGHT);
   
   log << debug << "lemon_menu: using font file " << font_path << endl;
   
   _title_font = TTF_OpenFont(font_path, title_height);
   if (!_title_font) {
      log << error << TTF_GetError() << endl;
      throw bad_lemon("lemon_menu: unable to create title font");
   }
   
   _list_font  = TTF_OpenFont(font_path, list_height);
   if (!_list_font) {
      log << error << TTF_GetError() << endl;
      throw bad_lemon("lemon_menu: unable to create list font");
   }

   /*
    * Should I be using hardware accelerating?  Most docs/guides suggest no..
    * I pass 0 as the alpha mask.  Surfaces don't need an alpha channel to
    * do per-surface alpha and blitting.  In fact I can't seem to get the
    * fadded snapshot blitting to work at all if the alpha channel is set!
    */
   _buffer = SDL_CreateRGBSurface(SDL_SWSURFACE,
      _screen->w, _screen->h, 32, // w,h,bpp
      0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000); // rgba masks, for big-endian
   
   if (!_buffer)
      throw bad_lemon("lemon_menu: unable to create back buffer");

   log << info << "lemon_menu: double buffer created" << endl;
}

lemon_menu::~lemon_menu()
{
   delete _top; // delete top menu will propigate to children
   
   SDL_FreeSurface(_buffer); // free back buffer

   // free fonts
   TTF_CloseFont(_title_font);
   TTF_CloseFont(_list_font);
}

void lemon_menu::load_menus()
{
   cfg_opt_t game_opts[] = {
      CFG_STR("rom", 0, CFGF_NODEFAULT),
      CFG_STR("title", 0, CFGF_NODEFAULT),
      CFG_STR("params", "", CFGF_NONE),
      CFG_END()
   };
   
   cfg_opt_t menu_opts[] = {
      CFG_SEC("game", game_opts, CFGF_MULTI),
      CFG_END()
   };
   
   cfg_opt_t root_opts[] = {
      CFG_SEC("menu", menu_opts, CFGF_TITLE | CFGF_MULTI),
      CFG_END()
   };
   
   cfg_opt_t opts[] = {
      CFG_SEC("root", root_opts, CFGF_TITLE),
      CFG_END()
   };
   
   cfg_t* cfg = cfg_init(root_opts, CFGF_NONE);
   if (cfg_parse(cfg, "games.conf") != CFG_SUCCESS)
      throw bad_lemon("load_menus: file parse error");

   // only one root supported for now, should be straight forward to support more
   //cfg_t* root = cfg_getsec(cfg, "root");
   
   //_top = new menu(cfg_title(root));
   _top = new menu("Arcade Games");
   
   // iterate over menu sections   
   int menu_cnt = cfg_size(cfg, "menu");
   for (int i=0; i<menu_cnt; i++) {
      cfg_t* m = cfg_getnsec(cfg, "menu", i);
      
      const char* mtitle = cfg_title(m);
      
      // "should" be safe to assume title is at least one charcter long 
      if (mtitle[0] != '.' || _show_hidden) {

         // create menu and add to root menu
         menu* pmenu = new menu(mtitle);
         _top->add_child(pmenu);
         
         // iterate over the game sections
         int game_cnt = cfg_size(m, "game");
         for (int j=0; j<game_cnt; j++) {
            cfg_t* g = cfg_getnsec(m, "game", j);
            
            char* rom = cfg_getstr(g, "rom");
            char* title = cfg_getstr(g, "title");
            char* params = cfg_getstr(g, "params");
            
            if (title[0] != '.' || _show_hidden)
               pmenu->add_child(new game(rom, title, params));
         }
      }
   }
   
   _current = _top;
   
   cfg_free(cfg);
}

void lemon_menu::render()
{
   log << debug << "render: menu " << _current->text() << endl;
   
   // clear back buffer
   SDL_Rect screen_rect = { 0, 0, _screen->w, _screen->h };
   SDL_FillRect(_buffer, &screen_rect, RGB(0,0,0));

   // draw the games screen shot
   if (_snap) {
      float xscale = (float)(_screen->w - 80) / _snap->w;
      float yscale = (float)(_screen->h - 80) / _snap->h;

      SDL_Rect snap_rect;

      if (xscale > yscale) {
         xscale = yscale;
         
         snap_rect.x = ((_screen->w - 80) / 2) - (int)((float)_snap->w * xscale / 2.0) + 40;
         snap_rect.y = 60;
      } else if (yscale > xscale) {
         yscale = xscale;

         snap_rect.x = 40;
         snap_rect.y = ((_screen->h - 80) / 2) - (int)((float)_snap->h * yscale / 2.0) + 60;
      } else {
         snap_rect.x = 40;
         snap_rect.y = 60;
      }

      // created scaled version of snapshot surface
      SDL_Surface* scaled = rotozoomSurfaceXY(_snap, 0.0, xscale, yscale, 0);
      snap_rect.w = scaled->w;  snap_rect.h = scaled->h;
      
      SDL_BlitSurface(scaled, NULL, _buffer, &snap_rect);
      
      // rotozoomer surface has alpha channel, clear it to do per-surface alpha blit
      scaled->format->Amask = 0x00000000;
      
      // fill scaled surface with black and do alpha blit
      SDL_FillRect(scaled, NULL, RGB(0,0,0));
      SDL_SetAlpha(scaled, SDL_SRCALPHA, 200);
      SDL_BlitSurface(scaled, NULL, _buffer, &snap_rect);
      
      // free scaled surface
      SDL_FreeSurface(scaled);
   }

   SDL_Color title_color = {0xEF, 0xEF, 0xEF};
   SDL_Surface* title =
      TTF_RenderText_Blended(_title_font, _current->text(), title_color);
   SDL_Rect title_rect =
      { (_screen->w / 2) - (title->w / 2), 8, title->w, title->h };

   // draw title to back buffer
   SDL_BlitSurface(title, NULL, _buffer, &title_rect);
   
   SDL_Rect underline =
      { title_rect.x - 10, title_rect.h + 10, title_rect.w + 10, 3};
   
   // draw line under the title
   SDL_FillRect(_buffer, &underline, RGB(0xEF, 0xEF, 0xEF));

   // finished with the title surface
   SDL_FreeSurface(title);
   
   SDL_Surface* item_surface;
   SDL_Rect item_rect;
   
   // get the selected item to a surface
   item_surface = _current->selected()->draw(_list_font);
   
   // draw the selected item in the middle of the screen
   item_rect.x = _screen->w / 2 - item_surface->w / 2;
   item_rect.y = _screen->h / 2 - item_surface->h / 2;
   item_rect.w = item_surface->w;
   item_rect.h = item_surface->h;
   
   SDL_BlitSurface(item_surface, NULL, _buffer, &item_rect);

   // free the surface for the selected item
   SDL_FreeSurface(item_surface);

   // set absolute top/bottom of list area
   int top = title_rect.y + title_rect.h + 16;
   int bottom = _screen->h;
   
   int yoff_above = item_rect.y - item_rect.h - 5;
   int yoff_bellow = item_rect.y + item_rect.h + 5;
   vector<item*>::iterator i = _current->selected_begin();
   
   // draw items above the selected item
   if (i != _current->first()) {
      do {
         --i;
         
         item_surface = (*i)->draw(_list_font);
         
         item_rect.x = _screen->w / 2 - item_surface->w / 2;
         item_rect.y = yoff_above;
         item_rect.w = item_surface->w;
         item_rect.h = item_surface->h;
         
         SDL_BlitSurface(item_surface, NULL, _buffer, &item_rect);
         SDL_FreeSurface(item_surface);
         
         yoff_above -= item_rect.h + 5;
      } while (i != _current->first() && yoff_above > top);
   }
   
   // draw items bellow the selected item
   i = _current->selected_begin();
   while (i+1 != _current->last() && yoff_bellow < bottom) {
      i++;
      
      item_surface = (*i)->draw(_list_font);
      
      item_rect.x = _screen->w / 2 - item_surface->w / 2;
      item_rect.y = yoff_bellow;
      item_rect.w = item_surface->w;
      item_rect.h = item_surface->h;
      
      SDL_BlitSurface(item_surface, NULL, _buffer, &item_rect);
      SDL_FreeSurface(item_surface);
      
      yoff_bellow += item_rect.h + 5;
   }

   // update the screen
   if (SDL_BlitSurface(_buffer, NULL, _screen, NULL))
      throw bad_lemon("render: double buffer blit failed");

   SDL_UpdateRect(_screen, 0, 0, 0, 0);
}

void lemon_menu::main_loop()
{
   log << info << "main_loop: starting render loop" << endl;

   render();
   reset_snap_timer();

   const int p1up_key = _opts->get_int(KEY_KEYCODE_P1_UP);
   const int p1down_key = _opts->get_int(KEY_KEYCODE_P1_DOWN);
   const int p1pgup_key = _opts->get_int(KEY_KEYCODE_P1_PGUP);
   const int p1pgdown_key = _opts->get_int(KEY_KEYCODE_P1_PGDOWN);
   const int p2up_key = _opts->get_int(KEY_KEYCODE_P2_UP);
   const int p2down_key = _opts->get_int(KEY_KEYCODE_P2_DOWN);
   const int p2pgup_key = _opts->get_int(KEY_KEYCODE_P2_PGUP);
   const int p2pgdown_key = _opts->get_int(KEY_KEYCODE_P2_PGDOWN);
   const int exit_key = _opts->get_int(KEY_KEYCODE_EXIT);
   const int p1b1_key = _opts->get_int(KEY_KEYCODE_P1_BTN1);
   const int p1b2_key = _opts->get_int(KEY_KEYCODE_P1_BTN2);
   const int p2b1_key = _opts->get_int(KEY_KEYCODE_P2_BTN1);
   const int p2b2_key = _opts->get_int(KEY_KEYCODE_P2_BTN2);
   const int snap_key = _opts->get_int(KEY_KEYCODE_SNAP);
   const int reload_key = _opts->get_int(KEY_KEYCODE_RELOAD);
   const int toggle_key = _opts->get_int(KEY_KEYCODE_TOGGLE);

   _running = true;
   while (_running) {
      SDL_Event event;
      SDL_WaitEvent(&event);

      switch (event.type) {
		   case SDL_QUIT: {
		      _running = false;
		      break;
         } case SDL_KEYDOWN: {
	         int key = event.key.keysym.sym;

	         if (key == exit_key)
	            _running = false;
	         else if (key == p1up_key || key == p2up_key)
	            handle_up();
	         else if (key == p1down_key || key == p2down_key)
	            handle_down();
	         else if (key == p1pgup_key || key == p2pgup_key)
	            handle_pgup();
	         else if (key == p1pgdown_key || key == p2pgdown_key)
	            handle_pgdown();
	         else if (key == p1b1_key || key == p2b1_key)
	            handle_activate();
	         else if (key == p1b2_key || key == p2b2_key)
	            handle_up_menu();
	         else if (key == snap_key) 
	            handle_snap();
	         else if (key == reload_key)
	         {
	            load_menus();
	            render();
	         }
	         else if (key == toggle_key)
	            handle_show_hide();

	         break;
	      } case SDL_USEREVENT: {
	         if (event.user.code == UPDATE_SNAP_EVENT)
	            update_snap();
	         break;
         }
      }
   }
   
   reset_snap_timer();
}

void lemon_menu::handle_up()
{
   // ignore event if already at the top of menu
   if (_current->select_previous()) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_down()
{
   // ignore event if already at the bottom of menu
   if (_current->select_next()) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_pgup()
{
   // ignore event if already at the top of menu
   if (_current->select_previous(_page_size)) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_pgdown()
{
   // ignore event if already at the bottom of menu
   if (_current->select_next(_page_size)) {
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_snap()
{
}

void lemon_menu::handle_activate()
{
   item* item = _current->selected();
   if (typeid(menu) == typeid(*item)) {
      handle_down_menu();
   } else if (typeid(game) == typeid(*item)) {
      handle_run();
   }
}

void lemon_menu::handle_run()
{
   game* g = (game*)_current->selected();

   bool full = _opts->get_bool(KEY_FULLSCREEN);
   const char* mame_path = _opts->get_string(KEY_MAME_PATH);
   const char* rom_path = _opts->get_string(KEY_MAME_ROM_PATH);
   const char* mame_params = _opts->get_string(KEY_MAME_PARAMS);
	
   log << info << "handle_run: launching game " << g->text() << endl;
   
	if (full) SDL_WM_ToggleFullScreen(_screen);

   stringstream cmd;
   cmd << mame_path << ' ' << rom_path << g->rom() << ' ' << g->params() << ' ' << mame_params;

   log << debug << "handle_run: " << cmd.str() << endl;

   system(cmd.str().c_str());

	if (full) SDL_WM_ToggleFullScreen(_screen);

   render();

	// dump the event queue
	SDL_Delay(1500);
	SDL_PumpEvents();

	SDL_Event evt;
	while (SDL_PeepEvents(&evt, 1, SDL_GETEVENT, 0xFFFFFFFF) > 0)
		SDL_PumpEvents();
}

void lemon_menu::handle_up_menu()
{
   if (_current != _top) {
      _current = (menu*)_current->parent();
      reset_snap_timer();
      render();
   }
}

void lemon_menu::handle_down_menu()
{
   _current = (menu*)_current->selected();
   reset_snap_timer();
   render();
}

void lemon_menu::handle_show_hide()
{
   log << info << "handle_show_hide: changing hidden status" << endl;
   
   _show_hidden = !_show_hidden;
   load_menus();
   render();
}

void lemon_menu::update_snap()
{
   item* item = _current->selected();
   
   if (typeid(game) == typeid(*item)) {
      get_game_snap();
   }
   
   /* Roland's origional version would iterate through all games in the menu and
    * look for four game snapshots to be placed in a 2x2 grid.  Would be cool if
    * this was not just limited to a 2x2 grid but the performance would be horrible!
    * So, this is skipped until something better can be conjured up.
   if (typeid(menu) == typeid(*item)) {
      get_menu_snap();
   }*/
}

void lemon_menu::get_game_snap()
{
   game* g = (game*)_current->selected();

   stringstream img;
   img << _opts->get_string(KEY_MAME_SNAPS_PATH) << '/' << g->rom() << "/0000.png";
   
   log << debug << "get_game_snap: " << img.str() << endl;

	_snap = IMG_Load(img.str().c_str());
	if (_snap) render();
}

void lemon_menu::get_menu_snap()
{
   
}

void lemon_menu::reset_snap_timer()
{
   if (_snap_timer)
      SDL_RemoveTimer(_snap_timer);

   if (_snap) {
      SDL_FreeSurface(_snap);
      _snap = NULL;
   }

   // schedule timer to run in 500 milliseconds
   _snap_timer = SDL_AddTimer(500, snap_timer_callback, NULL);
}

SDL_Surface* menu::draw(TTF_Font* font) const
{
   SDL_Color c = parent() && this == ((menu*)parent())->selected()?
         SDL_RGB(0x32, 0xE4, 0xFF) : SDL_RGB(0xC2, 0xF4, 0xFF);
   return TTF_RenderText_Blended(font, text(), c);
}

SDL_Surface* game::draw(TTF_Font* font) const
{
   SDL_Color c = this == ((menu*)parent())->selected()?
         SDL_RGB(0xFF, 0xE4, 0x32) : SDL_RGB(0xEF, 0xEF, 0xEF);
   return TTF_RenderText_Blended(font, text(), c);
}

Uint32 snap_timer_callback(Uint32 interval, void *param)
{
   SDL_Event evt;
   evt.type = SDL_USEREVENT;
   evt.user.code = UPDATE_SNAP_EVENT;
   
   SDL_PushEvent(&evt);

   return 0;
}
