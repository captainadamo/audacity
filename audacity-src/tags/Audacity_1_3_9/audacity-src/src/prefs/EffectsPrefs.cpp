/**********************************************************************

  Audacity: A Digital Audio Editor

  EffectsPrefs.cpp

  Brian Gunlogson
  Joshua Haberman
  Dominic Mazzoni
  James Crook


*******************************************************************//**

\class EffectsPrefs
\brief A PrefsPanel for general GUI prefernces.

*//*******************************************************************/

#include "../Audacity.h"

#include <wx/defs.h>

#include "../AudacityApp.h"
#include "../Languages.h"
#include "../Prefs.h"
#include "../ShuttleGui.h"

#include "EffectsPrefs.h"

EffectsPrefs::EffectsPrefs(wxWindow * parent)
:  PrefsPanel(parent, _("Effects"))
{
   Populate();
}

EffectsPrefs::~EffectsPrefs()
{
}

void EffectsPrefs::Populate()
{
   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is 
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

void EffectsPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);

   S.StartStatic(_("Enable"));
   {
#if USE_LADSPA
      S.TieCheckBox(_("LADSPA effects"),
                    wxT("/Ladspa/Enable"),
                    true);
#endif

#if USE_NYQUIST
      S.TieCheckBox(_("Nyquist effects"),
                    wxT("/Nyquist/Enable"),
                    true);
#endif

#if USE_VAMP
      S.TieCheckBox(_("VAMP effects"),
                    wxT("/VAMP/Enable"),
                    true);
#endif
      S.AddFixedText(_("Restart Audacity to apply changes."));
   }
   S.EndStatic();

#if USE_AUDIO_UNITS
   S.StartStatic(_("Audio Unit Effects"));
   {
      S.TieCheckBox(_("Enable Audio Unit effects"),
                    wxT("/AudioUnits/Enable"),
                    true);
      S.TieCheckBox(_("Display Audio Unit effects in graphical mode"), 
                    wxT("/AudioUnits/GUI"),
                    true);
#if 0
      S.TieCheckBox(_("Rescan VST effects next time Audacity is started"), 
                    wxT("/VST/Rescan"),
                    false);
#endif
   }
   S.EndStatic();
#endif

#if USE_VST
   S.StartStatic(_("VST Effects"));
   {
      S.TieCheckBox(_("Enable VST effects"),
                    wxT("/VST/Enable"),
                    true);
      S.TieCheckBox(_("Display VST effects in graphical mode"), 
                    wxT("/VST/GUI"),
                    true);
      S.TieCheckBox(_("Rescan VST effects next time Audacity is started"), 
                    wxT("/VST/Rescan"),
                    false);
   }
   S.EndStatic();
#endif
}

bool EffectsPrefs::Apply()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   // If language has changed, we want to change it now, not on the next reboot.
   wxString lang = gPrefs->Read(wxT("/Locale/Language"), wxT(""));
   wxGetApp().InitLang(lang);

   return true;
}


// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 7e997d04-6b94-4abb-b3d6-748400f86598
