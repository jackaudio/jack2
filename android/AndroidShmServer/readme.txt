IAndroidShm의 기능을 테스트 하기 위한 용도로 사용되는 프로그램입니다.

AndroidShm은 service manager에 shared memory를 할당해주는 서비스입니다.

service name: com.sec.apa.IAndroidShm

실행 파일:
/system/bin/androidshmservice

-------------------------------------------
./test

AndroidShmService를 테스트하는 프로그램

/system/bin/shmservicetest

AndroidShmService에서 제공하는 기능을 UnitTest합니다.

동작 확인 방법:
adb logcat 으로 로그로 성공/실패 확인함.

전제 조건:
/system/bin/androidshmservice를 실행중인 상태이어야 합니다.