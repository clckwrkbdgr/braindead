#include <stdio.h>
#include <assert.h>
#include <string.h>

extern const char * test_program;

void brainfuck(char*m,void(*i)(char*),void(*o)(char*),const char*c)
{
	//[ord(x) for x in "+-<>[],."]
		//[43, 45, 60, 62, 91, 93, 44, 46]
	while(*c) {
		switch(*c) {
			case 43: ++*m; break;
			case 45: --*m; break;
			case 60: m--; break;
			case 62: m++; break;
			case 46: o(m); break;
			case 44: i(m); break;
			case 91:
					  if(*m == 0) {
						  int counter = 0;
						  while(counter >= 0) {
							  ++c;
							  if(*c == '[') {
								  ++counter;
							  } else if(*c == ']'){
								  --counter;
							  }
						  }
					  }
					  break;
			case 93:
					  if(*m != 0) {
						  int counter = 0;
						  while(counter >= 0) {
							  --c;
							  if(*c == ']') {
								  ++counter;
							  } else if(*c == '['){
								  --counter;
							  }
						  }
					  }
					  break;
		}
		++c;
	}
	/*
	int l=0,f; while(l ?  ( ( !*m==!f&&(c+=1-f,l+=*c==(93-f) ?  -1 : *c==(91+f))
	) ?  *--c : (l=0)) : ( l=*c&-2,f=*c&2,l==60 ?  (m+=f-1,0) : ( l==44 ?  ((f?o
	:i)(m),0) : ( (l=*c&-6,f=*c&6)%6 ?  ( ( l==41 ?  (*m+=3-f,0) : l==89) ?  (--
	c),f-=2,l=0 : 0) : 0,l=!l))) , *c++);
	*/
}
int bfcheck(const char*c){int x=0;while((x+=(*c&-6)==89&&(*c&6)^6?3-(*c&6):0)>=0&&*c++);return!x;}
void bf_input(char*c){*c=getchar();}
void bf_output(char*c){putchar(*c);}

static char buffer[256];
static char * pstring;
void output_string(char *c) {
	*pstring = *c;
	putchar(*c);
	pstring++;
}

int main(void) {
	char memory[30000] = {0};

	memset(buffer, 0, 256); pstring = buffer;
	brainfuck(memory,bf_input,output_string, "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.");
	assert(!strcmp(buffer, "Hello World!\n"));

	memset(buffer, 0, 256); pstring = buffer;
	brainfuck(memory,bf_input,output_string, test_program);
	assert(!strcmp(buffer, "Hello World! 255\n"));
	return 0;
}

const char * test_program = /* (c) rdebath */
"[+++++++.+++.][   <- If you get this nl your loops are fucked. ->"
"    This routine is a demonstration of checking for the three cell sizes"
"    that are normal for Brainfuck. The demo code also checks for several"
"    bugs that have been noted in optimising and non-optimising interpreters"
"    and compilers."
"    It should print one of three slight variations of \"Hello world\" followed"
"    by an exclamation point then the maximum cell value (if it's less than a"
"    few thousand) and a newline."
"    If the interpreter is broken in some way it can print a lot of other"
"    different strings and frequently causes the interpreter to crash."
"    It does work correctly with 'bignum' cells."
"]"
">>"
"	This code runs at pointer offset two and unknown bit width; don't"
"	assume you have more that eight bits"
"	======= DEMO CODE ======="
"	First just print \"Hello\""
"	Notice that I reset the cells despite knowing that they are zero"
"	this is a test for proper functioning of the ability to skip over"
"	a loop that's never executed but isn't actually a comment loop"
"	Secondly there's a NOP movement between the two 'l' characters"
"	Also there's some commented out code afterwards"
"	[-]>[-]++++++++[<+++++++++>-]<.>+++++[<+++++>-]<++++."
"	+++++++.><.+++."
"	[-] [[-]++>[-]+++++[<++++++>-]<.++>+++++++[<+++++++>-]<.+>+"
"	+++[<+++++>-]<.+.+++++++++++.------------.---.----.+++.>++++"
"	++++[<-------->-]<---.>++++[<----->-]<---.[-][]]"
"	===== END DEMO CODE ====="
"<<"
"Calculate the value 256 and test if it's zero"
"If the interpreter errors on overflow this is where it'll happen"
"++++++++[>++++++++<-]>[<++++>-]"
"+<[>-<"
"Multiply by 256 again to get 65536"
"[>++++<-]>[<++++++++>-]<[>++++++++<-]"
"+>[>"
"	Cells should be 32bits at this point"
"	The pointer is at cell two and you can continue your code confident"
"	that there are big cells"
"	======= DEMO CODE ======="
"	This code rechecks that the test cells are in fact nonzero"
"	If the compiler notices the above is constant but doesn't"
"	properly wrap the values this will generate an incorrect"
"	string"
"	An optimisation barrier; unbalanced loops aren't easy"
"	>+[<]>-<"
"	Print a message"
"	++>[-]++++++[<+++++++>-]<.------------.[-]"
"	<[>+<[-]]>"
"	++++++++>[-]++++++++++[<+++++++++++>-]<.--------.+++.------."
"	--------.[-]"
"	===== END DEMO CODE ====="
"<[-]<[-]>] <[>>"
"	Cells should be 16bits at this point"
"	The pointer is at cell two and you can continue your code confident"
"	that there are medium sized cells; you can use all the cells on the"
"	tape but it is recommended that you leave the first two alone"
"	If you need 32bit cells you'll have to use a BF doubler"
"	======= DEMO CODE ======="
"	Space"
"	++>[-]+++++[<++++++>-]<.[-]"
"	I'm rechecking that the cells are 16 bits"
"	this condition should always be true"
"	+>>++++[-<<[->++++<]>[-<+>]>]< + <[ >>"
"	    Print a message"
"	    >[-]++++++++++[<+++++++++++>-]<+++++++++.--------."
"	    +++.------.--------.[-]"
"	<[-]<[-] ] >[> > Dead code here"
"	    This should never be executed because it's in an 8bit zone hidden"
"	    within a 16bit zone; a really good compiler should delete this"
"	    If you see this message you have dead code walking"
"	    Print a message"
"	    [-]>[-]+++++++++[<++++++++++>-]<."
"	    >++++[<+++++>-]<+.--.-----------.+++++++.----."
"	    [-]"
"	<<[-]]<"
"	===== END DEMO CODE ====="
"<<[-]] >[-]< ] >[>"
"	Cells should be 8bits at this point"
"	The pointer is at cell two but you only have 8 bits cells"
"	and it's time to use the really big and slow BF quad encoding"
"	======= DEMO CODE ======="
"	A broken wrapping check"
"	+++++[>++++<-]>[<+++++++++++++>-]<----[[-]>[-]+++++[<++++++>-]<++."
"	>+++++[<+++++++>-]<.>++++++[<+++++++>-]<+++++.>++++[<---->-]<-.++."
"	++++++++.------.-.[-]]"
"	Space"
"	++>[-]+++++[<++++++>-]<.[-]"
"        Another dead code check"
"        [-]>[-]>[-]<++[>++++++++<-]>[<++++++++>-]<[>++++++++<-]>[<++++++++>-"
"        ]<[<++++++++>-]<[[-]>[-]+++++++++[<++++++++++>-]<.>++++[<+++++>-]<+."
"        --.-----------.+++++++.----.>>[-]<+++++[>++++++<-]>++.<<[-]]"
"	Print a message"
"	[-] <[>+<[-]]> +++++>[-]+++++++++[<+++++++++>-]<."
"	>++++[<++++++>-]<.+++.------.--------."
"	[-]"
"	===== END DEMO CODE ====="
"<[-]]<"
"+[[>]<-]    Check unbalanced loops are ok"
">>"
"	======= DEMO CODE ======="
"	Back out and print the last two characters"
"	This loop is a bug check for handling of nested loops; it goes"
"	round the outer loop twice and the inner loop is skipped on the"
"	first pass but run on the second"
"	BTW: It's unlikely that an optimiser will notice how this works"
"	>"
"	    +[>["
"		Print the exclamation point"
"		[-]+++>[-]+++++[<+++2+++>-]<."
"	    <[-]>[-]]+<]"
"	<"
"	[<[[<[[<[[<[,]]]<]<]<]<][ Deep nesting non-comment comment loop ]]"
"	This part finds the actual value that the cell wraps at; even"
"	if it's not one of the standard ones; but it gets bored after"
"	a few thousand: any higher and we print nothing"
"	This has a reasonably deep nested loop and a couple of loops"
"	that have unbalanced pointer movements"
"	Find maxint (if small)"
"	[-]>[-]>[-]>[-]>[-]>[-]>[-]>[-]<<<<<<<++++[->>++++>>++++>>++"
"	++<<<<<<]++++++++++++++>>>>+>>++<<<<<<[->>[->+>[->+>[->+>+[>"
"	>>+<<]>>[-<<+>]<-[<<<[-]<<[-]<<[-]<<[-]>>>[-]>>[-]>>[-]>->+]"
"	<<<]>[-<+>]<<<]>[-<+>]<<<]>[-<+>]<<<]>+>[[-]<->]<[->>>>>>>[-"
"	<<<<<<<<+>>>>>>>>]<<<<<<<]<"
"	The number is only printed if we found the actual maxint"
"	["
"	    Space"
"	    >[-]>[-]+++++[<++++++>-]<++.[-]<"
"	    Print the number"
"	    [[->>+<<]>>[-<++>[-<+>[-<+>[-<+>[-<+>[-<+>[-<+>[-<+>[-<+>[<[-]+>"
"	    ->+<[<-]]]]]]]]]]>]<<[>++++++[<++++++++>-]<-.[-]<]]"
"	]"
"	This is a hard optimisation barrier"
"	It contains several difficult to 'prove' constructions close together"
"	and is likely to prevent almost all forms of optimisation"
"	+[[>]<-[,]+[>]<-]"
"	Clean up any debris"
"	>++++++++[[>]+[<]>-]>[>]<[[-]<]<"
"	Check that an offset of 128 will work"
"	+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
"	>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>-[+<-]"
"	[ Check there are enough cells. This takes 18569597 steps. ]"
"	["
"	    >++++++[<+++>-]<+[>+++++++++<-]>+[[->+>+<<]>>"
"	    [-<<+>>]<[<[->>+<<]+>[->>+<<]+[>]<-]<-]<[-<]"
"	]"
"	One last thing: an exclamation point is not a valid BF instruction!"
"	Print the newline"
"	[-]++++++++++.[-]"
"	["
"	    Oh, and now that I can use \"!\" the string you see should be one of:"
"	    Hello World! 255"
"	    Hello world! 65535"
"	    Hello, world!"
"	    And it should be followed by a newline."
"	]"
"	===== END DEMO CODE ====="
"<<  Finish at cell zero"
;
