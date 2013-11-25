' Script to generate version.c on windows

Set fso = CreateObject("Scripting.FileSystemObject")
On Error Resume Next
fso.DeleteFile("..\src\version.c")
On Error Goto 0

Set vfile = fso.CreateTextFile("..\src\version.c", True)
Set ofile = fso.OpenTextFile("..\src\version.c.SH", 1)

For x = 1 To 21
	' Skip shell script stuff
	ofile.SkipLine
Next

vGeneration = "1"
vCreation = Date & Chr(32) & Time

Do While Not ofile.AtEndOfStream
	line = ofile.ReadLine
	line = Replace(line, "$generation", vGeneration)
	line = Replace(line, "$creation", vCreation)
	If line <> "!SUB!THIS!" Then
		vfile.WriteLine line
	End If
Loop
vfile.Close
ofile.Close