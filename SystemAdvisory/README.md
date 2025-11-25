## _RV.WARN.SYSTEM.RVD.DISCONNECTED, _RV.INFO.SYSTEM.RVD.CONNECTED
```
rvd.exe -listen tcp:7500
T1: tibrvfttime.exe -service 7500 -ft-heartbeat 1.5 -ft-activate 4.8 -ft-weight 100
T2: tibrvfttime.exe -service 7500 -ft-heartbeat 1.5 -ft-activate 4.8 -ft-weight 80
```
- tibrvEvent_CreateListener함수에 "_RV.*.SYSTEM.>" 설정해서 확인 가능.
- rvd.exe를 죽이면, rvd.exe가 죽었다가 다시 살아나면서 T1과 T2 터미널에 _RV.WARN.SYSTEM.RVD.DISCONNECTED, _RV.INFO.SYSTEM.RVD.CONNECTED 출력.

## _RV.INFO.SYSTEM.UNREACHABLE.TRANSPORT.transport_id
```
rvd.exe -listen tcp:7500
T1: tibrvfttime.exe -service 7500 -ft-heartbeat 1.5 -ft-activate 4.8 -ft-weight 100
T2: tibrvfttime.exe -service 7500 -ft-heartbeat 1.5 -ft-activate 4.8 -ft-weight 80
```
### T1을 ctr+c 명령어로 종료 시키면, T2에 아래와 같이 출력됨.
```
#### RVFT ADVISORY: _RV.INFO.SYSTEM.UNREACHABLE.TRANSPORT.0A62A389.68EEF74D081
Advisory message is: {ADV_CLASS="INFO" ADV_SOURCE="SYSTEM" ADV_NAME="UNREACHABLE.TRANSPORT.0A62A389.68EEF74D081" tport="0A62A389.68EEF74D081"}
#### RVFT ADVISORY: _RV.INFO.SYSTEM.UNREACHABLE.TRANSPORT.0A62A389.68EEF74D082
Advisory message is: {ADV_CLASS="INFO" ADV_SOURCE="SYSTEM" ADV_NAME="UNREACHABLE.TRANSPORT.0A62A389.68EEF74D082" tport="0A62A389.68EEF74D082"}
```
- T1을 종료하면, RVD는 T1의 transport 객체가 종료된 것을 인식하고, 네트워크에 SYSTEM.UNREACHABLE 메시지를 broadcast 함.
- 0A62A389(ip주소), 68EEF74D081(시간정보나 프로세스id를 기반으로 생성된 고유값)
- 0A62A389.68EEF74D081,0A62A389.68EEF74D082 이렇게 메시지가 2개 출력된 이유는 tibrvTransport_Create(&data.transport, serviceStr,networkStr, daemonStr);, tibrvTransport_Create(&data.fttransport, ftserviceStr, ftnetworkStr, ftdaemonStr); 이렇게 두 개의 transport 객체를 생성하기 때문. 첫 번째 함수는 pubTime을 통해 메시지를 전송하는 목적으로 사용되고, 두 번째 함수는 tibrvftMember_Create 함수에 전달되어 FT 하트비트, 활성화/비활성화 지침 등 그룹 멤버십 통신에만 사용됩니다.

## _RV.INFO.SYSTEM.HOST.STATUS.hostid
- hostid는 ip주소의 16진수 값을 나타냄.
- rvd는 실행중인 호스트의 상태를 모니터링함. 기본적으로 HOST.STATUS Advisory로 30초마다 네트워크에 브로드캐스트함.
- 아래 메시지에서 up(업타임) 간격을 보면 90초 간격으로 출력되고 있음.
- cc:현재 이 RVD에 연결된 클라이언트(Transport) 수.
- ms, mr: RVD가 보냈거나(Sent), 받았던(Received) 총 메시지 수.
- ps, pr: RVD가 보냈거나, 받았던 총 네트워크 패킷 수.
- pm, idl, odl: 패킷을 놓치거나(Missed) 데이터가 손실된 횟수.
- ver: RVD 소프트웨어의 버전 번호.
```
#### RVFT ADVISORY: _RV.INFO.SYSTEM.HOST.STATUS.0A62A389
Advisory message is: {ADV_CLASS="INFO" ADV_SOURCE="SYSTEM" ADV_NAME="HOST.STATUS.0A62A389" 
hostaddr=10.98.163.137 os=2 ver="8.7.0" httpaddr=10.98.163.137 httpport=7580 time=2025-10-15 01:38:54.797000000Z 
up=990 rli=60.000000 ms=1915 bs=241142 mr=1156 br=129578 ps=1992 pr=0 rx=0 pm=0 idl=0 odl=0 irrs=0 orrs=0 cc=4 ipport=7500 service="7500" network=""}

#### RVFT ADVISORY: _RV.INFO.SYSTEM.HOST.STATUS.0A62A389
Advisory message is: {ADV_CLASS="INFO" ADV_SOURCE="SYSTEM" ADV_NAME="HOST.STATUS.0A62A389" 
hostaddr=10.98.163.137 os=2 ver="8.7.0" httpaddr=10.98.163.137 httpport=7580 time=2025-10-15 01:40:24.799000000Z 
up=1080 rli=60.000000 ms=2143 bs=270679 mr=1261 br=141293 ps=2173 pr=0 rx=0 pm=0 idl=0 odl=0 irrs=0 orrs=0 cc=4 ipport=7500 service="7500" network=""}
```

## _RV.ERROR.SYSTEM.CLIENT.SLOWCONSUMER
```
rvd.exe -listen tcp:7500 -max-consumer-buffer 1000000
tibrvcmsend.exe -cmname RVCMPUB -interval 1 -rounds 1000000 -msgsize 90400320 test.subject "Hello, World"
tibrvcmlisten.exe -service 7500 -cmname RVCMSUB test.subject
```
- tibrvcmlisten.c에 sleep을 주고, tibrvcmsend.exe에 interval을 0이나 0.01 이렇게 설정하고, -rounds 1000000000, 그리고 큰 msgsize 설정 없이 "Hello, World" 메시지를 보내면, _RV.ERROR.SYSTEM.CLIENT.SLOWCONSUMER 메시지를 확인하기 어렵고, 확인하지 못함.
- 그래서 tibrvcmsend.c에 사이즈가 크 메시지를 전송할 수 있도록, 아래와 같은 함수를 사용함.(90400320이나 100000000 사이즈의 메시지를 전송할 수 있음.
- OPAQUE 타입으로 바이트 배열을 메시지에 추가 err = tibrvMsg_AddOpaque(send_message, FIELD_NAME, bigDataBuffer, (tibrv_u32)msgSizeNum);
- tibrvcmlisten.c의 my_callback함수에 sleep(10); 설정하고 테스트 하면, 쉽게 확인 가능.(sleep 안주고 테스트 해도 확인함.
- tibrvcmsend.exe -cmname RVCMPUB -ledger RVCMPUB.ldeger -interval 1 -rounds 1000000 -msgsize 90400320 test.subject "Hello, World" 이렇게 -ledger 설정하고 테스트 해서 메시지 확인함.

## _RV.ERROR.SYSTEM.DATALOSS.OUTBOUND.MSG_TOO_LARGE
```
rvd.exe -listen tcp:7500 -max-consumer-buffer 1000000
tibrvcmsend.exe -cmname RVCMPUB -interval 1 -rounds 1000000 -msgsize 100400320 test.subject "Hello, World"
tibrvcmlisten.exe -service 7500 -cmname RVCMSUB test.subject
```
- -msgsize 100000000 이렇게 100MB로 설정하고 테스트 하면 아래와 같이 메시지를 확인할 수 있음.
```
2025-10-17 11:34:22 RV: TIB/Rendezvous Error Not Handled by Process:
{ADV_CLASS="ERROR" ADV_SOURCE="SYSTEM" ADV_NAME="DATALOSS.OUTBOUND.MSG_TOO_LARGE" ADV_DESC="dataloss: message was too large for transmission." host="10.98.163.137" lost=210339 scid=0}
```
