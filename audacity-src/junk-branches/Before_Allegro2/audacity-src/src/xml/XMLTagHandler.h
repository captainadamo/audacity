/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLTagHandler.h

  Dominic Mazzoni

  This class is an interface which should be implemented by
  classes which wish to be able to load and save themselves
  using XML files.

**********************************************************************/
#ifndef __AUDACITY_XML_TAG_HANDLER__
#define __AUDACITY_XML_TAG_HANDLER__

#include <wx/string.h>
#include <stdio.h>

#include "XMLWriter.h"

class XMLTagHandler {
 public:
   XMLTagHandler(){};
   virtual ~XMLTagHandler(){};
   //
   // Methods to override
   //

   // This method will be called on your class if your class has
   // been registered to handle this particular tag.  Parse the
   // tag and the attribute-value pairs (null-terminated), and
   // return true on success, and false on failure.  If you return
   // false, you will not get any calls about children.
   virtual bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) = 0;

   // This method will be called when a closing tag is encountered.
   // It is optional to override this method.
   virtual void HandleXMLEndTag(const wxChar *tag) {}

   // If the XML document has children of your tag, this method
   // should be called.  Typically you should construct a new
   // object for the child, insert it into your own local data
   // structures, and then return it.  If you do not wish to
   // handle this child, return NULL and it will be ignored.
   virtual XMLTagHandler *HandleXMLChild(const wxChar *tag) = 0;

   // When this method is called, write your own tag and tags for
   // all of your children to the file.  One tag should appear
   // per line, and each tag should be preceded with [depth]
   // tab characters.
   virtual void WriteXML(XMLWriter &xmlFile) = 0;

   //
   // Utility methods you should call
   //

   // Escape a string, replacing certain characters with their
   // XML encoding, i.e. '<' becomes '&lt;'
   static wxString XMLEsc(wxString s);

   // These functions recieve data from expat.  They do charset
   // conversion and then pass the data to the handlers above.
   bool ReadXMLTag(const char *tag, const char **attrs);
   void ReadXMLEndTag(const char *tag);
   XMLTagHandler *ReadXMLChild(const char *tag);
};

#endif // define __AUDACITY_XML_TAG_HANDLER__

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 7c9a9baa-c546-42de-afaa-d87e5e13bf5a

