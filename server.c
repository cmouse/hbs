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
 * What we need for server? 
 */

struct server_common_s server_common;

int server_wait = 0;            /* Seconds to wait before continue */

/* Server list , current server and start point */
struct server_s *server_current = 0;
struct server_s *server_start = 0;

/* Automatical server setup */
void server_setup(void)
{
  size_t servers, i;
  char key[9];
  /* Get nicks */
  server_common.nnicks = s_atoi(config_getval("server_common", "nicks"));
  if (server_common.nnicks)
    {
      server_common.nicks = malloc(sizeof(char *) * server_common.nnicks);
      for (i = 0; i < server_common.nnicks; i++)
        {
          snprintf(key, 7, "nick%zu", i);
          server_common.nicks[i] =
            strdup(config_getval("server_common", key));
        }
    }
  /* Get rest of common IRC */
  server_common.ident = strdup(config_getval("server_common", "user"));
  server_common.realname = strdup(config_getval("server_common", "realname"));
  servers = s_atoi(config_getval("server_common", "servers"));
  /* Get servers */
  for (i = 0; i < servers; i++)
    {
      snprintf(key, 9, "server%zu", i);
      server_add(config_getval(key, "server"),
                 config_getval(key, "port"),
                 config_getval(key, "pass"),
                 config_getval(key, "bind"),
                 s_atoi(config_getval(key, "msec")),
                 s_atoi(config_getval(key, "lines")));
    }
  server_setup_handlers();
}

/**
 * Adds a server to list 
 * @param host hostname
 * @param port port number
 * @param pass password (optional)
 * @param bind bind address (optional, ipv4)
 * @msec number of milliseconds to wait until reducing ob value
 * @lines number of lines to transfer at a time
 */
void
server_add(const char *host, const char *port, const char *pass,
           const char *bind, int msec, int lines)
{
  struct server_s *new, *ptr;
  /* Allocate new server */
  new = malloc(sizeof(struct server_s));
  memset(new, 0, sizeof(struct server_s));
  /* Setup values */
  new->host = strdup(host);
  new->port = strdup(port);
  if (pass)
    new->pass = strdup(pass);
  else
    new->pass = NULL;
  if (bind)
    {
      // parse bind addr
      if (!inet_aton(bind, &new->bindaddr.sin_addr))
        {
          memset(&new->bindaddr.sin_addr, 0, sizeof(new->bindaddr.sin_addr));
        }
    }
  else
    {
      memset(&new->bindaddr.sin_addr, 0, sizeof(new->bindaddr.sin_addr));
    }
  new->ob.msec = msec;
  new->ob.mlines = lines;
  new->next = server_start;
  new->fd = -1;
  /* Update list */
  if ((ptr = server_start) == NULL)
    {
      server_start = new;
      new->next = new;
    }
  else
    {
      while (ptr->next != server_start)
        ptr = ptr->next;
      ptr->next = new;
    }
}

/**
 * Moves to next server.
 */
void server_next(void)
{
  if (server_current == NULL)
    server_current = server_start;
  else
    server_current = server_current->next;
}

/**
 * Gets the tcp protocol number.
 * @return protocol number
 */
int server_proto(void)
{
  return tcp_protocol_number;
}

/**
 * Unlocks and deletes .oidentd.conf file from user's homedir
 */
void server_release_oidentd(void)
{
  struct passwd *pwent;
  size_t len;
  char *fbuf;
  /* Check that we have a valid pw entry */
  if ((pwent = getpwuid(geteuid())) == NULL)
    return;
  /* User homedir */
  len = strlen(pwent->pw_dir) + strlen(OIDENTD_FILE) + 2;
  fbuf = malloc(len);
  /* Write a path for the file */
  snprintf(fbuf, len, "%s/%s", pwent->pw_dir, OIDENTD_FILE);
  /* Unlock & close if open */
  if (server_current->ident)
    {
      flock(fileno(server_current->ident), LOCK_UN);
      fclose(server_current->ident);
      server_current->ident = NULL;
    }
  /* Destroy file */
  unlink(fbuf);
  free(fbuf);
  endpwent();
}

/**
 * Do the actual disconnection from server.
 * Called by server_poll() and server_handle_read().
 */
void _server_disconnect(void)
{
  socklen_t vlen;
  int val;
  const char *args[3];
  /* No double disconnects */
  server_current->tx = server_current->rx = 0;
  if (server_current->status == server_disconnected)
    return;
  /* Log disconnection */
  val = 0;
  getsockopt(server_current->fd, SOL_SOCKET, SO_ERROR, &val, &vlen);
  if (val)
    print("Lost connection to %s:%s (%s)", server_current->host,
          server_current->port, strerror(val));
  else
    print("Lost connection to %s:%s", server_current->host,
          server_current->port);
  /* Close the file descriptor */
  close(server_current->fd);
  /* Clean up messages */
  server_queue_clean();
  memset(server_current->buffer, 0, sizeof(server_current->buffer));

  /* Reset status */
  if (server_current->name)
    free(server_current->name);
  if (server_current->cnick)
    free(server_current->cnick);
  server_current->disconnect = 0;
  server_current->name = server_current->cnick = NULL;
  server_current->status = server_disconnected;
  server_next();
  /* If wait is not set, set wait now. This is for throttle.. */
  if (server_wait <= TIME)
    server_wait = TIME + 15;
  server_current->fd = -1;
  server_release_oidentd();
  /* Inform everyone about disconnection */
  args[0] = server_get_name();
  args[1] = "DISCONNECTED";
  args[2] = NULL;
  server_fire_event(server_current, "DISCONNECTED", args, 2);
}

/**
 * Frontend for disconnect.
 * If connected sends QUIT message and waits until server disconnects us.
 */
void server_disconnect(void)
{
  if ((server_current == NULL) || (server_current->status < server_ok))
    {
      if (server_current)
        {
          _server_disconnect();
        }
      return;
    }
  putfast("QUIT :Disconnect\r\n");
  while (server_isconnected())
    server_poll();
}

/**
 * Setup .oidentd.conf file for ident spoofing
 */
int server_oidentd(void)
{
  struct passwd *pwent;
  struct stat statent;
  size_t len;
  char *fbuf;
  /* Lookup homedir and setup file path */
  if ((pwent = getpwuid(geteuid())) == NULL)
    return 0;
  len = strlen(pwent->pw_dir) + strlen(OIDENTD_FILE) + 2;
  fbuf = malloc(len);
  snprintf(fbuf, len, "%s/%s", pwent->pw_dir, OIDENTD_FILE);
  if (!stat(fbuf, &statent) && (server_current->timeout < TIME))
    {
      print("server_oidentd(): timeout while waiting for file to unlock");
      return 1;
    }
  else if (server_current->timeout < TIME)
    {
      return 2;
    }
  /* Try to open */
  server_current->ident = fopen(fbuf, "w");
  free(fbuf);
  endpwent();
  /* No can do, it didn't open up. */
  if (server_current->ident == NULL)
    return 1;
  /* Lock, write, flush */
  flock(fileno(server_current->ident), LOCK_EX);
  fprintf(server_current->ident, "global { reply \"%s\" }\n",
          server_common.ident);
  fflush(server_current->ident);
  return 0;
}

/**
 * Handles connecting to server 
 */
void server_connect(void)
{
  int val;
  socklen_t len;
  /* create socket */
  if (server_current->status == server_disconnected)
    {
      server_current->timeout = TIME + 60;
      if (server_current->curaddr)
        {
          if ((server_current->curaddr =
               server_current->curaddr->ai_next) == NULL)
            server_current->curaddr = server_current->addrinfo;
        }
      else
        server_current->curaddr = server_current->addrinfo;
      print("Connecting to %s:%s",
            inet_ntoa(((struct sockaddr_in *)server_current->curaddr->
                       ai_addr)->sin_addr), server_current->port);
      server_current->fd =
        socket(server_current->curaddr->ai_family,
               server_current->curaddr->ai_socktype,
               server_current->curaddr->ai_protocol);
      if (server_current->fd == -1)
        {
          /* Boo it failed */
          print("server: socket() failed: %s", strerror(errno));
          if (server_wait < TIME)
            server_wait = TIME + 15;
          return;
        }
      val = 1;
      len = sizeof(val);
      /* Set reuseaddr on */
      setsockopt(server_current->fd, SOL_SOCKET, SO_REUSEADDR, &val, len);
      val = fcntl(server_current->fd, F_GETFL);
      fcntl(server_current->fd, F_SETFL, val | O_NONBLOCK);
      server_current->status = server_oident;
    }
  switch (server_oidentd())
    {
    case 2:
      return;
    case 1:
      if (server_wait < TIME)
        server_wait = TIME + 15;
      server_disconnect();
      return;
    }
  // perform bind()
  server_current->bindaddr.sin_family = AF_INET;
  server_current->bindaddr.sin_port = ntohs(0);
  if (bind
      (server_current->fd, &server_current->bindaddr,
       sizeof(server_current->bindaddr)))
    {
      print("Failed to bind %s: %s",
            inet_ntoa(server_current->bindaddr.sin_addr), strerror(errno));
      server_current->disconnect = 1;
      if (server_wait < TIME)
        server_wait = TIME + 15;
      return;
    }
  if (connect(server_current->fd, server_current->curaddr->ai_addr,
              server_current->curaddr->ai_addrlen))
    {
      if (errno != EINPROGRESS)
        {
          print("Failed to connect %s:%s - %s", server_current->host,
                server_current->port, strerror(errno));
          server_current->disconnect = 1;
          if (server_wait < TIME)
            server_wait = TIME + 15;
          return;
        }
      server_current->status = server_preconnect;
    }
  fcntl(server_current->fd, F_SETFL, val & (~O_NONBLOCK));
  server_current->timeout = TIME + 30;
}

/**
 * Checks if we can send. Might reset ob counters.
 * @return non-zero if cleared to send
 */
int server_check_ob(void)
{
  struct timeval tv;
  unsigned long cmsec, vmsec;
  gettimeofday(&tv, NULL);
  cmsec = (unsigned long)tv.tv_sec * 1000000 + tv.tv_usec;
  vmsec =
    (unsigned long)server_current->ob.lr.tv_sec * 1000000 +
    server_current->ob.lr.tv_usec;
  /* Check if ob needs resetting */
  if (cmsec - vmsec > server_current->ob.msec * 1000)
    {
      /* reset */
      server_current->ob.clines = 0;
      memcpy(&server_current->ob.lr, &tv, sizeof(tv));
    }
  return (server_current->ob.clines < server_current->ob.mlines);
}

/**
 * Handles output 
 */
void server_handle_write(void)
{
  int w;
  int i;
  if (server_current->status < server_ok)
    return;
  /* As long as we can send data ... */
  for (i = 0; i < 3 && server_check_ob(); i++)
    {
      char *message;
      /* ... get next message and stop if no messages .. */
      if ((message = server_deque_next_message()) == NULL)
        break;
      /* ... update ob and write ... */
      if ((w = write(server_current->fd, message, strlen(message))) < 0)
        {
          /* .. guess we ain't writin' .. */
          server_current->disconnect = 1;
          free(message);
          return;
        }
      server_current->ob.clines++;
      server_current->tx += w;
      free(message);
    }
}

/**
 * Returns server's name or host
 * @return server name or host
 */
const char *server_get_name(void)
{
  if (server_current->name == NULL)
    return server_current->host;
  return server_current->name;
}

/**
 * Handles server message splitting
 * @param line RFC1459 message
 * @param argc pointer to argument count
 * @return splitted string
 */
char **server_split(const char *line, int *argc)
{
  char **rv, *cp, *ocp;
  int nargs;
  nargs = 1;
  /* Put the string into index 0 */
  rv = malloc(sizeof(char **) * nargs);
  rv[0] = strdup(line);
  ocp = rv[0];
  /* first, see if we have an address... */
  if (*ocp == ':')
    {
      /* oh yes. */
      memmove(ocp, ocp + 1, strlen(ocp + 1) + 1);
      cp = strchr(ocp, ' ');
      *cp = '\0';
      ocp = cp + 1;
    }
  else
    {
      /* Make up an address */
      size_t namelen;
      const char *name;
      name = server_get_name();
      namelen = strlen(name);
      rv[0] = realloc(rv[0], strlen(rv[0]) + namelen + 2);
      memmove(rv[0] + namelen + 1, rv[0], strlen(rv[0]) + 1);
      memcpy(rv[0], name, namelen + 1);
      ocp = rv[0] + namelen + 1;
    }
  /* that's the name part, now to the argument part. */
  while ((*ocp != ':') && (cp = strchr(ocp, ' ')))
    {
      /* Skip white space */
      while ((*cp == ' '))
        {
          *cp = '\0';
          cp++;
        }
      /* Update pointers */
      nargs++;
      rv = realloc(rv, sizeof(char **) * nargs);
      rv[nargs - 1] = ocp;
      /* but if it was last, we bail out */
      ocp = cp;
    }
  nargs++;
  /* Handle the LAST element */
  rv = realloc(rv, sizeof(char **) * nargs);
  if (*ocp == ':')
    ocp++;
  rv[nargs - 1] = ocp;
  *argc = nargs;
  return rv;
}

/**
 * Deallocates a splitted string
 * @param a pointer to splitted string
 */
void server_free_args(char **args)
{
  free(args[0]);
  free(args);
}

/**
 * Handles server input and fires event
 * @param line rfc1459 server message
 */
void server_parse_input(const char *line)
{
  char **args;
  int argc;
  /* Split 'n' fire */
  args = server_split(line, &argc);
  server_fire_event(server_current, (const char *)args[1],
                    (const char **)args, argc);
  server_free_args(args);
}

/**
 * Does buffered reading 
 */
void server_handle_read(void)
{
  char *cp, *ocp, *ch;
  size_t len, left, r;
  socklen_t vlen;
  int val;
  /* Find out how much we can read */
  if (server_current->status == server_preconnect)
    {
      /* check that we are really connected ... */
      val = 0;
      /* Should put val == 0 */
      vlen = sizeof val;
      getsockopt(server_current->fd, SOL_SOCKET, SO_ERROR, &val, &vlen);
      if (val)
        {
          print("getsockopt returned %d: %s", val, strerror(val));
          server_current->disconnect = 1;
          return;
        }
      val = fcntl(server_current->fd, F_GETFL);
      fcntl(server_current->fd, F_SETFL, val & (~O_NONBLOCK));


      /* make sure nothing is lurking in queue... */
      server_queue_clean();

      /* Register */
      if (server_current->pass)
        putfast("PASS %s\r\n", server_current->pass);
      putfast("NICK %s\r\n", server_common.nicks[0]);
      putfast("USER %s na na :%s\r\n",
              server_common.ident, server_common.realname);
      server_current->status = server_ok;

    }
  len = strlen(server_current->buffer);
  left = SERVER_MLEN - len;
  /* Try read */
  r = read(server_current->fd, server_current->buffer + len, left);
  /* Bummer - it failed */
  if (r < 1)
    {
      server_current->disconnect = 1;
      return;
    }
  server_current->rx += r;
  /* Process line at a time */
  *(server_current->buffer + len + r) = '\0';
  ocp = server_current->buffer;
  /* Look line end */
  while ((cp = strchr(ocp, '\n')))
    {
      /* Parse and go for next */
      *cp = '\0';
      if ((ch = strchr(ocp, '\r')))
        *ch = 0;
      server_parse_input(ocp);
      ocp = cp + 1;
    }
  if (*ocp)
    {
      /* Out of space, forced parse and cleanup */
      if (strlen(ocp) == SERVER_MLEN)
        {
          server_parse_input(ocp);
          *server_current->buffer = '\0';
        }
      else
        {
          /* Clean up leftovers after move */
          len = strlen(ocp);
          memmove(server_current->buffer, ocp, len);
          /* keep things tidy! */
          *(server_current->buffer + len) = '\0';
        }
    }
  else
    {
      /* Nothing left, ensure buffer clean */
      *server_current->buffer = '\0';
    }
}

/**
 * Polls the server fd for events 
 * @return non-zero if unapplicable
 */
int server_poll(void)
{
  struct pollfd pfd;
  if ((server_current == NULL) || (server_current->fd == -1))
    {
      return 1;
    }
  memset(&pfd, 0, sizeof pfd);
  pfd.fd = server_current->fd;
  pfd.events = POLLIN | POLLOUT;
  /* Do poll and act accordingly */
  if (poll(&pfd, 1, 100))
    {
      if (pfd.revents & POLLIN)
        {
          server_handle_read();
        }
      if (pfd.revents & POLLOUT)
        {
          server_handle_write();
        }
      if ((server_current->status != server_preconnect)
          && ((pfd.revents & (POLLNVAL | POLLHUP | POLLERR))
              || (server_current->disconnect)))
        {
          _server_disconnect();
          return 1;
        }
    }
  return 0;
}

/**
 * Check that server status is a-ok
 */
void server_check(void)
{
  struct addrinfo hints;
  int err;
  if (server_current == NULL)
    server_next();
  /* Still nothing? */
  if (server_current == NULL)
    return;
  /* wait until server_wait has gone.. */
  if (server_wait > TIME)
    return;
  /* Sanity check */
  if (server_current == NULL)
    {
      print("server: I have no servers");
      server_wait = TIME + 600;
      return;
    }
  /* If poll fails, disconnect */
  if ((server_current->status > server_oident) && server_poll())
    {
      _server_disconnect();
      return;
    }
  if ((server_current->status == server_preconnect) &&
      (server_current->timeout < TIME))
    {
      print("Timeout while connecting to server");
      _server_disconnect();
      return;
    }
  /* Not connected? Get connected */
  if (server_current->status < server_preconnect)
    {
      if (server_current->addrinfo == NULL)
        {
          memset(&hints, 0, sizeof hints);
          hints.ai_socktype = SOCK_STREAM;
          hints.ai_protocol = server_proto();
          print("Resolving host %s...", server_current->host);
          if ((err =
               getaddrinfo(server_current->host, server_current->port,
                           &hints, &server_current->addrinfo)))
            {
              /* Didn't succeed in looking up server info */
              if (server_current->addrinfo)
                freeaddrinfo(server_current->addrinfo);
              server_current->addrinfo = NULL;
              print("Unable to connect %s:%s - getaddrinfo failed: (%d) %s",
                    server_current->host, server_current->port, err,
                    gai_strerror(err));
              server_wait = TIME + 30;
              return;
            }
        }
      /* Connect */
      server_connect();
    }
}

/**
 * Check if we are connected
 * @return non-zero if not disconnected
 */
int server_isconnected(void)
{
  return (server_current->status > server_preconnect);
}

/**
 * Sets the server wait timer
 * @param t new wait value
 */
void server_set_wait(time_t t)
{
  server_wait = t;
}

SERVER *server_get_current(void)
{
  return server_current;
}
