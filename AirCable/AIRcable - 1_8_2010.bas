@ERASE

0 REM BlueSpells Scanner Version 0.1

@INIT 1
2 PRINTU "\n\n\n\rBLUESPELLS"
3 PRINTU " SENSOR V0.1\n\r"
0 REM Set LED on
5 A = pioset 20
0 REM Baud rate for serial line
6 A = baud 1152
0 REM Debug toggle
7 Z = 0
0 REM power for scanning in dBm
8 REM A = defpower 10
400 RESERVED
500 001E377281F0
502 0025BF100359
503 0025BF100357
10 M = 0
11 I = 0
0 REM PRINTU "Press 's' "
0 REM PRINTU "to start "
0 REM $300[0] = 0
0 REM TIMEOUTU 1
0 REM INPUTU $300
0 REM IF $300[0] <> 115 THEN 20;
18 A = zerocnt
19 RETURN

@IDLE 20
20 REM A = slave -1
21 PRINTU "\n\rIDLE\n\r"
22 REM A = pioset 20
23 A = pioclr 20
24 WAIT 1
25 REM A = pioset 20
48 ALARM 1
49 RETURN

@SLAVE 800 
800 A = link 2 
805 PRINTM "SPP CONNECTED\n\r"
810 A = getaddr
815 $0 = $503
820 A = strcmp A
825 IF A = 0 GOTO 894
830 A = master $503
894 ALARM 4
895 RETURN


@MASTER 500
500 REM A = unlink 2
505 A = link 3
510 A = pioset 20
520 A = pioclr 20
525 A = pioset 20
530 A = pioclr 20
540 REM ALARM 10
595 RETURN

@ALARM 50
50 IF I <> 0 THEN 54
51 A = slave 10
52 WAIT 11
53 I = 1
54 A = status
55 IF A <> 0 THEN 60
56 A = pioclr 20
57 A = pioset 20
58 A = slave -20
59 GOTO 94
60 A = pioclr 20
61 M = M + 1
62 IF M = 11 THEN 100
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
76 $300[0] = 0
77 REM TIMEOUTM 1
78 REM INPUTM $300
79 REM IF $300[0] = 0 GOTO 85
80 REM PRINTS $300
81 $300[0] = 0
85 TIMEOUTS 1
86 INPUTS $300
87 REM TIMEOUTU 1
90 REM INPUTU $300
91 IF $300[0] = 115 THEN 100
92 REM A = master $503
93 A = inquiry -10
94 ALARM 18
99 RETURN

@INQUIRY 900
0 REM Single LED blink to show device found
900 A = status;
901 IF A <> 0 THEN 910;
902 A = cancel
903 GOTO 995;
910 A = pioset 20
915 A = pioclr 20;
920 A = readcnt;
925 PRINTS A;
930 PRINTS " ";
940 PRINTU $0;
950 PRINTU "\n\r";
960 PRINTS $0;
970 PRINTS "\n\r";
995 RETURN


100 PRINTS "\n\rTERMINATED\n\r"
101 PRINTS "\n\rPress 'r' "
102 PRINTS "to reboot\n\r"
103 PRINTS "\n\rPress 'd' "
104 PRINTS "to disconnect\n\r"
105 PRINTS "\n\rPress any "
106 PRINTS "other key "
107 PRINTS "to resume\n\r"
108 $300[0] = 0
109 A = status
110 IF A <> 0 THEN 113
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
135 GOTO 50
150 A = getaddr
155 PRINTS $0
160 PRINTS "  "
165 PRINTS "DISCONNECTING\n\r"
170 A = disconnect 0
175 WAIT 3
180 GOTO 50
190 PRINTS "REBOOTING\n\r"
195 A = reboot
196 RETURN