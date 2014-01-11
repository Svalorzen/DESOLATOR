#DESOLATOR
DEcentralized SOLutions And Tactics Of RTS

##Description
A track project given in June 2013 for the MSc Artificial Intelligence of the University of Amsterdam.
In this project we try to extend the approach of C. Jackson and K. Bogert.
They implemented an application of a Dec-POMDP in a real-time strategy game (Starcraft) using Joint Equilibrium-based Search for Policy (JESP) algorithm.
We propose to extend this application by applying the same algorithm for heterogeneous teams.

##Setting up the environment
###Step 1: Install Starcraft
1. Download starcraft.zip
2. Unpack starcraft.zip
3. Add registry key 'InstallPath' containing the path of the Starcraft folder, which can be done at: ‘HKEY_CURRENT_USER -> Software -> Blizzard Entertainment -> Starcraft’. If Windows is 64 bits, the key needs to be in ‘HKEY_CURRENT_USER -> Software -> Wow6432Node -> Blizzard Entertainment -> Starcraft‘.

###Step 2: Install Microsoft Visual Studio 2013
1. Download and install Microsoft Visual Studio 2013 Express.

###Step 3: Install BWAPI 4.0.0 Beta
1. Download and unzip BWAPI 4.0.0: http://code.google.com/p/bwapi/downloads/list
2. Follow ReadMe instruction of BWAPI (you don't need to compile the example)
