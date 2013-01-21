
EasyMercurial
=============

EasyMercurial is a user interface for the Mercurial distributed
version control system.

EasyMercurial is intended to be:

 * simple to teach and to learn
 * indicative of repository state using a history graph representation
 * recognisably close to normal command-line workflow for Mercurial
 * consistent across platforms

We are not trying to produce "the best" Mercurial client for any one
purpose. We actively encourage users to move on to other clients as
their needs evolve. The aim is simply to provide something accessible
for beginners in small project groups working with a shared remote
repository.

The application is developed by Chris Cannam for SoundSoftware.ac.uk,
based on the HgExplorer application by Jari Korhonen, and is published
under the GPL.  See the file COPYING for license details.


Building EasyMercurial
======================

EasyMercurial is written in C++ using the Qt4 toolkit.  On most
platforms, you can build it by running "qmake" followed by "make".  Qt
version 4.6 or newer is required.


To run EasyMercurial
====================

Just run the EasyMercurial application that is produced by the build.
You will of course also need to have Mercurial installed (version 1.7
or newer).  If you want to use the EasyHg authentication extension,
you will also need PyQt4 (the Python bindings for Qt4); you may also
wish to install the python-crypto library for the password store.
Finally, an external diff/merge utility is required, typically kdiff3.


EasyMercurial is
Copyright 2010 Jari Korhonen
Copyright 2010-2013 Chris Cannam
Copyright 2010-2013 Queen Mary, University of London




