/*
 * ftCmSend.c - RVFT + RVCM Certified Message Sender Example
 * 이 프로그램은 Fault Tolerance 그룹에 참여하며, Active 멤버일 때만
 * Certified Message를 전송합니다. Active/Standby 서버는 동일한 CM 이름과
 * 공유된 Ledger 파일을 사용해 메시지 전송의 연속성을 보장합니다.
 * TIBCO Rendezvous 8.7.0 기반
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

// TIBCO RV 라이브러리 경로에 맞게 수정
#include <c:/tibco/tibrv/8.7/include/tibrv/tibrv.h>
#include <c:/tibco/tibrv/8.7/include/tibrv/ft.h>
#include <c:/tibco/tibrv/8.7/include/tibrv/cm.h>

static char progname[] = "ftCmSend";

/* --- 전역 변수 --- */
// (CM Transport와 FT Transport를 분리하지 않고 하나의 Transport로 사용)
static tibrvTransport transport = TIBRV_INVALID_ID;
static tibrvcmTransport cmtransport = TIBRV_INVALID_ID;

static char* subjectName = NULL;
static char* messageText = NULL;
static char* cmName = "FT_CM_SENDER"; // 기본 CM 이름
static char* ledgerFile = "ft_cm_sender.ledger"; // 기본 Ledger 파일 (공유 필수)
static double sendInterval = 10.0;
static tibrv_u32 roundsNum = 0;
static tibrv_u32 msgCount = 0;

static tibrvMsg send_message = TIBRV_INVALID_ID;
static tibrvEvent timerId = TIBRV_INVALID_ID;
static tibrvEvent advId = TIBRV_INVALID_ID;
static tibrvftMember groupId = TIBRV_INVALID_ID;
static tibrv_u32 ftWeight = 50;


/* --- 데이터 구조체 --- */
typedef struct
{
    tibrvEvent timerId;
    char* cmName;
    char* ledgerFile;
} AppData;

static AppData app_data;


/* --- 함수 선언 --- */
static tibrv_status get_InitParms(int argc, char** argv, char** subject);
static void advCB(tibrvEvent listener, tibrvMsg message, void* closure);
static void sendTimerCB(tibrvEvent timer, void* closure);
static void processInstruction(tibrvEvent ftMember, tibrv_i32 instruction, void* closure);
static void sig_handler(int signo);


/* --- 콜백 함수 구현 --- */

/**
 * @brief Certified Message 전송 타이머 콜백
 * Active 상태일 때만 주기적으로 호출되어 CM 메시지를 전송합니다.
 */
static void
sendTimerCB(tibrvEvent timer, void* closure)
{
    tibrv_status err;
    char buffer[256];

    // 1. 메시지 내용 업데이트 (시퀀스 번호 추적용)
    msgCount++;
    snprintf(buffer, sizeof(buffer), "%s (Seq: %u, Time: %ld)", messageText, msgCount, (long)time(NULL));

    err = tibrvMsg_UpdateString(send_message, "DATA", buffer);
    if (err != TIBRV_OK)
    {
        fprintf(stderr, "%s: Failed to update message string --%s\n", progname, tibrvStatus_GetText(err));
        return;
    }

    // 2. Certified Message 전송
    err = tibrvcmTransport_Send(cmtransport, send_message);
    if (err != TIBRV_OK)
    {
        fprintf(stderr, "%s: Failed to send CM message --%s\n", progname, tibrvStatus_GetText(err));
        // TIBCO 권장: 전송 실패 시 CM Transport를 파괴하고 다시 생성해 재연결 시도 로직 필요 (예제 단순화를 위해 생략)
    }
    else
    {
        fprintf(stderr, "%s: ACTIVE - Sent CM message: \"%s\" to subject \"%s\"\n", progname, buffer, subjectName);
    }

    // 3. 라운드 수 확인
    if (roundsNum > 0 && msgCount >= roundsNum)
    {
        fprintf(stderr, "%s: Completed %u rounds. Stopping.\n", progname, roundsNum);
        // 타이머 중지 및 프로그램 종료
        tibrvEvent_Destroy(timerId);
        timerId = TIBRV_INVALID_ID;
    }
}


/**
 * @brief RVFT 그룹 상태 변경 콜백 (Active/Standby 전환)
 */
static void
processInstruction(tibrvEvent ftMember, tibrv_i32 instruction, void* closure)
{
    AppData* data = (AppData*)closure;
    tibrv_status err;

    switch (instruction)
    {
        case TIBRVFT_ACTIVATE:
            fprintf(stderr, "%s: >>>>> TIBRVFT_ACTIVATE: Becoming ACTIVE CM Sender. <<<<<\n", progname);

            // 1. CM Transport 생성 (Active 인계 시점)
            err = tibrvcmTransport_Create(&cmtransport, transport, data->cmName, data->ledgerFile, TIBRV_TRUE, NULL);
            if (err != TIBRV_OK)
            {
                fprintf(stderr, "%s: Failed to create CM Transport --%s\n", progname, tibrvStatus_GetText(err));
                // 심각한 오류이므로 Active 권한을 포기해야 할 수 있음.
                break;
            }
            fprintf(stderr, "%s: CM Transport created with Name: %s and Ledger: %s\n", progname, data->cmName, data->ledgerFile);


            // 2. CM 전송 타이머 시작
            if (timerId == TIBRV_INVALID_ID)
            {
                err = tibrvEvent_CreateTimer(&timerId, TIBRV_DEFAULT_QUEUE, sendTimerCB, sendInterval, NULL);
                if (err != TIBRV_OK)
                {
                    fprintf(stderr, "%s: Failed to start CM send timer --%s\n", progname, tibrvStatus_GetText(err));
                    break;
                }
            }
            break;

        case TIBRVFT_DEACTIVATE:
            fprintf(stderr, "%s: ----- TIBRVFT_DEACTIVATE: Becoming STANDBY. -----\n", progname);
            
            // 1. CM 전송 타이머 중지 (메시지 전송 중단)
            if (timerId != TIBRV_INVALID_ID)
            {
                tibrvEvent_Destroy(timerId);
                timerId = TIBRV_INVALID_ID;
            }

            // 2. CM Transport 파괴 (Ledger 파일 해제)
            if (cmtransport != TIBRV_INVALID_ID)
            {
                tibrvcmTransport_Destroy(cmtransport);
                cmtransport = TIBRV_INVALID_ID;
            }
            break;

        case TIBRVFT_PREPARE_TO_ACTIVATE:
            fprintf(stderr, "%s: ***** TIBRVFT_PREPARE_TO_ACTIVATE: Preparing for activation. *****\n", progname);
            break;

        default:
            break;
    }
}

/**
 * @brief Advisory 메시지 처리 콜백
 */
static void
advCB(tibrvEvent listener, tibrvMsg message, void* closure)
{
    const char* advMsgString = NULL;
    tibrvMsg_ConvertToString(message, &advMsgString);
    fprintf(stderr, "\n#### RVFT ADVISORY: %s\n", advMsgString);
}


/**
 * @brief SIGINT (Ctrl+C) 핸들러
 */
static void
sig_handler(int signo)
{
    if (timerId != TIBRV_INVALID_ID) tibrvEvent_Destroy(timerId);
    if (advId != TIBRV_INVALID_ID) tibrvEvent_Destroy(advId);
    if (groupId != TIBRV_INVALID_ID) tibrvftMember_Destroy(groupId);
    if (cmtransport != TIBRV_INVALID_ID) tibrvcmTransport_Destroy(cmtransport);
    if (transport != TIBRV_INVALID_ID) tibrvTransport_Destroy(transport);
    if (send_message != TIBRV_INVALID_ID) tibrvMsg_Destroy(send_message);
    tibrv_Close();
    exit(0);
}


/* --- 파라미터 파싱 및 초기화 --- */
static tibrv_status
get_InitParms(int argc, char** argv, char** subject)
{
    // ... (기존 tibrvfttime.c의 파싱 로직을 FT/CM에 맞게 재구성) ...
    // 편의를 위해 파싱 로직을 단순화하고 기본값을 사용
    // 실제 사용 시에는 모든 옵션을 파싱해야 함

    int i = 1;
    char *serviceStr = NULL;
    char *networkStr = NULL;
    char *daemonStr = NULL;
    
    // FT 파라미터
    char *groupName = "TIBRVFT_CM_GROUP";
    tibrv_u32 numActive = 1;
    double hbInterval = 3.0;
    double prepareInterval = 10.0;
    double activateInterval = 20.0;
    
    // 사용자 옵션 파싱
    while (i < argc - 2) { // 최소한 CM Name, Ledger, Subject, Message가 필요
        if (strcmp(argv[i], "-service") == 0) {
            serviceStr = argv[++i];
        } else if (strcmp(argv[i], "-network") == 0) {
            networkStr = argv[++i];
        } else if (strcmp(argv[i], "-daemon") == 0) {
            daemonStr = argv[++i];
        } else if (strcmp(argv[i], "-cmname") == 0) {
            cmName = argv[++i];
        } else if (strcmp(argv[i], "-ledger") == 0) {
            ledgerFile = argv[++i];
        } else if (strcmp(argv[i], "-weight") == 0) {
            ftWeight = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-interval") == 0) {
            sendInterval = atof(argv[++i]);
        } else if (strcmp(argv[i], "-rounds") == 0) {
            roundsNum = atoi(argv[++i]);
        } else {
            i++;
        }
    }

    // 최종 위치 확인 (Subject와 Message)
    if (i + 1 >= argc) {
        fprintf(stderr, "\nUsage: %s [options] <subject> <message>\n", progname);
        fprintf(stderr, "  Options:\n");
        fprintf(stderr, "    -service <service>\n");
        fprintf(stderr, "    -network <network>\n");
        fprintf(stderr, "    -daemon <daemon>\n");
        fprintf(stderr, "    -cmname <cmname>      (Default: FT_CM_SENDER)\n");
        fprintf(stderr, "    -ledger <filename>    (Default: ft_cm_sender.ledger)\n");
        fprintf(stderr, "    -weight <number>      (Default: 50)\n");
        fprintf(stderr, "    -interval <seconds>   (Default: 10.0)\n");
        fprintf(stderr, "    -rounds <number>      (Default: 0, infinite)\n\n");
        return TIBRV_INVALID_ARG;
    }

    subjectName = argv[i];
    messageText = argv[i + 1];

    // TIBRV 초기화
    tibrv_status err = tibrv_Open();
    if (err != TIBRV_OK) return err;

    // Transport 생성 (FT와 CM 모두에 사용)
    err = tibrvTransport_Create(&transport, serviceStr, networkStr, daemonStr);
    if (err != TIBRV_OK) return err;
    
    // CM/FT 전역 데이터 설정
    app_data.cmName = cmName;
    app_data.ledgerFile = ledgerFile;

    fprintf(stderr, "%s: FT Group Name: %s, Weight: %u\n", progname, groupName, ftWeight);
    fprintf(stderr, "%s: CM Name: %s, Ledger: %s\n", progname, cmName, ledgerFile);
    fprintf(stderr, "%s: Sending Subject: %s, Message: \"%s\"\n", progname, subjectName, messageText);


    // FT 멤버 생성
    err = tibrvftMember_Create(&groupId, TIBRV_DEFAULT_QUEUE, processInstruction, 
                                transport, groupName, ftWeight, numActive, 
                                hbInterval, prepareInterval, activateInterval, 
                                &app_data);

    if (err != TIBRV_OK)
    {
        fprintf(stderr, "%s: Failed to join FT group - %s\n", progname, tibrvStatus_GetText(err));
        return err;
    }

    // Advisory 리스너 등록 (FT, CM Advisory 모니터링)
    err = tibrvEvent_CreateListener(&advId, TIBRV_DEFAULT_QUEUE, advCB, transport, 
                                    "_RV.*.RVCM.>", NULL); // CM Advisory
    if (err != TIBRV_OK) return err;
    err = tibrvEvent_CreateListener(&advId, TIBRV_DEFAULT_QUEUE, advCB, transport, 
                                    "_RV.*.RVFT.>", NULL); // FT Advisory
    if (err != TIBRV_OK) return err;
    
    // 초기 메시지 객체 생성 (재사용)
    err = tibrvMsg_Create(&send_message);
    if (err != TIBRV_OK) return err;
    err = tibrvMsg_SetSendSubject(send_message, subjectName);
    if (err != TIBRV_OK) return err;
    err = tibrvMsg_AddField(send_message, "DATA", TIBRV_DATA_TYPE_STRING, messageText, TIBRV_TRUE);
    if (err != TIBRV_OK) return err;

    return TIBRV_OK;
}


/* --- 메인 함수 --- */

int
main(int argc, char** argv)
{
    tibrv_status err;
    
    // Ctrl+C 시그널 핸들러 등록
    signal(SIGINT, sig_handler);
    
    // 1. 초기화 및 FT 그룹 가입
    err = get_InitParms(argc, argv, &subjectName);
    if (err != TIBRV_OK)
    {
        fprintf(stderr, "%s: Initialization failed --%s\n", progname, tibrvStatus_GetText(err));
        sig_handler(0); // 에러 발생 시 정리 후 종료
    }
    
    fprintf(stderr, "\n%s: Waiting for TIBRVFT_ACTIVATE...\n", progname);


    // 2. 이벤트 디스패치 루프
    while (tibrvQueue_Dispatch(TIBRV_DEFAULT_QUEUE) == TIBRV_OK)
    {
        // Active 상태가 되면 processInstruction 콜백에서 타이머가 시작되어 메시지를 전송함.
    }
    
    // 3. 종료 정리
    sig_handler(0);
    return 0;
}
