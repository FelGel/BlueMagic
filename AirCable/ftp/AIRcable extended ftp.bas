@ERASE

@INIT 1
0 REM Set LED on
1 A = pioset 20
0 REM Baud rate for serial line
2 A = baud 1152
0 REM Debug toggle
7 Z = 0
29 RETURN

@IDLE 30
30 ALARM 1
31 A = pioset 20
32 RETURN

@SLAVE 70
70 A = pioclr 20
75 ALARM 1
197 RETURN


@ALARM 600
600 A = status
610 IF A <> 0 THEN 660
620 A = slave 15
650 ALARM 10
655 GOTO 700
660 A = exist "found.log"
663 IF A = 0 THEN 693
664 F = open "found.log"
665 REM GOSUB 860
666 A = getaddr
667 PRINTS $0
668 PRINTS "\n\r"
670 A = getconn
671 PRINTS $0
672 PRINTS "\n\r"
675 REM $0 = "0025BF100308"
676 T = ftp "from.txt"
680 A = success
685 PRINTS A
687 PRINTS "\n\r"
688 F = close
690 GOTO 695
693 PRINTS "NO FILE FOUND"
695 A = disconnect 0
700 RETURN


0 REM PRINT OPEN FILE
860 C = size
871 B = 32
872 IF C > B THEN 874
873 B = C
874 C = C - B
875 A = read B
876 $0[B] = 0
877 PRINTS $0
878 IF C > 0 THEN 872
900 RETURN


