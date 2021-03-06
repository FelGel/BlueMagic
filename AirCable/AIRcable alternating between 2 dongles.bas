@ERASE

0 REM BlueSpells Scanner Version 0.1

@INIT 1
0 REM Set LED on
1 A = pioset 20
0 REM Baud rate for serial line
2 A = baud 1152
0 REM SENSOR_ID
3 N = 2
0 REM INQUIRY SCAN COUNTER
4 M = 0
0 REM IS_DISCOVERABLE 
5 I = 0
0 REM VERSION
6 V = 1
0 REM Debug toggle
7 Z = 0
0 REM power for scanning in dBm
0 REM A = defpower 10
400 RESERVED
500 001E377281F0
501 0025BF100308
502 0025BF100359
503 0025BF100357
9 A = exist "found.log"
10 IF A = 0 THEN 12
11 F = delete "found.log"
12 F = open "found.log"
13 A = getaddr
14 REM IF $0 <> "0025BF100308" THEN 16
15 REM N = 1
16 REM IF $0 <> "0025BF100359" THEN 18
17 REM N = 2 
18 REM IF $0 <> "0025BF100357" THEN 21
19 REM N = 3
0 REM PRINTV " SENSOR_ID = "
0 REM PRINTV N
0 REM PRINTV "\n"
21 REM B = strlen $0
22 REM F = write B
23 F = close
24 A = zerocnt
25 PRINTU $0
26 STTYS 2
29 RETURN

@IDLE 30
30 REM A = slave 0
31 PRINTU "\n\rIDLE\n\r"
33 A = pioclr 20
48 ALARM 1
49 RETURN

@INQUIRY 900
0 REM Single LED blink to show device found
900 GOSUB 800;
901 IF E <> 0 THEN 905;
902 A = cancel;
903 GOTO 939;
905 A = strlen $0;
906 IF A = 21 THEN 911;
907 REM PRINTS "SIZE ERROR: ";
908 REM PRINTS A;
909 REM PRINTS "\n\r";
910 GOTO 939;
911 A = pioset 20
912 A = pioclr 20;
913 $300 = $0;
914 $0[0] = "1";
915 PRINTV " ";
916 PRINTV N;
917 PRINTV " ";
918 A = readcnt;
919 PRINTV A;
921 $301 = $300[17];
922 PRINTV $301;
923 PRINTV " ";
924 $300[12] = 0;
925 REM $301 = $300[0];
926 PRINTV $300;
927 PRINTV "\n\r";
931 F = append "found.log";
932 Q = size;
0 REM If file is too large, we just have to let the results go :(
933 IF Q > 8000 THEN 939;
934 B = strlen $0;
935 F = write B;
936 PRINTS $0;
937 REM PRINTS "\r";
938 F = close;
939 RETURN

@SLAVE 940
941 REM A = slave 0
942 REM A = link 2 
943 REM A = getaddr
944 REM PRINTS $0
945 REM PRINTS " CONNECTED\n\r"
950 A = getaddr
953 REM $0 = $501
955 REM A = strcmp $501
965 REM IF A = 0 GOTO 977
970 REM GOSUB 280
971 A = pioclr 20
972 REM GOSUB 750
977 REM A = slave -1
978 ALARM 2
979 RETURN


@MASTER 980
980 REM A = unlink 2
985 REM A = link 3
990 A = pioset 20
995 A = pioclr 20
1000 A = pioset 20
1001 A = pioclr 20
1005 A = 6
1007 REM PRINTM A
1010 REM ALARM 10
1015 RETURN

@ALARM 50
50 GOSUB 800
51 IF E <> 0 THEN 60
52 C = 20
53 A = pioset 20
54 IF I <> 0 THEN 58
55 A = slave 20
56 I = 1
57 GOTO 98
58 A = slave -20
59 GOTO 98
60 A = pioclr 20
64 $300[0] = 0
65 TIMEOUTS 1
66 INPUTS $300
67 PRINTM $300[0]
68 IF $300[0] = 109 THEN 70
69 GOTO 71
70 A = disconnect 1
71 IF $300[0] = 115 THEN 100
72 IF $300[0] <> 54 THEN 76
0 REM SEND BTB_INFO
73 REM GOSUB 280
74 REM PRINTM "6"
75 GOSUB 750
0 REM SEND KEEP_ALIVE
76 GOSUB 780
77 IF E = 1 THEN 84
78 $0[0] = 0
79 TIMEOUTM 1
80 INPUTM $0
81 IF $0[0] = 0 THEN 84
82 PRINTS $0
83 PRINTS "\n\r"
84 GOTO 200
98 ALARM 1
99 RETURN

100 PRINTS "\n\rTERMINATED\n\r"
101 PRINTS "\n\rPress 'r' "
102 PRINTS "to reboot\n\r"
103 PRINTS "\n\rPress 'd' "
104 PRINTS "to disconnect\n\r"
105 PRINTS "\n\rPress any "
106 PRINTS "other key "
107 PRINTS "to resume\n\r"
108 $300[0] = 0
109 GOSUB 800
110 IF E <> 0 THEN 113
111 ALARM 1
112 RETURN
113 TIMEOUTS 1
114 INPUTS $300
119 IF $300[0] = 0 THEN 109;

120 PRINTS $300;
121 PRINTS "\n\r"
122 IF $300[0] = 114 THEN 190
123 IF $300[0] = 100 THEN 150
125 M = 0
130 PRINTS "\n\rRESUMING\n\r"
135 GOTO 196
150 A = getaddr
155 PRINTS $0
160 PRINTS "  "
165 PRINTS "DISCONNECTING\n\r"
170 A = disconnect 0
175 WAIT 3
180 GOTO 196
190 PRINTS "REBOOTING\n\r"
191 A = disconnect 1
192 A = disconnect 0
195 A = reboot
196 ALARM 1
197 RETURN




0 REM BTB_INFO
750 PRINTS "3 "
751 PRINTS N
752 PRINTS " "
753 A = readcnt
754 PRINTS A
755 PRINTS " "
756 PRINTS V
757 PRINTS " "
758 A = getaddr 
759 PRINTS $0
760 PRINTS "\n\r"
779 RETURN


0 REM KEEP_ALIVE
780 PRINTS "0 "
781 PRINTS N
782 PRINTS " "
783 A = readcnt
784 PRINTS A
785 PRINTS " "
786 PRINTS C
795 PRINTS "\n\r"
799 RETURN

0 REM CONNECTION STATUS
800 A = status
801 E = 0
802 IF A < 10000 THEN 805
803 A = A - 10000
804 REM E = E + 4
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
845 A = disconnect 1
846 REM ALARM 1
847 E = 0
849 RETURN 


0 REM DONGLES TOGGLE 
200 IF C < 18 THEN 274
245 A =  disconnect 1
246 WAIT 1
247 TIMEOUTM 1
250 INPUTM $0
270 C = 0
272 A = inquiry -20
274 C = C + 1
275 IF C <> 9 THEN 279
277 GOSUB 280
279 GOTO 98

0 REM CONNECT to MASTER
280 TIMEOUTM 1
285 INPUTM $0
290 A = master $501
299 RETURN

