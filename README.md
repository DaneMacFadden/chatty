Chatty is a simple, multithreaded chat program by Dane MacFadden.
It's inspired by an assignment I did in university that was a similar project
and is intended to be an exercise in networked and multithreaded programming.
Check out Beej's guide to network programming (and his other guides) if you want
to build something similar. Much of my code is based on his client and server.
Shoutout James for helping me write the list library back in the day.

I used ncurses to manage the messages presented on the screen. Since chatty
is multithreaded, this causes some visual issues sometimes. However, the purpose
of this program is to practice networked, multithreaded programming, so visual
bugs weren't the highest priority.
