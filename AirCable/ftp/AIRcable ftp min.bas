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
670 A = getconn
676 T = ftp "from.txt"
680 A = success
685 PRINTS A
687 PRINTS "\n\r"
688 F = close
690 GOTO 695
693 PRINTS "NO FILE FOUND"
695 A = disconnect 0
700 RETURN



