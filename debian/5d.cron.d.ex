#
# Regular cron jobs for the 5d package
#
0 4	* * *	root	[ -x /usr/bin/5d_maintenance ] && /usr/bin/5d_maintenance
