' Make script for windows
Set fso = CreateObject("Scripting.FileSystemObject")
Set wshShell = CreateObject("WScript.Shell")
bconfs = "Please select which build configuration to use:" & vbCrLf
bconfs = bconfs & "1 - Debug" & vbCrLf
bconfs = bconfs & "    (debugging symbols, no optimizations)" & vbCrLf
bconfs = bconfs & "2 - Release" & vbCrLf
bconfs = bconfs & "    (use this for live servers)" & vbCrLf
intromsg = "This script will build and install UltimateIRCd for you." & vbCrLf & vbCrLf
intromsg = intromsg & "Please make sure you have the Microsoft C++ Development Tools (i.e. Visual C++) installed. These are required to build UltimateIRCd on Windows." & vbCrLf
intromsg = intromsg & "Click OK to proceed"

On Error Resume Next
fso.GetFile("..\UltimateIRCd.exe").Delete()
On Error Goto 0

MsgBox intromsg
buildconf = InputBox(bconfs, "Building UltimateIRCd")

If buildconf Then
	Select Case buildconf
		Case 1
			wshShell.Run "nmake /f UltimateIRCd.mak CFG=" & Chr(34) & "UltimateIRCd - Win32 Debug" & Chr(34) & " clean", 1, True
			wshShell.Run "nmake /f UltimateIRCd.mak CFG=" & Chr(34) & "UltimateIRCd - Win32 Debug" & Chr(34), 1, True
			fso.GetFile("Debug\UltimateIRCd.exe").Copy("..\")
			MsgBox "UltimateIRCd has been built successfully!"
		Case 2
			wshShell.Run "nmake /f UltimateIRCd.mak CFG=" & Chr(34) & "UltimateIRCd - Win32 Release" & Chr(34) & " clean", 1, True
			wshShell.Run "cmd /k nmake /f UltimateIRCd.mak CFG=" & Chr(34) & "UltimateIRCd - Win32 Release" & Chr(34), 1, True
			fso.GetFile("Release\UltimateIRCd.exe").Copy("..\")
			MsgBox "UltimateIRCd has been built successfully!"
		Case Else
			MsgBox "Invalid build configuration specified!"
	End Select
End If