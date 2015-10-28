/* (c) Aki Tuomi 2005

This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
/**
 * XML parser for config
 *
 */

#include "hbs.h"

// XML configuration document
xmlDocPtr config = 0;
// Config file.
char *config_file = 0;

/**
 * Read in XML configuration file
 * @param file - XML file to read
 * @return 0 on success, 1 on failure
 */
int config_read (const char *file)
{
  xmlDocPtr newconfig;
  newconfig = NULL;
  // if file has been given, free up old file and replace with new file
  if (file)
    {
      if (config_file)
	free (config_file);
      config_file = strdup (file);
    }
  // read XML configuration
  newconfig =
    xmlReadFile (config_file, NULL,
		 XML_PARSE_NOERROR | XML_PARSE_NONET | XML_PARSE_NOCDATA);
  // should it fail, tell about it.
  if (newconfig == NULL)
    {
      xmlErrorPtr error;
      error = xmlGetLastError ();
      if (error)
	{
	  char *msg, *ch;
	  msg = strdup (error->message);
	  if ((ch = strrchr (msg, '\n')))
	    *ch = '\0';
	  if ((ch = strrchr (msg, '\r')))
	    *ch = '\0';
	  print ("Config: Unable to parse %s: line %d: %s", config_file,
		 error->line, msg);
	  free (msg);
	}
      else
	{
	  print ("Config: Unable to parse %s", config_file);
	}
      return 1;
    }
  // release old config, put in new config.
  if (config)
    xmlFreeDoc (config);
  config = newconfig;
  return 0;
}

/**
 * Function for retrieving configuration variables.
 * @param sect - Config section
 * @param key - Config key
 * @return NULL if not found
 */
const char *config_getval (const char *sect, const char *key)
{
  xmlNodePtr ptr;
  ptr =
    xmlGetNodeWithAttrValue (xmlDocGetRootElement (config), "section", "name",
			     sect);
  if (ptr)
    ptr = xmlGetNodeWithAttrValue (ptr, "variable", "name", key);
  return (const char *) xmlNodeGetValue (ptr);
}

/**
 * Free up config
 */
void config_free (void)
{
  if (config)
    xmlFreeDoc (config);
  if (config_file)
    free (config_file);
}
