#include "hbs.h"

//
// this works almost like the timer.c.
// WE MUST NOT LOSE CALLBACK DURING CALLBACK
// THAT BE VERY DANGEROUS. 
//

// retval nonzero removes, param int fd

struct xpoll_callback_s
{
  int fd;
  xpoll_callback_t read;
  xpoll_callback_t write;
  xpoll_callback_t close;
} *xpolls = 0;

size_t nxpolls = 0;

void
xpoll_register_fd (int fd, xpoll_callback_t read, xpoll_callback_t write,
		   xpoll_callback_t close)
{
  struct xpoll_callback_s *new;
  if ((read == NULL) && (write == NULL) && (close == NULL))
    return;
  xpolls = realloc (xpolls, sizeof (struct xpoll_callback_s) * (nxpolls + 1));
  new = xpolls + nxpolls;
  nxpolls++;
  new->fd = fd;
  new->read = read;
  new->write = write;
  new->close = close;
}

void xpoll_unregister_fd (int fd)
{
  size_t i;
  for (i = 0; i < nxpolls; i++)
    {
      if (xpolls[i].fd == fd)
	{
	  if (i < nxpolls + 1)
	    memmove (xpolls + i, xpolls + i + 1, nxpolls - i - 1);
	  nxpolls--;
	  i--;
	}
    }
  if (nxpolls)
    xpolls = realloc (xpolls, sizeof (struct xpoll_callback_s) * nxpolls);
  else
    {
      free (xpolls);
      xpolls = NULL;
    }
}

// drops any callback with func
void xpoll_unregister_cb (xpoll_callback_t func)
{
  size_t i;
  for (i = 0; i < nxpolls; i++)
    {
      if ((xpolls[i].read == func) ||
	  (xpolls[i].write == func) || (xpolls[i].close == func))
	{
	  if (i < nxpolls + 1)
	    memmove (xpolls + i, xpolls + i + 1, nxpolls - i - 1);
	  nxpolls--;
	  i--;
	}
    }
  if (nxpolls)
    xpolls = realloc (xpolls, sizeof (struct xpoll_callback_s) * nxpolls);
  else
    {
      free (xpolls);
      xpolls = NULL;
    }
}

void xpoll (void)
{
  struct xpoll_callback_s *tmp;
  struct pollfd *polls;
  size_t npoll;
  size_t i;
  if (nxpolls == 0)
    return;
  npoll = nxpolls;
  tmp = malloc (sizeof (struct xpoll_callback_s) * nxpolls);
  memcpy (tmp, xpolls, sizeof (struct xpoll_callback_s) * nxpolls);
  polls = malloc (sizeof (struct pollfd) * nxpolls);
  for (i = 0; i < npoll; i++)
    {
      polls[i].fd = tmp[i].fd;
      polls[i].events = POLLIN | POLLOUT;
      polls[i].revents = 0;
    }
  if (poll (polls, npoll, 1000))
    {
      for (i = 0; i < npoll; i++)
	{
	  if (polls[i].revents & POLLIN)
	    {
	      if (tmp[i].read != NULL)
		if ((*tmp[i].read) (tmp[i].fd) != 0)
		  xpoll_unregister_fd (tmp[i].fd);
	    }
	  if (polls[i].revents & POLLOUT)
	    {
	      if (tmp[i].write != NULL)
		if ((*tmp[i].write) (tmp[i].fd) != 0)
		  xpoll_unregister_fd (tmp[i].fd);
	    }
	  if (polls[i].revents & (POLLNVAL | POLLHUP | POLLERR))
	    {
	      if (tmp[i].close != NULL)
		(*tmp[i].close) (tmp[i].fd);
	      xpoll_unregister_fd (tmp[i].fd);
	    }
	}
    }
  free (polls);
  free (tmp);
}
