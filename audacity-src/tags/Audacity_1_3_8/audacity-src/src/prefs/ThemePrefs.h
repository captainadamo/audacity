/**********************************************************************

  Audacity: A Digital Audio Editor

  ThemePrefs.h

  James Crook

  Audacity is free software.
  This file is licensed under the wxWindows license, see License.txt

**********************************************************************/

#ifndef __AUDACITY_THEME_PREFS__
#define __AUDACITY_THEME_PREFS__

#include <wx/defs.h>
#include <wx/window.h>

#include "../ShuttleGui.h"

#include "PrefsPanel.h"

class ThemePrefs:public PrefsPanel
{
 public:
   ThemePrefs(wxWindow * parent);
   ~ThemePrefs(void);
   virtual bool Apply();

 private:
   void Populate();
   void PopulateOrExchange(ShuttleGui & S);
   void OnLoadThemeComponents(wxCommandEvent & e);
   void OnSaveThemeComponents(wxCommandEvent & e);
   void OnLoadThemeCache(wxCommandEvent & e);
   void OnSaveThemeCache(wxCommandEvent & e);
   void OnReadThemeInternal(wxCommandEvent & e);
   void OnSaveThemeAsCode(wxCommandEvent & e);

   DECLARE_EVENT_TABLE();
};

#endif

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: f09afeeb-9805-463a-b3ca-e3e3bfe05549
