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

#include "lemonui.h"
#include "options.h"
#include "log.h"
#include "error.h"
#include "default_font.h"

#include <sys/stat.h>
#include <SDL/SDL_rwops.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#include <cstring>

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

lemonui::lemonui(const char* theme_file):
   _bg(NULL), _snap(NULL), _buffer(NULL), _screen(NULL),
   _title_font(NULL), _list_font(NULL)
{
   _rotate = g_opts.get_int(KEY_ROTATE);
   _scrnw = g_opts.get_int(KEY_SCREEN_WIDTH);
   _scrnh = g_opts.get_int(KEY_SCREEN_HEIGHT);
   
   /*
    * When rotation is requested we swap the width/height for the drawing
    * buffer and simply draw as if it was oriented the same as the screen
    * resolution.  Then after drawing is finished, the drawing buffer is
    * rotated before it is blitted to the screen.
    */
   if (_rotate == 90 || _rotate == 270) {
      _buffw = _scrnh;
      _buffh = _scrnw;
   } else {
      _buffw = _scrnw;
      _buffh = _scrnh;
   }
      
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
      CFG_INT("alpha", 0x96, CFGF_NONE),
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
   
   // parse theme file with libconfuse
   int result = cfg_parse(cfg, theme_file);
   if (result == CFG_FILE_ERROR) {
      log << warn << "layout: file error, using defaults" << endl;
      cfg_parse_buf(cfg, "");
   } else if (result == CFG_PARSE_ERROR) {
      throw bad_lemon("layout: parse error");
   } else {
      // extract directory path from the path of the them file and
      // only do so when cfg_parse returned success (file found/parsed)
      
      _theme_dir.assign(theme_file);
      _theme_dir.erase(_theme_dir.rfind('/') + 1);
   }
   
   string font, background;
   
   normalize(cfg_getstr(cfg, "font"), font);
   normalize(cfg_getstr(cfg, "background"), background);
   
   const char* font_file = font.c_str();
   
   _bg = IMG_Load(background.c_str());
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
   
   // init the font engine
   if (TTF_Init())
      throw bad_lemon("layout: unable to start font engine");
   
   struct stat fstat;
   if (stat(font_file, &fstat) == 0) {
      log << debug << "layout: using font file " << font_file << endl;
      
      _title_font = TTF_OpenFont(font_file, _title_font_height);
      if (!_title_font) {
         // title/list font are same file, so only check for error once
         
         log << error << TTF_GetError() << endl;
         TTF_Quit();
         
         throw bad_lemon("layout: unable to create font");
      }
      
      _list_font = TTF_OpenFont(font_file, _list_font_height);
   } else {
      log << warn << "layout: \"" << font_file << "\" not found" << endl;
      log << warn << "layout: using default font" << endl;
      
      SDL_RWops* rw;
      
      rw = SDL_RWFromMem((void*)default_font, default_font_size);
      _title_font = TTF_OpenFontRW(rw, 0, _title_font_height);
      
      rw = SDL_RWFromMem((void*)default_font, default_font_size);
      _list_font = TTF_OpenFontRW(rw, 0, _list_font_height);
   }
   
   cfg_free(cfg);
}

lemonui::~lemonui()
{
   if (_bg) // free background image
      SDL_FreeSurface(_bg);
   
   if (_buffer) // free rendering buffer
      SDL_FreeSurface(_buffer);
   
   if (_title_font && _list_font) { // free fonts
      TTF_CloseFont(_title_font);
      TTF_CloseFont(_list_font);
   }
   
   if (_snap)  // free snapshot if there is one
      SDL_FreeSurface(_snap);
   
   TTF_Quit(); // shutdown ttf
   SDL_Quit(); // shutdown sdl
}

void lemonui::setup_screen() throw(bad_lemon&)
{
   // initialize sdl
   SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER);
           
   // hide mouse cursor
   SDL_ShowCursor(SDL_DISABLE);
  
   // enable key-repeat, use defaults delay and interval for now
   SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
        
   int bits = g_opts.get_int(KEY_SCREEN_BPP);
   bool full = g_opts.get_bool(KEY_FULLSCREEN);
   
   log << info << "layout: using graphics mode: " <<
         _scrnw <<'x'<< _scrnh <<'x'<< bits << endl;
   
   _screen = SDL_SetVideoMode(_scrnw, _scrnh, bits, SDL_SWSURFACE |
         (full ? SDL_FULLSCREEN : 0));
   
   if (!_screen)
      throw bad_lemon("layou: unable to open screen");
   
   /*
    * Should I be using hardware surface?  Most docs/guides suggest no..
    * I pass 0 as the alpha mask.  Surfaces don't need an alpha channel to
    * do per-surface alpha and blitting.  In fact I can't seem to get the
    * fadded snapshot blitting to work at all if the alpha channel is set!
    */
   _buffer = SDL_CreateRGBSurface(SDL_SWSURFACE,
      _buffw, _buffh, 32, // w,h,bpp
      0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000); // rgba masks, for big-endian

   if (!_buffer)
      throw bad_lemon("layout: unable to create drawing buffer");
   
}

void lemonui::snap(SDL_Surface* snap)
{
   if (_snap)
      SDL_FreeSurface(_snap);
   
   _snap = snap;
}

void lemonui::parse_dimensions(SDL_Rect* rect, cfg_t* sec)
{
   int w = cfg_getnint(sec, "dimensions", 0);
   int h = cfg_getnint(sec, "dimensions", 1);
   
   rect->w = w != DIMENSION_FULL? w : _buffw - rect->x;
   rect->h = h != DIMENSION_FULL? h : _buffh - rect->y;
}

void lemonui::normalize(const char* path, string& new_path)
{
   if (strlen(path) > 0 && path[0] != '/') {
      new_path.assign(_theme_dir);
      new_path.append(path);
   } else {
      new_path.assign(path);
   }
}

void lemonui::render_item(SDL_Surface* buffer, item* i, int yoff)
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

void lemonui::render(menu* current)
{
   // clear back buffer
   if (_bg == NULL)
      SDL_FillRect(_buffer, NULL, RGB(0,0,0));
   else
      SDL_BlitSurface(_bg, NULL, _buffer, NULL);

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
      
      SDL_BlitSurface(scaled, NULL, _buffer, &snap_rect);
      
      // rotozoomer surface has alpha channel, clear it to do per-surface alpha blit
      scaled->format->Amask = 0x00000000;
      
      // fill scaled surface with black and do alpha blit
      SDL_FillRect(scaled, NULL, RGB(0,0,0));
      SDL_SetAlpha(scaled, SDL_SRCALPHA, _snap_alpha);
      SDL_BlitSurface(scaled, NULL, _buffer, &snap_rect);
      
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
   SDL_BlitSurface(title, NULL, _buffer, &title_rect);

   // finished with the title surface
   SDL_FreeSurface(title);
   
   // only render list of children, if there is any
   if (current->has_children()) {
      int yoff = _list_rect.y + ((_list_rect.h - _list_font_height) / 2);
      
      // draw the selected item in the middle of the list region
      render_item(_buffer, current->selected(), yoff);
   
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
            
            render_item(_buffer, *i, yoff_above);
            yoff_above -= _list_font_height + _list_item_spacing;
         } while (i != current->first() && yoff_above > top);
      }
      
      // draw items bellow the selected item
      i = current->selected_begin();
      while (i+1 != current->last() && yoff_bellow + _list_font_height < bottom) {
         i++;
         
         render_item(_buffer, *i, yoff_bellow);
         
         yoff_bellow += _list_font_height + _list_item_spacing;
      }
   }
   
   if (_rotate != 0) {
      SDL_Surface* tmp = rotozoomSurface(_buffer, _rotate, 1, 0);
      SDL_BlitSurface(tmp, NULL, _screen, NULL);
      SDL_UpdateRect(_screen, 0, 0, 0, 0);
      SDL_FreeSurface(tmp);
   } else {
      SDL_BlitSurface(_buffer, NULL, _screen, NULL);
      SDL_UpdateRect(_screen, 0, 0, 0, 0);
   }
}
