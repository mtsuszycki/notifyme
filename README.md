# notifyme
Notifyme is Linux/FreeBSD/OpenBSD  console  background utility that notifies the user about friends' logins and logouts

Notifyme is a GPL'ed Unix (tested under Linux, FreeBSD and OpenBSD) console 
utility that stays in a background (it isn't a daemon but it doesn't 
block terminal either) and prints a message if a specified 
login and/or logout occurs.
You can run it just with command line arguments (see below) but if you
want more flexibility then resource file should be created.
In a resource file ($HOME/.notifyrc by default) you can specify
(extended regular expressions are allowed) usernames, hostnames and terminals 
that should be monitored, optional messages that will be displayed and
other options (beep, report logouts, run external program etc.)
See notifyrc.sample for example. It should be self explanatory.

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

 
