// Thomas Polasek - Brainfuck Embedded Compiler
#include <stdint.h>
#include <sys/mman.h>
#include <iostream>
#include <vector>
#include <stack>
#include <fstream>

using namespace std;

class Instruction{
public: enum OptcodeType { JMP_FORWARD, JMP_BACKWARD, REGULAR, OUTPUT };
	
	
private:
	uint8_t * code;
	uint32_t size;
	OptcodeType type;
	
protected:
	void setCode(OptcodeType t, uint8_t optcode[],int n){
		if(code != NULL)
			delete code;
		
		type = t;
		size = n;
		code = new uint8_t[n];
		memcpy (code,optcode,n);
	}
	
	uint8_t * getCodePtr(){
		return code;
	}
	
public:
	
	Instruction() :code(NULL), size(0), type(REGULAR){}
	
	~Instruction(){
		if(code != NULL)
			delete code;
	}
	
	uint32_t getSize(){
		return size;
	}
	
	OptcodeType getType(){
		return type;
	}
	
	
	uint8_t const * getCode(){
		return code;
	}
	
	virtual void print(){
		cout << "{";
		for(int i =0; i < size; i++){
			cout << "0x" << hex << (uint32_t)code[i];
			if(i + 1 != size)
				cout << ", ";
		}
		cout << "}" << endl;
	}
};

class StartInstruction : public Instruction{
public:
	StartInstruction(uint64_t argument){
		// movabsq	$0, %rbx
		uint8_t optcode[] = {0x48,0xbb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; 
		*(uint64_t *)&optcode[2] = argument;
		setCode(REGULAR,optcode,sizeof(optcode));
	}
};

class EndInstruction : public Instruction{
public:
	EndInstruction(){
		// ret
		uint8_t optcode[] = {0xc3}; 
		setCode(REGULAR,optcode,sizeof(optcode));
	}
};

class ValueInstruction : public Instruction{
public:
	ValueInstruction(uint32_t argument){
		// addq	$0, (%rbx)
		uint8_t optcode[] = {0x48,0x81,0x03,0x00,0x00,0x00,0x00,}; 
		*(uint32_t *)&optcode[3] = argument;
		setCode(REGULAR,optcode,sizeof(optcode));
	}
};

class AddressInstruction : public Instruction{
public:
	AddressInstruction(uint32_t argument){
		// addq	$0, %rbx
		uint8_t optcode[] = {0x48,0x81,0xc3,0x00,0x00,0x00,0x00,}; 
		*(uint32_t *)&optcode[3] = (argument << 2);
		setCode(REGULAR,optcode,sizeof(optcode));
	}
};

class OutputInstruction : public Instruction{
	
private:
	void __calcPutCharAddrInternal(uint8_t * code, uint64_t codePosition){
		
		uint64_t __putcharAddr = (uint64_t)(void *)putchar;
		
		uint32_t offset = 0;
		if(__putcharAddr > codePosition){
			offset = (uint32_t)(  __putcharAddr - (codePosition ));
		}
		else{
			offset = -1*(uint32_t)(  codePosition - __putcharAddr );
		}
		*(uint32_t *)&code[1] = (offset - 5);
	}
	
public:
	OutputInstruction(){
		// movl	(%rbx), %edi
		// callq putchar (has to be calculated, relative offset)
		uint8_t optcode[] = {0x8b,0x3b,
			0xE8,0x00,0x00,0x00,0x00}; 
		setCode(OUTPUT,optcode,sizeof(optcode));
	}
	
	// we dont care about the move instruction
	void caclulatePutCharAddress(uint64_t codePosition){
		__calcPutCharAddrInternal(&getCodePtr()[2],codePosition+2);
	}
	
	
};


class JumpForwardInstruction : public Instruction{
public:
	JumpForwardInstruction(){
		// cmpl	$0, (%rbx)
		// je $0
		uint8_t optcode[] = {0x83,0x3b,0x00,
			0x0F,0x84,0x00,0x00,0x00,0x00}; 
		setCode(JMP_FORWARD,optcode,sizeof(optcode));
	}
	
	void setJumpAddress(uint32_t argument){
		*(uint32_t *)&getCodePtr()[5] = argument;
	}
	void print(){
		cout << "{";
		for(int i =0; i < 3; i++){
			cout << "0x" << hex << (uint32_t)getCodePtr()[i];
			if(i + 1 != 3)
				cout << ", ";
		}
		cout << "}" << endl;
		
		cout << "{";
		for(int i =3; i < getSize(); i++){
			cout << "0x" << hex << (uint32_t)getCodePtr()[i];
			if(i +  1 != getSize() )
				cout << ", ";
		}
		cout << "}" << endl;
	}
};

class JumpBackwardInstruction : public Instruction{
public:
	JumpBackwardInstruction(){
		// jmp 0
		uint8_t optcode[] = {0xE9,0x00,0x00,0x00,0x00}; 
		setCode(JMP_BACKWARD,optcode,sizeof(optcode));
	}
	
	void setJumpAddress(uint32_t argument){
		*(uint32_t *)&getCodePtr()[1] = argument;
	}
};



class Assembler{
	
private:
	vector<Instruction *> instructions;
	uint32_t codeSize;
	bool calculatedJumps;
	
	typedef struct{
		JumpForwardInstruction * instruction;
		uint32_t index;
	}ii;
	
	void caclulateJumps(){
		
		if(calculatedJumps)
			return;
		
		stack<ii> jumpstack;
		
		ii i;
		ii forward;
		JumpBackwardInstruction * backward;
		
		uint32_t index = 0;
		vector<Instruction *>::iterator it = instructions.begin();
		while(it != instructions.end()){
			switch((*it)->getType()){
					
					// TODO change to dynamic casts
				case Instruction::JMP_FORWARD:
					i.instruction =  (JumpForwardInstruction *)(*it);
					i.index = index;
					jumpstack.push(i);
					break;
					
				case Instruction::JMP_BACKWARD:
					
					if(jumpstack.empty()){
						cout << "ERROR: unmatched jumps\n";
						return;
					}
					forward = jumpstack.top();
					
					backward = (JumpBackwardInstruction *)(*it);
					
					forward.instruction->setJumpAddress(
														(index + backward->getSize()) - (forward.index + forward.instruction->getSize()) );
					backward->setJumpAddress( (forward.index - index) - backward->getSize() );
					
					
					jumpstack.pop();
					break;
					
			}
			index += (*it)->getSize();
			it++;
		}
		calculatedJumps = true;
	}
	
public:
	Assembler() : instructions(),codeSize(0),calculatedJumps(false) {}
	~Assembler() {instructions.clear();}
	
	void addInstruction(Instruction * i){
		codeSize += i->getSize();
		instructions.push_back(i);
		calculatedJumps = false;
	}
	
	void printCode(){
		
		caclulateJumps();
		
		vector<Instruction *>::iterator it = instructions.begin();
		
		cout << endl << "==========code" << endl;
		while(it != instructions.end()){
			(*it)->print();
			it++;
		}
		cout << endl;
	}
	
	// the code generated uses relative offsets (don't memcpy it)
	void generateCode(uint8_t * code){
		
		caclulateJumps();
		
		int j = 0;
		vector<Instruction *>::iterator it = instructions.begin();
		while(it != instructions.end()){
			
			if((*it)->getType() ==  Instruction::OUTPUT){
				uint64_t __optcodeMem = (uint64_t)(void *)&code[j];
				((OutputInstruction *)(*it))->caclulatePutCharAddress(__optcodeMem);
				
				
			}
			for(int i = 0; i < (*it)->getSize(); i++){
				code[j++] = (*it)->getCode()[i];
			}
			
			it++;
		}
		if(j != codeSize){
			cout << "ERROR: mismatch code size.\n";
			return;
		}
		
		cout << "Generated code" << endl;
		
	}
	
	
	// TODO add other information here
	void getStats(){
		cout << "Codesize(Bytes)/" << codeSize  << endl;
	}
};




static Assembler assembler;

int main2(){
	// prints 'hello'
	assembler.addInstruction(new StartInstruction(0));
	assembler.addInstruction(new ValueInstruction(10));
	assembler.addInstruction(new JumpForwardInstruction());
	assembler.addInstruction(new AddressInstruction(1));
	assembler.addInstruction(new ValueInstruction(7));
	assembler.addInstruction(new AddressInstruction(1));
	assembler.addInstruction(new ValueInstruction(10));
	assembler.addInstruction(new AddressInstruction(1));
	assembler.addInstruction(new ValueInstruction(3));
	assembler.addInstruction(new AddressInstruction(1));
	assembler.addInstruction(new ValueInstruction(1));
	assembler.addInstruction(new AddressInstruction(-4));
	assembler.addInstruction(new ValueInstruction(-1));
	assembler.addInstruction(new JumpBackwardInstruction());
	assembler.addInstruction(new AddressInstruction(1));
	assembler.addInstruction(new ValueInstruction(2));
	assembler.addInstruction(new OutputInstruction());
	assembler.addInstruction(new AddressInstruction(1));
	assembler.addInstruction(new ValueInstruction(1));
	assembler.addInstruction(new OutputInstruction());
	assembler.addInstruction(new ValueInstruction(7));
	assembler.addInstruction(new OutputInstruction());
	assembler.addInstruction(new OutputInstruction());
	assembler.addInstruction(new ValueInstruction(3));
	assembler.addInstruction(new OutputInstruction());
	assembler.addInstruction(new EndInstruction());
	
	assembler.printCode();
	
	uint8_t code[10000];
	assembler.generateCode(code);
	
	// TODO figure out page size
	void * page = (void*)((uint64_t)code & ((0UL - 1UL) ^ 0xfff));
    if (mprotect(page, 10000, PROT_READ|PROT_EXEC))
		return -1;
	
	int32_t memory[10000];
	*(uint64_t *)(code + 2) = (uint64_t)(int32_t *)&memory[0];
	
	void (*f)();
	f = (void(*)())code;
	f();
	
	return 0;
}


int main(int argc, char **argv) {
	int args;
	
	
	ifstream stream;
	
	assembler.addInstruction(new StartInstruction(0));
	
	for (args = 1; args < argc; args++) {
		
		int counter_plus = 0;
		int counter_minus = 0;
		
		int counter_right = 0;
		int counter_left = 0;
		
		
		stream.open(argv[args]);
		while (stream.good()){
			char c = stream.get();     
			if (stream.good()){
				
				if(c == 43) { //+
					counter_plus++;
					
				}
				else if(counter_plus){
					assembler.addInstruction(new ValueInstruction(counter_plus));
					counter_plus = 0;
				}
				
				if(c == 45) { //-
					counter_minus++;
				}
				else if(counter_minus){
					assembler.addInstruction(new ValueInstruction(-1*counter_minus));
					counter_minus = 0;
				}
				
				if(c == 62) { //>
					counter_right++;
				}
				else if(counter_right){
					assembler.addInstruction(new AddressInstruction(counter_right));
					counter_right = 0;
				}
				
				if(c == 60) { //<
					counter_left++;
				}
				else if(counter_left){
					assembler.addInstruction(new AddressInstruction(-1*counter_left));
					counter_left = 0;
				}
				
				
				if(c == 46 ){//.
					assembler.addInstruction(new OutputInstruction());
				}
				if(c == 91){//[
					assembler.addInstruction(new JumpForwardInstruction());
				}
				if(c == 93){
					assembler.addInstruction(new JumpBackwardInstruction());
				}
				
				
			}
		}
		stream.close();
		
	}
	assembler.addInstruction(new EndInstruction());
	
	uint8_t code[100000];
	assembler.generateCode(code);
	
	void * page = (void*)((uint64_t)code & ((0UL - 1UL) ^ 0xfff));
    if (mprotect(page, 100000, PROT_READ|PROT_WRITE|PROT_EXEC))
		return -1;
	
	int32_t memory[50000];
	*(uint64_t *)(code + 2) = (uint64_t)(int32_t *)&memory[0];
	
	void (*f)();
	f = (void(*)())code;
	f();
	
	return 0;
}
