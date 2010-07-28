@ERASE

0 REM AIRscanner Version 1.0
0 REM Simple Bluetooth scanner written for AIRcable OS

@INIT 5
0 REM Set LED on
6 A = pioset 20
0 REM Baud rate for serial line
7 A = baud 1152
0 REM Start of result table
8 L = 900
0 REM Result Index
9 M = 0
0 REM Debug toggle
10 Z = 1
0 REM power for scanning in dBm
11 A = defpower 24
12 RETURN

@IDLE 15
0 REM Booted OK, turn off LED
15 A = pioclr 20
0 REM Delete old log, should only be done once
16 A = delete "found.log"
17 ALARM 1
18 RETURN

@ALARM 20
0 REM Use ALARM to keep INQUIRY running forever
20 A = inquiry -24
21 ALARM 1
22 RETURN

@INQUIRY 25
0 REM Single LED blink to show device found
25 A = pioset 20;
26 A = pioclr 20
0 REM Compare result to current table
27 GOSUB 50
0 REM Check table size
28 GOSUB 100
29 RETURN

0 REM ADDED by Felix - Bypass the search
30 GOSUB 60

0 REM Search result table
50 FOR C = 0 TO M;
0 REM If $0 matches string in table, go to RETURN
51 B = strcmp $(L+C);
52 IF B = 0 THEN 55;
53 NEXT C
0 REM If we get here, then there was no match, so write to table
54 GOSUB 60
55 RETURN

0 REM Write current hash value to table
60 $(L+M) = $0
0 REM Increment table index
61 M = M + 1
0 REM Double flash to indicate new result
62 A = pioset 20;
63 A = pioclr 20
64 A = pioset 20;
65 A = pioclr 20
66 GOSUB 110
67 RETURN

0 REM Check size of table, every 100 devices it will start back from 0.
100 IF M > 100 THEN 102
0 REM If under 100, do nothing
101 RETURN
0 REM Getting a little big, let's reset
102 M = 0
103 RETURN

0 REM File handling code
110 A = exist "found.log"
111 IF A <> 0 THEN 114
112 F = open "found.log"
113 GOTO 115
0 REM File exists, so we start at end
114 F = append "found.log"
0 REM Add newline
115 PRINTV "\n"
0 REM Find bytes length, write to file, then close
116 B = strlen $0
0 REM Before we write, check the file size
117 Q = size
0 REM If file is too large, we just have to let the results go :(
118 IF Q > 1000 THEN 120
0 REM File size is OK, write the data then close the file
119 F = write B
120 F = close
121 RETURN