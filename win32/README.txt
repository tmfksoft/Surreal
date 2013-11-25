To compile UltimateIRCd on windows you'll need the Microsoft C++ Compiler installed and working. I am using
Visual C++ 6.0 (Service Pack 5), but other versions should work aswell. If you manage to compile the IRCd
using a different (preferably newer) version, please let me know.

There are two so called "build configurations", the Release configuration and Debug configuration. The Release
configuration creates a binary with speed optimizations enabled and without debugging symbols, which will result
in a very fast and small binary. Use this configuration if you are not going to do any debugging. The Debug
configuration compiles with no optimizations at all, with debugging symbols and will create a couple of other
files that can be used with debugging.

There are 3 ways to compile UltimateIRCd on Windows:

1) Using the Make.vbs script
   In the win32 directory you'll find a Make.vbs script, which will allow you to select a build configuration and
   will then build the IRCd for you. I recommend you choose this way, especially if you're not familiour with the
   compiler tools or IDE. When the build process is completed, this script will copy the executable into the
   root of the source tree. From there you can just edit your configuration and run it.

2) Using the Visual C++ IDE
   Open the "UltimateIRCd.dsw" workspace file in Visual C++, then click Compile or press F7. You'll need to compile
   using this way if you want to be able to easely debug the ircd. In the Project settings you can set the working
   directory for debugging sessions, enter "..\..\" there to make it execute the ircd in the root of the source
   tree. Now you can run the ircd (press F5) from the IDE and debug it if a problem occurs. If you want to make a
   binary with the "Release" configuration in the Visual C++ IDE, select "Set Active Configuration" from the Build
   menu and select "Win32 Release". Now compile the same way, but remember that a binary created using the Release
   configuration cannot be used for debugging.

3) Manually, using the commandline compiler tools
   Open a command prompt and change to the "win32" directory in your source tree. Now run
   nmake /f UltimateIRCd.mak CFG="UltimateIRCd - Win32 Debug"
   to make a binary using the Debug configuration, or run
   nmake /f UltimateIRCd.mak CFG="UltimateIRCd - Win32 Release"
   to make a binary using the Release configuration. The nmake program works pretty much the same as GNU make.
