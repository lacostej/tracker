#!/usr/bin/wish

set red_threshold [expr 60 * 10]

proc every {ms body} {
     eval $body
     after $ms [list every $ms $body]
}

set user $env(USER)

pack [label .tracker -textvariable time -font "Times 36" -relief sunken]
pack [ttk::progressbar .bar]

every 1000 {
	global user red_threshold
	set f [open "/var/log/tracker/$user" "r"]
	gets $f l1
	gets $f l2
	gets $f l3
	close $f

	# Time left in seconds
	set total [lindex $l1 0]
	set used [lindex $l1 1]
	set left 0
	catch { set left [expr $total - $used] }

	# Informational last line #3?
	switch "$l3" {
		"" {
			set l3 "Not tracking"
		}
	}
	set ::time "$l3"

	if {$left < $red_threshold} {
		.tracker configure -foreground white -background red
	} {
		.tracker configure -foreground black -background white
	}
	.bar configure -max $total -value $left
}
