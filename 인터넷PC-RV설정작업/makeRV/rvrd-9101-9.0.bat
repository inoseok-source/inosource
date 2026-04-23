set TIBRV_LICENSE=C:\tibco\tibrv\license\RV-license-to20271231.bin
C:\tibco\tibrv\9.0\bin\rvrd -listen tcp:9101 -http 9131 -http-only -store c:\tibco\cfg-log\rvrd9001.cfg -logfile c:\tibco\cfg-log\rvrd9101.log -log-max-size 20000 -log-max-rotations 10 -reliability 60
