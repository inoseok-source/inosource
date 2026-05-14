tibrvsend, tibrvlisten, rvrd 테스트
====
```
local network interface 설정
- Service 20111
- Network Specification ;230.2.1.11;

tibrvlisten -service 20111 -network ";230.2.1.11" -daemon tcp:9201 "TEST.>"
```