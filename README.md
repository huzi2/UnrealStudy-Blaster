# UnrealStudy-Blaster

<img width="765" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/3beb7b00-3725-408b-8668-06671b06e428">

## 프로젝트 개요

- 언리얼 5.3 C++을 활용한 멀티플레이 3인칭 슈팅 게임
  
## 제작 기간

- 24.01.31 ~ 24.03.31 (2개월)

## 개발 환경

- Unreal Engine 5.3
- C++
- IDE : Visual Studio 2022

## 사용 기술

### 멀티플레이
- 세션 생성, 참가, 시작
- 변수 레플리케이션
- RPC 함수
- 서버와 클라이언트 간 시간 동기화
  - <img width="515" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/80b1adae-2516-4032-984a-6b59b99436fd">
- 서버 되감기로 핑이 높은 클라이언트의 사격을 보정
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/24c26d5d-5e11-45db-b567-347595003caf)
  - 서버에서 클라이언트의 히트박스를 일정 시간동안 저장
  - 클라이언트가 사격할 때의 시간을 서버에게 보내서 서버가 해당 시간의 히트박스를 확인해서 충돌 확인
  - 클라이언트의 핑이 높아도 정확한 사격이 가능해짐
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/0aa156f7-2bcc-4024-8c50-8642d0931657)
  - 발사체도 발사체 예측(PredictProjectilePath)을 통해서 발사체의 이동 경로를 확인 후 서버 되감기를 통해서 충돌 확인

### 게임 로직
- 게임 모드의 매치 기능을 활용한 매치 상태 구현
  - <img width="900" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/75625170-943f-4be5-9d81-f77de72ad23e">
  - 매치 시작 전 대기 상태
  - <img width="1472" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/1e956a8c-0324-4254-8dbe-086c46dee989">
  - 매치 중
  - <img width="523" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/1387343c-e206-4b39-a2bc-a06cfa807308">
  - 매치 사이의 대기 시간(커스텀 매치 구현). 최고 득점자 출력
- 팀 게임 모드 구현
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/1134b35f-1b6c-4ab6-ba2b-0f835bc27da6)
  - 팀별로 다른 머티리얼 사용하도록 함. 팀별로 플레이어스타트 지정. 게임스테이트에서 팀 점수 구현
- 팀 깃발 뺏기 게임 모드 구현
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/2f17f478-1e29-4ec9-931c-97c749a39150)
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/745d5082-245f-416b-9733-4cd5739eb9ed)
  - 적 팀의 깃발을 뺏어서
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/0652ae97-f57c-482f-925b-a951c4d32324)
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/7287552a-b748-4fae-88cd-eabcbf8e2b51)
  - 아군 깃발 존에 넣으면 득점하도록 구현
- 공격 시 탄피 구현
  - <img width="833" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/1dfa9cce-e4cc-41db-ae1f-0d2021a07b20">
  - <img width="208" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/4960afa7-ed0c-462a-be8d-b2e37001d662">
- 다양한 무기 구현
  - <img width="271" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/2e5e1a26-11a5-4a66-8151-dc3bce96f46b">
  - 발사체 연사할 수 있는 소총
  - <img width="502" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/bef1cd67-0aeb-461e-8e89-6505a38c99b1">
  - 범위 공격 발사체 로켓 런처
  - <img width="232" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/497406dd-5ac1-40f1-86ac-8830422756b8">
  - 라인 트레이스 권총
  - <img width="238" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/a66b5764-a4f3-4b3d-ab7b-a1aa926c5654">
  - 라인 트레이스 저격총
  - <img width="296" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/84049de1-5952-42bc-86a0-cbd028ec7721">
  - 라인 트레이스가 퍼지는 연사할 수 있는 기관단총
  - <img width="338" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/b16512df-dea0-4650-affd-f7e7eb424418">
  - 방사된 라인 트레이스 여러개를 쏘는 샷건
  - <img width="648" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/acdb6a5a-c911-4932-b12f-8f223528771b">
  - 투사체가 튕기는 유탄 발사기
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/c6bde558-75a3-4e10-9c8c-5e5429a7d31d)
  - 수류탄
- 주무기, 보조무기 구현
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/82091042-8e14-4d69-8648-49e502dd0dc5)
  - 보조무기는 등에 붙이고 E키를 통해 무기를 스왑
- 피직스 에셋을 활용한 메시 일부분 물리 적용
  - <img width="550" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/8262c8d6-1d02-43a4-b611-ca31bc32fbd4">
  - <img width="179" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/7cc1f458-d386-4805-81cd-da6c6ed251f2">
- 필드 내 아이템 구현
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/c7af4336-a5f0-4e70-8d6d-279d783ee941)
  - 체력 회복, 보호막, 이동속도, 점프높이 등 다양한 버프형 아이템 구현
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/70668899-bf3c-46cb-9957-5e440b1b3bce)
  - 무기 탄약 아이템
- 헤드샷 구현
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/a466c38a-00e8-4c9c-b907-353eb8e617ba)
  - 충돌한 히트박스를 확인해서 헤드(빨간색 박스)면 헤드샷으로 처리하도록 구현
- 향상된 입력

### 프로그래밍
- 커스텀 플러그인(멀티플레이어 세션 관리 플러그인) 추가

### 애니메이션
- 애니메이션 블루프린트 활용
  - <img width="517" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/4c2443f0-8a14-4bbb-af08-04f396988400">
  - <img width="785" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/0691b4ad-5e99-4985-a74a-b34aedab8f7e">
  - 무기를 장착한, 해제한 상태, 웅크리기 상태 등을 분리하고 조준 방향으로 몸을 움직이고 에임오프셋 등을 구현
- 에임오프셋
- FABRIK

### UI
- 로비 메뉴 UI
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/48ad0cd3-d64d-42ae-959f-e0612333ff16)
  - 게임 인원과 게임 모드(개인전, 팀전, 깃발뺏기)를 선택해서 들어갈 수 있음
- 전체적인 게임 HUD
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/8c5dbdee-4996-4fae-ac56-ac536f0080b7)
  - 체력, 보호막, 타이머, 킬데스, 탄약, 여분 탄약, 수류탄 표시
- 십자선
  - <img width="242" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/92ebe8de-3d6c-4cca-8fcb-a4a05283a80a">
  - 평소 상태의 십자선
  - <img width="517" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/98b89a5c-e088-4f56-8a7d-0a6bba94a3ec">
  - 조준 상태의 십자선
  - <img width="640" alt="image" src="https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/1587cfa1-95a7-44ab-9408-39a42c9cc162">
  - 저격총용 스코프 십자선(위젯 애니메이션으로 구현)
- 처치 메시지
  - ![image](https://github.com/huzi2/UnrealStudy-Blaster/assets/31639085/de7a62f3-b59d-4c39-ba26-32069b223675)
  - 처치한 클라이언트와 죽은 클라이언트가 각자에게 맞는 메시지를 띄우도록 구현

