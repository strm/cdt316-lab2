#ifndef NAMESERVER_H

#ifdef LOCAL_VERSION
#define NAMESERVER "localhost"
#else
#ifndef NAMESERVER

/*--------------------------------------------------- 
  The system is configured as networked, 
  with a central nameserver running on IDt webserver.
  If you want to run a stand-alone version, you must 
  change this to our local machine and recompile 
  (e.g. #define NAMESERVER "lab6-4.idt.mdh.se")
  See README file for more details.
 ---------------------------------------------------*/
#define NAMESERVER "localhost"


#endif
#endif

#ifndef NSPORT
#define NSPORT 4713
#endif

#define NSDB "nameserver"

#define NAMESERVER_H
#endif
