#include "chip8.hpp"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <chrono>
#include <random>

// Questions
// Do the two data points stored in memory /opcode instruction represent two different...
// ...things? Maybe instruction & location?
// What does the & operator do? Bitwise comparison maybe?

const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;

uint8_t fontset[FONTSET_SIZE] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

// Constructor
Chip8::Chip8()
	: randGen(std::chrono::system_clock::now().time_since_epoch().count())
{
    // Initialize [program counter]
    pc = START_ADDRESS;

    // Load fonts into memory
    // Why is ++i standard practice over i++?
    for (unsigned int i = 0; i < FONTSET_SIZE; ++i)
	{
		memory[FONTSET_START_ADDRESS + 1] = fontset[i];
	}

    // Initialize random number generator
    // Is this used to get a random byte in some given range?*
    // *Any random number as an unsigned 8 bit/1 byte int ranges 0 to 255 - can be thought of as random 8 bits
    randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

    // Set up function pointer table
    // [Pass in address of opcode functions]
    table[0x0] = &Chip8::Table0;
	table[0x1] = &Chip8::OP_1nnn;
	table[0x2] = &Chip8::OP_2nnn;
	table[0x3] = &Chip8::OP_3xkk;
	table[0x4] = &Chip8::OP_4xkk;
	table[0x5] = &Chip8::OP_5xy0;
	table[0x6] = &Chip8::OP_6xkk;
	table[0x7] = &Chip8::OP_7xkk;
	table[0x8] = &Chip8::Table8;
	table[0x9] = &Chip8::OP_9xy0;
	table[0xA] = &Chip8::OP_Annn;
	table[0xB] = &Chip8::OP_Bnnn;
	table[0xC] = &Chip8::OP_Cxkk;
	table[0xD] = &Chip8::OP_Dxyn;
	table[0xE] = &Chip8::TableE;
	table[0xF] = &Chip8::TableF;

    // What is size_t? Why i++ instead of ++i?
    for (size_t i = 0; i <= 0xE; i++)
	{
		table0[i] = &Chip8::OP_NULL;
		table8[i] = &Chip8::OP_NULL;
		tableE[i] = &Chip8::OP_NULL;
	}

    table0[0x0] = &Chip8::OP_00E0;
	table0[0xE] = &Chip8::OP_00EE;

	table8[0x0] = &Chip8::OP_8xy0;
	table8[0x1] = &Chip8::OP_8xy1;
	table8[0x2] = &Chip8::OP_8xy2;
	table8[0x3] = &Chip8::OP_8xy3;
	table8[0x4] = &Chip8::OP_8xy4;
	table8[0x5] = &Chip8::OP_8xy5;
	table8[0x6] = &Chip8::OP_8xy6;
	table8[0x7] = &Chip8::OP_8xy7;
	table8[0xE] = &Chip8::OP_8xyE;

	tableE[0x1] = &Chip8::OP_ExA1;
	tableE[0xE] = &Chip8::OP_Ex9E;

	for (size_t i = 0; i <= 0x65; i++)
	{
		tableF[i] = &Chip8::OP_NULL;
	}

	tableF[0x07] = &Chip8::OP_Fx07;
	tableF[0x0A] = &Chip8::OP_Fx0A;
	tableF[0x15] = &Chip8::OP_Fx15;
	tableF[0x18] = &Chip8::OP_Fx18;
	tableF[0x1E] = &Chip8::OP_Fx1E;
	tableF[0x29] = &Chip8::OP_Fx29;
	tableF[0x33] = &Chip8::OP_Fx33;
	tableF[0x55] = &Chip8::OP_Fx55;
	tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::LoadROM(char const* filename)
{
    // Open file as stream of binary & move pointer to end
    // What is the i in ifstream? Ios? Ios::ate?
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        // Get size & allocate buffer to hold information
        // What is a buffer? streampos? tellg()? Is char used because of similar behavior to int?
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        // Go back to beginning of file & fill buffer
        // What is seekg? ios::beg? Does closing a file change its status in memory?
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        // Load ROM contents into memory
        // Assuming this takes individual instructions from the ROM & copies them
        // Do the addresses start from 1/0x201? How large can i/size get?
        for (long i = 0; i < size; ++i)
        {
            memory[START_ADDRESS + i] = buffer[i];
        }

        // Free the buffer
        // Does anything else need to be deleted? Why is delete an array?
        delete[] buffer;
    }
}

// Cycle includes: fetching instruction, decoding instruction, executing instruction
void Chip8::Cycle()
{
    // Fetch
    opcode = (memory[pc] << 8u) | memory[pc + 1];

    // Increment PC before execution
    pc += 2;

    // Decode & execute
    ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

    // Decremenet delay timer if set
    if (delayTimer > 0)
    {
        --delayTimer;
    }

    // Decrement sound timer if set
    if (soundTimer > 0)
    {
        --soundTimer;
    }
}

void Chip8::Table0()
{
    ((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8()
{
    ((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE()
{
    ((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip8::TableF()
{
    ((*this).*(tableF[opcode & 0x000Fu]))();
}

void Chip8::OP_NULL()
{
}

// ***** EMULATOR INSTRUCTIONS *****

// [General themes of intstructions] [Program counter/stack operations, calculations]

// Naming convention?* Location in memory of some kind? Does this correspond to the address format in the memory array?
// *Possibly similar to [machine syntax] $7522 / [instruction, data, location]
// *[OP (opcode), index in category, type of instruction, type of data]
// Meaning of Vx? Vx is possibly data? -> Vx is a position in the set of CPU registers where x (& y) is given
// Meaning of 0x0... in operation w/ opcode?
// kk = given value as opposed to other data stored in memory?
// Example of opcode and operations involving the opcode?

// CLR
// Clear display
void Chip8::OP_00E0()
{
    // Set all values in video to 0
    // Function to set memory? Where does this function come from? Purpose of sizeof?
    memset(video, 0, sizeof(video));
}

// RET
// Return from subroutine
void Chip8::OP_00EE()
{
    // Move pointer down after function finishes/returns
    --sp;
    // Move program counter to [new memory address]
    pc = stack[sp];
}

// JP addr
// Jump to location nnn (Set program counter to nnn)
// What is nnn? Various hex digits?
void Chip8::OP_1nnn()
{
    // [Breakdown of process below]
    uint16_t address = opcode & 0x0FFFu;
    pc = address;
}

// CALL addr
// Call subroutine at nnn
void Chip8::OP_2nnn()
{
    // [Breakdown of process below]
    uint16_t address = opcode & 0x0FFFu;

    stack[sp] = pc;
    ++sp;
    pc = address;
}

// SE Vx, byte
// Skip next instruction if Vx = kk
// "Since PC has been incremented by 2 in cycle() ... increment by two again to skip next..."
// ...instruction" -> Where is cycle()?
void Chip8::OP_3xkk()
{
    // What is the operation >> 8u? Right shift by 8? Purpose?
    // [Breakdown of process below]
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    // Given this condition is the premise of the function, would this if statmenent be a form of double checking?
    if (registers[Vx] == byte)
    {
        pc += 2;
    }
}

// SNE Vx, byte
// Skip next instruction if Vx != kk
void Chip8::OP_4xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] != byte)
    {
        pc += 2;
    }
}

// SE Vx, Vy
// Skip next instruction if Vx = Vy
void Chip8::OP_5xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00FFu) >> 4u;

    if (registers[Vx] == registers[Vy])
    {
        pc += 2;
    }
}

// LD Vx, byte
// Set Vx = kk
void Chip8::OP_6xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x0FFu;

    registers[Vx] = byte;
}

// ADD Vx, byte
// Set Vx += kk
void Chip8::OP_7xkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] += byte;
}

// LD Vx, Vy
// Set Vx = Vy
void Chip8::OP_8xy0()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    // Why [4 bits]?
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] = registers[Vy];
}

// OR Vx, Vy
// Set Vx = Vx | Vy
void Chip8::OP_8xy1()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] |= registers[Vy];
}

// AND Vx, Vy
// Set Vx = Vx AND Vy
void Chip8::OP_8xy2()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] &= registers[Vy];
}

// XOR Vx, Vy
// Set Vx = Vx XOR Vy
void Chip8::OP_8xy3()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    registers[Vx] ^= registers[Vy];
}

// ADD Vx, Vy
// Set Vx += Vy, set VF = carry (in event Vx + Vy > max 8 bit value 255)
void Chip8::OP_8xy4()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    // Get sum of data Vx & Vy
    uint16_t sum = registers[Vx] + registers[Vy];

    if (sum > 255U)
    {
        // Set overflow register [#16?] to 1
        registers[0xF] = 1;
    }
    else
    {
        registers[0xF] = 0;
    }

    // What would sum & 0xFFu look like?
    registers[Vx] = sum & 0xFFu;
}

// SUB Vx, Vy
// Set Vx - Vy, meaning of "set VF = NOT borrow?"
// If Vx > Vy -> set VF to 1 otherwise 0 -> subtract and store in Vx
void Chip8::OP_8xy5()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    // [Breakdown?]
    // What here prevents negative values (if that is necessary?)
    if (registers[Vx] > registers[Vy])
    {
        registers[0xF] = 1;
    }
    else
    {
        registers[0xF] = 0;
    }

    registers[Vx] -= registers[Vy];
}

// SHR Vx
// Set Vx = Vx SHR 1 (right shift by 1)
// If least significant bit of Vx is 1 -> set VF to 1 otherwise 0 -> divide Vx by 2
// Right is division by 2 [for unsigned only?] w/w/o remainder (which would be VF)
void Chip8::OP_8xy6()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save LSB [whats that?] in VF
    registers[0xF] = (registers[Vx] & 0x1u);

    registers[Vx] >>= 1;
}

// SUBN Vx, Vy
// Set Vx = Vy - Vx, set VF = NOT borrow
// If Vy > Vx -> set VF to 1 otherwise 0 -> Vx = Vy - Vx
void Chip8::OP_8xy7()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vy] > registers[Vx])
    {
        registers[0xF] = 1;
    }
    else
    {
        registers[0xF] = 0;
    }

    registers[Vx] = registers[Vy] - registers[Vx];
}

// SHL Vx {, Vy}
// Set Vx = Vx SHL 1
// If most significant digit in Vx = 1 -> set VF 1 otherwise 0 -> Vx *= 2
void Chip8::OP_8xyE()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    // Save MSB in VF
    // What does 0x08u represent? What is operation >> 7u?
    registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

    registers[Vx] <<= 1;
}

// SNE Vx, Vy
// Skip next if Vx != Vy
// Why is a skip next instruction all the way up here / addressed as 9xy0?
void Chip8::OP_9xy0()
{
    u_int8_t Vx = (opcode & 0x0F00u) >> 8u;
    u_int8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] != registers[Vy])
    {
        pc += 2;
    }
}

// LD I, addr
// Set I = nnn
// What is I?
void Chip8::OP_Annn()
{
    uint16_t address = opcode & 0x0FFFu;

    index = address;
}

// JP V0, addr
// Jump to location nnn + V0
void Chip8::OP_Bnnn()
{
    uint16_t address = opcode & 0x0FFFu;

    pc = registers[0] + address;
}

// RND Vx, byte
// Set Vx = random byte AND kk
void Chip8::OP_Cxkk()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    registers[Vx] = randByte(randGen) & byte;
}

// DRW Vy, Vy, nibble
// Display n-byte sprite starting at mem location I at (Vx, Vy), set VF = collision
void Chip8::OP_Dxyn()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    // How does this expression get height?
	uint8_t height = opcode & 0x000Fu;

	// Wrap if going beyond screen boundaries
	uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
	uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

	registers[0xF] = 0;

    // for each row in sprite
	for (unsigned int row = 0; row < height; ++row)
	{
        // get byte (stored sequentially)
		uint8_t spriteByte = memory[index + row];

		for (unsigned int col = 0; col < 8; ++col)
		{
			uint8_t spritePixel = spriteByte & (0x80u >> col);
			uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

			// Sprite pixel is on
			if (spritePixel)
			{
				// Screen pixel also on -> collision indicator at VF
				if (*screenPixel == 0xFFFFFFFF)
				{
					registers[0xF] = 1;
				}

				// XOR with sprite pixel[?]
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

// SKP Vx
// Skip next if key w/ value value of Vx is pressed
void Chip8::OP_Ex9E()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[Vx];

    if (keypad[key])
    {
        pc += 2;
    }
}

// SKNP Vx
// Skip next if key w/ value !value of Vx is pressed
void Chip8::OP_ExA1()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    uint8_t key = registers[Vx];

    if (!keypad[key])
    {
        pc += 2;
    }
}

// LD Vx, DT
// Set Vx to delay timer value
void Chip8::OP_Fx07()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[Vx] = delayTimer;
}

// LD Vx, K
// On key press -> store key value in Vx
// "Waiting" can be accomplished by decrementing pc by 2 whenever keypad value [specific one...
// ... or any?] is not detected -> same instruction will run repeatedly
void Chip8::OP_Fx0A()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    if (keypad[0])
    {
        registers[Vx] = 0;
    }
    else if (keypad[1])
    {
        registers[Vx] = 1;
    }
    else if (keypad[2])
    {
        registers[Vx] = 2;
    }
    else if (keypad[3])
    {
        registers[Vx] = 3;
    }
    else if (keypad[4])
    {
        registers[Vx] = 4;
    }
    else if (keypad[5])
    {
        registers[Vx] = 5;
    }
    else if (keypad[6])
    {
        registers[Vx] = 6;
    }
    else if (keypad[7])
    {
        registers[Vx] = 7;
    }
    else if (keypad[8])
    {
        registers[Vx] = 8;
    }
    else if (keypad[9])
    {
        registers[Vx] = 9;
    }
    else if (keypad[10])
    {
        registers[Vx] = 10;
    }
    else if (keypad[11])
    {
        registers[Vx] = 11;
    }
    else if (keypad[12])
    {
        registers[Vx] = 12;
    }
    else if (keypad[13])
    {
        registers[Vx] = 13;
    }
    else if (keypad[14])
    {
        registers[Vx] = 14;
    }
    else if (keypad[15])
    {
        registers[Vx] = 15;
    }
    else
    {
        pc -= 2;
    }
}

// LD DT, Vx
// Set delay timer = Vx
void Chip8::OP_Fx15()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    delayTimer = registers[Vx];
}

// LD ST, Vx
// Set sound timer = Vx
void Chip8::OP_Fx18()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    soundTimer = registers[Vx];
}

// ADD I, Vx
// Set I (/ index) += Vx
void Chip8::OP_Fx1E()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    index += registers[Vx];
}

// Load F, Vx
// Set I = location of sprite for digit Vx
// For font - only start address needed as rest can be sequentially accessed + operated on
// Sprite means font only?
void Chip8::OP_Fx29()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t digit = registers[Vx];

    index = FONTSET_START_ADDRESS + (5 * digit);
}

// LD B, Vx
// Store BCD representation of Vx in indices I, I+1, I+2
// What is BCD representation?
// Is this function only meant to separate a value into its digits
void Chip8::OP_Fx33()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];

    // Ones place
    memory[index + 2] = value % 10;
    value /= 10;

    // Tens place
    memory[index + 1] = value % 10;
    value /= 10;

    // Hundreds place
    memory[index] = value % 10;
}

// LD [I], Vx
// Store registers V0 to Vx in memory starting at I
void Chip8::OP_Fx55()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i)
    {
        memory[index + i] = registers[i];
    }
}

// LD Vx, [I]
// Read registers V0 to Vx from memory startimg at I
void Chip8::OP_Fx65()
{
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i <= Vx; ++i)
    {
        registers[i] = memory[index + i];
    }
}