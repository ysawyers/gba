#include "cpu.hpp"

std::uint32_t CPU::thumb_translate_1(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100001101100000000000000000000;
    std::uint32_t shift_amount = (instr >> 6) & 0x1F;
    std::uint32_t rs = (instr >> 3) & 0x7;
    std::uint32_t rd = instr & 0x7;
    std::uint32_t shift_type = (instr >> 11) & 0x3;
    translation |= (rd << 12);
    translation |= (shift_amount << 7);
    translation |= (shift_type << 5);
    translation |= rs;
    return translation;
}

std::uint32_t CPU::thumb_translate_2(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100000000100000000000000000000;
    std::uint32_t rn_or_nn = (instr >> 6) & 0x7;
    std::uint32_t rs = (instr >> 3) & 0x7;
    std::uint32_t rd = instr & 0x7;
    std::uint32_t i = (instr >> 10) & 1;
    std::uint32_t arm_opcode = 0x2 << !((instr >> 9) & 1);
    translation |= (i << 25);
    translation |= (arm_opcode << 21);
    translation |= (rs << 16);
    translation |= (rd << 12);
    translation |= rn_or_nn;
    return translation;
}

std::uint32_t CPU::thumb_translate_3(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100010000100000000000000000000;
    std::uint32_t rd = (instr >> 8) & 0x7;
    std::uint32_t nn = instr & 0xFF;
    std::uint32_t thumb_opcode = (instr >> 11) & 0x3;
    std::uint32_t arm_opcode = 0b1101;
    arm_opcode = (arm_opcode << thumb_opcode) & 0xF;
    arm_opcode >>= (!(~thumb_opcode & 0x3) << 1);
    translation |= (arm_opcode << 21);
    translation |= (rd << 16);
    translation |= (rd << 12);
    translation |= nn;
    return translation;
}

std::uint32_t CPU::thumb_translate_5_alu(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100000000000000000000000000000;
    std::uint32_t rd = (((instr >> 7) & 1) << 3) | (instr & 0x7);
    std::uint32_t rs = (((instr >> 6) & 1) << 3) | ((instr >> 3) & 0x7);
    std::uint32_t arm_opcode = 0b110100;
    std::uint32_t thumb_opcode = (instr >> 8) & 0x3;
    arm_opcode = (arm_opcode >> thumb_opcode) & 0xF;
    translation |= ((thumb_opcode == 0x1) << 20);
    translation |= (arm_opcode << 21);
    translation |= (rd << 16);
    translation |= (rd << 12);
    translation |= rs;
    return translation;
}

std::uint32_t CPU::thumb_translate_5_bx(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100001001011111111111100010000;
    std::uint32_t rs = (((instr >> 6) & 1) << 3) | ((instr >> 3) & 0x7);
    translation |= rs;
    return translation;
}

std::uint32_t CPU::thumb_translate_7(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100111100000000000000000000000;
    std::uint32_t ro = (instr >> 6) & 0x7;
    std::uint32_t rb = (instr >> 3) & 0x7;
    std::uint32_t rd = instr & 0x7;
    std::uint32_t b = (instr >> 10) & 1;
    std::uint32_t l = (instr >> 11) & 1;
    translation |= (b << 22);
    translation |= (l << 20);
    translation |= (rb << 16);
    translation |= (rd << 12);
    translation |= ro;
    return translation;
}

std::uint32_t CPU::thumb_translate_8(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100001100000000000000010010000;
    std::uint32_t ro = (instr >> 6) & 0x7;
    std::uint32_t rb = (instr >> 3) & 0x7;
    std::uint32_t rd = instr & 0x7;
    std::uint32_t thumb_opcode = (instr >> 10) & 0x3;
    std::uint32_t arm_opcode = (((thumb_opcode << 1) & 0x3) | ((thumb_opcode >> 1) & 1)) | !thumb_opcode;
    std::uint32_t l = !!thumb_opcode;
    translation |= (l << 20);
    translation |= (rb << 16);
    translation |= (rd << 12);
    translation |= (arm_opcode << 5);
    translation |= ro;
    return translation;
}

std::uint32_t CPU::thumb_translate_9(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100101100000000000000000000000;
    std::uint32_t rb = (instr >> 3) & 0x7;
    std::uint32_t rd = instr & 0x7;
    std::uint32_t b = (instr >> 12) & 1;
    std::uint32_t l = (instr >> 11) & 1;
    std::uint32_t nn = ((instr >> 6) & 0x1F) << (!b << 1);
    translation |= (b << 22);
    translation |= (l << 20);
    translation |= (rb << 16);
    translation |= (rd << 12);
    translation |= nn;
    return translation;
}

std::uint32_t CPU::thumb_translate_10(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100001110000000000000010110000;
    std::uint32_t rb = (instr >> 3) & 0x7;
    std::uint32_t rd = instr & 0x7;
    std::uint32_t nn = ((instr >> 6) & 0x1F) << 1;
    std::uint32_t l = ((instr >> 11) & 1);
    translation |= (l << 20);
    translation |= (rd << 12);
    translation |= (rb << 16);
    translation |= (((nn & 0xF0) >> 4) << 8);
    translation |= (nn & 0xF);
    return translation;
}

std::uint32_t CPU::thumb_translate_11(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100101100011010000000000000000;
    std::uint32_t l = (instr >> 11) & 1;
    std::uint32_t rd = (instr >> 8) & 0x7;
    std::uint32_t nn = (instr & 0xFF) << 2;
    translation |= (l << 20);
    translation |= (rd << 12);
    translation |= nn;
    return translation;
}

std::uint32_t CPU::thumb_translate_13(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11100010000011011101111100000000;
    translation |= (4 << (21 - ((instr >> 7) & 1)));
    translation |= (instr & 0x7F);
    return translation;
}

std::uint32_t CPU::thumb_translate_14(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11101000001011010000000000000000;
    std::uint32_t opcode = (instr >> 11) & 1;
    std::uint32_t pc_or_lr = (instr >> 8) & 1;
    std::uint32_t reg_list = instr & 0xFF;
    translation |= (!opcode << 24);
    translation |= (opcode << 23);
    translation |= (opcode << 20);
    translation |= reg_list;
    translation |= ((pc_or_lr & 1) << (0xE | opcode));
    return translation;
}

std::uint32_t CPU::thumb_translate_15(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11101000101000000000000000000000;
    std::uint32_t opcode = (instr >> 11) & 1;
    std::uint32_t rb = (instr >> 8) & 0x7;
    std::uint32_t reg_list = instr & 0xFF;
    translation |= (opcode << 20);
    translation |= (rb << 16);
    translation |= reg_list;
    return translation;
}

std::uint32_t CPU::thumb_translate_16(std::uint16_t instr) 
{
    std::uint32_t translation = 0b00001010000000000000000000000000;
    std::uint32_t cond = (instr >> 8) & 0xF;
    std::uint32_t offset = static_cast<std::int32_t>(static_cast<std::int8_t>((instr & 0xFF)));
    translation |= (cond << 28);
    translation |= (offset & 0xFFFFFF);
    return translation;
}

std::uint32_t CPU::thumb_translate_17(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11101111000000000000000000000000;
    translation |= (instr & 0xFF);
    return translation;
}

std::uint32_t CPU::thumb_translate_18(std::uint16_t instr) 
{
    std::uint32_t translation = 0b11101010000000000000000000000000;
    std::uint32_t offset = static_cast<std::int32_t>((static_cast<std::int16_t>((instr & 0x7FF) << 5) >> 5));
    translation |= (offset & 0xFFFFFF);
    return translation;
}
