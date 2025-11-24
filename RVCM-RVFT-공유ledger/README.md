## RVCM + RVFT + 공유ledger 테스트
- tibrvcmsend.c, tibrvfttime.c 활용해서 ftCmSend.c 소스 코드 만듬.
- 컴파일 하기 위해서 Makefile.windows 파일에 ftCmSend를 추가해줌.

## 터미널 명령어
- T1: rvd.exe -listen tcp:7500
- T2: ftCmSend.exe -service 7500 -ft-weight 50
- T3: ftCmSend.exe -service 7500 -ft-weight 100
- T4: tibrvcmlisten.exe -service 7500 -cmname RVCMSUB -ledger cm_receiver.ledger TIBRVFT_TIME

## 테스트 진행 
- T2 실행하면 active상태가 되고, tibrvcmlisten.exe에 메시지를 보내면서, 시퀀스 번호를 확인함.
- T3 실행하면, T2는 deactivate 상태가 되고, T3가 activate 상태가 됨. ledger파일을 공유하기 때문에 T2가 보내고 멈춘 이후의 시퀀스 번호로 메시지를 보냄.
- T2나 T3가 실행될 때 다음과 같이 메시지를 출력함.
```
subject=_RV.INFO.RVCM.REGISTRATION.MOVED.RVCMPUB, reply=none, message={ADV_CLASS="INFO" ADV_SOURCE="RVCM" ADV_NAME="REGISTRATION.MOVED.RVCMPUB" name="RVCMPUB"}, certified sender=FALSE, receipt uncertified 
```
