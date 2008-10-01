#!/usr/bin/wish

set red_threshold [expr 60 * 10]

proc every {ms body} {
     eval $body
     after $ms [list every $ms $body]
}

set user $env(USER)

pack [label .tracker -textvariable time -font "Times 36" -relief sunken]

every 1000 {
	global user red_threshold
	set f [open "/var/log/tracker/$user" "r"]
	gets $f l1
	gets $f l2
	gets $f l3
	close $f
	set ::time "$l3"
	if {[expr [lindex $l1 0] - [lindex $l1 1]] < $red_threshold} {
		.tracker configure -foreground white -background red
	} {
		.tracker configure -foreground black -background white
	}
}
