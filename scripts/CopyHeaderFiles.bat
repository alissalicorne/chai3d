rem ===========================================================================
rem
rem    - CHAI 3D -    
rem
rem    Copyright (C) 2003. Stanford University - Robotics Laboratory
rem    
rem    Author:	  Francois Conti
rem    		  francois@chai3d.org
rem    
rem    Date:	  11/2003
rem    
rem    Purpose:	  This batch file copies all header files located in 'src' 
rem  		  directory to 'include' directory. 
rem
rem ===========================================================================


rem -----------------------------------------
rem INITIALISATION
rem -----------------------------------------

echo off

path
cd ..


rem -----------------------------------------
rem COPY ALL INCLUDE FILES
rem -----------------------------------------
	
xcopy /Y src\collisions\*.h 		include
xcopy /Y src\devices\*.h		include
xcopy /Y src\display\*.h 		include
xcopy /Y src\files\*.h 			include
xcopy /Y src\forces\*.h 		include
xcopy /Y src\graphics\*.h 		include
xcopy /Y src\math\*.h 			include
xcopy /Y src\timers\*.h 		include
xcopy /Y src\tools\*.h 			include

rem ----------------------------------------------------
rem COPY MSVC7 LIBRARY FILES NOT WRITTEN AT COMPILE-TIME
rem ----------------------------------------------------

xcopy /Y lib\msvc6\DLPORTIO.lib lib\msvc7
xcopy /Y lib\msvc6\PhantomDriver.lib lib\msvc7
xcopy /Y lib\msvc6\dhdms.lib lib\msvc7

rem -----------------------------------------
rem FINALIZE
rem -----------------------------------------

echo on






