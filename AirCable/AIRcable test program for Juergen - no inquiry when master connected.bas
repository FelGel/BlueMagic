@ERASE

@INIT 1
0 REM Set LED on
1 A = pioset 20
0 REM Baud rate for serial line
2 A = baud 1152
0 REM Debug toggle
7 Z = 0

0 REM SET MASTER ADDRESS
501 0025BF100308

24 A = zerocnt
26 STTYS 2
29 RETURN



@IDLE 30
33 A = pioclr 20
48 ALARM 1
49 RETURN



@INQUIRY 900
0 REM Check connection status
900 GOSUB 800;
901 IF E <> 0 THEN 905;
0 REM Cancel INQUIRY if disconnected
902 A = cancel;
903 GOTO 939;
0 REM Blink LED
911 A = pioset 20
912 A = pioclr 20;
0 REM Print INQUIRY result
936 PRINTS $0;
937 PRINTS "\n\r"
939 RETURN



@SLAVE 940
943 A = getaddr
944 PRINTS $0
945 PRINTS " CONNECTED\n\r"
950 PRINTS "MASTER " 
955 PRINTS "DISCONNECTED\n\r"
966 PRINTS "Press 'm' to "
967 PRINTS "toggle MASTER "
968 PRINTS "connection\n\r\n\r"
971 A = pioclr 20
978 ALARM 3
979 RETURN



@MASTER 1000
1000 A = pioset 20
1005 A = pioclr 20
1015 RETURN



@ALARM 50
0 REM Check connection status
50 GOSUB 800
51 IF E <> 0 THEN 60
0 REM NO connection. Turn the LED ON and wait for connection
53 A = pioset 20
55 A = slave 20
57 GOTO 98

0 REM CONNECTED
60 A = pioclr 20
65 TIMEOUTS 1
66 INPUTS $0
0 REM Check for 'm' key press
68 IF $0[0] = 109 THEN 70
0 REM 'm' NOT pressed 
69 GOTO 90

0 REM 'm' pressed
70 IF E = 1 THEN 75
0 REM MASTER connected -> disconnect
71 PRINTS "\n\rDISCONNECTING "
72 PRINTS "MASTER\n\r\n\r" 
73 A = disconnect 1
74 GOTO 90

0 REM MASTER disconnected -> connect
75 A = master $501
76 WAIT 4
0 REM Check if connected to MASTER
77 GOSUB 800
82 IF E <> 3 THEN 88
0 REM Connected
83 PRINTS "\n\rMASTER "
84 PRINTS "CONNECTED\n\r\n\r"
87 GOTO 90
0 REM Not connected
88 PRINTS "\n\rCONNECTION "
89 PRINTS "FAILED\n\r\n\r"

0 REM Run RSSI INQUIRY
90 PRINTS "Starting INQUIRY. "
91 A = inquiry -5
93 PRINTS " TIME: "
94 A = readcnt
95 PRINTS A
96 PRINTS "\n\r"
98 ALARM 7
99 RETURN

0 REM CONNECTION STATUS
800 A = status
801 E = 0
802 IF A < 10000 THEN 805
803 A = A - 10000
805 IF A < 1000 THEN 810
806 A = A - 1000
810 IF A < 100 THEN 820
815 A = A - 100
820 IF A < 10 THEN 830
823 E = E + 2
825 A = A - 10
830 IF A < 1 THEN 840
835 E = E + 1
840 IF E <> 2 THEN 849
0 REM Disconnect MASTER if SLAVE disconnected
845 A = disconnect 1
847 E = 0
849 RETURN 
