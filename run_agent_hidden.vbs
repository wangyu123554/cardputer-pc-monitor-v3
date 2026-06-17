' Runs PC Monitor Agent v3 in background (prefers PCMonitorAgent.exe).
Set sh = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")
dir = fso.GetParentFolderName(WScript.ScriptFullName)
sh.CurrentDirectory = dir

exe = dir & "\PCMonitorAgent.exe"
If fso.FileExists(exe) Then
    sh.Run """" & exe & """", 0, False
    WScript.Quit 0
End If

pythonw = "pythonw.exe"
Set wshExec = sh.Exec("cmd /c where pythonw 2>nul")
Do While wshExec.Status = 0
    WScript.Sleep 50
Loop
If wshExec.ExitCode <> 0 Then
    WScript.Quit 1
End If
pythonw = Trim(wshExec.StdOut.ReadLine())
If pythonw = "" Then
    WScript.Quit 1
End If

agent = dir & "\pc_monitor_agent.py"
If Not fso.FileExists(agent) Then
    WScript.Quit 1
End If

sh.Run """" & pythonw & """ """ & agent & """", 0, False
