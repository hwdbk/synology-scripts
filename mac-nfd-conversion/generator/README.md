# syn2mac and mac2syn generator

This directory contains the script that generated the syn2mac and mac2syn scripts. It uses iconv to do the conversion
(Note: iconv -t UTF-8-MAC is usually not implemented / available on non-Mac OS X systems, hence the generator needs
to be run from MacOS) and store the UTF-8 character mapping in the form of a sed recipe, written as part of a bash script.
