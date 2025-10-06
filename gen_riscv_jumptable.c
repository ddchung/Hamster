// gen.c

#include <stdio.h>
#include <stdlib.h>

const char *strings[32768];

int main()
{
    for (size_t i = 0; i < 32768; ++i)
    {
        switch (i & 0x1F)
        {
        case 0:
            switch (i & 0x7000)
            {
            case 0:
                strings[i] = "&&case_lb";
                break;
            case 0x1000:
                strings[i] = "&&case_lh";
                break;
            case 0x2000:
                strings[i] = "&&case_lw";
                break;
            case 0x4000:
                strings[i] = "&&case_lbu";
                break;
            case 0x5000:
                strings[i] = "&&case_lhu";
                break;
            default:
                strings[i] = "&&case_invalid_op";
            }
            break;
        case 1:
            strings[i] = "&&case_op_flw";
            break;
        case 3:
            strings[i] = "&&case_op_misc_mem";
            break;
        case 4:
            switch (i & 0x7000)
            {
            case 0:
                strings[i] = "&&case_addi";
                break;
            case 0x1000:
                strings[i] = "&&case_slli";
                break;
            case 0x2000:
                strings[i] = "&&case_slti";
                break;
            case 0x3000:
                strings[i] = "&&case_sltui";
                break;
            case 0x4000:
                strings[i] = "&&case_xori";
                break;
            case 0x5000:
                if ((i >> 5) & 0x20)
                    strings[i] = "&&case_srai";
                else
                    strings[i] = "&&case_srli";
                break;
            case 0x6000:
                strings[i] = "&&case_ori";
                break;
            case 0x7000:
                strings[i] = "&&case_andi";
                break;
            }
            break;
        case 5:
            strings[i] = "&&case_op_auipc";
            break;
        case 8:
            switch (i & 0x7000)
            {
            case 0:
                strings[i] = "&&case_sb";
                break;
            case 0x1000:
                strings[i] = "&&case_sh";
                break;
            case 0x2000:
                strings[i] = "&&case_sw";
                break;
            default:
                strings[i] = "&&case_invalid_op";
            }
            break;
        case 9:
            strings[i] = "&&case_op_fsw";
            break;
        case 11:
            strings[i] = "&&case_op_atomic";
            break;
        case 12:
            if (((i >> 5) & 1 ) == 0)
            {
                switch (i & 0x7000)
                {
                case 0:
                    if ((i >> 5) & 0x20)
                        strings[i] = "&&case_sub";
                    else
                        strings[i] = "&&case_add";
                    break;
                case 0x1000:
                    strings[i] = "&&case_sll";
                    break;
                case 0x2000:
                    strings[i] = "&&case_slt";
                    break;
                case 0x3000:
                    strings[i] = "&&case_sltu";
                    break;
                case 0x4000:
                    strings[i] = "&&case_xor";
                    break;
                case 0x5000:
                    if ((i >> 5) & 0x20)
                        strings[i] = "&&case_sra";
                    else
                        strings[i] = "&&case_srl";
                    break;
                case 0x6000:
                    strings[i] = "&&case_or";
                    break;
                case 0x7000:
                    strings[i] = "&&case_and";
                    break;
                }
            }
            else
            {
                switch (i & 0x7000)
                {
                case 0x0000:
                    strings[i] = "&&case_mul";
                    break;
                case 0x1000:
                    strings[i] = "&&case_mulh";
                    break;
                case 0x2000:
                    strings[i] = "&&case_mulhsu";
                    break;
                case 0x3000:
                    strings[i] = "&&case_mulhu";
                    break;
                case 0x4000:
                    strings[i] = "&&case_div";
                    break;
                case 0x5000:
                    strings[i] = "&&case_divu";
                    break;
                case 0x6000:
                    strings[i] = "&&case_rem";
                    break;
                case 0x7000:
                    strings[i] = "&&case_remu";
                    break;
                }
            }
            break;
        case 13:
            strings[i] = "&&case_op_lui";
            break;
        case 16:
            strings[i] = "&&case_op_fmadd";
            break;
        case 17:
            strings[i] = "&&case_op_fmsub";
            break;
        case 18:
            strings[i] = "&&case_op_fnmsub";
            break;
        case 19:
            strings[i] = "&&case_op_fnmadd";
            break;
        case 20:
            strings[i] = "&&case_op_freg";
            break;
        case 24:
            strings[i] = "&&case_op_branch";
            break;
        case 25:
            strings[i] = "&&case_op_jalr";
            break;
        case 27:
            strings[i] = "&&case_op_jal";
            break;
        case 28:
            strings[i] = "&&case_op_system";
            break;
        default:
            strings[i] = "&&case_invalid_op";
            break;
        }
    }

    printf("{");

    for (size_t i = 0; i < 32768; ++i)
    {
        printf("%s%s", strings[i], i == 32767 ? "" : ", ");

        if (i % 64 == 0)
            printf("\n");
    }

    printf ("};");
}
