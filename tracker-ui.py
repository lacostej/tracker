#!/usr/bin/python
import sys
import os
import gtk
import gobject

def cb_function(l, b):
  lines = open('/var/log/tracker/%s' % os.getenv('USER')).readlines()
  words = lines[0].split()
  maxtime = words[0]
  usedtime = words[1]
  prettytime = lines[2].strip()
  frac = float(usedtime)/float(maxtime)
  if (frac < 0.1):
    l.set_markup('<span color="#FF0000">%s</span>' % prettytime)
  else:
    l.set_text(prettytime)
  b.set_fraction(frac)
  return 1

win = gtk.Window()
win.set_title("Time Left")
win.set_border_width(5)
win.connect("destroy", gtk.main_quit)

vbox = gtk.VBox()
vbox.set_spacing(5)
win.add(vbox)

bar = gtk.ProgressBar()
vbox.add(bar)

label = gtk.Label()
vbox.add(label)

gobject.timeout_add(1000, cb_function, label, bar)
cb_function(label, bar)

win.show_all()
gtk.main()
