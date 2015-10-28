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

#define MODULE_LOAD 1
#define MODULE_UNLOAD 2

/**
 * <module>_info();
 */

// all ops are done post poned.
struct module_op_s
{
  int op;
  char *name;
  struct module_op_s *next;
};

struct module_op_s *module_op_start = 0;
struct module_op_s *module_op_stop = 0;

struct module_s
{
  const struct module_info_s *info;
  void *token;
};

typedef const struct module_info_s *(*mod_info_s_t) (void);

ARRAY modules = 0;

const char *modulepath;

void
module_command_load (NICK * nick, CHANNEL * channel, const char *cmd,
		     const char **args, int argc)
{
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s module\r\n", nick->nick, cmd);
      return;
    }
  if (module_load (args[0]))
    puttext ("NOTICE %s :Failed to load module %s\r\n", nick->nick, args[0]);
  else
    puttext ("NOTICE %s :Module %s load queued\r\n", nick->nick, args[0]);
}

void
module_command_unload (NICK * nick, CHANNEL * channel, const char *cmd,
		       const char **args, int argc)
{
  if (argc < 1)
    {
      puttext ("NOTICE %s :Usage: %s module\r\n", nick->nick, cmd);
      return;
    }
  if (module_unload (args[0]))
    puttext ("NOTICE %s :Failed to unload module %s\r\n", nick->nick,
	     args[0]);
  else
    puttext ("NOTICE %s :Module %s unload queued\r\n", nick->nick, args[0]);
}

int module_comparator (const void *key, const void *elem)
{
  const struct module_s *module;
  const char *modname;
  modname = (const char *) key;
  module = (const struct module_s *) elem;
  return strcasecmp (modname, module->info->modname);
}

/**
 * Sets up module system
 * @return 1 fail, 0 ok
 */
int module_setup (void)
{
  if ((modulepath = config_getval ("global", "modulepath")) == NULL)
    {
      print ("module_setup(): modulepath missing");
      return 1;
    }

  if ((modules == NULL)
      && !(modules =
	   array_create (sizeof (struct module_s), module_comparator, NULL)))
    {
      print ("module: Fatal error: Failed to allocate space for modules");
      return 1;
    }

  command_register ("load", 0, 1, 0, 1, module_command_load);
  command_register ("unload", 0, 1, 0, 1, module_command_unload);
  return 0;
}

/**
 * Inserts module into running system
 * @param name - Module name
 * @return 1 fail, 0 ok
 */
int _module_load (const char *name)
{
  char *modfile;
  char *modsym;
  struct module_s *modptr;
  struct module_s module;
  mod_info_s_t info_fptr;
  // check if module already loaded
  if (array_find (modules, name))
    {
      print ("module: Module %s already loaded", name);
      return 1;
    }
  // set up file name and path
  modfile = malloc (strlen (modulepath) + strlen (name) + 5);
  snprintf (modfile, strlen (modulepath) + strlen (name) + 5, "%s/%s.so",
	    modulepath, name);
  // open module
  module.token = dlopen (modfile, RTLD_NOW);
  free (modfile);
  // check for error.
  if (module.token == NULL)
    {
      print ("module: Failed to open %s: %s", name, dlerror ());
      return 1;
    }
  // try to locate module_info function
  modsym = malloc (strlen (name) + 6);
  snprintf (modsym, strlen (name) + 6, "%s_info", name);
  // if this fails, bail out
  if ((info_fptr = (mod_info_s_t) dlsym (module.token, modsym)) == NULL)
    {
      char *err;
      if ((err = dlerror ()))
	{
	  print ("module: Error resolving symbol %s from %s: %s", modsym,
		 name, err);
	}
      else
	{
	  print ("module: Module %s does not have symbol %s - unable to load",
		 name, modsym);
	}
      free (modsym);
      dlclose (module.token);
      return 1;
    }
  free (modsym);
  // call this function
  module.info = (*info_fptr) ();
  // if the module wants to do something after being loaded
  // tell it to do it now. if it returns something else than 0
  // unload it.
  if (module.info->load)
    {
      if ((*module.info->load) ())
	{
	  print ("module: Module %s did not want to load",
		 module.info->modname);
	  dlclose (module.token);
	  return 1;
	}
    }
  // add loaded module to array and inform.
  modptr = array_add (modules, module.info->modname);
  memcpy (modptr, &module, sizeof (module));
  print ("module: Loaded module %s", modptr->info->modname);
  return 0;
}

/**
 * Remove module from running system
 * @param name - name of the module
 * @return 1 fail, 0 ok
 */
int _module_unload (const char *name)
{
  struct module_s *module;
  void *token;
  if (!(module = array_find (modules, name)))
    return 1;
  // if the module has unload function, use it.
  if (module->info->unload)
    if ((*module->info->unload) ())
      {
	// should _never_ happen without a blody good reason.
	print ("module: Module %s did not want to unload",
	       module->info->modname);
	return 1;
      }
  // close and delete module
  token = module->token;
  array_delete (modules, name);
  dlclose (token);
  print ("module: Module %s unloaded", name);
  return 0;
}

/**
 * Load all modules specified by module file
 * @param file - module file
 * @return 1 fail, 0 ok
 */
int modules_load (const char *file)
{
  char buf[512];
  char *cp, *cp2;
  int err;
  FILE *fin;
  /* Perhaps no modules are to be loaded. This is not really an error... */
  if (!(fin = fopen (file, "r")))
    return 0;
  err = 0;
  while ((fgets (buf, sizeof buf, fin)))
    {
      cp = buf;
      while ((*cp) && (isspace (*cp)))
	cp++;
      // fix the module name, remove trailing whitespace
      if ((*cp == '\0') || (*cp == '#'))
	continue;
      if ((cp2 = strchr (cp, ' ')))
	*cp2 = '\0';
      if ((cp2 = strchr (cp, '\r')))
	*cp2 = '\0';
      if ((cp2 = strchr (cp, '\n')))
	*cp2 = '\0';
      // try to load it
      if (_module_load (cp))
	{
	  err = 1;
	  break;
	}
    }
  if (err)
    {
      print ("module: Failed to load initial command set");
    }
  fclose (fin);
  return err;
}

/**
 * Remove all modules
 */
void modules_unload (void)
{
  struct module_s *module;
  size_t i;
  for (i = 0; i < array_count (modules); i++)
    {
      module = (struct module_s *) array_get_index (modules, i);
      if (module->info->unload)
	(*module->info->unload) ();
      dlclose (module->token);
    }
  array_destroy (modules, NULL);
  modules = NULL;
}

/**
 * Return list of modules
 * @return array of module info pointers
 */
const struct module_info_s **module_list (void)
{
  size_t i;
  const struct module_info_s **rv;
  rv =
    malloc (sizeof (struct module_info_s **) * (array_count (modules) + 1));
  memset (rv, 0,
	  sizeof (struct module_info_s **) * (array_count (modules) + 1));
  for (i = 0; i < array_count (modules); i++)
    rv[i] = ((struct module_s *) array_get_index (modules, i))->info;
  return rv;
}

void module_queue_op (const char *name, int op)
{
  struct module_op_s *new;
  new = malloc (sizeof (struct module_op_s));
  new->name = strdup (name);
  new->op = op;
  new->next = NULL;
  if (module_op_start == NULL)
    module_op_start = new;
  if (module_op_stop != NULL)
    module_op_stop->next = new;
  module_op_stop = new;
}

void module_run_ops (void)
{
  struct module_op_s *op;
  while ((op = module_op_start) != NULL)
    {
      module_op_start = module_op_start->next;
      switch (op->op)
	{
	case MODULE_LOAD:
	  _module_load (op->name);
	  break;
	case MODULE_UNLOAD:
	  _module_unload (op->name);
	}
      free (op->name);
      free (op);
    }
  module_op_start = module_op_stop = NULL;
}

int module_load (const char *name)
{
  char modpath[4096];
  if (array_find (modules, name) != NULL)
    {
      print ("Module %s is already loaded", name);
      return 1;
    }
  /* perhaps we could check that the module is loadable? */
  snprintf (modpath, sizeof modpath, "%s/%s.so", modulepath, name);
  if (access (modpath, R_OK | X_OK))
    {
      print ("module: Cannot access file (need read-exec) %s", modpath);
      return 1;
    }
  module_queue_op (name, MODULE_LOAD);
  return 0;
}

int module_unload (const char *name)
{
  if (array_find (modules, name) == NULL)
    {
      print ("Module %s is not loaded", name);
      return 1;
    }
  module_queue_op (name, MODULE_UNLOAD);
  return 0;
}
