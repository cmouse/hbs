#include "hbs.h"

ARRAY bots=0;

int botnet_cmp (const void *a, const void *b)
{
  struct botnet_bot_s *bot;
  bot = (struct botnet_bot_s *) b;
  return strcasecmp ((const char *) a, bot->name);
}

void botnet_eraser (void *elem, void *opaque)
{
  struct botnet_bot_s *bot;
  bot = (struct botnet_bot_s *) elem;
  free (bot->name);
}

void botnet_init (void)
{
  bots = array_create (sizeof (struct botnet_bot_s),
		       botnet_cmp, botnet_eraser);
}

int botnet_has_bot (const char *name)
{
  if (array_find (bots, name) != NULL)
    return 1;
  return 0;
}

int botnet_add_bot (const char *name)
{
  struct botnet_bot_s *bot;
  bot = array_add (bots, name);
  if (bot->name != NULL)
    return 1;
  bot->name = strdup (name);
  bot->channels = 0;
  return 0;
}

void botnet_set_channels (const char *name, int count)
{
  struct botnet_bot_s *bot;
  if ((bot = array_find (bots, name)) == NULL)
    return;
  bot->channels = count;
}

void botnet_remove_bot (const char *name)
{
  array_delete (bots, name);
}

void botnet_walker (array_walker_t walker, void *opaque)
{
  array_walk (bots, walker, opaque);
}
