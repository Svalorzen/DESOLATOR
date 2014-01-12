#DESOLATOR
DEcentralized SOLutions And Tactics Of RTS

##Description
A track project given in June 2013 for the MSc Artificial Intelligence of the University of Amsterdam.
In this project we try to extend the approach of C. Jackson and K. Bogert.
They implemented an application of a Dec-POMDP in a real-time strategy game (Starcraft) using Joint Equilibrium-based Search for Policy (JESP) algorithm.
We propose to extend this application by applying the same algorithm for heterogeneous teams.

##Installation
###Step 1: Clone our repository
1. Clone the repository to a directory of your choosing.

###Step 2: Install Starcraft
1. Download starcraft.zip
2. Unpack starcraft.zip
3. Merge the Starcraft folder with the one created during the cloning of this repo.
4. Add registry key 'InstallPath' containing the path of the Starcraft folder, which can be done at: ‘HKEY_CURRENT_USER -> Software -> Blizzard Entertainment -> Starcraft’. If Windows is 64 bits, the key needs ALSO to be in ‘HKEY_CURRENT_USER -> Software -> Wow6432Node -> Blizzard Entertainment -> Starcraft‘ (ChaosLauncher will look only at the first, while the BWAPI installer will look at the second).

###Step 3: Install Microsoft Visual Studio 2010 and 2013
1. Download and install Microsoft Visual Studio 2010 and 2013 Express.

###Step 4: Install BWAPI 4.0.0 Beta
1. Download and unzip BWAPI 4.0.0: http://code.google.com/p/bwapi/downloads/list
2. Merge the BWAPI folder with the BWAPI_4.0.0 folder created during the cloning of this repo. Note that the two project folders should be at the same level of all other BWAPI folders (i.e. example projects, include dirs, etc.)
