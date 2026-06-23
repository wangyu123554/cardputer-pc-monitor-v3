' Start LHM via hidden PowerShell (for logon scheduled task — no console flash).
Set sh = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")
dir = fso.GetParentFolderName(WScript.ScriptFullName)
cmd = "powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File """ & dir & "\start_lhm.ps1"" -Quiet"
sh.CurrentDirectory = dir
sh.Run cmd, 0, False
