@ERASE

0 REM BlueSpells Scanner Version 0.1

@INIT 1
0 REM Set LED on
1 A = pioset 20
0 REM Baud rate for serial line
2 A = baud 1152
0 REM SENSOR_ID
3 N = 10
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
502 0025BF100359
503 0025BF100357
9 A = exist "found.log"
10 IF A = 0 THEN 12
11 F = delete "found.log"
12 F = open "found.log"
13 A = getaddr
14 REM IF $0 <> "0025BF100359" THEN 16
15 REM N = 2 
16 REM IF $0 <> "0025BF100357" THEN 18
17 REM N = 3
18 PRINTV " SENSOR_ID = "
19 PRINTV N
20 PRINTV "\n"
21 B = strlen $0
22 F = write B
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
940 A = slave -1
941 REM A = slave 0
942 REM A = link 2 
943 A = getaddr
944 REM PRINTS $0
945 REM PRINTS " CONNECTED\n\r"
950 A = getaddr
953 $0 = $503
955 A = strcmp A
965 IF A = 0 GOTO 978
970 REM A = master $503
971 A = pioclr 20
978 ALARM 2
979 RETURN


@MASTER 980
980 REM A = unlink 2
985 A = link 3
990 A = pioset 20
995 A = pioclr 20
1000 A = pioset 20
1005 A = pioclr 20
1010 REM ALARM 10
1015 RETURN

@ALARM 50
50 GOSUB 800
51 IF E <> 0 THEN 60
52 A = pioclr 20
53 A = pioset 20
54 IF I <> 0 THEN 58
55 A = slave 20
56 I = 1
57 GOTO 98
58 A = slave -20
59 GOTO 98
60 A = pioclr 20
61 M = M + 1
62 IF M > 100 THEN 100
63 REM PRINTS "\n\r#"
64 REM PRINTS M
65 REM PRINTS "  "
66 A = getms
67 REM PRINTS A
68 REM PRINTS "  "
69 A = getaddr
70 REM PRINTS $0
71 REM PRINTS "  "
72 REM PRINTS "\n\rPress 's' "
75 REM PRINTS "to stop\n\r"
0 REM $300[0] = 0
0 REM TIMEOUTM 1
0 REM INPUTM $300
0 REM IF $300[0] = 0 GOTO 85
0 REM PRINTS $300
84 $300[0] = 0
85 TIMEOUTS 1
86 INPUTS $300
87 REM TIMEOUTU 1
90 REM INPUTU $300
91 IF $300[0] = 115 THEN 100
92 IF $300[0] <> 54 THEN 96
93 GOSUB 750
96 GOSUB 780
97 A = inquiry -10
98 ALARM 18
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
846 ALARM 1
847 E = 0
849 RETURN 
