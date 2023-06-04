# notifyme
Notifyme is Linux/FreeBSD/OpenBSD  console  background utility that notifies the user about friends' logins and logouts

Notifyme is a GPL'ed Unix (tested under Linux, FreeBSD and OpenBSD) console 
utility that stays in a background (it isn't a daemon but it doesn't 
block terminal either) and prints a pre-defined message (and optinally a beep)  if one of the specified 
users logs in or logs out.

It can watch for login names, host or ttys.
It's fully configurable, allowing to specify a message, duration, beep and option action (by executing external script)
for each watched username.
See notifyrc.sample below.

You can run it just with command line arguments (see below) but if you
want more flexibility then resource file should be created.
In a resource file ($HOME/.notifyrc by default) you can specify
(extended regular expressions are allowed) usernames, hostnames and terminals 
that should be monitored, optional messages that will be displayed and
other options (beep, report logouts, run external program etc.)
See notifyrc.sample for example. It should be self explanatory.

Compile:
./configure && make
optional: make install

Usage:
```
notifyme [-c file] [-C] [-l] [-q] [-h] [USERNAME] ...

Options:
	-c file		use alternate resource file
	-C		always print messages in the center of the screen
	-l 		don't report logouts 
	-q		quiet - no beep with message
	-w              report present online user and exits - Added by SK
	-h		help
	
  ```
Rest of the arguments are usernames to watch (extended regular expressions
are allowed).
If no usernames are specified _and_ there is no resource file then ALL logins
and logouts are reported.

USR1 signal can be used to re-read resource file.

Command line options have highest precedence.
(ie. if '-l' then logouts won't be reported no matter what was specified in 
config file)

SAMPLE RC CONFIG FILE
 ```
 # Simple configuration file for notifyme.
# Please edit this file - it is just an example. 
#
# Each directive in separate line, except '{' that can be
# proceeded with some identifier.
# "users:" "hosts:" and "ttys:" are reserved words.
#
# In messages following characters can be used:
# %u - username, %h - hostname, %t - tty
#
# Extended regular expressions can be used to specify
# user, host or tty
#
# General rule: first match wins.
# For example if "johnny" is in users section (which is first) and
# some.host.org is in host section and johnny just logged in
# from some.host.org then settings from "johnny" will be used.

######
# Global configuration. 
# Each directive listed below can also be used in 
# users/hosts/ttys sections and entry section, and it will override 
# previous settings.
# 

Logins 		yes	# report logins
Logouts 	yes	# and logouts
LoginBeep	yes	# make sound if login
LogoutBeep	yes	# make sound if logout
PrintWelcome	yes	# report logged users at program's start
CenterMsg	no	# center messages

LoginMsg	"%u from %h arrived!"	
LogoutMsg	"%u logged out."

# message printed if it was last login
LastMsg		"%u - no more logins."	

# number of seconds to hold a message on the screen
HoldMsg		5

# This is only a fictional example of usage.
# Call external program if login occurs.
# "some_prog" may be for example a shell script that takes 3 arguments
# (login name, host, tty) and sends email.
#LoginExec	"/path/to/some_prog %u %h %t"

# Call external program if logout occurs.
#LogoutExec	"/path/to/some_prog %u %h %t"

######
# Watch following users
#

users:
	# Only example. Doesn't change anything because it was set in globals
	Logouts yes
	
	john
	betty {
		# Don't report logouts for this particular user
		Logouts no
		LogoutMsg "My SweetHeart just left"
		LoginMsg "Betty came from %h"
		LastMsg "How pity, I'm alone"
		LogoutBeep no
		LoginBeep no
	}
	# Don't have to use braces here because it is only one directive    
	peter 	LoginMsg "Watch out! Boss is here!"
	badboy {
		PrintWelcome no
		CenterMsg yes
		HoldMsg 30
		LoginMsg "Alert! BADBOY from %h on %t!"
	}

######
# Watch logins and logouts made from following hosts

hosts:
	Logouts no
	.+.some.domain {
		# It overrides previous setting  
		Logouts yes
		LoginMsg "%u from %h is here."
	}
#####
# Watch following terminals

ttys: 
	# Some of these settings are pointless because they were
	# set in global configuration to the same values.
	LoginBeep	yes
	LogoutBeep	no
	PrintWelcome	yes	
	
	tty[1-9] { 
		LoginMsg "%u is on the CONSOLE (%t)."
	} 
	
# that's all. 
```
