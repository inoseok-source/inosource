set TIBRV_LICENSE=C:\tibco\tibrv\license\RV-license-to20271231.bin
C:\tibco\tibrv\9.0\bin\rvrd -listen tcp:9102 -http 9132 -http-only -store c:\tibco\cfg-log\rvrd9002.cfg -logfile c:\tibco\cfg-log\rvrd9102.log -log-max-size 20000 -log-max-rotations 10 -reliability 60
