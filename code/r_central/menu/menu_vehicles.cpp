/*
    MIT Licence
    Copyright (c) 2024 Petru Soroaga petrusoroaga@yahoo.com
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the organization nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.
        * Military use is not permited.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Julien Verneuil BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "menu.h"
#include "menu_vehicles.h"
#include "menu_confirmation.h"
#include "menu_vehicle_import.h"
#include "menu_vehicle_selector.h"

#include "../osd/osd_common.h"
#include "../../common/favorites.h"
//#include "../../base/radio_utils.h"

const char* s_textTitle[] = { "My Vehicles",  NULL };
const char* s_szVehicleNone = "No vehicles defined. To add a vehicle:\n Search for a vehicle and then connect to it as controller.";

MenuVehicles::MenuVehicles(void)
:Menu(MENU_ID_VEHICLES, "My Vehicles", NULL)
{
   m_Width = 0.33;
   m_xPos = menu_get_XStartPos(m_Width); m_yPos = 0.2;
   m_IndexSelectedVehicle = -1;

   log_line("[Menu] MenuVehicles: On open, this is the current radio interfaces configuration:");
   hardware_load_radio_info();

   m_bDisableStacking = true;
   m_bDisableBackgroundAlpha = true;
}

void MenuVehicles::onShow()
{
   m_Height = 0.0;
   
   if ( (NULL != g_pCurrentModel) && ( 0 != g_uActiveControllerModelVID) )
      log_line("[Menu] MenuVehicles: Current vehicle id: %u (%u)", g_pCurrentModel->vehicle_id, g_uActiveControllerModelVID);
   else
      log_line("[Menu] MenuVehicles: No current vehicle.");
   removeAllTopLines();
   removeAllItems();

   m_IndexSelectedVehicle = -1;

   addTopLine("Select the vehicle to control:");
   bool bCurrentVehicleFound = false;

   for( int i=0; i<getControllerModelsCount(); i++ )
   {
      Model *p = getModelAtIndex(i);
      log_line("[Menu] MenuVehicles: Iterating vehicles: id: %u", p->vehicle_id);
      char szBuff[256];
      if ( 1 == p->radioLinksParams.links_count )
         sprintf(szBuff, "%s, %s", p->getLongName(), str_format_frequency(p->radioLinksParams.link_frequency_khz[0]));
      else if ( 2 == p->radioLinksParams.links_count )
      {
         char szFreq1[64];
         char szFreq2[64];
         strcpy(szFreq1, str_format_frequency(p->radioLinksParams.link_frequency_khz[0]));
         strcpy(szFreq2, str_format_frequency(p->radioLinksParams.link_frequency_khz[1]));
         sprintf(szBuff, "%s, %s/%s", p->getLongName(), szFreq1, szFreq2);
      }
      else
      {
         char szFreq1[64];
         char szFreq2[64];
         char szFreq3[64];
         strcpy(szFreq1, str_format_frequency(p->radioLinksParams.link_frequency_khz[0]));
         strcpy(szFreq2, str_format_frequency(p->radioLinksParams.link_frequency_khz[1]));
         strcpy(szFreq3, str_format_frequency(p->radioLinksParams.link_frequency_khz[2]));
         sprintf(szBuff, "%s, %s/%s/%s", p->getLongName(), szFreq1, szFreq2, szFreq3);
      }

      MenuItemVehicle* pItem = new MenuItemVehicle(szBuff);
      pItem->setVehicleIndex(i, false);
      addMenuItem( pItem );
      if ( (NULL != g_pCurrentModel) && (!g_pCurrentModel->is_spectator) )
      if ( (g_uActiveControllerModelVID == p->vehicle_id) && (g_pCurrentModel->vehicle_id == p->vehicle_id) )
      {
         log_line("[Menu] MenuVehicles: Found current vehicle in the list.");
         bCurrentVehicleFound = true;
      }
   }

   if ( ! bCurrentVehicleFound )
      log_softerror_and_alarm("[Menu] MenuVehicles: Current vehicle not found in the vehicles list!");
   if ( 0 == getControllerModelsCount() )
   {
      removeAllTopLines();
      addTopLine(s_szVehicleNone);
   }

   addSeparator();
   m_IndexImport = addMenuItem(new MenuItem("Import Vehicle", "Imports a new vehicle from a model file on a USB stick."));

   Menu::onShow();
}


void MenuVehicles::onReturnFromChild(int iChildMenuId, int returnValue)
{
   Menu::onReturnFromChild(iChildMenuId, returnValue);
   invalidate();
}

void MenuVehicles::Render()
{
   RenderPrepare();
   float yTop = RenderFrameAndTitle();
   float y = yTop;

   float height_text = g_pRenderEngine->textHeight(g_idFontMenu);
   float dy = 0.2*height_text;
   float fFavoriteHeight = 1.2*g_pRenderEngine->textHeight(g_idFontMenu);
   float fFavoriteWidth = fFavoriteHeight / g_pRenderEngine->getAspectRatio();

   for( int i=0; i<m_ItemsCount; i++ )
   {
      float y0 = y;
      y += RenderItem(i,y);
      if ( i >= getControllerModelsCount() )
         continue;

      Model *pModel = getModelAtIndex(i);
      if ( NULL == pModel )
         continue;
      if ( vehicle_is_favorite(pModel->vehicle_id) )
      {
         if ( (NULL != g_pCurrentModel) && (pModel->vehicle_id == g_pCurrentModel->vehicle_id) )
            g_pRenderEngine->drawIcon(m_xPos + m_RenderWidth - m_sfMenuPaddingX - fFavoriteWidth, y0-dy+0.6*height_text, fFavoriteWidth, fFavoriteHeight, g_idIconFavorite);
         else
            g_pRenderEngine->drawIcon(m_xPos + m_RenderWidth - m_sfMenuPaddingX - fFavoriteWidth, y0-dy, fFavoriteWidth, fFavoriteHeight, g_idIconFavorite);
      }
   }
   RenderEnd(yTop);
}


void MenuVehicles::onSelectItem()
{
   if ( m_IndexImport == m_SelectedIndex )
   {
      if ( ! hardware_try_mount_usb() )
      {
         log_line("[Menu] MenuVehicles: No USB memory stick available.");
         Popup* p = new Popup("Please insert a USB memory stick to import the vehicle from.",0.28, 0.32, 0.32, 3);
         p->setCentered();
         p->setIconId(g_idIconInfo, get_Color_IconWarning());
         popups_add_topmost(p);
         return;
      }

      MenuVehicleImport* pM = new MenuVehicleImport();
      pM->setCreateNew();
      pM->buildSettingsFileList();
      if ( 0 == pM->getSettingsFilesCount() )
      {
         hardware_unmount_usb();
         ruby_signal_alive();
         sync();
         ruby_signal_alive();
         delete pM;
         Menu* pm = new Menu(MENU_ID_SIMPLE_MESSAGE+10*1000,"No vehicle settings files",NULL);
         pm->m_xPos = 0.4; pm->m_yPos = 0.4;
         pm->m_Width = 0.36;
         pm->addTopLine("There are no vehicle settings files on the USB stick.");
         add_menu_to_stack(pm);
         return;
      }
      add_menu_to_stack(pM);
      return;
   }

   if ( 0 == getControllerModelsCount() )
   {
      menu_stack_pop(0);
      return;
   }

   Model *pModel = getModelAtIndex(m_SelectedIndex);
   if ( NULL == pModel )
   {
      log_softerror_and_alarm("[Menu] MenuVehicles: NULL model for vehicle: %s.", m_pMenuItems[m_SelectedIndex]->getTitle());
      return;
   }

   m_IndexSelectedVehicle = m_SelectedIndex;
   log_line("[Menu] MenuVehicles: Adding menu selector for vehicle index %d", m_IndexSelectedVehicle);
   
   MenuVehicleSelector* pMenu = new MenuVehicleSelector();
   pMenu->m_IndexSelectedVehicle = m_SelectedIndex;
   pMenu->m_yPos = m_pMenuItems[m_SelectedIndex]->getItemRenderYPos() - g_pRenderEngine->textHeight(g_idFontMenu);
   add_menu_to_stack(pMenu);
}
