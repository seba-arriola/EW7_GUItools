Originated by DavidK 2001/07/17.

Last touched by DavidK 2001/07/17.


HOW TO use the Earthworm Database API (EWDB_API)?

This document contains a list of frequently asked questions,
about working with the EW Database and EWDB_API.

1) What's the difference between an Event and an Origin?

A: An Event is a catch all means of associating data.  In 95% of
the cases, an Event will represent an earthquake.  An Origin, 
along with derived magnitudes and mechanisms, represents an estimate
of what happened during a quake.  An Origin is specifically a
guess as to where the quake originated.  There may be multiple guesses
from multiple institutions, that all belong to the same quake.  
Following the analogy, their may be multiple Origins associated with
a single Event.  The Origins represent estimates or guesses as to
where the quake originated, and there may be more than one.  In most
cases, users will be interested in a single best guess, which the
DBMS terms the Preferred Origin.  One or more Magnitudes may be 
associated with each Origin.  Most users will also be interested in
a preferred magnitude, or atleast a preferred magnitude of each 
type(ML,MD,MW).  Multiple waveforms, picks, and amplitudes
may also be associated with the Event, even though they are not associated
with the Preferred Origin or Magnitude.

The remaining 5% of uses for Event are imagined to be very diverse, 
where the only thing the Events have in common is that they are used
as an associator to associate a bunch of data for a given purpose, 
such as associating a bunch of picks and waveforms for a tremor.

2) What is the EWDB_APPS_LIB and what is it used for?
The EWDB_APPS_LIB is a collection of C functions that allow convenient
access to data in the database.
The calls within the EWDB_APPS_LIB that are oriented towards the
95% usage(described in #1).  They insert and retrieve information 
from the database oriented around Events with a single preferred 
origin, with a bunch of derived magnitudes.  For each channel 
associated with the Event there may be arrivals, amplitudes, and 
waveforms, which may be associated with the origin and/or magnitudes.  
The EWDB_APPS_LIB functions, call EWDB_API_LIB functions, so that 
everything goes through "the API", but the EWDB_APPS_LIB functions 
provide a fairly simple interface to the application developer on 
one end, and then break down all of the neccessary work into small 
blocks that can be handled by individual EWDB_API_LIB functions 
on the other side.


2) How do I insert an Event into the database?

Because the potential uses for Event are envisioned to be so vast,
there are many lower level calls.  Each call has a certain purpose,
such as creating an event, inserting an origin, inserting an arrival,
associating an arrival with an Event, etc...
In an attempt to effectively serve the 95% of clients that are 
interested in stuffing earthquakes, there is a call 
ewdb_apps_PutDBEventInfo(), that reads a mammoth structure (and all
of its substructures) and inserts its contents into the DB.  So you
must fill the structure(s) and then make a single call to stuff it
into the DB.  If the structures and the function does not serve your
needs, then you can always do all the legwork yourself by calling the
lower level EWDB_API_LIB functions.


3) How do I retrieve an Event from the database?

Similar to ewdb_apps_PutDBEventInfo() mentioned above, there is 
an EWDB_APPS_LIB function ewdb_apps_GetDBEventInfo(), that provides
a means of retrieving the data normally associated with an Event into
a single mammoth structure.


4) What is a channel?

Um.....  This is a simple question, but talk of channels usually leads
to channel naming which is a very ugly topic.
A channel is an imaginary pipe.  At one end is the ground, and at
the other end is a seismic processing system.  Ground motion
goes into the channel and a set of digital waveforms comes out.
Starting at the ground end, the channel usually begins with 
a seismic sensor that converts ground motion to analog data,
then through a digitizer with zero or more amplifiers, decimators,
and other junk, and pops out the far end of the pipe as digital 
waveforms.  The channel is the pipe, or the path that the data
took to go from ground motion to digital waveforms. 

Station or Site:  A relatively small physical site where zero or
more pieces of seismic monitoring equipment may be placed.

Component:  A piece of seismic monitoring equipment at a Site that
measures one range of seismic motion.

Channel:  A path/pipe from a Component to a seismic acquisition/processing
system that utilizes digital waveforms.

Network:  A seismic data acquisition and processing system/organization 
that is made up of a lot of paperwork, people, and seismic monitoring 
equipment.  The University of Utah has a seismic network.

A confusing twist:
Channels don't have to have anything to do with seismic
data.  They could be used to transmit tide levels, geomagnetic information,
State of Health, or GPS info.  For the most part, we will stick
to talking about the 95% case where we are transmitting seismic data.


5) How do I lookup a channel in the EWDB?
In order to lookup a channel in the DB, you first need 
someway of identifying the channel.  The EWDB utilizes
identification numbers (IDs) to identify each entity in 
the world of seismic processing.  The ID for a channel is
called an "idChan".  You lookup a channel in the DB via its
idChan.  This kind of begs the question: 
How do I get the idChan for a channel?

6) How do I get the idChan for a channel?
Ah, the heart of the matter....
The best way to get an idChan for a channel is to find
a good psychic. 
(http://google.yahoo.com/bin/query?p=psychic+channel+id&hc=0&hs=15)

If that fails, then the next step is to try to find a channel utilizing
some sort of naming scheme.  There are two routes to go: the internal,
or the external(cheater).

The Internal
Given the SCNL(Site, Channel, Network, Location) codes for a particular
component, and a time of interest, you can call ewdb_api_GetidChansFromSCNLT(),
to get a list of channels that come from a given component.  The SCNL identifies
a Component (see #4), and then the time allows the database to lookup what 
channels were generated from that component at the given time.  If you are pulling
SCN from an automated Earthworm system, be sure to read the "External" section.

The External
Earthworm and potentially other seismic processing software is based
upon an imperfect channel naming system.  Earthworm channel naming
is currently based upon SCN (Station, Channel, Network).  For most 
channels, this SCN identifies the component in the field from which
the data came.  In some instances, where there are arrays, or multiple
instruments with the same component at the same site, then the SCN
is bastardized in order to be unique.  Usually the station name is
fudged to include a component reference.  Any which way, the naming
system at best identifies the instrument in the field that recorded
the data, and not then entire path that the data traveled before
hitting the network processing center.  So, in order to get an
idChan, you need to use a cheater table.  The basis of the cheater
table, is that the network operator has apriori knowledge about
what SCN labeled data is associated with which channel.  The operator
creates a channel(10 for example) in the database, and then sets up the 
cheater table, so that data coming in that is marked with SCN XXX,EHZ,US, 
gets associated with channel #10.  A person looking for idchan for
data that is marked as XXX,EHZ,US, can utilize the same table.
Use int ewdb_apps_RetrieveIdChanExternal(), to lookup an idChan via
the default earthworm cheater table.

Example:
Data is acquired and digitized at a site in the field.  The data is given
the SCN AAA,EHZ,XX, and then piped out to two institutions, who apply two
different sets of filters to the data, resulting in two differnt data streams,
both labeled AAA,LHZ,XX.  How do you know which one is which?  You don't
by looking at the labels inside the data streams, but the operator knows 
he/she/it is getting data from institution A, not institution B.  So he
adds a link in the cheater table between AAA,LHZ,XX, and a channel, and
he hopes that data from same component via institution B, never finds its
way into his real time network.


6) Why should I use the EWDB_API_LIB or EWDB_APPS_LIB functions?
The EWDB_APPS_LIB functions provide convenient ways of accomplishing
popular tasks.  By using the EWDB_APPS_LIB functions, you save yourself
time by not having to go through all of the legwork to break out 
data into small chunks that can be used with EWDB_API_LIB, you use
the same code as many other applications, which means it is fairly well
tested, and it probably works right, even if it doesn't match the 
underlying code changes or doesn't match specifications.

The EWDB_API_LIB is the only supported method for writing data to the EWDB.
(Note the EWDB_APPS_LIB utilizes the EWDB_API_LIB, so it is therefore
 supported).
Starting with EW version 6.0, the EWDB_API_LIB will be backwards compatible,
(reasonably so), but the underlying SQL code and table layout probably will
not be.  The EWDB_API_LIB severely limits the way that data can be written
to the database, and applies business logic rules to data being entered.
Without it, assumptions by applications on the retrieval end may fail.
There are a lot of things that can go wrong when business logic rules and
underlying assumptions are circumvented, and it makes it very difficult
to troubleshoot and maintain a production grade database under such situations.
The idea is that there is a limited number of paths for data to go through
on the way in, and so there is a limited number of things that can go wrong, 
and a limited number of paths that must be tested when things go awry.
The more paths that are added, the more potential for trouble, and the more 
time required to troubleshoot problems.  As much business logic as possible
has been integrated into the PL/SQL procedures that lie inside of the API,
but some of the neccessary logic was built into the API, and thus circumventing
the API, potentialy circumvents logic.  TECHNICALLY, IT WILL BE MUCH MORE 
DIFFICULT TO SUPPORT DATABASES THAT ALLOW APPLICATIONS ACCESS VIA METHODS 
OTHER THAN THE EWDB_API_LIB!


7) How do I get ahold of the developers of the EW v6 EWDB_API_LIB, to thank
them for doing such a wonderful job, or heaven forbid, choking the snot out
of them because functions don't work??

While it is rumored that EWDB_API developers frequent bazaars, this is
probably only at night and in the darkest of corners.  While never going
near catherdrals, these developers live in Ivory Towers, and never on the
ground floor(the 5th floor is popular).  When not writing code, and fixing
bugs, they practice writing in Greek and speaking in tongues.

Due to the excessive number of requests and death threats, direct contact
with the developers is limited, and you should contact the powers that be
with all requests, complaints, and questions.  A good place to start is
the person who distributed this product to you.  As a last resort, you
can always post items to the Earthworm Mailing List ( earthw-list@nmt.edu )




