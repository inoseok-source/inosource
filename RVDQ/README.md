## 주의 사항
- cmname MyDQGroup -schedulerWeight 1 -heartbeat 1.5 -activation 3.5 test.subject 이 값들은 모두 동일하게 주어야함.

## QUEUE.SCHEDULER.ACTIVE, QUEUE.SCHEDULER.INACTIVE
### 터미널 실행 명령어
- T1:tibrvdqlisten.exe -service 7500 -cmname MyDQGroup -workerWeight 1 -workerTasks 1 -schedulerWeight 1 -heartbeat 1.5 -activation 3.5 test.subject
- T2:tibrvdqlisten.exe -service 7500 -cmname MyDQGroup -workerWeight 2 -workerTasks 3 -schedulerWeight 1 -heartbeat 1.5 -activation 3.5 test.subject
- T3:tibrvdqlisten.exe -service 7500 -cmname MyDQGroup -workerWeight 6 -workerTasks 7 -schedulerWeight 1 -heartbeat 1.5 -activation 3.5 test.subject
- T4:rvd.exe -listen tcp:7500
### 테스트 과정
- T4 실행.
- T1을 먼저 실행하면, _RV.INFO.RVCM.QUEUE.SCHEDULER.ACTIVE.MyDQGroup 출력함.
- T4의 rvd.exe를 ctr+c나 작업관리자에서 죽이면(죽었다가 다시 살아남), T1은 _RV.INFO.RVCM.QUEUE.SCHEDULER.INACTIVE.MyDQGroup 출력하고, INACTIVE로 바뀜.
- T3이 _RV.INFO.RVCM.QUEUE.SCHEDULER.ACTIVE.MyDQGroup 출력하고, ACTIVE로 바뀜. (workerWeight 값이 큰 T3이 ACTIVE로 바뀜.)
- T4의 rvd.exe를 실행하고, T1을 실행하면, T1은 ACTIVE가 됨. 이어서 T2,T3 실행하면, INATIVE 상태이고, 다른 로그를 출력하지 않음. 이 상태에서 T1을 ctr+c로
죽이면, T3이 _RV.INFO.RVCM.QUEUE.SCHEDULER.ACTIVE.MyDQGroup 출력함. 따로 INACTIVE advisory 메시지를 출력하지 않음.

## QUEUE.SCHEDULER.OVERFLOW
### 터미널 실행 명령어
- T1:tibrvdqlisten.exe -service 7500 -cmname MyDQGroup -workerWeight 1 -workerTasks 1 -schedulerWeight 1 -heartbeat 1.5 -activation 3.5 -delay 2.0 -backlogMsgs 50 test.subject
- T2:tibrvdqlisten.exe -service 7500 -cmname MyDQGroup -workerWeight 3 -workerTasks 3 -schedulerWeight 1 -heartbeat 1.5 -activation 3.5 -delay 2.0 -backlogMsgs 50 test.subject
- T3:tibrvcmsend.exe -service 7500 -rounds 500 -interval 0.01 test.subject "X"
- T4:rvd.exe -listen tcp:7500
### 테스트 과정
- T3을 실행하면, T1에 아래와 같이 출력됨.
```
  DEBUG: group=MyDQGroup workerWeight=1 workerTasks=1 schedulerWeight=1 heartbeat=1.500000 activation=3.500000 backlogMsgs=50
  tibrvdqlisten.exe: Listening to subject test.subject
  subject=_RV.INFO.RVCM.QUEUE.SCHEDULER.ACTIVE.MyDQGroup, reply=none, message={ADV_CLASS="INFO" ADV_SOURCE="RVCM" ADV_NAME="QUEUE.SCHEDULER.ACTIVE.MyDQGroup" name="MyDQGroup"}, certified= FALSE
  subject=test.subject, reply=none, message={DATA="X"}, certified= TRUE, sequence=4
  subject=_RV.WARN.RVCM.QUEUE.SCHEDULER.OVERFLOW.MyDQGroup, reply=none, message={ADV_CLASS="WARN" ADV_SOURCE="RVCM" ADV_NAME="QUEUE.SCHEDULER.OVERFLOW.MyDQGroup" name="MyDQGroup" subject="test.subject" sender="RVCMPUB"     seqno=55 reason="Task backlog limit reached."}, certified= FALSE
  subject=_RV.WARN.RVCM.QUEUE.SCHEDULER.OVERFLOW.MyDQGroup, reply=none, message={ADV_CLASS="WARN" ADV_SOURCE="RVCM" ADV_NAME="QUEUE.SCHEDULER.OVERFLOW.MyDQGroup" name="MyDQGroup" subject="test.subject" sender="RVCMPUB"   seqno=55 reason="Task backlog limit reached."}, certified= FALSE
  subject=test.subject, reply=none, message={DATA="X"}, certified= TRUE, sequence=6
  subject=test.subject, reply=none, message={DATA="X"}, certified= TRUE, sequence=8
  subject=test.subject, reply=none, message={DATA="X"}, certified= TRUE, sequence=10
  subject=test.subject, reply=none, message={DATA="X"}, certified= TRUE, sequence=12
```
- task queue의 backlogMsgs limit을 50으로 잡아서, ADV_NAME="QUEUE.SCHEDULER.OVERFLOW.MyDQGroup" name="MyDQGroup" subject="test.subject" sender="RVCMPUB" seqno=55 reason="Task backlog limit reached  
