[![](https://www.getmangos.eu/images/primus/blue/misc/logo.png)](http://www.getmangos.eu)&nbsp;

[![](https://github.com/mangoszero/server/raw/master/icons/FORUM.gif)](https://www.getmangos.eu/forum.php)
[![](https://github.com/mangoszero/server/raw/master/icons/WIKI.gif)](http://github.com/mangoswiki/wiki/wiki)
[![](https://github.com/mangoszero/server/raw/master/icons/TOOLS.gif)](http://github.com/mangostools)
[![](https://github.com/mangoszero/server/raw/master/icons/TRACKER.gif)](https://www.getmangos.eu/project.php)

Mangos Dependencies
------------
*Mangos* stands on the shoulders of well-known Open Source
libraries, and a few awesome, but less known libraries to prevent us from
inventing the wheel again.

*Please note that Linux and Mac OS X users should install packages using
their systems package management instead of source packages.*

* **MySQL** / **PostgreSQL**: to store content, and user data, we rely on
  [MySQL][1]/[MariaDB][2] and [PostgreSQL][3] to handle data.
* **ACE**: the [ADAPTIVE Communication Environment][4] aka. *ACE* provides us
  with a solid cross-platform framework for abstracting operating system
  specific details.
* **Recast**: in order to create navigation data from the client's map files,
  we use [Recast][5] to do the dirty work. It provides functions for
  rendering, pathing, etc.
* **G3D**: the [G3D][6] engine provides the basic framework for handling 3D
  data, and is used to handle basic map data.
* **libmpq**: [libmpq][7] provides an abstraction layer for reading from the
  client's data files.
* **Zlib**: [Zlib][12] ([Zlib for Windows][10]) provides compression algorithms
  used in both MPQ archive handling and the client/server protocol.
* **Bzip2**: [Bzip2][13] ([Bzip2 for Windows][11]) provides compression
  algorithms used in MPQ archives.
* **OpenSSL**: [OpenSSL][8] ([OpenSSL for Windows][14]) provides encryption
  algorithms used when authenticating clients.
* **Lua**: [Lua 5.2][15] ([Lua 5.2 for Windows][16]) provides a convenient, fast
  scripting environment, which allows us to make live changes to scripted
  content.

*Recast*, *G3D* and *libmpq* are included in the *Mangos* distribution as
we rely on specific versions. *libmpq* is to be replaced with *stormlib* shortly. 

Optional dependencies
---------------------

* **Doxygen**: if you want to export HTML or PDF formatted documentation for the
  *Mangos* API, you should install [Doxygen][9].

[1]: http://www.mysql.com/ "MySQL · The world's most popular open source database"
[2]: http://www.mariadb.org/ "MariaDB · An enhanced, drop-in replacement for MySQL"
[3]: http://www.postgresql.org/ "PostgreSQL · The world's most advanced open source database"
[4]: http://www.cs.wustl.edu/~schmidt/ACE.html "ACE · The ADAPTIVE Communication Environment"
[5]: http://github.com/memononen/recastnavigation "Recast · Navigation-mesh Toolset for Games"
[6]: http://sourceforge.net/projects/g3d/ "G3D · G3D Innovation Engine"
[7]: http://github.com/ge0rg/libmpq "libmpq · A library for reading data from MPQ archives"
[8]: http://www.openssl.org/ "OpenSSL · The Open Source toolkit for SSL/TLS"
[9]: http://www.stack.nl/~dimitri/doxygen/ "Doxygen · API documentation generator"
[10]: http://gnuwin32.sourceforge.net/packages/zlib.htm "Zlib for Windows"
[11]: http://gnuwin32.sourceforge.net/packages/bzip2.htm "Bzip2 for Windows"
[12]: http://www.zlib.net/ "Zlib"
[13]: http://www.bzip.org/ "Bzip2"
[14]: http://slproweb.com/products/Win32OpenSSL.html "OpenSSL for Windows"
[15]: http://www.lua.org/ "Lua"
[16]: https://code.google.com/p/luaforwindows/ "Lua for Windows"
