/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <file/file_path.h>

#include "../menu.h"
#include "../menu_cbs.h"
#include "../menu_hash.h"
#include "../menu_input.h"
#include "../menu_setting.h"
#include "../menu_shader.h"
#include "../menu_navigation.h"

#include "../../general.h"
#include "../../retroarch.h"

#ifdef HAVE_SHADER_MANAGER
static int generic_shader_action_parameter_left(
      struct video_shader *shader, struct video_shader_parameter *param,
      unsigned type, const char *label, bool wraparound)
{
   if (shader)
   {
      param->current -= param->step;
      param->current = min(max(param->minimum, param->current), param->maximum);
   }
   return 0;
}

static int shader_action_parameter_left(unsigned type, const char *label,
      bool wraparound)
{
   struct video_shader          *shader = video_shader_driver_get_current_shader();
   struct video_shader_parameter *param = &shader->parameters[type - MENU_SETTINGS_SHADER_PARAMETER_0];
   return generic_shader_action_parameter_left(shader, param, type, label, wraparound);
}

static int shader_action_parameter_preset_left(unsigned type, const char *label,
      bool wraparound)
{
   menu_handle_t                  *menu = menu_driver_get_ptr();
   struct video_shader          *shader = menu ? menu->shader : NULL;
   struct video_shader_parameter *param = &shader->parameters[type - MENU_SETTINGS_SHADER_PRESET_PARAMETER_0];
   return generic_shader_action_parameter_left(shader, param, type, label, wraparound);
}
#endif

static int action_left_cheat(unsigned type, const char *label,
      bool wraparound)
{
   size_t idx             = type - MENU_SETTINGS_CHEAT_BEGIN;
   return generic_action_cheat_toggle(idx, type, label,
         wraparound);
}

static int action_left_input_desc(unsigned type, const char *label,
      bool wraparound)
{
   unsigned inp_desc_index_offset = type - MENU_SETTINGS_INPUT_DESC_BEGIN;
   unsigned inp_desc_user         = inp_desc_index_offset / (RARCH_FIRST_CUSTOM_BIND + 4);
   unsigned inp_desc_button_index_offset = inp_desc_index_offset - (inp_desc_user * (RARCH_FIRST_CUSTOM_BIND + 4));
   settings_t *settings = config_get_ptr();

   if (settings->input.remap_ids[inp_desc_user][inp_desc_button_index_offset] > 0)
      settings->input.remap_ids[inp_desc_user][inp_desc_button_index_offset]--;

   return 0;
}

static int action_left_save_state(unsigned type, const char *label,
      bool wraparound)
{
   event_command(EVENT_CMD_SAVE_STATE_DECREMENT);
   return 0;
}

static int action_left_scroll(unsigned type, const char *label,
      bool wraparound)
{
   size_t selection;
   unsigned scroll_speed = 0, fast_scroll_speed = 0;
   menu_navigation_t *nav = menu_navigation_get_ptr();
   menu_list_t *menu_list = menu_list_get_ptr();
   if (!nav || !menu_list)
      return -1;

   if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
      return -1;

   scroll_speed      = (max(nav->scroll.acceleration, 2) - 2) / 4 + 1;
   fast_scroll_speed = 4 + 4 * scroll_speed;

   if (selection > fast_scroll_speed)
      menu_navigation_set(nav, selection - fast_scroll_speed, true);
   else
   {
      bool pending_push = false;
      menu_navigation_ctl(MENU_NAVIGATION_CTL_CLEAR, &pending_push);
   }

   return 0;
}

static int action_left_mainmenu(unsigned type, const char *label,
      bool wraparound)
{
   menu_file_list_cbs_t *cbs = NULL;
   unsigned        push_list = 0;
   menu_list_t    *menu_list = menu_list_get_ptr();
   menu_handle_t       *menu = menu_driver_get_ptr();
   menu_navigation_t   *nav  = menu_navigation_get_ptr();
   unsigned           action = MENU_ACTION_LEFT;
   size_t          list_size = menu_driver_list_get_size(MENU_LIST_PLAIN);
   if (!menu)
      return -1;

   if (list_size == 1)
   {
      nav->selection_ptr = 0;
      if (menu_driver_list_get_selection() != 0)
         push_list = 1;
   }
   else
      push_list = 2;

   cbs = menu_list_get_actiondata_at_offset(menu_list->selection_buf,
         nav->selection_ptr);

   switch (push_list)
   {
      case 1:
         menu_driver_list_cache(MENU_LIST_HORIZONTAL, action);

         if (cbs && cbs->action_content_list_switch)
            return cbs->action_content_list_switch(
                  menu_list->selection_buf, menu_list->menu_stack,
                  "", "", 0);
         break;
      case 2:
         action_left_scroll(0, "", false);
         break;
      case 0:
      default:
         break;
   }

   return 0;
}

static int action_left_shader_scale_pass(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   unsigned pass = type - MENU_SETTINGS_SHADER_PASS_SCALE_0;
   struct video_shader *shader = NULL;
   struct video_shader_pass *shader_pass = NULL;
   menu_handle_t *menu = menu_driver_get_ptr();
   if (!menu)
      return -1;

   shader = menu->shader;
   if (!shader)
      return -1;
   shader_pass = &shader->pass[pass];
   if (!shader_pass)
      return -1;

   {
      unsigned current_scale   = shader_pass->fbo.scale_x;
      unsigned delta           = 5;
      current_scale            = (current_scale + delta) % 6;

      shader_pass->fbo.valid   = current_scale;
      shader_pass->fbo.scale_x = shader_pass->fbo.scale_y = current_scale;

   }
#endif
   return 0;
}

static int action_left_shader_filter_pass(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   unsigned delta = 2;
   unsigned pass = type - MENU_SETTINGS_SHADER_PASS_FILTER_0;
   struct video_shader *shader = NULL;
   struct video_shader_pass *shader_pass = NULL;
   menu_handle_t *menu = menu_driver_get_ptr();
   if (!menu)
      return -1;

   shader = menu->shader;
   if (!shader)
      return -1;
   shader_pass = &shader->pass[pass];
   if (!shader_pass)
      return -1;

   shader_pass->filter = ((shader_pass->filter + delta) % 3);
#endif
   return 0;
}

static int action_left_shader_filter_default(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   rarch_setting_t *setting = menu_setting_find(menu_hash_to_str(MENU_LABEL_VIDEO_SMOOTH));
   if (!setting)
      return -1;
   return menu_action_handle_setting(setting,
         setting->type, MENU_ACTION_LEFT, wraparound);
#else
   return 0;
#endif
}

static int action_left_cheat_num_passes(unsigned type, const char *label,
      bool wraparound)
{
   unsigned new_size = 0;
   global_t *global       = global_get_ptr();
   cheat_manager_t *cheat = global->cheat;

   if (!cheat)
      return -1;

   if (cheat_manager_get_size(cheat))
      new_size = cheat_manager_get_size(cheat) - 1;
   menu_entries_set_refresh(false);
   cheat_manager_realloc(cheat, new_size);

   return 0;
}

static int action_left_shader_num_passes(unsigned type, const char *label,
      bool wraparound)
{
#ifdef HAVE_SHADER_MANAGER
   struct video_shader *shader = NULL;
   menu_handle_t *menu = menu_driver_get_ptr();
   if (!menu)
      return -1;

   shader = menu->shader;
   if (!shader)
      return -1;

   if (shader->passes)
      shader->passes--;
   menu_entries_set_refresh(false);
   video_shader_resolve_parameters(NULL, menu->shader);

#endif
   return 0;
}

static int action_left_video_resolution(unsigned type, const char *label,
      bool wraparound)
{
   global_t *global = global_get_ptr();

   (void)global;

#if defined(__CELLOS_LV2__)
   if (global->console.screen.resolutions.current.idx)
   {
      global->console.screen.resolutions.current.idx--;
      global->console.screen.resolutions.current.id =
         global->console.screen.resolutions.list
         [global->console.screen.resolutions.current.idx];
   }
#else
   video_driver_get_video_output_prev();
#endif

   return 0;
}

static int core_setting_left(unsigned type, const char *label,
      bool wraparound)
{
   unsigned idx     = type - MENU_SETTINGS_CORE_OPTION_START;
   rarch_system_info_t *system = rarch_system_info_get_ptr();

   (void)label;

   core_option_prev(system->core_options, idx);

   return 0;
}

static int disk_options_disk_idx_left(unsigned type, const char *label,
      bool wraparound)
{
   event_command(EVENT_CMD_DISK_PREV);
   return 0;
}

static int bind_left_generic(unsigned type, const char *label,
      bool wraparound)
{
   return menu_setting_set(type, label, MENU_ACTION_LEFT, wraparound);
}

static int menu_cbs_init_bind_left_compare_label(menu_file_list_cbs_t *cbs,
      const char *label, uint32_t label_hash, uint32_t menu_label_hash, const char *elem0)
{
   unsigned i;

   if (cbs->setting)
   {
      uint32_t parent_group_hash = menu_hash_calculate(cbs->setting->parent_group);

      if ((parent_group_hash == MENU_VALUE_MAIN_MENU) && (cbs->setting->type == ST_GROUP))
      {
         cbs->action_left = action_left_scroll;
         return 0;
      }
   }

   for (i = 0; i < MAX_USERS; i++)
   {
      uint32_t label_setting_hash;
      char label_setting[PATH_MAX_LENGTH];

      label_setting[0] = '\0';
      snprintf(label_setting, sizeof(label_setting), "input_player%d_joypad_index", i + 1);

      label_setting_hash = menu_hash_calculate(label_setting);

      if (label_hash != label_setting_hash)
         continue;

      cbs->action_left = bind_left_generic;
      return 0;
   }

   if (strstr(label, "rdb_entry"))
      cbs->action_left = action_left_scroll;
   else
   {
      switch (label_hash)
      {
         case MENU_LABEL_SAVESTATE:
         case MENU_LABEL_LOADSTATE:
            cbs->action_left = action_left_save_state;
            break;
         case MENU_LABEL_VIDEO_SHADER_SCALE_PASS:
            cbs->action_left = action_left_shader_scale_pass;
            break;
         case MENU_LABEL_VIDEO_SHADER_FILTER_PASS:
            cbs->action_left = action_left_shader_filter_pass;
            break;
         case MENU_LABEL_VIDEO_SHADER_DEFAULT_FILTER:
            cbs->action_left = action_left_shader_filter_default;
            break;
         case MENU_LABEL_VIDEO_SHADER_NUM_PASSES:
            cbs->action_left = action_left_shader_num_passes;
            break;
         case MENU_LABEL_CHEAT_NUM_PASSES:
            cbs->action_left = action_left_cheat_num_passes;
            break;
         case MENU_LABEL_SCREEN_RESOLUTION: 
            cbs->action_left = action_left_video_resolution;
            break;
         case MENU_LABEL_NO_PLAYLIST_ENTRIES_AVAILABLE:
            switch (menu_label_hash)
            {
               case MENU_VALUE_HORIZONTAL_MENU:
               case MENU_VALUE_MAIN_MENU:
                  cbs->action_left = action_left_mainmenu;
                  break;
            }
         default:
            return -1;
      }
   }

   return 0;
}

static int menu_cbs_init_bind_left_compare_type(menu_file_list_cbs_t *cbs,
      unsigned type, uint32_t label_hash, uint32_t menu_label_hash)
{
   if (type >= MENU_SETTINGS_CHEAT_BEGIN
         && type <= MENU_SETTINGS_CHEAT_END)
      cbs->action_left = action_left_cheat;
#ifdef HAVE_SHADER_MANAGER
   else if (type >= MENU_SETTINGS_SHADER_PARAMETER_0
         && type <= MENU_SETTINGS_SHADER_PARAMETER_LAST)
      cbs->action_left = shader_action_parameter_left;
   else if (type >= MENU_SETTINGS_SHADER_PRESET_PARAMETER_0
         && type <= MENU_SETTINGS_SHADER_PRESET_PARAMETER_LAST)
      cbs->action_left = shader_action_parameter_preset_left;
#endif
   else if (type >= MENU_SETTINGS_INPUT_DESC_BEGIN
         && type <= MENU_SETTINGS_INPUT_DESC_END)
      cbs->action_left = action_left_input_desc;
   else if ((type >= MENU_SETTINGS_CORE_OPTION_START))
      cbs->action_left = core_setting_left;
   else
   {
      switch (type)
      {
         case MENU_SETTINGS_CORE_DISK_OPTIONS_DISK_INDEX:
            cbs->action_left = disk_options_disk_idx_left;
            break;
         case MENU_FILE_PLAIN:
         case MENU_FILE_DIRECTORY:
         case MENU_FILE_CARCHIVE:
         case MENU_FILE_IN_CARCHIVE:
         case MENU_FILE_CORE:
         case MENU_FILE_RDB:
         case MENU_FILE_RDB_ENTRY:
         case MENU_FILE_RPL_ENTRY:
         case MENU_FILE_CURSOR:
         case MENU_FILE_SHADER:
         case MENU_FILE_SHADER_PRESET:
         case MENU_FILE_IMAGE:
         case MENU_FILE_OVERLAY:
         case MENU_FILE_VIDEOFILTER:
         case MENU_FILE_AUDIOFILTER:
         case MENU_FILE_CONFIG:
         case MENU_FILE_USE_DIRECTORY:
         case MENU_FILE_PLAYLIST_ENTRY:
         case MENU_FILE_DOWNLOAD_CORE:
         case MENU_FILE_CHEAT:
         case MENU_FILE_REMAP:
         case MENU_FILE_MOVIE:
         case MENU_FILE_MUSIC:
         case MENU_FILE_IMAGEVIEWER:
         case MENU_FILE_PLAYLIST_COLLECTION:
         case MENU_FILE_DOWNLOAD_CORE_CONTENT:
         case MENU_FILE_SCAN_DIRECTORY:
         case MENU_SETTING_GROUP:
            switch (menu_label_hash)
            {
               case MENU_VALUE_HORIZONTAL_MENU:
               case MENU_VALUE_MAIN_MENU:
                  cbs->action_left = action_left_mainmenu;
                  break;
               default:
                  cbs->action_left = action_left_scroll;
                  break;
            }
            break;
         case MENU_SETTING_ACTION:
         case MENU_FILE_CONTENTLIST_ENTRY:
            cbs->action_left = action_left_mainmenu;
            break;
         default:
            return -1;
      }
   }

   return 0;
}

int menu_cbs_init_bind_left(menu_file_list_cbs_t *cbs,
      const char *path, const char *label, unsigned type, size_t idx,
      const char *elem0, const char *elem1, const char *menu_label,
      uint32_t label_hash, uint32_t menu_label_hash)
{
   if (!cbs)
      return -1;

   cbs->action_left = bind_left_generic;

   if (menu_cbs_init_bind_left_compare_label(cbs, label, label_hash, menu_label_hash, elem0) == 0)
      return 0;

   if (menu_cbs_init_bind_left_compare_type(cbs, type, label_hash, menu_label_hash) == 0)
      return 0;

   return -1;
}
