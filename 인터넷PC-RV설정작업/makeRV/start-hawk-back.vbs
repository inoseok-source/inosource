Set objShell=CreateObject("Shell.Application")
objShell.ShellExecute "c:\work\makeRV\hawk-agent.bat","/c lodctr.exe /r","","runas",0
objShell.ShellExecute "c:\work\makeRV\hawk-hma.bat","/c lodctr.exe /r","","runas",0
