ShuttlePRO
==========

Deamen for interpreting key, shuttle, and jog events from a Contour Design ShuttlePRO v2 and ShuttleXpress S-XPRS 
and sending them to uinput.

This daemon is a stripped down fork of ShuttlePRO. I am in the process of removing all the user-space portions 
of the code and setting up threads for repeated character sends when using the shuttle-wheel.

Once it's stripped down and working properly I will implement the ability to change the stroke/button translations
on the fly from an indicator applet and on a per window basis ( like the original SuttlePRO app )
