@ERASE

0 REM BlueSpells Scanner Version 0.1

@INIT 1
1 PRINTU "\n\n\n\rBLUESPELLS"
2 PRINTU " SENSOR V0.1\n\r"
0 REM Set LED on
3 A = pioset 20
0 REM Baud rate for serial line
4 A = baud 1152
0 REM Debug toggle
5 Z = 0
0 REM power for scanning in dBm
6 REM A = defpower 10
400 RESERVED
500 001E377281F0
502 0025BF100359
503 0025BF100357
7 M = 0
8 I = 0
9 A = exist "found.log"
10 IF A = 0 THEN 12
11 F = delete "found.log"
12 F = open "found.log"
13 A = getaddr
14 PRINTV "\n"
15 B = strlen $0
16 F = write B
17 F = close
18 A = zerocnt
19 PRINTU $0
29 RETURN

@IDLE 30
30 REM A = slave -1
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
907 PRINTS "SIZE ERROR: ";
908 PRINTS A;
909 PRINTS "\n\r";
910 GOTO 939;
911 A = pioset 20
912 A = pioclr 20;
913 PRINTV " ";
914 A = readcnt;
915 PRINTV A;
916 PRINTV "\n";
917 F = append "found.log";
918 Q = size;
0 REM If file is too large, we just have to let the results go :(
919 IF Q > 8000 THEN 939;
920 B = strlen $0;
921 F = write B;
922 PRINTS $0;
923 PRINTS "\r";
930 F = close;
939 RETURN

@SLAVE 940
940 A = slave -1
941 REM A = link 2 
942 A = getaddr
943 PRINTS $0
945 PRINTS " CONNECTED\n\r"
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
62 IF M > 10 THEN 100
63 PRINTS "\n\r#"
64 PRINTS M
65 PRINTS "  "
66 A = getms
67 PRINTS A
68 PRINTS "  "
69 A = getaddr
70 PRINTS $0
71 PRINTS "  "
72 PRINTS "\n\rPress 's' "
75 PRINTS "to stop\n\r"
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
93 A = inquiry -10
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
195 A = reboot
196 ALARM 1
197 RETURN


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
845 A = disconnect 1
846 ALARM 1
847 E = 0
849 RETURN 
