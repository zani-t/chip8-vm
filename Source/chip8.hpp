#pragma once

#include <cstdint>
#include <random>

const unsigned int VIDEO_WIDTH = 64;
const unsigned int VIDEO_HEIGHT = 32;

class Chip8
{
public:
    Chip8();

    void LoadROM(char const *filename);
    void Cycle();

    // 16 input values of 0-F (mapped to 1-V on keyboard)
    uint8_t keypad[16]{};

    // 64x32 screen w/ on & off set to 0xFFFFFFFF & 0x00000000
    // Uses XOR of sprite pixel & display pixel
    uint32_t video[64 * 32]{};

private:
    void Table0();
    void Table8();
    void TableE();
    void TableF();

    void OP_NULL();

    void OP_00E0();

    void OP_00EE();

    void OP_1nnn();

    void OP_2nnn();

    void OP_3xkk();

    void OP_4xkk();

    void OP_5xy0();

    void OP_6xkk();

    void OP_7xkk();

    void OP_8xy0();

    void OP_8xy1();

    void OP_8xy2();

    void OP_8xy3();

    void OP_8xy4();

    void OP_8xy5();

    void OP_8xy6();

    void OP_8xy7();

    void OP_8xyE();

    void OP_9xy0();

    void OP_Annn();

    void OP_Bnnn();

    void OP_Cxkk();

    void OP_Dxyn();

    void OP_Ex9E();

    void OP_ExA1();

    void OP_FxO7();

    void OP_FxOA();

    void OP_Fx15();

    void OP_Fx18();

    void OP_Fx1E();

    void OP_Fx29();

    void OP_Fx33();

    void OP_Fx55();

    void OP_Fx65();

    // Is this everything required for a basic CPU to process?

    // CPU storage (16b),, 0x00 - 0xFF (necessitating only [8 bits])
    // Information comes to register from memory to be operated on
    uint8_t registers[16]{};

    // Memory storage (4kb),, 0x000 - 0xFFF (necessitating [16 bits])
    // 0x000 - 0x1FF - typically for interpreter
    // 0x050 - 0x0A0 - required input characters 0 to F
    // 0x200 - 0xFFF - instructions for ROM
    uint8_t memory[4096]{};

    // Storage of memory addresses to be operated on
    uint16_t index{};

    // Holds address of next instruction
    // Opcode [and//memory?] is addressed as two bytes whereas memory is stored per byte - therefore upon a new...
    // instruction the pc must increment by two - [meaning of rest of description?]
    uint16_t pc{};

    // Stack (in form of array) - instruction execution order
    // Memory addresses stored as new instructions added to stack, but not deleted after operation finished
    uint16_t stack[16];

    // Stack pointer - Moves up and down depending on instruction assignment & completion
    uint8_t sp{};

    // Timing (usually when set to nonzero value declining at 60Hz)
    uint8_t delayTimer{};

    // Sound timing
    uint8_t soundTimer{};

    // Operation for CPU - e.g. ->
    // $7522 ([Machine code?])
    // ADD V5, $22 ([Assembly language?])
    // registers[5] += 0x22; (C++ emulator)
    // Add 22 (hex) to register 5
    uint16_t opcode;

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;

    typedef void (Chip8::*Chip8Func)();

    Chip8Func table[0xF + 1];
    Chip8Func table0[0xE + 1];
    Chip8Func table8[0xE + 1];
    Chip8Func tableE[0xE + 1];
    Chip8Func tableF[0x65 + 1];
};