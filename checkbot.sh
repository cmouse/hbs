#!/bin/sh

hbs=hbs
botdir=/home/hbs/HBS
pidfile=hbs.pid
conffile=hbs.conf

kill -CHLD `cat ${botdir}/${pidfile}` >/dev/null 2>&1

if [ $? -ne 0 ]
 then
  ${botdir}/${hbs} -c $botdir/{$conffile}
 fi
