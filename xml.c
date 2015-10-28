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
#include "hbs.h"

/**
 * Some XML helper functions
 */

/**
 * Return XML node which has tag name
 * @param node - XML document
 * @param name - tag name to look for
 * @return xml node
 */
xmlNodePtr xmlGetNodeByName (xmlNodePtr node, const char *name)
{
  xmlNodePtr ptr;
  ptr = node->children;
  while (ptr && strcasecmp ((const char *) ptr->name, name))
    ptr = ptr->next;
  return ptr;
}

/**
 * Return XML node which has attribute with value
 * @param node - XML document
 * @param name - tag to look for
 * @param attr - attribute to look for
 * @param value = attribute value to look for
 * @return xml node
 */
xmlNodePtr
xmlGetNodeWithAttrValue (xmlNodePtr node, const char *name, const char *attr,
			 const char *value)
{
  xmlNodePtr ptr;
  ptr = node->children;
  while (ptr)
    {
      const char *val;
      if (!strcasecmp ((const char *) ptr->name, name))
	{
	  val = xmlGetNodeAttrValue (ptr, attr);
	  if (val && !strcasecmp (val, value))
	    return ptr;
	}
      ptr = ptr->next;
    }
  return NULL;
}

/**
 * Return XML node's named attribute's value
 * @param node - XML document
  * @param attr - attribute to look for
  * @return xml node
 */
const char *xmlGetNodeAttrValue (xmlNodePtr node, const char *attr)
{
  xmlAttrPtr ptr;
  if (node->properties)
    {
      ptr = node->properties;
      while (ptr)
	{
	  if (!strcasecmp ((const char *) ptr->name, attr))
	    {
	      return xmlAttrGetValue (ptr);
	    }
	  ptr = ptr->next;
	}
    }
  return NULL;
}

const char *xmlAttrGetValue (xmlAttrPtr ptr)
{
  if (ptr != NULL)
    {
      if (ptr->children != NULL)
	{
	  if (ptr->children->type == XML_TEXT_NODE)
	    {
	      return ptr->children->content;
	    }
	}
    }
  return NULL;
}

const char *xmlNodeGetValue (xmlNodePtr ptr)
{
  if (ptr != NULL)
    {
      if (ptr->children != NULL)
	{
	  if (ptr->children->type == XML_TEXT_NODE)
	    {
	      return ptr->children->content;
	    }
	}
    }
  return NULL;
}

char *xmlNodeGetLat1Value (xmlNodePtr ptr)
{
  const char *UTF8ptr;
  char *rv;
#if SIZEOF_SIZE_T == SIZEOF_UNSIGNED_INT
  size_t len, ilen;
#else
  unsigned int len, ilen;
#endif
  if ((UTF8ptr = xmlNodeGetValue (ptr)) == NULL)
    return NULL;
  len = xmlStrlen (UTF8ptr) + 1;
  rv = malloc (len);
  ilen = len;
  if (UTF8Toisolat1 (rv, &len, UTF8ptr, &ilen) < 0)
    {
      free (rv);
      return NULL;
    }
  return rv;
}
