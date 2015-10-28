HBS README
==========
This text has been written so that when you encounter the bot you will be
able to get a little more out of it. This is not comprehensive document on
how to use it. This is a simple document that gets you on your way. 

1. INSTALLING
-------------

1.1. Preconfiguring
-------------------

Use the ./configure script to configure the bot. See the various options
from ./configure --help. The most important ones you should know are 
--enable-debug and --enable-calc. 

You don't need to define prefix here. It will be provided via other means. 

1.2. Compilation
----------------
 
Everything should go well if you use GNU Make (gmake in some systems). Some
points of interest however. If you run into compiler warnings, and have no
interest in debugging them, just ignore them. Especially if you are using
non-linux system you are bound to see these. 

1.3. Installing
--------------- 
To install the bot somewhere, run the following command
  
  make install DESTDIR=/bot/install/dir

This will copy the bot binarier to the directory, and create modules file
that is used to load modules. Edit this file if you do not with to load
a certain module. 

You are also handed hbs.conf.example, user.conf.example and
channel.conf.example. You need to configure these before running the bot. 

If you are only planning to run one bot, it is enough just to copy these
files to hbs.conf, user.conf and channel.conf. After doing this, edit the
hbs.conf to suit your needs. Note that to define server password, add
&lt;variable name="pass">password&lt;/variable> to the server configuration. 

User configuration can be done by just changing the Q account name. But if
you want to set your password before booting the bot, you can add a password
by running 'echo -n password | sha1sum'. Add this to your user account with
&lt;password>result here&lt;/password>. 

Channel configuration, just change the channel name. 

To start the bot, run ./hbs -c config.file. Default is hbs.conf. If everything
goes well, the bot will appear to your channel. 
