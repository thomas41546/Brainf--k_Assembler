
#include <iostream>
#include <vector>
#include <fstream>
#include <vector>
#include <stack>
#include <sys/mman.h>
using namespace std;
// assembly to binary code
//cat asm | ./llvm-mc -x86-asm-syntax=att -show-encoding | grep encoding | sed 's/## encoding: \[/*\//g' | sed 's/\]/,\/*/g'

//#define SYM_OPT
#define ASM

typedef struct {
	int id;
	int arg;
	int arg1;
	int arg2;
} bytecode;

int main(int argc, char **argv) {
	int pc, args, xc, l,i = 0;
	
	vector<int> x (32768);
	vector<bytecode> p (100);
	
	ifstream stream;
	
	long operations;
	
	
	long ins[11];
	for(i=0;i<11;i++){
		ins[i] = 0;
	}
	
	for (args = 1; args < argc; args++) {
		
		int counter_plus = 0;
		int counter_minus = 0;
		
		int counter_right = 0;
		int counter_left = 0;
		
		pc = 0;
		
		stream.open(argv[args]);
		while (stream.good()){
			char c = stream.get();     
			if (stream.good()){
				
				if(c == 43) { //+
					counter_plus++;
					
				}
				else if(counter_plus){
					bytecode b;
					b.id = 43;
					b.arg = counter_plus;
					p.push_back(b);
					counter_plus = 0;
					pc++;
				}
				
				if(c == 45) { //-
					counter_minus++;
				}
				else if(counter_minus){
					bytecode b;
					b.id = 43;
					b.arg = -1*counter_minus;
					p.push_back(b);
					counter_minus = 0;
					pc++;
				}
				
				
				if(c == 62) { //>
					counter_right++;
				}
				else if(counter_right){
					bytecode b;
					b.id = 62;
					b.arg = counter_right;
					p.push_back(b);
					counter_right = 0;
					pc++;
				}
				
				if(c == 60) { //<
					counter_left++;
				}
				else if(counter_left){
					bytecode b;
					b.id = 62;
					b.arg = -1*counter_left;
					p.push_back(b);
					counter_left = 0;
					pc++;
				}
				
				
				if(c == 46 || c == 44 || c == 91 || c == 93){
					bytecode b;
					b.id = c;
					b.arg = -1;
					p.push_back(b);
					pc++;
				}
			}
		}
		stream.close();
		
		//end
		bytecode b;
		b.id = 1;
		b.arg = -1;
		p.push_back(b);
		pc++;
		
		cout << "program size: " << pc << "\n";
		
		
#ifdef SYM_OPT
		int canop = 0;
		
		for(pc = 0; pc < p.size()-2; pc++) {
			
			//<+> <-> from
			if(p[pc].id == 60 && (p[pc+1].id == 43 ) && p[pc+2].id == 62){
				bytecode b;			
				b.id = 100;
				b.arg = p[pc].arg;
				b.arg1 = p[pc+1].arg;
				
				b.arg2 = p[pc+2].arg;
				p[pc] = b;
				
				p[pc+1].id = 0;
				p[pc+2].id = 0;
				
				pc+=2;
				canop++;
			}
			// >+< >-< form
			else if(p[pc].id == 62 && (p[pc+1].id == 43) && p[pc+2].id == 60){
				bytecode b;			
				b.id = 100;
				b.arg = -1*p[pc].arg;
				b.arg1 = p[pc+1].arg;
				
				b.arg2 = -1*p[pc+2].arg;
				p[pc] = b;
				
				p[pc+1].id = 0;
				p[pc+2].id = 0;
				
				pc+=2;
				canop++;
			}
			//<+ or <- form
			else if(p[pc].id == 60 && (p[pc+1].id == 43)){
				bytecode b;			
				b.id = 101;
				b.arg = p[pc].arg;
				b.arg1 = p[pc+1].arg;
				b.arg2 = 0;
				p[pc] = b;
				
				p[pc+1].id = 0;
				pc+=1;
				canop++;
			}
			//>+ or >- form
			else if(p[pc].id == 62 && (p[pc+1].id == 43)){
				bytecode b;			
				b.id = 101;
				
				b.arg = -1*p[pc].arg;
				
				b.arg1 = p[pc+1].arg;
				
				b.arg2 = 0;
				
				p[pc] = b;
				p[pc+1].id = 0;
				
				pc+=1;
				canop++;
			}
			
		}
		cout <<"can optimize: "  << canop << "\n";
#endif
		
		//process loops
		for(pc = 0; pc < p.size(); pc++) {
			
			int ori_p = pc;	
			
			switch(p[pc].id){
					
					
				case 91:
					if(p[pc].arg == -1){
						pc++;
						while (l > 0 || p[pc].id != 93) {
							if (p[pc].id == 91) l++;
							if (p[pc].id == 93) l--;
							pc++;
						}
						p[ori_p].arg = pc;
						
						pc = ori_p;
					}
					break;
					
				case 93:
					if(p[pc].arg == -1){
						int ori_p = pc;
						pc--;
						while (l > 0 || p[pc].id != 91) {
							if (p[pc].id == 93) l++;
							if (p[pc].id == 91) l--;
							pc--;
						}
						pc--;
						p[ori_p].arg = pc;
						pc = ori_p;
					}
					break;
					
				default:
					break;
					
			}
		}
		
		//%rbx replaced %rax since the function call messes up %rax
		unsigned char shellcode_93[] = {
			/*movabsq	$4294967295, %rbx   */0x48,0xbb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			/*movq	$-1, %rcx*/               0x48,0xc7,0xc1,0xff,0xff,0xff,0xff,
			/*movq	$-1, %rdx*/			      0x48,0xc7,0xc2,0xff,0xff,0xff,0xff,
			/*pushq	%rcx*/                    0x51,
			/*movq	%rdx, %rcx */             0x48,0x89,0xd1,
			/*shlq	$4, %rcx   */             0x48,0xc1,0xe1,0x04,
			/*leaq	4(%rbx,%rcx), %rcx */     0x48,0x8d,0x4c,0x0b,0x04,
			/*movq	(%rcx), %rdx        */    0x48,0x8b,0x11,
			/*popq	%rcx                  */  0x59,
			/*addq	$1, %rdx             */   0x48,0x83,0xc2,0x01,
			/*ret						*/    0xc3
		};
		
		void *page = (void*)((unsigned long)shellcode_93 & ((0UL - 1UL) ^ 0xfff));
		int ans = mprotect(page, 1, PROT_READ|PROT_WRITE|PROT_EXEC);
		if (ans)
		{
			perror("mprotect");
			exit(EXIT_FAILURE);
		}
		void (*f_93)();
		f_93 = (void(*)())shellcode_93;
		
		// setup memory
		for(xc = 0; xc < 32768; xc++)
			x[xc] = 0;
		xc = 0;
		
		
		operations = 0;
		pc = 0;
		while(1){
			unsigned long __p = (unsigned long)(int *)&p[0];
			unsigned long __x = (unsigned long)(int *)&x[0];
			unsigned long __xc = (unsigned long)xc;
			unsigned long __pc = (unsigned long)pc;
			
			int expected_pc = (!x[xc] ? p[pc].arg : pc) + 1;
			
			
			switch(p[pc].id){
					
#ifdef SYM_OPT
				case 100:
					xc-= p[pc].arg;
					x[xc] += p[pc].arg1;
					xc+= p[pc].arg2;
					pc+=3;
					
					ins[0]++;
					break;
					
				case 101:
					xc-= p[pc].arg;
					x[xc] += p[pc].arg1;
					pc+=2;
					ins[1]++;
					break;
#endif
					
				case 43:
					
	
#ifdef ASM
					__asm__ __volatile__ (
										  "movq %2, %%rax \n\t" //p
										  "movq %3, %%rbx \n\t" //x
										  
										  "movq %4, %%rcx \n\t" //xc
										  "movq %5, %%rdx \n\t" //pc
										  
										  "pushq %%rdx \n\t" // save pc 
										  "pushq %%rax \n\t" // save p 
										  "shl $4, %%rdx \n\t"  //pc*16
										  "leaq 4(%%rax,%%rdx), %%rdx \n\t" //rdx = pc*16 + 4 + p
										  "movl (%%rdx),%%edx \n\t"
										  
										  "leaq (%%rbx,%%rcx,4), %%rax \n\t" //rax = x + xc*4
										  
										  "addl %%edx,(%%rax) \n\t"	//(x + xc*4) += (pc*16 + 4 + p)
										  "popq %%rax \n\t" // restore p
										  "popq %%rdx \n\t" // restore pc 
										  "addq $1,%%rdx \n\t"
										  
										  "movq %%rdx,%0\n\t"
										  "movq %%rcx,%1\n\t"
										  
										  : "=m" (pc), "=m" (xc)
										  : "m" (__p) ,"m" (__x), "m" (__xc), "m" (__pc)
										  );
#else
					x[xc] += p[pc++].arg;
#endif
					ins[2]++;
					break;
					
				case 62:

#ifdef ASM
					__asm__ __volatile__ (
										  "movq %2, %%rax \n\t" //p
										  "movq %3, %%rbx \n\t" //x
										  
										  "movq %4, %%rcx \n\t" //xc
										  "movq %5, %%rdx \n\t" //pc
										  
										  "pushq %%rdx \n\t" // save pc 
										  "shl $4, %%rdx \n\t"  //pc*16
										  "leaq 4(%%rax,%%rdx), %%rdx \n\t" //rdx = pc*16 + 4 + p
										  "addl (%%rdx),%%ecx \n\t"
										  "popq %%rdx \n\t" // restore pc 
										  
										  "addq $1,%%rdx \n\t"
										  
										  "movq %%rdx,%0\n\t"
										  "movq %%rcx,%1\n\t"
										  
										  : "=m" (pc), "=m" (xc)
										  : "m" (__p) ,"m" (__x), "m" (__xc), "m" (__pc)
										  );
#else
					xc+= p[pc++].arg;
#endif
					ins[6]++;
					break;

													
																																																																																																																																																																																																	case 91:
					
#ifdef ASM
					__asm__ __volatile__ (
										  "movq %2, %%rax \n\t" //p
										  "movq %3, %%rbx \n\t" //x
										  
										  "movq %4, %%rcx \n\t" //xc
										  "movq %5, %%rdx \n\t" //pc
										  
										  "pushq %%rax \n\t" 
										  "leaq (%%rbx,%%rcx,4), %%rax \n\t" //rax = x + xc*4
										
										  "cmpl $0,(%%rax) \n\t"
										  "popq %%rax \n\t"
										  "jne .ENDYY \n\t"
										  
										  //x[xc] == 0
										  "pushq %%rcx \n\t" // save pc 
										  "movq %%rdx, %%rcx \n\t" //xc
										  "shl $4, %%rcx \n\t"  //pc*16
										  "leaq 4(%%rax,%%rcx), %%rcx \n\t" //rdx = pc*16 + 4 + p
										  "movq (%%rcx),%%rdx \n\t"
										  "popq %%rcx \n\t" // restore pc 
										  
										  ".ENDYY: \n\t"
										  "addq $1,%%rdx \n\t"
										  "movq %%rdx,%0\n\t"
										  "movq %%rcx,%1\n\t"
										  
										  : "=m" (pc), "=m" (xc)
										  : "m" (__p) ,"m" (__x), "m" (__xc), "m" (__pc)
										  );
					
#else
					pc = !x[xc] ? p[pc].arg : pc;
					pc++;
#endif
	
					ins[8]++;
					break;
					
				case 93:

						
					// rdx,rcx have our pc,xc
					
					
#ifdef ASM
					*(long *)(shellcode_93 + 2 ) = __p; //set %rbx					  );
					*(int *)(shellcode_93 + 13) = xc; //set %rcx
					*(int *)(shellcode_93 + 20) = pc; //set %rdx
					
					
					f_93(); //fucks with %rax
					__asm__ __volatile__ (
										  "movq %%rdx,%0\n\t"
										  "movq %%rcx,%1\n\t"
										  
										  : "=m" (pc), "=m" (xc)
										  :
										  );
					

					
					/*
					__asm__ __volatile__ (
										  "movq %2, %%rax \n\t" //p
										  "movq %3, %%rbx \n\t" //x
										  
										  "movq %4, %%rcx \n\t" //xc
										  "movq %5, %%rdx \n\t" //pc
										  
										  "pushq %%rcx \n\t" // save pc 
										  "movq %%rdx, %%rcx \n\t" //xc
										  "shl $4, %%rcx \n\t"  //pc*16
										  "leaq 4(%%rax,%%rcx), %%rcx \n\t" //rdx = pc*16 + 4 + p
										  "movq (%%rcx),%%rdx \n\t"
										  "popq %%rcx \n\t" // restore pc 
										  
										  "addq $1,%%rdx \n\t"
										  
										  "movq %%rdx,%0\n\t"
										  "movq %%rcx,%1\n\t"
										  
										  : "=m" (pc), "=m" (xc)
										  : "m" (__p) ,"m" (__x), "m" (__xc), "m" (__pc)
										  );
					 */
#else
					pc = p[pc].arg;
					pc++;
#endif

					ins[9]++;
					break;
					
				case 46:
#ifndef ASM
					__asm__ __volatile__ ( //TODO make this work
										  "movq %2, %%rax \n\t" //p
										  "movq %3, %%rbx \n\t" //x
										  
										  "movq %4, %%rcx \n\t" //xc
										  "movq %5, %%rdx \n\t" //pc
										  
										  "pushq %%rax \n\t" 
										  "leaq (%%rbx,%%rcx,4), %%rax \n\t" //rax = x + xc*4
										  
										  "movl $96,%%edi \n\t"
										  "call  _putchar \n\t"
										  "popq %%rax \n\t"
										  
										  "addq $1,%%rdx \n\t"
										  
										  "movq %%rdx,%0\n\t"
										  "movq %%rcx,%1\n\t"
										  
										  : "=m" (pc), "=m" (xc)
										  : "m" (__p) ,"m" (__x), "m" (__xc), "m" (__pc)
										  );
#else
					//		__asm__ __volatile__ ("int $3 \n\t");
					putchar(x[xc]);
					pc++;
					ins[4]++;
#endif
					

					break;
					
				case 44:
					cin >> x[xc];
					pc++;
					ins[5]++;
					break;
					
				case 1:
					ins[10]++;
					goto END;
					
					// 0
				default:
					pc++;
					break;
					
			}
			operations++;
		}  
	}
END:
	for(i=0;i<11;i++){
		cout << "instruction_count: " << ins[i] << "/" << i << "\n";
	}
	
	cout << "\noperations: " << operations << "\n";
	putchar(10);  
	return 0;
}
