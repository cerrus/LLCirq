This is a simplistic implementation of a circular queue for multiple writer threads and a single
reader thread using atomics only.  It is meant to demonstrate a lockless design, but is not battle-tested.

The current version is very much alpha and should be only used at your own risk.  Currently it is GCC specific
and only tested on an Ubuntu 20.04 instance running multipass on a Mac M1 with gcc 9.4.0.

Currently it only allows copies to the queue and static sizing at initialization, see test.c or test.cpp for 
some examples.   
