# apsudo
권한 통재된 시스템을 위한 승인 주도 권한 상승 프로그램

## 사용
```
apsudo <옵션> <명령어>
```

## 개념
apsudo를 사용하여 명령어를 수행하려 시도하면 중앙 서버에 승인 요청이 가서 승인 또는 거절이 전자서명을 통해서 전달되고 명령어를 수행하게된다.
이때 중앙서버에서는 규칙 테이블에 따라 승인을 해준다. 다음과 같이 프로그램에 따라 규칙을 부여한다.

| Program | Argument | Rule |
| ---- | ------- | -------- |
| rm | | Deny | 
| chown | | Deny |
| chmod | | Deny | 
| apt | | Pend |
| mkdir | | Approve | 
| apt | update | Approve |

위와 같이 Pend인 경우에만 관리자가 승인해주면 Pending리스트에 있는 명령어가 Approve queue에 들어가서 연결이 되있으면 토큰을 전송해 명령어를 수행하도록한다.

## 예제
```
$ apsudo mkdir dir
Approved!
$ apsudo apt install gcc
Pending!
Run 'apsudo --ls-pending' to see approved commend.
$ apsudo --ls-pending
1 Pending
'apt install gcc'
1 Approved
'apt install vim'
Run 'apsudo --run' to run approved commend.
$ apsudo --run
Running 'apt install vim'
....
Complete!
```
