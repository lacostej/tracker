#!/usr/bin/wish

proc every {ms body} {
     eval $body
     after $ms [list every $ms $body]
}

set user $env(USER)

pack [label .tracker -textvariable time]

every 1000 {
	global user
	set f [open "/var/log/tracker/$user" "r"]
	gets $f l1
	gets $f l2
	gets $f l3
	close $f
	set ::time "$l3"
}
