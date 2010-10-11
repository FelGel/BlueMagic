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
13 A = getaddr
14 REM IF $0 <> "0025BF100359" THEN 16
15 REM N = 2 
16 REM IF $0 <> "0025BF100357" THEN 18
17 REM N = 3
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

@SLAVE 940
941 REM A = slave 0
942 REM A = link 2 
943 A = getaddr
944 REM PRINTS $0
945 REM PRINTS " CONNECTED\n\r"
950 A = getaddr
953 REM $0 = $503
955 REM A = strcmp A
965 REM IF A = 0 GOTO 978
970 A = master $501
971 A = pioclr 20
972 A = slave -1
978 ALARM 1
979 RETURN


@MASTER 980
980 REM A = unlink 2
985 REM A = link 3
990 A = pioset 20
995 A = pioclr 20
1000 A = pioset 20
1005 A = pioclr 20
1010 REM ALARM 10
1015 RETURN

@ALARM 50
50 GOSUB 800
51 IF E = 3 THEN 60
52 REM A = pioclr 20
53 A = pioset 20
54 IF I <> 0 THEN 58
55 A = slave 20
56 I = 1
57 GOTO 98
58 A = slave -20
59 GOTO 98
60 A = pioclr 20
65 TIMEOUTS 1
67 INPUTS $0
69 PRINTM $0[0]
70 IF $0[0] <> 54 THEN 75
73 GOSUB 750
75 GOSUB 780
85 TIMEOUTM 3
86 INPUTM $0
91 IF $0[0] = 0 THEN 98
95 PRINTS $0
96 PRINTS "\n\r"
98 ALARM 1
99 RETURN

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
795 PRINTS "\n\r"
799 RETURN