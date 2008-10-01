#!/usr/bin/wish

proc every {ms body} {
     eval $body
     after $ms [list every $ms $body]
}

set user $env(USER)

pack [label .tracker -textvariable time]
pack [ttk::progressbar .bar]

every 1000 {
	global user
	set f [open "/var/log/tracker/$user" "r"]
	gets $f l1
	gets $f l2
	gets $f l3
	close $f
	set ::time "$l3"
	set times [split "$l1" " "]
	set max [lindex $times 0]
	set cur [lindex $times 1]
	set left [expr $max - $cur]
	.bar configure -max $max -value $left
}
