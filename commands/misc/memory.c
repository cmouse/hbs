#include "misc.h"
#include <malloc.h>

void misc_command_meminfo (NICK * nick, CHANNEL * channel, const char *cmd,
			   const char **args, int argc)
{
  pid_t pid;
  char line[512];
  FILE *fin;
  size_t vmsize;
  size_t vmlck;
  size_t vmrss;
  size_t vmdata;
  size_t vmstk;
  size_t vmexe;
  size_t vmlib;
  if (!(fin = fopen ("/proc/self/status", "r")))
    {
      puttext ("NOTICE %s :Memory stats not available\r\n", nick->nick);
      return;
    }
  pid = 0;
  vmsize = vmlck = vmrss = vmdata = vmstk = vmexe = vmlib = 0;
  while (fgets (line, sizeof line, fin))
    {
      if (sscanf (line, "Pid: %d", &pid))
	continue;
      if (sscanf (line, "VmSize: " FSO "", &vmsize))
	continue;
      if (sscanf (line, "VmLck: " FSO "", &vmlck))
	continue;
      if (sscanf (line, "VmRSS: " FSO "", &vmrss))
	continue;
      if (sscanf (line, "VmData: " FSO "", &vmdata))
	continue;
      if (sscanf (line, "VmStk: " FSO "", &vmstk))
	continue;
      if (sscanf (line, "VmExe: " FSO "", &vmexe))
	continue;
      if (sscanf (line, "VmLib: " FSO "", &vmlib))
	continue;
    }
  fclose (fin);
  puttext ("NOTICE %s :Pid: %d - Locked: " FSO " kb - Resident stack: " FSO
	   " kb - Data size: " FSO " kb - Stack size: " FSO
	   " kb - Executable: " FSO " kb - Libaries: " FSO " kb - Total " FSO
	   " kb\r\n", nick->nick, pid, vmlck, vmrss, vmdata, vmstk, vmexe,
	   vmlib, vmsize);
}
