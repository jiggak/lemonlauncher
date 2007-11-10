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

#include "layout.h"
#include "options.h"
#include "log.h"
#include "error.h"
#include "default_font.h"

#include <SDL/SDL_rwops.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#include <cstring>
#include <fstream>

#define RGB(r,g,b) (((Uint32)b << 16) | ((Uint32)g << 8) | ((Uint32)r))
#define SDL_RGB(r,g,b) ((SDL_Color){r, g, b})
#define RGB_SDL_Color(rgb) SDL_RGB((rgb&0xff0000) >> 16, (rgb&0xff00) >> 8, rgb&0xff)

using namespace ll;
using namespace std;

const inline int min(int a, int b)
{ return a > b? b : a; }

int cb_dimension(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
   if (strcmp(value, "full") == 0)
      *(int *)result = DIMENSION_FULL;
   else
      *(int *)result = atoi(value);
   
   return 0;
}

int cb_justify(cfg_t *cfg, cfg_opt_t *opt, const char *value, void *result)
{
   if (strcmp(value, "left") == 0)
         *(justify_t *)result = left_justify;
      else if (strcmp(value, "right") == 0)
         *(justify_t *)result = right_justify;
      else if (strcmp(value, "center") == 0)
         *(justify_t *)result = center_justify;
      else {
         cfg_error(cfg, "invalid value for option %s: %s", opt->name, value);
         return -1;
      }
   
   return 0;
}

/** Validation callback for sections containing position and dimensions */
int cb_validate_pos_dims(cfg_t *cfg, cfg_opt_t *opt)
{
   if (cfg_size(opt->values[0]->section, "position") != 2) {
      cfg_error(cfg, "position must have two values in section '%s'", opt->name);
      return -1;
   }

   if (cfg_size(opt->values[0]->section, "dimensions") != 2) {
      cfg_error(cfg, "dimensions must have two values in section '%s'", opt->name);
      return -1;
   }
   
   return 0;
}

layout::layout(const char* skin, Uint16 width, Uint16 height) :
   _snap(NULL)
{
   _screen.x = 0;
   _screen.y = 0;
   _screen.w = width;
   _screen.h = height;
   
   cfg_opt_t title_opts[] = {
      CFG_INT_LIST("position", "{0,0}", CFGF_NONE),
      CFG_INT_LIST_CB("dimensions", "{full,56}", CFGF_NONE, &cb_dimension),
      CFG_INT("font_height", 40, CFGF_NONE),
      CFG_INT_CB("justify", center_justify, CFGF_NONE, &cb_justify),
      CFG_INT("color", 0xefefef, CFGF_NONE),
      CFG_END()
   };
   
   cfg_opt_t list_opts[] = {
      CFG_INT_LIST("position", "{0,56}", CFGF_NONE),
      CFG_INT_LIST_CB("dimensions", "{full,full}", CFGF_NONE, &cb_dimension),
      CFG_INT("font_height", 28, CFGF_NONE),
      CFG_INT("spacing", 4, CFGF_NONE),
      CFG_INT_CB("justify", center_justify, CFGF_NONE, &cb_justify),
      CFG_INT("color", 0xc2f4ff, CFGF_NONE),
      CFG_INT("hover_color", 0x32E4ff, CFGF_NONE),
      CFG_END()
   };
   
   cfg_opt_t snap_opts[] = {
      CFG_INT_LIST("position", "{0,56}", CFGF_NONE),
      CFG_INT_LIST_CB("dimensions", "{full,full}", CFGF_NONE, &cb_dimension),
      CFG_INT("alpha", 0xc8, CFGF_NONE),
      CFG_END()
   };
   
   cfg_opt_t opts[] = {
      CFG_STR("font", "", CFGF_NONE),
      CFG_STR("background", "", CFGF_NONE),
      CFG_SEC("title", title_opts, CFGF_NONE),
      CFG_SEC("list", list_opts, CFGF_NONE),
      CFG_SEC("snapshot", snap_opts, CFGF_NONE),
      CFG_END()
   };
   
   cfg_t* cfg = cfg_init(opts, CFGF_NONE);
   
   // set validate callback for position and dimensions
   cfg_set_validate_func(cfg, "title", &cb_validate_pos_dims);
   cfg_set_validate_func(cfg, "list", &cb_validate_pos_dims);
   cfg_set_validate_func(cfg, "snapshot", &cb_validate_pos_dims);
   
   int result = cfg_parse(cfg, skin);
   if (result == CFG_FILE_ERROR) {
      log << warn << "layout: file error, using defaults" << endl;
      cfg_parse_buf(cfg, "");
   } else if (result == CFG_PARSE_ERROR) {
      throw bad_lemon("layout: parse error");
   }
   
   const char* font_file = cfg_getstr(cfg, "font");
   const char* bg_image = cfg_getstr(cfg, "background");
   _bg = IMG_Load(bg_image);
   if (_bg == NULL)
      log << warn << "layout: background image not found" << endl;
   
   cfg_t* title = cfg_getsec(cfg, "title");
   
   _title_rect.x = cfg_getnint(title, "position", 0);
   _title_rect.y = cfg_getnint(title, "position", 1);
   parse_dimensions(&_title_rect, title);
   _title_font_height = cfg_getint(title, "font_height");
   _title_justify = (justify_t)cfg_getint(title, "justify");
   _title_color  = cfg_getint(title, "color");
   
   cfg_t* list = cfg_getsec(cfg, "list");
   
   _list_rect.x = cfg_getnint(list, "position", 0);
   _list_rect.y = cfg_getnint(list, "position", 1);
   parse_dimensions(&_list_rect, list);
   
   _list_font_height = cfg_getint(list, "font_height");
   _list_item_spacing = cfg_getint(list, "spacing");
   _list_justify = (justify_t)cfg_getint(list, "justify");
   
   _page_size = _list_rect.h / (_list_font_height + _list_item_spacing);
   
   _list_color = RGB_SDL_Color(cfg_getint(list, "color"));
   _list_hover_color = RGB_SDL_Color(cfg_getint(list, "hover_color"));
   
   cfg_t* snapshot = cfg_getsec(cfg, "snapshot");
   
   _snap_rect.x = cfg_getnint(snapshot, "position", 0);
   _snap_rect.y = cfg_getnint(snapshot, "position", 1);
   parse_dimensions(&_snap_rect, snapshot);
   
   _snap_alpha = cfg_getint(snapshot, "alpha");
   
   // try and open the font file
   fstream f(font_file, ios::in);
   if (f.is_open()) {
      f.close();
      
      log << debug << "layout: using font file " << font_file << endl;
      
      _title_font = TTF_OpenFont(font_file, _title_font_height);
      if (!_title_font) {
         log << error << TTF_GetError() << endl;
         throw bad_lemon("layout: unable to create title font");
      }
      
      _list_font  = TTF_OpenFont(font_file, _list_font_height);
      if (!_list_font) {
         log << error << TTF_GetError() << endl;
         throw bad_lemon("layout: unable to create list font");
      }
   } else {
      log << warn << "layout: font missing, using default" << endl;
      
      SDL_RWops* rw;
      
      rw = SDL_RWFromMem((void*)default_font, default_font_size);
      _title_font = TTF_OpenFontRW(rw, 0, _title_font_height);
      
      rw = SDL_RWFromMem((void*)default_font, default_font_size);
      _list_font = TTF_OpenFontRW(rw, 0, _list_font_height);
   }
   
   cfg_free(cfg);
}

layout::~layout()
{
   // free background image
   SDL_FreeSurface(_bg);
   
   // free fonts
   TTF_CloseFont(_title_font);
   TTF_CloseFont(_list_font);
   
   // free snapshot if there is one
   if (_snap)
      SDL_FreeSurface(_snap);
}

void layout::snap(SDL_Surface* snap)
{
   if (_snap)
      SDL_FreeSurface(_snap);
   
   _snap = snap;
}

void layout::parse_dimensions(SDL_Rect* rect, cfg_t* sec)
{
   int w = cfg_getnint(sec, "dimensions", 0);
   int h = cfg_getnint(sec, "dimensions", 1);
   
   rect->w = w != DIMENSION_FULL? w : _screen.w - rect->x;
   rect->h = h != DIMENSION_FULL? h : _screen.h - rect->y;
}

void layout::render_item(SDL_Surface* buffer, item* i, int yoff)
{
   SDL_Surface* surface = i->draw(_list_font, _list_color, _list_hover_color);
   
   SDL_Rect src, dest;

   src.x = 0; src.y = 0;
   src.w = min(surface->w, _list_rect.w);
   src.h = surface->h;
   
   if (_list_justify == left_justify)
      dest.x = _list_rect.x;
   else if (_list_justify == right_justify)
      dest.x = _list_rect.x + (_list_rect.w - src.w);
   else
      dest.x = _list_rect.x + ((_list_rect.w - src.w) / 2);
   
   dest.y = yoff;
   
   SDL_BlitSurface(surface, &src, buffer, &dest);
   SDL_FreeSurface(surface);
}

void layout::render(SDL_Surface* buffer, menu* current)
{
   // clear back buffer
   if (_bg == NULL)
      SDL_FillRect(buffer, &_screen, RGB(0,0,0));
   else
      SDL_BlitSurface(_bg, NULL, buffer, &_screen);

   // draw the games screen shot
   if (_snap) {
      float xscale = (float)_snap_rect.w / _snap->w;
      float yscale = (float)_snap_rect.h / _snap->h;

      // width aspect is larger than target, use 
      if (xscale > yscale) {
         xscale = yscale;
      } else if (yscale > xscale) {
         yscale = xscale;
      }

      // created scaled version of snapshot surface
      SDL_Surface* scaled = rotozoomSurfaceXY(_snap, 0.0, xscale, yscale, 0);
      
      // center the snapshot within the target rect
      SDL_Rect snap_rect;
      snap_rect.w = scaled->w;
      snap_rect.h = scaled->h;
      snap_rect.x = _snap_rect.x + (_snap_rect.w - snap_rect.w) / 2;
      snap_rect.y = _snap_rect.y + (_snap_rect.h - snap_rect.h) / 2;
      
      SDL_BlitSurface(scaled, NULL, buffer, &snap_rect);
      
      // rotozoomer surface has alpha channel, clear it to do per-surface alpha blit
      scaled->format->Amask = 0x00000000;
      
      // fill scaled surface with black and do alpha blit
      SDL_FillRect(scaled, NULL, RGB(0,0,0));
      SDL_SetAlpha(scaled, SDL_SRCALPHA, _snap_alpha);
      SDL_BlitSurface(scaled, NULL, buffer, &snap_rect);
      
      // free scaled surface
      SDL_FreeSurface(scaled);
   }

   SDL_Surface* title =
      TTF_RenderText_Blended(_title_font, current->text(), RGB_SDL_Color(_title_color));
   
   SDL_Rect title_rect = _title_rect;
   
   if (_title_justify == right_justify)
      title_rect.x += _title_rect.w - title->w;
   else if (_title_justify == center_justify)
      title_rect.x += (_title_rect.w - title->w) / 2;
   
   // draw title to back buffer
   SDL_BlitSurface(title, NULL, buffer, &title_rect);

   // finished with the title surface
   SDL_FreeSurface(title);
   
   // if the menu doesn't have any children, just return now
   if (!current->has_children())
      return;
   
   int yoff = _list_rect.y + ((_list_rect.h - _list_font_height) / 2);
   
   // draw the selected item in the middle of the list region
   render_item(buffer, current->selected(), yoff);

   // set absolute top/bottom of list area
   int top = _list_rect.y;
   int bottom = _list_rect.y + _list_rect.h;
   
   int yoff_above = yoff - _list_font_height - _list_item_spacing;
   int yoff_bellow = yoff + _list_font_height + _list_item_spacing;
   
   vector<item*>::iterator i = current->selected_begin();
   
   // draw items above the selected item
   if (i != current->first()) {
      do {
         --i;
         
         render_item(buffer, *i, yoff_above);
         yoff_above -= _list_font_height + _list_item_spacing;
      } while (i != current->first() && yoff_above > top);
   }
   
   // draw items bellow the selected item
   i = current->selected_begin();
   while (i+1 != current->last() && yoff_bellow < bottom) {
      i++;
      
      render_item(buffer, *i, yoff_bellow);
      
      yoff_bellow += _list_font_height + _list_item_spacing;
   }
}
