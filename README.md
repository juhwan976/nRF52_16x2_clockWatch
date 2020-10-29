# nRF52_16x2_clockWatch
# 
## 설명
- nRF52DK(52832)와 1602 Character LCD를 이용한 전자시계.
- 일반적인 1602 Character LCD는 너무 많은 핀을 사용해서 추가로 __I2C 통신을 할 수 있는 모듈 사용__.
- 사용 가능한 기능
  1. 시간 표시 (초기화면)
  
      <img src="https://github.com/juhwan976/nRF52_16x2_clockWatch/blob/main/photo/1.png" width="420" />
      
  2. 스탑 워치
    
      <img src="https://github.com/juhwan976/nRF52_16x2_clockWatch/blob/main/photo/2.png" width="500" />
    
  3. 알람 설정 및 울림
  
      <img src="https://github.com/juhwan976/nRF52_16x2_clockWatch/blob/main/photo/3.png" width="500" />
      
      
      - 알람이 울릴 때
      
          <img src="https://github.com/juhwan976/nRF52_16x2_clockWatch/blob/main/photo/5.png" width="300" />
      
      
  4. 시간 변경
  
      <img src="https://github.com/juhwan976/nRF52_16x2_clockWatch/blob/main/photo/4.png" width="500" />
      
- 시연 영상

  https://youtu.be/r9JvFbu81vI

#
## 개발환경 및 사용언어
- 개발환경
  - SEGGER Embedded Studio for ARM v4.52c
- 사용언어
  - C
#
## 설치 및 실행
- __Segger Embedded Studio for ARM 필요__
1. 폴더 안의 LCD.emProject 실행
2. LCD의 SCL을 P0.27, SDA를 P0.26, VCC를 5V, GND를 GND에 각각 연결
3. nRF52DK의 전원을 킨다
4. F5를 누른다
