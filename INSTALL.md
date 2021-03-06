TractoR consists of several R packages and a set of other files which provide
an interface to key functions within those packages. The interface requires a
"bash" shell, and is therefore only designed to work on Unix-like operating
systems, such as Linux or Mac OS X. In particular, it will not work directly on
Windows, and Windows users are therefore recommended to run Linux within a
virtual machine, and then install TractoR (and R) within that virtual
environment. The virtual machine image provided by the [FSL
package](http://www.fmrib.ox.ac.uk/fsl/) is one suitable example.


Quick installation for knowledgeable users
------------------------------------------

1. Install R if necessary. See <http://www.r-project.org>.

2. Open a terminal, navigate to the uncompressed "tractor" directory and type
   `make install`. A C/C++ compiler must be available and known to R.

3. Create or update the `TRACTOR_HOME` environment variable to point to the
   "tractor" directory, include `${TRACTOR_HOME}/bin` on the `PATH` and
   `${TRACTOR_HOME}/share/man` on the `MANPATH`.

4. Consider installing [FSL](http://fsl.fmrib.ox.ac.uk/), if it is not
   already installed. It is used by some elements of TractoR's functionality.


Details and step-by-step explanation
------------------------------------

Before installing TractoR, R must be installed. R runs on almost all platforms
and is simple to install. It can be obtained from <http://www.r-project.org>.

A C/C++ compiler is also required to build parts of the package. A suitable
compiler can be installed using an appropriate package manager (`aptitude`,
`yum`, etc.) on Linux, or with
[Xcode](https://itunes.apple.com/gb/app/xcode/id497799835) (from the Mac App
Store) on OS X. R handles all the details of actually compiling code.

Once R and the C/C++ compiler are installed, the TractoR R packages can be
installed by opening a terminal window and running

    make install

from the uncompressed "tractor" directory. To test that the installation was
successful, type

    make clean test

and a series of standard tests will be run.

For the "tractor" interface to function correctly, the `TRACTOR_HOME` and
`PATH` environment variables should be set appropriately. To do this, add the
following lines to your `~/.bashrc` (or equivalent for other shells):

    export TRACTOR_HOME=/usr/local/tractor
    export PATH=${TRACTOR_HOME}/bin:${PATH}
    export MANPATH=${TRACTOR_HOME}/share/man:${MANPATH}

(Note that the first line may need to be modified to reflect the actual
location of the uncompressed "tractor" directory on your system. You should get
a prompt giving the appropriate lines for your case after running
`make install` above.) To make sure that "login" shells also pick up these
variables, you may also wish to point your `~/.bash_profile` at `~/.bashrc`.
This can be achieved with the line

    [ -f ~/.bashrc ] && source ~/.bashrc

in `~/.bash_profile` (create this file if it doesn't exist). To check that
everything is functioning correctly, you can try the following commands after
opening a new terminal window:

    cat ${TRACTOR_HOME}/VERSION   # should print the current TractoR version
    man tractor                   # should open the "tractor" man page
    tractor list                  # should list the TractoR scripts installed

For further information on using the TractoR interface, type `tractor -h` or
take a look at the man page. To find out what a particular script does, type
`tractor -o (script_name)`, replacing `(script_name)` with the name of the
script you are interested in.

Finally, you may want to consider also installing
[FSL](http://fsl.fmrib.ox.ac.uk/), if it is not already installed. It is used
by some elements of TractoR's functionality, so installing it now will avoid
any problems with missing software later.
