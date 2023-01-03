#ifndef	_ELF_MSG_DOT_H
#define	_ELF_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_ELFCLASSNONE	1
#define	MSG_ELFCLASSNONE_SIZE	12

#define	MSG_ELFCLASSNONE_ALT	14
#define	MSG_ELFCLASSNONE_ALT_SIZE	4

#define	MSG_ELFCLASS32	19
#define	MSG_ELFCLASS32_SIZE	10

#define	MSG_ELFCLASS32_ALT	30
#define	MSG_ELFCLASS32_ALT_SIZE	6

#define	MSG_ELFCLASS64	37
#define	MSG_ELFCLASS64_SIZE	10

#define	MSG_ELFCLASS64_ALT	48
#define	MSG_ELFCLASS64_ALT_SIZE	6

#define	MSG_ELFDATANONE	55
#define	MSG_ELFDATANONE_SIZE	11

#define	MSG_ELFDATANONE_ALT	14
#define	MSG_ELFDATANONE_ALT_SIZE	4

#define	MSG_ELFDATA2LSB	67
#define	MSG_ELFDATA2LSB_SIZE	11

#define	MSG_ELFDATA2LSB_ALT1	74
#define	MSG_ELFDATA2LSB_ALT1_SIZE	4

#define	MSG_ELFDATA2LSB_ALT2	75
#define	MSG_ELFDATA2LSB_ALT2_SIZE	3

#define	MSG_ELFDATA2MSB	79
#define	MSG_ELFDATA2MSB_SIZE	11

#define	MSG_ELFDATA2MSB_ALT1	86
#define	MSG_ELFDATA2MSB_ALT1_SIZE	4

#define	MSG_ELFDATA2MSB_ALT2	87
#define	MSG_ELFDATA2MSB_ALT2_SIZE	3

#define	MSG_EM_NONE	91
#define	MSG_EM_NONE_SIZE	7

#define	MSG_EM_NONE_ALT	99
#define	MSG_EM_NONE_ALT_SIZE	7

#define	MSG_EM_M32	107
#define	MSG_EM_M32_SIZE	6

#define	MSG_EM_M32_ALT	114
#define	MSG_EM_M32_ALT_SIZE	7

#define	MSG_EM_SPARC	122
#define	MSG_EM_SPARC_SIZE	8

#define	MSG_EM_SPARC_ALT	125
#define	MSG_EM_SPARC_ALT_SIZE	5

#define	MSG_EM_386	131
#define	MSG_EM_386_SIZE	6

#define	MSG_EM_386_ALT	138
#define	MSG_EM_386_ALT_SIZE	5

#define	MSG_EM_68K	144
#define	MSG_EM_68K_SIZE	6

#define	MSG_EM_68K_ALT	151
#define	MSG_EM_68K_ALT_SIZE	5

#define	MSG_EM_88K	157
#define	MSG_EM_88K_SIZE	6

#define	MSG_EM_88K_ALT	164
#define	MSG_EM_88K_ALT_SIZE	5

#define	MSG_EM_486	170
#define	MSG_EM_486_SIZE	6

#define	MSG_EM_486_ALT	177
#define	MSG_EM_486_ALT_SIZE	5

#define	MSG_EM_860	183
#define	MSG_EM_860_SIZE	6

#define	MSG_EM_860_ALT	190
#define	MSG_EM_860_ALT_SIZE	4

#define	MSG_EM_MIPS	195
#define	MSG_EM_MIPS_SIZE	7

#define	MSG_EM_MIPS_ALT	203
#define	MSG_EM_MIPS_ALT_SIZE	9

#define	MSG_EM_S370	213
#define	MSG_EM_S370_SIZE	7

#define	MSG_EM_MIPS_RS3_LE	221
#define	MSG_EM_MIPS_RS3_LE_SIZE	14

#define	MSG_EM_MIPS_RS3_LE_ALT	236
#define	MSG_EM_MIPS_RS3_LE_ALT_SIZE	9

#define	MSG_EM_RS6000	246
#define	MSG_EM_RS6000_SIZE	9

#define	MSG_EM_RS6000_ALT	249
#define	MSG_EM_RS6000_ALT_SIZE	6

#define	MSG_EM_UNKNOWN12	256
#define	MSG_EM_UNKNOWN12_SIZE	12

#define	MSG_EM_UNKNOWN13	269
#define	MSG_EM_UNKNOWN13_SIZE	12

#define	MSG_EM_UNKNOWN14	282
#define	MSG_EM_UNKNOWN14_SIZE	12

#define	MSG_EM_PA_RISC	295
#define	MSG_EM_PA_RISC_SIZE	10

#define	MSG_EM_PA_RISC_ALT	298
#define	MSG_EM_PA_RISC_ALT_SIZE	7

#define	MSG_EM_nCUBE	306
#define	MSG_EM_nCUBE_SIZE	8

#define	MSG_EM_nCUBE_ALT	309
#define	MSG_EM_nCUBE_ALT_SIZE	5

#define	MSG_EM_VPP500	315
#define	MSG_EM_VPP500_SIZE	9

#define	MSG_EM_VPP500_ALT	318
#define	MSG_EM_VPP500_ALT_SIZE	6

#define	MSG_EM_SPARC32PLUS	325
#define	MSG_EM_SPARC32PLUS_SIZE	14

#define	MSG_EM_SPARC32PLUS_ALT	328
#define	MSG_EM_SPARC32PLUS_ALT_SIZE	11

#define	MSG_EM_960	340
#define	MSG_EM_960_SIZE	6

#define	MSG_EM_PPC	347
#define	MSG_EM_PPC_SIZE	6

#define	MSG_EM_PPC_ALT	354
#define	MSG_EM_PPC_ALT_SIZE	7

#define	MSG_EM_PPC64	362
#define	MSG_EM_PPC64_SIZE	8

#define	MSG_EM_PPC64_ALT	371
#define	MSG_EM_PPC64_ALT_SIZE	9

#define	MSG_EM_S390	381
#define	MSG_EM_S390_SIZE	7

#define	MSG_EM_UNKNOWN23	389
#define	MSG_EM_UNKNOWN23_SIZE	12

#define	MSG_EM_UNKNOWN24	402
#define	MSG_EM_UNKNOWN24_SIZE	12

#define	MSG_EM_UNKNOWN25	415
#define	MSG_EM_UNKNOWN25_SIZE	12

#define	MSG_EM_UNKNOWN26	428
#define	MSG_EM_UNKNOWN26_SIZE	12

#define	MSG_EM_UNKNOWN27	441
#define	MSG_EM_UNKNOWN27_SIZE	12

#define	MSG_EM_UNKNOWN28	454
#define	MSG_EM_UNKNOWN28_SIZE	12

#define	MSG_EM_UNKNOWN29	467
#define	MSG_EM_UNKNOWN29_SIZE	12

#define	MSG_EM_UNKNOWN30	480
#define	MSG_EM_UNKNOWN30_SIZE	12

#define	MSG_EM_UNKNOWN31	493
#define	MSG_EM_UNKNOWN31_SIZE	12

#define	MSG_EM_UNKNOWN32	506
#define	MSG_EM_UNKNOWN32_SIZE	12

#define	MSG_EM_UNKNOWN33	519
#define	MSG_EM_UNKNOWN33_SIZE	12

#define	MSG_EM_UNKNOWN34	532
#define	MSG_EM_UNKNOWN34_SIZE	12

#define	MSG_EM_UNKNOWN35	545
#define	MSG_EM_UNKNOWN35_SIZE	12

#define	MSG_EM_V800	558
#define	MSG_EM_V800_SIZE	7

#define	MSG_EM_FR20	566
#define	MSG_EM_FR20_SIZE	7

#define	MSG_EM_RH32	574
#define	MSG_EM_RH32_SIZE	7

#define	MSG_EM_RCE	582
#define	MSG_EM_RCE_SIZE	6

#define	MSG_EM_ARM	589
#define	MSG_EM_ARM_SIZE	6

#define	MSG_EM_ARM_ALT	592
#define	MSG_EM_ARM_ALT_SIZE	3

#define	MSG_EM_ALPHA	596
#define	MSG_EM_ALPHA_SIZE	8

#define	MSG_EM_ALPHA_ALT	605
#define	MSG_EM_ALPHA_ALT_SIZE	5

#define	MSG_EM_SH	611
#define	MSG_EM_SH_SIZE	5

#define	MSG_EM_SPARCV9	617
#define	MSG_EM_SPARCV9_SIZE	10

#define	MSG_EM_SPARCV9_ALT	620
#define	MSG_EM_SPARCV9_ALT_SIZE	7

#define	MSG_EM_TRICORE	628
#define	MSG_EM_TRICORE_SIZE	10

#define	MSG_EM_ARC	639
#define	MSG_EM_ARC_SIZE	6

#define	MSG_EM_H8_300	646
#define	MSG_EM_H8_300_SIZE	9

#define	MSG_EM_H8_300H	656
#define	MSG_EM_H8_300H_SIZE	10

#define	MSG_EM_H8S	667
#define	MSG_EM_H8S_SIZE	6

#define	MSG_EM_H8_500	674
#define	MSG_EM_H8_500_SIZE	9

#define	MSG_EM_IA_64	684
#define	MSG_EM_IA_64_SIZE	8

#define	MSG_EM_IA_64_ALT	687
#define	MSG_EM_IA_64_ALT_SIZE	5

#define	MSG_EM_MIPS_X	693
#define	MSG_EM_MIPS_X_SIZE	9

#define	MSG_EM_COLDFIRE	703
#define	MSG_EM_COLDFIRE_SIZE	11

#define	MSG_EM_68HC12	715
#define	MSG_EM_68HC12_SIZE	9

#define	MSG_EM_MMA	725
#define	MSG_EM_MMA_SIZE	6

#define	MSG_EM_PCP	732
#define	MSG_EM_PCP_SIZE	6

#define	MSG_EM_NCPU	739
#define	MSG_EM_NCPU_SIZE	7

#define	MSG_EM_NDR1	747
#define	MSG_EM_NDR1_SIZE	7

#define	MSG_EM_STARCORE	755
#define	MSG_EM_STARCORE_SIZE	11

#define	MSG_EM_ME16	767
#define	MSG_EM_ME16_SIZE	7

#define	MSG_EM_ST100	775
#define	MSG_EM_ST100_SIZE	8

#define	MSG_EM_TINYJ	784
#define	MSG_EM_TINYJ_SIZE	8

#define	MSG_EM_AMD64	793
#define	MSG_EM_AMD64_SIZE	8

#define	MSG_EM_AMD64_ALT	796
#define	MSG_EM_AMD64_ALT_SIZE	5

#define	MSG_EM_PDSP	802
#define	MSG_EM_PDSP_SIZE	7

#define	MSG_EM_UNKNOWN64	810
#define	MSG_EM_UNKNOWN64_SIZE	12

#define	MSG_EM_UNKNOWN65	823
#define	MSG_EM_UNKNOWN65_SIZE	12

#define	MSG_EM_FX66	836
#define	MSG_EM_FX66_SIZE	7

#define	MSG_EM_ST9PLUS	844
#define	MSG_EM_ST9PLUS_SIZE	10

#define	MSG_EM_ST7	855
#define	MSG_EM_ST7_SIZE	6

#define	MSG_EM_68HC16	862
#define	MSG_EM_68HC16_SIZE	9

#define	MSG_EM_68HC11	872
#define	MSG_EM_68HC11_SIZE	9

#define	MSG_EM_68HC08	882
#define	MSG_EM_68HC08_SIZE	9

#define	MSG_EM_68HC05	892
#define	MSG_EM_68HC05_SIZE	9

#define	MSG_EM_SVX	902
#define	MSG_EM_SVX_SIZE	6

#define	MSG_EM_ST19	909
#define	MSG_EM_ST19_SIZE	7

#define	MSG_EM_VAX	917
#define	MSG_EM_VAX_SIZE	6

#define	MSG_EM_VAX_ALT	920
#define	MSG_EM_VAX_ALT_SIZE	3

#define	MSG_EM_CRIS	924
#define	MSG_EM_CRIS_SIZE	7

#define	MSG_EM_JAVELIN	932
#define	MSG_EM_JAVELIN_SIZE	10

#define	MSG_EM_FIREPATH	943
#define	MSG_EM_FIREPATH_SIZE	11

#define	MSG_EM_ZSP	955
#define	MSG_EM_ZSP_SIZE	6

#define	MSG_EM_MMIX	962
#define	MSG_EM_MMIX_SIZE	7

#define	MSG_EM_HUANY	970
#define	MSG_EM_HUANY_SIZE	8

#define	MSG_EM_PRISM	979
#define	MSG_EM_PRISM_SIZE	8

#define	MSG_EM_AVR	988
#define	MSG_EM_AVR_SIZE	6

#define	MSG_EM_FR30	995
#define	MSG_EM_FR30_SIZE	7

#define	MSG_EM_D10V	1003
#define	MSG_EM_D10V_SIZE	7

#define	MSG_EM_D30V	1011
#define	MSG_EM_D30V_SIZE	7

#define	MSG_EM_V850	1019
#define	MSG_EM_V850_SIZE	7

#define	MSG_EM_M32R	1027
#define	MSG_EM_M32R_SIZE	7

#define	MSG_EM_MN10300	1035
#define	MSG_EM_MN10300_SIZE	10

#define	MSG_EM_MN10200	1046
#define	MSG_EM_MN10200_SIZE	10

#define	MSG_EM_PJ	1057
#define	MSG_EM_PJ_SIZE	5

#define	MSG_EM_OPENRISC	1063
#define	MSG_EM_OPENRISC_SIZE	11

#define	MSG_EM_ARC_A5	1075
#define	MSG_EM_ARC_A5_SIZE	9

#define	MSG_EM_XTENSA	1085
#define	MSG_EM_XTENSA_SIZE	9

#define	MSG_ET_NONE	1095
#define	MSG_ET_NONE_SIZE	7

#define	MSG_ET_NONE_ALT	14
#define	MSG_ET_NONE_ALT_SIZE	4

#define	MSG_ET_REL	1103
#define	MSG_ET_REL_SIZE	6

#define	MSG_ET_REL_ALT	1110
#define	MSG_ET_REL_ALT_SIZE	5

#define	MSG_ET_EXEC	1116
#define	MSG_ET_EXEC_SIZE	7

#define	MSG_ET_EXEC_ALT	1124
#define	MSG_ET_EXEC_ALT_SIZE	4

#define	MSG_ET_DYN	1129
#define	MSG_ET_DYN_SIZE	6

#define	MSG_ET_DYN_ALT	1136
#define	MSG_ET_DYN_ALT_SIZE	3

#define	MSG_ET_CORE	1140
#define	MSG_ET_CORE_SIZE	7

#define	MSG_ET_CORE_ALT	1148
#define	MSG_ET_CORE_ALT_SIZE	4

#define	MSG_ET_SUNWPSEUDO	1153
#define	MSG_ET_SUNWPSEUDO_SIZE	13

#define	MSG_ET_SUNWPSEUDO_ALT	1167
#define	MSG_ET_SUNWPSEUDO_ALT_SIZE	10

#define	MSG_EV_NONE	1178
#define	MSG_EV_NONE_SIZE	7

#define	MSG_EV_NONE_ALT	1186
#define	MSG_EV_NONE_ALT_SIZE	7

#define	MSG_EV_CURRENT	1194
#define	MSG_EV_CURRENT_SIZE	10

#define	MSG_EV_CURRENT_ALT	1205
#define	MSG_EV_CURRENT_ALT_SIZE	7

#define	MSG_EF_SPARC_32PLUS	1213
#define	MSG_EF_SPARC_32PLUS_SIZE	15

#define	MSG_EF_SPARC_SUN_US1	1229
#define	MSG_EF_SPARC_SUN_US1_SIZE	16

#define	MSG_EF_SPARC_SUN_US3	1246
#define	MSG_EF_SPARC_SUN_US3_SIZE	16

#define	MSG_EF_SPARC_HAL_R1	1263
#define	MSG_EF_SPARC_HAL_R1_SIZE	15

#define	MSG_EF_SPARCV9_TSO	1279
#define	MSG_EF_SPARCV9_TSO_SIZE	14

#define	MSG_EF_SPARCV9_PSO	1294
#define	MSG_EF_SPARCV9_PSO_SIZE	14

#define	MSG_EF_SPARCV9_RMO	1309
#define	MSG_EF_SPARCV9_RMO_SIZE	14

#define	MSG_OSABI_NONE	1324
#define	MSG_OSABI_NONE_SIZE	13

#define	MSG_OSABI_NONE_ALT	1338
#define	MSG_OSABI_NONE_ALT_SIZE	12

#define	MSG_OSABI_HPUX	1351
#define	MSG_OSABI_HPUX_SIZE	13

#define	MSG_OSABI_HPUX_ALT	1365
#define	MSG_OSABI_HPUX_ALT_SIZE	5

#define	MSG_OSABI_NETBSD	1371
#define	MSG_OSABI_NETBSD_SIZE	15

#define	MSG_OSABI_NETBSD_ALT	1387
#define	MSG_OSABI_NETBSD_ALT_SIZE	6

#define	MSG_OSABI_LINUX	1394
#define	MSG_OSABI_LINUX_SIZE	14

#define	MSG_OSABI_LINUX_ALT	1409
#define	MSG_OSABI_LINUX_ALT_SIZE	5

#define	MSG_OSABI_UNKNOWN4	1415
#define	MSG_OSABI_UNKNOWN4_SIZE	17

#define	MSG_OSABI_UNKNOWN5	1433
#define	MSG_OSABI_UNKNOWN5_SIZE	17

#define	MSG_OSABI_SOLARIS	1451
#define	MSG_OSABI_SOLARIS_SIZE	16

#define	MSG_OSABI_SOLARIS_ALT	1468
#define	MSG_OSABI_SOLARIS_ALT_SIZE	7

#define	MSG_OSABI_AIX	1476
#define	MSG_OSABI_AIX_SIZE	12

#define	MSG_OSABI_AIX_ALT	1485
#define	MSG_OSABI_AIX_ALT_SIZE	3

#define	MSG_OSABI_IRIX	1489
#define	MSG_OSABI_IRIX_SIZE	13

#define	MSG_OSABI_IRIX_ALT	1498
#define	MSG_OSABI_IRIX_ALT_SIZE	4

#define	MSG_OSABI_FREEBSD	1503
#define	MSG_OSABI_FREEBSD_SIZE	16

#define	MSG_OSABI_FREEBSD_ALT	1520
#define	MSG_OSABI_FREEBSD_ALT_SIZE	7

#define	MSG_OSABI_TRU64	1528
#define	MSG_OSABI_TRU64_SIZE	14

#define	MSG_OSABI_TRU64_ALT	1543
#define	MSG_OSABI_TRU64_ALT_SIZE	5

#define	MSG_OSABI_MODESTO	1549
#define	MSG_OSABI_MODESTO_SIZE	16

#define	MSG_OSABI_MODESTO_ALT	1566
#define	MSG_OSABI_MODESTO_ALT_SIZE	7

#define	MSG_OSABI_OPENBSD	1574
#define	MSG_OSABI_OPENBSD_SIZE	16

#define	MSG_OSABI_OPENBSD_ALT	1591
#define	MSG_OSABI_OPENBSD_ALT_SIZE	7

#define	MSG_OSABI_OPENVMS	1599
#define	MSG_OSABI_OPENVMS_SIZE	16

#define	MSG_OSABI_OPENVMS_ALT	1616
#define	MSG_OSABI_OPENVMS_ALT_SIZE	7

#define	MSG_OSABI_NSK	1624
#define	MSG_OSABI_NSK_SIZE	12

#define	MSG_OSABI_NSK_ALT	1633
#define	MSG_OSABI_NSK_ALT_SIZE	3

#define	MSG_OSABI_AROS	1637
#define	MSG_OSABI_AROS_SIZE	13

#define	MSG_OSABI_AROS_ALT	1651
#define	MSG_OSABI_AROS_ALT_SIZE	17

#define	MSG_OSABI_ARM	1669
#define	MSG_OSABI_ARM_SIZE	12

#define	MSG_OSABI_ARM_ALT	592
#define	MSG_OSABI_ARM_ALT_SIZE	3

#define	MSG_OSABI_STANDALONE	1682
#define	MSG_OSABI_STANDALONE_SIZE	19

#define	MSG_OSABI_STANDALONE_ALT	1702
#define	MSG_OSABI_STANDALONE_ALT_SIZE	10

#define	MSG_GBL_ZERO	193
#define	MSG_GBL_ZERO_SIZE	1

#define	MSG_STR_EMPTY	0
#define	MSG_STR_EMPTY_SIZE	0

static const char __sgs_msg[1713] = { 
/*    0 */ 0x00,  0x45,  0x4c,  0x46,  0x43,  0x4c,  0x41,  0x53,  0x53,  0x4e,
/*   10 */ 0x4f,  0x4e,  0x45,  0x00,  0x4e,  0x6f,  0x6e,  0x65,  0x00,  0x45,
/*   20 */ 0x4c,  0x46,  0x43,  0x4c,  0x41,  0x53,  0x53,  0x33,  0x32,  0x00,
/*   30 */ 0x33,  0x32,  0x2d,  0x62,  0x69,  0x74,  0x00,  0x45,  0x4c,  0x46,
/*   40 */ 0x43,  0x4c,  0x41,  0x53,  0x53,  0x36,  0x34,  0x00,  0x36,  0x34,
/*   50 */ 0x2d,  0x62,  0x69,  0x74,  0x00,  0x45,  0x4c,  0x46,  0x44,  0x41,
/*   60 */ 0x54,  0x41,  0x4e,  0x4f,  0x4e,  0x45,  0x00,  0x45,  0x4c,  0x46,
/*   70 */ 0x44,  0x41,  0x54,  0x41,  0x32,  0x4c,  0x53,  0x42,  0x00,  0x45,
/*   80 */ 0x4c,  0x46,  0x44,  0x41,  0x54,  0x41,  0x32,  0x4d,  0x53,  0x42,
/*   90 */ 0x00,  0x45,  0x4d,  0x5f,  0x4e,  0x4f,  0x4e,  0x45,  0x00,  0x4e,
/*  100 */ 0x6f,  0x20,  0x6d,  0x61,  0x63,  0x68,  0x00,  0x45,  0x4d,  0x5f,
/*  110 */ 0x4d,  0x33,  0x32,  0x00,  0x57,  0x45,  0x33,  0x32,  0x31,  0x30,
/*  120 */ 0x30,  0x00,  0x45,  0x4d,  0x5f,  0x53,  0x50,  0x41,  0x52,  0x43,
/*  130 */ 0x00,  0x45,  0x4d,  0x5f,  0x33,  0x38,  0x36,  0x00,  0x38,  0x30,
/*  140 */ 0x33,  0x38,  0x36,  0x00,  0x45,  0x4d,  0x5f,  0x36,  0x38,  0x4b,
/*  150 */ 0x00,  0x36,  0x38,  0x30,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,
/*  160 */ 0x38,  0x38,  0x4b,  0x00,  0x38,  0x38,  0x30,  0x30,  0x30,  0x00,
/*  170 */ 0x45,  0x4d,  0x5f,  0x34,  0x38,  0x36,  0x00,  0x38,  0x30,  0x34,
/*  180 */ 0x38,  0x36,  0x00,  0x45,  0x4d,  0x5f,  0x38,  0x36,  0x30,  0x00,
/*  190 */ 0x69,  0x38,  0x36,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x4d,  0x49,
/*  200 */ 0x50,  0x53,  0x00,  0x52,  0x53,  0x33,  0x30,  0x30,  0x30,  0x5f,
/*  210 */ 0x42,  0x45,  0x00,  0x45,  0x4d,  0x5f,  0x53,  0x33,  0x37,  0x30,
/*  220 */ 0x00,  0x45,  0x4d,  0x5f,  0x4d,  0x49,  0x50,  0x53,  0x5f,  0x52,
/*  230 */ 0x53,  0x33,  0x5f,  0x4c,  0x45,  0x00,  0x52,  0x53,  0x33,  0x30,
/*  240 */ 0x30,  0x30,  0x5f,  0x4c,  0x45,  0x00,  0x45,  0x4d,  0x5f,  0x52,
/*  250 */ 0x53,  0x36,  0x30,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x55,
/*  260 */ 0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x31,  0x32,  0x00,  0x45,
/*  270 */ 0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x31,
/*  280 */ 0x33,  0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,
/*  290 */ 0x57,  0x4e,  0x31,  0x34,  0x00,  0x45,  0x4d,  0x5f,  0x50,  0x41,
/*  300 */ 0x5f,  0x52,  0x49,  0x53,  0x43,  0x00,  0x45,  0x4d,  0x5f,  0x6e,
/*  310 */ 0x43,  0x55,  0x42,  0x45,  0x00,  0x45,  0x4d,  0x5f,  0x56,  0x50,
/*  320 */ 0x50,  0x35,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x53,  0x50,
/*  330 */ 0x41,  0x52,  0x43,  0x33,  0x32,  0x50,  0x4c,  0x55,  0x53,  0x00,
/*  340 */ 0x45,  0x4d,  0x5f,  0x39,  0x36,  0x30,  0x00,  0x45,  0x4d,  0x5f,
/*  350 */ 0x50,  0x50,  0x43,  0x00,  0x50,  0x6f,  0x77,  0x65,  0x72,  0x50,
/*  360 */ 0x43,  0x00,  0x45,  0x4d,  0x5f,  0x50,  0x50,  0x43,  0x36,  0x34,
/*  370 */ 0x00,  0x50,  0x6f,  0x77,  0x65,  0x72,  0x50,  0x43,  0x36,  0x34,
/*  380 */ 0x00,  0x45,  0x4d,  0x5f,  0x53,  0x33,  0x39,  0x30,  0x00,  0x45,
/*  390 */ 0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x32,
/*  400 */ 0x33,  0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,
/*  410 */ 0x57,  0x4e,  0x32,  0x34,  0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,
/*  420 */ 0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x32,  0x35,  0x00,  0x45,  0x4d,
/*  430 */ 0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x32,  0x36,
/*  440 */ 0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,
/*  450 */ 0x4e,  0x32,  0x37,  0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,
/*  460 */ 0x4e,  0x4f,  0x57,  0x4e,  0x32,  0x38,  0x00,  0x45,  0x4d,  0x5f,
/*  470 */ 0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x32,  0x39,  0x00,
/*  480 */ 0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,
/*  490 */ 0x33,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,
/*  500 */ 0x4f,  0x57,  0x4e,  0x33,  0x31,  0x00,  0x45,  0x4d,  0x5f,  0x55,
/*  510 */ 0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x33,  0x32,  0x00,  0x45,
/*  520 */ 0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x33,
/*  530 */ 0x33,  0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,
/*  540 */ 0x57,  0x4e,  0x33,  0x34,  0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,
/*  550 */ 0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x33,  0x35,  0x00,  0x45,  0x4d,
/*  560 */ 0x5f,  0x56,  0x38,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x46,
/*  570 */ 0x52,  0x32,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x52,  0x48,  0x33,
/*  580 */ 0x32,  0x00,  0x45,  0x4d,  0x5f,  0x52,  0x43,  0x45,  0x00,  0x45,
/*  590 */ 0x4d,  0x5f,  0x41,  0x52,  0x4d,  0x00,  0x45,  0x4d,  0x5f,  0x41,
/*  600 */ 0x4c,  0x50,  0x48,  0x41,  0x00,  0x41,  0x6c,  0x70,  0x68,  0x61,
/*  610 */ 0x00,  0x45,  0x4d,  0x5f,  0x53,  0x48,  0x00,  0x45,  0x4d,  0x5f,
/*  620 */ 0x53,  0x50,  0x41,  0x52,  0x43,  0x56,  0x39,  0x00,  0x45,  0x4d,
/*  630 */ 0x5f,  0x54,  0x52,  0x49,  0x43,  0x4f,  0x52,  0x45,  0x00,  0x45,
/*  640 */ 0x4d,  0x5f,  0x41,  0x52,  0x43,  0x00,  0x45,  0x4d,  0x5f,  0x48,
/*  650 */ 0x38,  0x5f,  0x33,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x48,
/*  660 */ 0x38,  0x5f,  0x33,  0x30,  0x30,  0x48,  0x00,  0x45,  0x4d,  0x5f,
/*  670 */ 0x48,  0x38,  0x53,  0x00,  0x45,  0x4d,  0x5f,  0x48,  0x38,  0x5f,
/*  680 */ 0x35,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x49,  0x41,  0x5f,
/*  690 */ 0x36,  0x34,  0x00,  0x45,  0x4d,  0x5f,  0x4d,  0x49,  0x50,  0x53,
/*  700 */ 0x5f,  0x58,  0x00,  0x45,  0x4d,  0x5f,  0x43,  0x4f,  0x4c,  0x44,
/*  710 */ 0x46,  0x49,  0x52,  0x45,  0x00,  0x45,  0x4d,  0x5f,  0x36,  0x38,
/*  720 */ 0x48,  0x43,  0x31,  0x32,  0x00,  0x45,  0x4d,  0x5f,  0x4d,  0x4d,
/*  730 */ 0x41,  0x00,  0x45,  0x4d,  0x5f,  0x50,  0x43,  0x50,  0x00,  0x45,
/*  740 */ 0x4d,  0x5f,  0x4e,  0x43,  0x50,  0x55,  0x00,  0x45,  0x4d,  0x5f,
/*  750 */ 0x4e,  0x44,  0x52,  0x31,  0x00,  0x45,  0x4d,  0x5f,  0x53,  0x54,
/*  760 */ 0x41,  0x52,  0x43,  0x4f,  0x52,  0x45,  0x00,  0x45,  0x4d,  0x5f,
/*  770 */ 0x4d,  0x45,  0x31,  0x36,  0x00,  0x45,  0x4d,  0x5f,  0x53,  0x54,
/*  780 */ 0x31,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x54,  0x49,  0x4e,
/*  790 */ 0x59,  0x4a,  0x00,  0x45,  0x4d,  0x5f,  0x41,  0x4d,  0x44,  0x36,
/*  800 */ 0x34,  0x00,  0x45,  0x4d,  0x5f,  0x50,  0x44,  0x53,  0x50,  0x00,
/*  810 */ 0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,
/*  820 */ 0x36,  0x34,  0x00,  0x45,  0x4d,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,
/*  830 */ 0x4f,  0x57,  0x4e,  0x36,  0x35,  0x00,  0x45,  0x4d,  0x5f,  0x46,
/*  840 */ 0x58,  0x36,  0x36,  0x00,  0x45,  0x4d,  0x5f,  0x53,  0x54,  0x39,
/*  850 */ 0x50,  0x4c,  0x55,  0x53,  0x00,  0x45,  0x4d,  0x5f,  0x53,  0x54,
/*  860 */ 0x37,  0x00,  0x45,  0x4d,  0x5f,  0x36,  0x38,  0x48,  0x43,  0x31,
/*  870 */ 0x36,  0x00,  0x45,  0x4d,  0x5f,  0x36,  0x38,  0x48,  0x43,  0x31,
/*  880 */ 0x31,  0x00,  0x45,  0x4d,  0x5f,  0x36,  0x38,  0x48,  0x43,  0x30,
/*  890 */ 0x38,  0x00,  0x45,  0x4d,  0x5f,  0x36,  0x38,  0x48,  0x43,  0x30,
/*  900 */ 0x35,  0x00,  0x45,  0x4d,  0x5f,  0x53,  0x56,  0x58,  0x00,  0x45,
/*  910 */ 0x4d,  0x5f,  0x53,  0x54,  0x31,  0x39,  0x00,  0x45,  0x4d,  0x5f,
/*  920 */ 0x56,  0x41,  0x58,  0x00,  0x45,  0x4d,  0x5f,  0x43,  0x52,  0x49,
/*  930 */ 0x53,  0x00,  0x45,  0x4d,  0x5f,  0x4a,  0x41,  0x56,  0x45,  0x4c,
/*  940 */ 0x49,  0x4e,  0x00,  0x45,  0x4d,  0x5f,  0x46,  0x49,  0x52,  0x45,
/*  950 */ 0x50,  0x41,  0x54,  0x48,  0x00,  0x45,  0x4d,  0x5f,  0x5a,  0x53,
/*  960 */ 0x50,  0x00,  0x45,  0x4d,  0x5f,  0x4d,  0x4d,  0x49,  0x58,  0x00,
/*  970 */ 0x45,  0x4d,  0x5f,  0x48,  0x55,  0x41,  0x4e,  0x59,  0x00,  0x45,
/*  980 */ 0x4d,  0x5f,  0x50,  0x52,  0x49,  0x53,  0x4d,  0x00,  0x45,  0x4d,
/*  990 */ 0x5f,  0x41,  0x56,  0x52,  0x00,  0x45,  0x4d,  0x5f,  0x46,  0x52,
/* 1000 */ 0x33,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x44,  0x31,  0x30,  0x56,
/* 1010 */ 0x00,  0x45,  0x4d,  0x5f,  0x44,  0x33,  0x30,  0x56,  0x00,  0x45,
/* 1020 */ 0x4d,  0x5f,  0x56,  0x38,  0x35,  0x30,  0x00,  0x45,  0x4d,  0x5f,
/* 1030 */ 0x4d,  0x33,  0x32,  0x52,  0x00,  0x45,  0x4d,  0x5f,  0x4d,  0x4e,
/* 1040 */ 0x31,  0x30,  0x33,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,  0x4d,
/* 1050 */ 0x4e,  0x31,  0x30,  0x32,  0x30,  0x30,  0x00,  0x45,  0x4d,  0x5f,
/* 1060 */ 0x50,  0x4a,  0x00,  0x45,  0x4d,  0x5f,  0x4f,  0x50,  0x45,  0x4e,
/* 1070 */ 0x52,  0x49,  0x53,  0x43,  0x00,  0x45,  0x4d,  0x5f,  0x41,  0x52,
/* 1080 */ 0x43,  0x5f,  0x41,  0x35,  0x00,  0x45,  0x4d,  0x5f,  0x58,  0x54,
/* 1090 */ 0x45,  0x4e,  0x53,  0x41,  0x00,  0x45,  0x54,  0x5f,  0x4e,  0x4f,
/* 1100 */ 0x4e,  0x45,  0x00,  0x45,  0x54,  0x5f,  0x52,  0x45,  0x4c,  0x00,
/* 1110 */ 0x52,  0x65,  0x6c,  0x6f,  0x63,  0x00,  0x45,  0x54,  0x5f,  0x45,
/* 1120 */ 0x58,  0x45,  0x43,  0x00,  0x45,  0x78,  0x65,  0x63,  0x00,  0x45,
/* 1130 */ 0x54,  0x5f,  0x44,  0x59,  0x4e,  0x00,  0x44,  0x79,  0x6e,  0x00,
/* 1140 */ 0x45,  0x54,  0x5f,  0x43,  0x4f,  0x52,  0x45,  0x00,  0x43,  0x6f,
/* 1150 */ 0x72,  0x65,  0x00,  0x45,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,
/* 1160 */ 0x50,  0x53,  0x45,  0x55,  0x44,  0x4f,  0x00,  0x53,  0x55,  0x4e,
/* 1170 */ 0x57,  0x50,  0x73,  0x65,  0x75,  0x64,  0x6f,  0x00,  0x45,  0x56,
/* 1180 */ 0x5f,  0x4e,  0x4f,  0x4e,  0x45,  0x00,  0x49,  0x6e,  0x76,  0x61,
/* 1190 */ 0x6c,  0x69,  0x64,  0x00,  0x45,  0x56,  0x5f,  0x43,  0x55,  0x52,
/* 1200 */ 0x52,  0x45,  0x4e,  0x54,  0x00,  0x43,  0x75,  0x72,  0x72,  0x65,
/* 1210 */ 0x6e,  0x74,  0x00,  0x45,  0x46,  0x5f,  0x53,  0x50,  0x41,  0x52,
/* 1220 */ 0x43,  0x5f,  0x33,  0x32,  0x50,  0x4c,  0x55,  0x53,  0x00,  0x45,
/* 1230 */ 0x46,  0x5f,  0x53,  0x50,  0x41,  0x52,  0x43,  0x5f,  0x53,  0x55,
/* 1240 */ 0x4e,  0x5f,  0x55,  0x53,  0x31,  0x00,  0x45,  0x46,  0x5f,  0x53,
/* 1250 */ 0x50,  0x41,  0x52,  0x43,  0x5f,  0x53,  0x55,  0x4e,  0x5f,  0x55,
/* 1260 */ 0x53,  0x33,  0x00,  0x45,  0x46,  0x5f,  0x53,  0x50,  0x41,  0x52,
/* 1270 */ 0x43,  0x5f,  0x48,  0x41,  0x4c,  0x5f,  0x52,  0x31,  0x00,  0x45,
/* 1280 */ 0x46,  0x5f,  0x53,  0x50,  0x41,  0x52,  0x43,  0x56,  0x39,  0x5f,
/* 1290 */ 0x54,  0x53,  0x4f,  0x00,  0x45,  0x46,  0x5f,  0x53,  0x50,  0x41,
/* 1300 */ 0x52,  0x43,  0x56,  0x39,  0x5f,  0x50,  0x53,  0x4f,  0x00,  0x45,
/* 1310 */ 0x46,  0x5f,  0x53,  0x50,  0x41,  0x52,  0x43,  0x56,  0x39,  0x5f,
/* 1320 */ 0x52,  0x4d,  0x4f,  0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,
/* 1330 */ 0x42,  0x49,  0x5f,  0x4e,  0x4f,  0x4e,  0x45,  0x00,  0x47,  0x65,
/* 1340 */ 0x6e,  0x65,  0x72,  0x69,  0x63,  0x20,  0x53,  0x59,  0x53,  0x56,
/* 1350 */ 0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,
/* 1360 */ 0x48,  0x50,  0x55,  0x58,  0x00,  0x48,  0x50,  0x2d,  0x55,  0x58,
/* 1370 */ 0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,
/* 1380 */ 0x4e,  0x45,  0x54,  0x42,  0x53,  0x44,  0x00,  0x4e,  0x65,  0x74,
/* 1390 */ 0x42,  0x53,  0x44,  0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,
/* 1400 */ 0x42,  0x49,  0x5f,  0x4c,  0x49,  0x4e,  0x55,  0x58,  0x00,  0x4c,
/* 1410 */ 0x69,  0x6e,  0x75,  0x78,  0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,
/* 1420 */ 0x41,  0x42,  0x49,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,
/* 1430 */ 0x4e,  0x34,  0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,
/* 1440 */ 0x49,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x35,
/* 1450 */ 0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,
/* 1460 */ 0x53,  0x4f,  0x4c,  0x41,  0x52,  0x49,  0x53,  0x00,  0x53,  0x6f,
/* 1470 */ 0x6c,  0x61,  0x72,  0x69,  0x73,  0x00,  0x45,  0x4c,  0x46,  0x4f,
/* 1480 */ 0x53,  0x41,  0x42,  0x49,  0x5f,  0x41,  0x49,  0x58,  0x00,  0x45,
/* 1490 */ 0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,  0x49,  0x52,
/* 1500 */ 0x49,  0x58,  0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,
/* 1510 */ 0x49,  0x5f,  0x46,  0x52,  0x45,  0x45,  0x42,  0x53,  0x44,  0x00,
/* 1520 */ 0x46,  0x72,  0x65,  0x65,  0x42,  0x53,  0x44,  0x00,  0x45,  0x4c,
/* 1530 */ 0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,  0x54,  0x52,  0x55,
/* 1540 */ 0x36,  0x34,  0x00,  0x54,  0x72,  0x75,  0x36,  0x34,  0x00,  0x45,
/* 1550 */ 0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,  0x4d,  0x4f,
/* 1560 */ 0x44,  0x45,  0x53,  0x54,  0x4f,  0x00,  0x4d,  0x6f,  0x64,  0x65,
/* 1570 */ 0x73,  0x74,  0x6f,  0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,
/* 1580 */ 0x42,  0x49,  0x5f,  0x4f,  0x50,  0x45,  0x4e,  0x42,  0x53,  0x44,
/* 1590 */ 0x00,  0x4f,  0x70,  0x65,  0x6e,  0x42,  0x53,  0x44,  0x00,  0x45,
/* 1600 */ 0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,  0x4f,  0x50,
/* 1610 */ 0x45,  0x4e,  0x56,  0x4d,  0x53,  0x00,  0x4f,  0x70,  0x65,  0x6e,
/* 1620 */ 0x56,  0x4d,  0x53,  0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,
/* 1630 */ 0x42,  0x49,  0x5f,  0x4e,  0x53,  0x4b,  0x00,  0x45,  0x4c,  0x46,
/* 1640 */ 0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,  0x41,  0x52,  0x4f,  0x53,
/* 1650 */ 0x00,  0x41,  0x6d,  0x69,  0x67,  0x61,  0x20,  0x52,  0x65,  0x73,
/* 1660 */ 0x65,  0x61,  0x72,  0x63,  0x68,  0x20,  0x4f,  0x53,  0x00,  0x45,
/* 1670 */ 0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,  0x5f,  0x41,  0x52,
/* 1680 */ 0x4d,  0x00,  0x45,  0x4c,  0x46,  0x4f,  0x53,  0x41,  0x42,  0x49,
/* 1690 */ 0x5f,  0x53,  0x54,  0x41,  0x4e,  0x44,  0x41,  0x4c,  0x4f,  0x4e,
/* 1700 */ 0x45,  0x00,  0x53,  0x74,  0x61,  0x6e,  0x64,  0x61,  0x6c,  0x6f,
/* 1710 */ 0x6e,  0x65,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_ELFCLASSNONE	"ELFCLASSNONE"
#define	MSG_ELFCLASSNONE_SIZE	12

#define	MSG_ELFCLASSNONE_ALT	"None"
#define	MSG_ELFCLASSNONE_ALT_SIZE	4

#define	MSG_ELFCLASS32	"ELFCLASS32"
#define	MSG_ELFCLASS32_SIZE	10

#define	MSG_ELFCLASS32_ALT	"32-bit"
#define	MSG_ELFCLASS32_ALT_SIZE	6

#define	MSG_ELFCLASS64	"ELFCLASS64"
#define	MSG_ELFCLASS64_SIZE	10

#define	MSG_ELFCLASS64_ALT	"64-bit"
#define	MSG_ELFCLASS64_ALT_SIZE	6

#define	MSG_ELFDATANONE	"ELFDATANONE"
#define	MSG_ELFDATANONE_SIZE	11

#define	MSG_ELFDATANONE_ALT	"None"
#define	MSG_ELFDATANONE_ALT_SIZE	4

#define	MSG_ELFDATA2LSB	"ELFDATA2LSB"
#define	MSG_ELFDATA2LSB_SIZE	11

#define	MSG_ELFDATA2LSB_ALT1	"2LSB"
#define	MSG_ELFDATA2LSB_ALT1_SIZE	4

#define	MSG_ELFDATA2LSB_ALT2	"LSB"
#define	MSG_ELFDATA2LSB_ALT2_SIZE	3

#define	MSG_ELFDATA2MSB	"ELFDATA2MSB"
#define	MSG_ELFDATA2MSB_SIZE	11

#define	MSG_ELFDATA2MSB_ALT1	"2MSB"
#define	MSG_ELFDATA2MSB_ALT1_SIZE	4

#define	MSG_ELFDATA2MSB_ALT2	"MSB"
#define	MSG_ELFDATA2MSB_ALT2_SIZE	3

#define	MSG_EM_NONE	"EM_NONE"
#define	MSG_EM_NONE_SIZE	7

#define	MSG_EM_NONE_ALT	"No mach"
#define	MSG_EM_NONE_ALT_SIZE	7

#define	MSG_EM_M32	"EM_M32"
#define	MSG_EM_M32_SIZE	6

#define	MSG_EM_M32_ALT	"WE32100"
#define	MSG_EM_M32_ALT_SIZE	7

#define	MSG_EM_SPARC	"EM_SPARC"
#define	MSG_EM_SPARC_SIZE	8

#define	MSG_EM_SPARC_ALT	"SPARC"
#define	MSG_EM_SPARC_ALT_SIZE	5

#define	MSG_EM_386	"EM_386"
#define	MSG_EM_386_SIZE	6

#define	MSG_EM_386_ALT	"80386"
#define	MSG_EM_386_ALT_SIZE	5

#define	MSG_EM_68K	"EM_68K"
#define	MSG_EM_68K_SIZE	6

#define	MSG_EM_68K_ALT	"68000"
#define	MSG_EM_68K_ALT_SIZE	5

#define	MSG_EM_88K	"EM_88K"
#define	MSG_EM_88K_SIZE	6

#define	MSG_EM_88K_ALT	"88000"
#define	MSG_EM_88K_ALT_SIZE	5

#define	MSG_EM_486	"EM_486"
#define	MSG_EM_486_SIZE	6

#define	MSG_EM_486_ALT	"80486"
#define	MSG_EM_486_ALT_SIZE	5

#define	MSG_EM_860	"EM_860"
#define	MSG_EM_860_SIZE	6

#define	MSG_EM_860_ALT	"i860"
#define	MSG_EM_860_ALT_SIZE	4

#define	MSG_EM_MIPS	"EM_MIPS"
#define	MSG_EM_MIPS_SIZE	7

#define	MSG_EM_MIPS_ALT	"RS3000_BE"
#define	MSG_EM_MIPS_ALT_SIZE	9

#define	MSG_EM_S370	"EM_S370"
#define	MSG_EM_S370_SIZE	7

#define	MSG_EM_MIPS_RS3_LE	"EM_MIPS_RS3_LE"
#define	MSG_EM_MIPS_RS3_LE_SIZE	14

#define	MSG_EM_MIPS_RS3_LE_ALT	"RS3000_LE"
#define	MSG_EM_MIPS_RS3_LE_ALT_SIZE	9

#define	MSG_EM_RS6000	"EM_RS6000"
#define	MSG_EM_RS6000_SIZE	9

#define	MSG_EM_RS6000_ALT	"RS6000"
#define	MSG_EM_RS6000_ALT_SIZE	6

#define	MSG_EM_UNKNOWN12	"EM_UNKNOWN12"
#define	MSG_EM_UNKNOWN12_SIZE	12

#define	MSG_EM_UNKNOWN13	"EM_UNKNOWN13"
#define	MSG_EM_UNKNOWN13_SIZE	12

#define	MSG_EM_UNKNOWN14	"EM_UNKNOWN14"
#define	MSG_EM_UNKNOWN14_SIZE	12

#define	MSG_EM_PA_RISC	"EM_PA_RISC"
#define	MSG_EM_PA_RISC_SIZE	10

#define	MSG_EM_PA_RISC_ALT	"PA_RISC"
#define	MSG_EM_PA_RISC_ALT_SIZE	7

#define	MSG_EM_nCUBE	"EM_nCUBE"
#define	MSG_EM_nCUBE_SIZE	8

#define	MSG_EM_nCUBE_ALT	"nCUBE"
#define	MSG_EM_nCUBE_ALT_SIZE	5

#define	MSG_EM_VPP500	"EM_VPP500"
#define	MSG_EM_VPP500_SIZE	9

#define	MSG_EM_VPP500_ALT	"VPP500"
#define	MSG_EM_VPP500_ALT_SIZE	6

#define	MSG_EM_SPARC32PLUS	"EM_SPARC32PLUS"
#define	MSG_EM_SPARC32PLUS_SIZE	14

#define	MSG_EM_SPARC32PLUS_ALT	"SPARC32PLUS"
#define	MSG_EM_SPARC32PLUS_ALT_SIZE	11

#define	MSG_EM_960	"EM_960"
#define	MSG_EM_960_SIZE	6

#define	MSG_EM_PPC	"EM_PPC"
#define	MSG_EM_PPC_SIZE	6

#define	MSG_EM_PPC_ALT	"PowerPC"
#define	MSG_EM_PPC_ALT_SIZE	7

#define	MSG_EM_PPC64	"EM_PPC64"
#define	MSG_EM_PPC64_SIZE	8

#define	MSG_EM_PPC64_ALT	"PowerPC64"
#define	MSG_EM_PPC64_ALT_SIZE	9

#define	MSG_EM_S390	"EM_S390"
#define	MSG_EM_S390_SIZE	7

#define	MSG_EM_UNKNOWN23	"EM_UNKNOWN23"
#define	MSG_EM_UNKNOWN23_SIZE	12

#define	MSG_EM_UNKNOWN24	"EM_UNKNOWN24"
#define	MSG_EM_UNKNOWN24_SIZE	12

#define	MSG_EM_UNKNOWN25	"EM_UNKNOWN25"
#define	MSG_EM_UNKNOWN25_SIZE	12

#define	MSG_EM_UNKNOWN26	"EM_UNKNOWN26"
#define	MSG_EM_UNKNOWN26_SIZE	12

#define	MSG_EM_UNKNOWN27	"EM_UNKNOWN27"
#define	MSG_EM_UNKNOWN27_SIZE	12

#define	MSG_EM_UNKNOWN28	"EM_UNKNOWN28"
#define	MSG_EM_UNKNOWN28_SIZE	12

#define	MSG_EM_UNKNOWN29	"EM_UNKNOWN29"
#define	MSG_EM_UNKNOWN29_SIZE	12

#define	MSG_EM_UNKNOWN30	"EM_UNKNOWN30"
#define	MSG_EM_UNKNOWN30_SIZE	12

#define	MSG_EM_UNKNOWN31	"EM_UNKNOWN31"
#define	MSG_EM_UNKNOWN31_SIZE	12

#define	MSG_EM_UNKNOWN32	"EM_UNKNOWN32"
#define	MSG_EM_UNKNOWN32_SIZE	12

#define	MSG_EM_UNKNOWN33	"EM_UNKNOWN33"
#define	MSG_EM_UNKNOWN33_SIZE	12

#define	MSG_EM_UNKNOWN34	"EM_UNKNOWN34"
#define	MSG_EM_UNKNOWN34_SIZE	12

#define	MSG_EM_UNKNOWN35	"EM_UNKNOWN35"
#define	MSG_EM_UNKNOWN35_SIZE	12

#define	MSG_EM_V800	"EM_V800"
#define	MSG_EM_V800_SIZE	7

#define	MSG_EM_FR20	"EM_FR20"
#define	MSG_EM_FR20_SIZE	7

#define	MSG_EM_RH32	"EM_RH32"
#define	MSG_EM_RH32_SIZE	7

#define	MSG_EM_RCE	"EM_RCE"
#define	MSG_EM_RCE_SIZE	6

#define	MSG_EM_ARM	"EM_ARM"
#define	MSG_EM_ARM_SIZE	6

#define	MSG_EM_ARM_ALT	"ARM"
#define	MSG_EM_ARM_ALT_SIZE	3

#define	MSG_EM_ALPHA	"EM_ALPHA"
#define	MSG_EM_ALPHA_SIZE	8

#define	MSG_EM_ALPHA_ALT	"Alpha"
#define	MSG_EM_ALPHA_ALT_SIZE	5

#define	MSG_EM_SH	"EM_SH"
#define	MSG_EM_SH_SIZE	5

#define	MSG_EM_SPARCV9	"EM_SPARCV9"
#define	MSG_EM_SPARCV9_SIZE	10

#define	MSG_EM_SPARCV9_ALT	"SPARCV9"
#define	MSG_EM_SPARCV9_ALT_SIZE	7

#define	MSG_EM_TRICORE	"EM_TRICORE"
#define	MSG_EM_TRICORE_SIZE	10

#define	MSG_EM_ARC	"EM_ARC"
#define	MSG_EM_ARC_SIZE	6

#define	MSG_EM_H8_300	"EM_H8_300"
#define	MSG_EM_H8_300_SIZE	9

#define	MSG_EM_H8_300H	"EM_H8_300H"
#define	MSG_EM_H8_300H_SIZE	10

#define	MSG_EM_H8S	"EM_H8S"
#define	MSG_EM_H8S_SIZE	6

#define	MSG_EM_H8_500	"EM_H8_500"
#define	MSG_EM_H8_500_SIZE	9

#define	MSG_EM_IA_64	"EM_IA_64"
#define	MSG_EM_IA_64_SIZE	8

#define	MSG_EM_IA_64_ALT	"IA_64"
#define	MSG_EM_IA_64_ALT_SIZE	5

#define	MSG_EM_MIPS_X	"EM_MIPS_X"
#define	MSG_EM_MIPS_X_SIZE	9

#define	MSG_EM_COLDFIRE	"EM_COLDFIRE"
#define	MSG_EM_COLDFIRE_SIZE	11

#define	MSG_EM_68HC12	"EM_68HC12"
#define	MSG_EM_68HC12_SIZE	9

#define	MSG_EM_MMA	"EM_MMA"
#define	MSG_EM_MMA_SIZE	6

#define	MSG_EM_PCP	"EM_PCP"
#define	MSG_EM_PCP_SIZE	6

#define	MSG_EM_NCPU	"EM_NCPU"
#define	MSG_EM_NCPU_SIZE	7

#define	MSG_EM_NDR1	"EM_NDR1"
#define	MSG_EM_NDR1_SIZE	7

#define	MSG_EM_STARCORE	"EM_STARCORE"
#define	MSG_EM_STARCORE_SIZE	11

#define	MSG_EM_ME16	"EM_ME16"
#define	MSG_EM_ME16_SIZE	7

#define	MSG_EM_ST100	"EM_ST100"
#define	MSG_EM_ST100_SIZE	8

#define	MSG_EM_TINYJ	"EM_TINYJ"
#define	MSG_EM_TINYJ_SIZE	8

#define	MSG_EM_AMD64	"EM_AMD64"
#define	MSG_EM_AMD64_SIZE	8

#define	MSG_EM_AMD64_ALT	"AMD64"
#define	MSG_EM_AMD64_ALT_SIZE	5

#define	MSG_EM_PDSP	"EM_PDSP"
#define	MSG_EM_PDSP_SIZE	7

#define	MSG_EM_UNKNOWN64	"EM_UNKNOWN64"
#define	MSG_EM_UNKNOWN64_SIZE	12

#define	MSG_EM_UNKNOWN65	"EM_UNKNOWN65"
#define	MSG_EM_UNKNOWN65_SIZE	12

#define	MSG_EM_FX66	"EM_FX66"
#define	MSG_EM_FX66_SIZE	7

#define	MSG_EM_ST9PLUS	"EM_ST9PLUS"
#define	MSG_EM_ST9PLUS_SIZE	10

#define	MSG_EM_ST7	"EM_ST7"
#define	MSG_EM_ST7_SIZE	6

#define	MSG_EM_68HC16	"EM_68HC16"
#define	MSG_EM_68HC16_SIZE	9

#define	MSG_EM_68HC11	"EM_68HC11"
#define	MSG_EM_68HC11_SIZE	9

#define	MSG_EM_68HC08	"EM_68HC08"
#define	MSG_EM_68HC08_SIZE	9

#define	MSG_EM_68HC05	"EM_68HC05"
#define	MSG_EM_68HC05_SIZE	9

#define	MSG_EM_SVX	"EM_SVX"
#define	MSG_EM_SVX_SIZE	6

#define	MSG_EM_ST19	"EM_ST19"
#define	MSG_EM_ST19_SIZE	7

#define	MSG_EM_VAX	"EM_VAX"
#define	MSG_EM_VAX_SIZE	6

#define	MSG_EM_VAX_ALT	"VAX"
#define	MSG_EM_VAX_ALT_SIZE	3

#define	MSG_EM_CRIS	"EM_CRIS"
#define	MSG_EM_CRIS_SIZE	7

#define	MSG_EM_JAVELIN	"EM_JAVELIN"
#define	MSG_EM_JAVELIN_SIZE	10

#define	MSG_EM_FIREPATH	"EM_FIREPATH"
#define	MSG_EM_FIREPATH_SIZE	11

#define	MSG_EM_ZSP	"EM_ZSP"
#define	MSG_EM_ZSP_SIZE	6

#define	MSG_EM_MMIX	"EM_MMIX"
#define	MSG_EM_MMIX_SIZE	7

#define	MSG_EM_HUANY	"EM_HUANY"
#define	MSG_EM_HUANY_SIZE	8

#define	MSG_EM_PRISM	"EM_PRISM"
#define	MSG_EM_PRISM_SIZE	8

#define	MSG_EM_AVR	"EM_AVR"
#define	MSG_EM_AVR_SIZE	6

#define	MSG_EM_FR30	"EM_FR30"
#define	MSG_EM_FR30_SIZE	7

#define	MSG_EM_D10V	"EM_D10V"
#define	MSG_EM_D10V_SIZE	7

#define	MSG_EM_D30V	"EM_D30V"
#define	MSG_EM_D30V_SIZE	7

#define	MSG_EM_V850	"EM_V850"
#define	MSG_EM_V850_SIZE	7

#define	MSG_EM_M32R	"EM_M32R"
#define	MSG_EM_M32R_SIZE	7

#define	MSG_EM_MN10300	"EM_MN10300"
#define	MSG_EM_MN10300_SIZE	10

#define	MSG_EM_MN10200	"EM_MN10200"
#define	MSG_EM_MN10200_SIZE	10

#define	MSG_EM_PJ	"EM_PJ"
#define	MSG_EM_PJ_SIZE	5

#define	MSG_EM_OPENRISC	"EM_OPENRISC"
#define	MSG_EM_OPENRISC_SIZE	11

#define	MSG_EM_ARC_A5	"EM_ARC_A5"
#define	MSG_EM_ARC_A5_SIZE	9

#define	MSG_EM_XTENSA	"EM_XTENSA"
#define	MSG_EM_XTENSA_SIZE	9

#define	MSG_ET_NONE	"ET_NONE"
#define	MSG_ET_NONE_SIZE	7

#define	MSG_ET_NONE_ALT	"None"
#define	MSG_ET_NONE_ALT_SIZE	4

#define	MSG_ET_REL	"ET_REL"
#define	MSG_ET_REL_SIZE	6

#define	MSG_ET_REL_ALT	"Reloc"
#define	MSG_ET_REL_ALT_SIZE	5

#define	MSG_ET_EXEC	"ET_EXEC"
#define	MSG_ET_EXEC_SIZE	7

#define	MSG_ET_EXEC_ALT	"Exec"
#define	MSG_ET_EXEC_ALT_SIZE	4

#define	MSG_ET_DYN	"ET_DYN"
#define	MSG_ET_DYN_SIZE	6

#define	MSG_ET_DYN_ALT	"Dyn"
#define	MSG_ET_DYN_ALT_SIZE	3

#define	MSG_ET_CORE	"ET_CORE"
#define	MSG_ET_CORE_SIZE	7

#define	MSG_ET_CORE_ALT	"Core"
#define	MSG_ET_CORE_ALT_SIZE	4

#define	MSG_ET_SUNWPSEUDO	"ET_SUNWPSEUDO"
#define	MSG_ET_SUNWPSEUDO_SIZE	13

#define	MSG_ET_SUNWPSEUDO_ALT	"SUNWPseudo"
#define	MSG_ET_SUNWPSEUDO_ALT_SIZE	10

#define	MSG_EV_NONE	"EV_NONE"
#define	MSG_EV_NONE_SIZE	7

#define	MSG_EV_NONE_ALT	"Invalid"
#define	MSG_EV_NONE_ALT_SIZE	7

#define	MSG_EV_CURRENT	"EV_CURRENT"
#define	MSG_EV_CURRENT_SIZE	10

#define	MSG_EV_CURRENT_ALT	"Current"
#define	MSG_EV_CURRENT_ALT_SIZE	7

#define	MSG_EF_SPARC_32PLUS	"EF_SPARC_32PLUS"
#define	MSG_EF_SPARC_32PLUS_SIZE	15

#define	MSG_EF_SPARC_SUN_US1	"EF_SPARC_SUN_US1"
#define	MSG_EF_SPARC_SUN_US1_SIZE	16

#define	MSG_EF_SPARC_SUN_US3	"EF_SPARC_SUN_US3"
#define	MSG_EF_SPARC_SUN_US3_SIZE	16

#define	MSG_EF_SPARC_HAL_R1	"EF_SPARC_HAL_R1"
#define	MSG_EF_SPARC_HAL_R1_SIZE	15

#define	MSG_EF_SPARCV9_TSO	"EF_SPARCV9_TSO"
#define	MSG_EF_SPARCV9_TSO_SIZE	14

#define	MSG_EF_SPARCV9_PSO	"EF_SPARCV9_PSO"
#define	MSG_EF_SPARCV9_PSO_SIZE	14

#define	MSG_EF_SPARCV9_RMO	"EF_SPARCV9_RMO"
#define	MSG_EF_SPARCV9_RMO_SIZE	14

#define	MSG_OSABI_NONE	"ELFOSABI_NONE"
#define	MSG_OSABI_NONE_SIZE	13

#define	MSG_OSABI_NONE_ALT	"Generic SYSV"
#define	MSG_OSABI_NONE_ALT_SIZE	12

#define	MSG_OSABI_HPUX	"ELFOSABI_HPUX"
#define	MSG_OSABI_HPUX_SIZE	13

#define	MSG_OSABI_HPUX_ALT	"HP-UX"
#define	MSG_OSABI_HPUX_ALT_SIZE	5

#define	MSG_OSABI_NETBSD	"ELFOSABI_NETBSD"
#define	MSG_OSABI_NETBSD_SIZE	15

#define	MSG_OSABI_NETBSD_ALT	"NetBSD"
#define	MSG_OSABI_NETBSD_ALT_SIZE	6

#define	MSG_OSABI_LINUX	"ELFOSABI_LINUX"
#define	MSG_OSABI_LINUX_SIZE	14

#define	MSG_OSABI_LINUX_ALT	"Linux"
#define	MSG_OSABI_LINUX_ALT_SIZE	5

#define	MSG_OSABI_UNKNOWN4	"ELFOSABI_UNKNOWN4"
#define	MSG_OSABI_UNKNOWN4_SIZE	17

#define	MSG_OSABI_UNKNOWN5	"ELFOSABI_UNKNOWN5"
#define	MSG_OSABI_UNKNOWN5_SIZE	17

#define	MSG_OSABI_SOLARIS	"ELFOSABI_SOLARIS"
#define	MSG_OSABI_SOLARIS_SIZE	16

#define	MSG_OSABI_SOLARIS_ALT	"Solaris"
#define	MSG_OSABI_SOLARIS_ALT_SIZE	7

#define	MSG_OSABI_AIX	"ELFOSABI_AIX"
#define	MSG_OSABI_AIX_SIZE	12

#define	MSG_OSABI_AIX_ALT	"AIX"
#define	MSG_OSABI_AIX_ALT_SIZE	3

#define	MSG_OSABI_IRIX	"ELFOSABI_IRIX"
#define	MSG_OSABI_IRIX_SIZE	13

#define	MSG_OSABI_IRIX_ALT	"IRIX"
#define	MSG_OSABI_IRIX_ALT_SIZE	4

#define	MSG_OSABI_FREEBSD	"ELFOSABI_FREEBSD"
#define	MSG_OSABI_FREEBSD_SIZE	16

#define	MSG_OSABI_FREEBSD_ALT	"FreeBSD"
#define	MSG_OSABI_FREEBSD_ALT_SIZE	7

#define	MSG_OSABI_TRU64	"ELFOSABI_TRU64"
#define	MSG_OSABI_TRU64_SIZE	14

#define	MSG_OSABI_TRU64_ALT	"Tru64"
#define	MSG_OSABI_TRU64_ALT_SIZE	5

#define	MSG_OSABI_MODESTO	"ELFOSABI_MODESTO"
#define	MSG_OSABI_MODESTO_SIZE	16

#define	MSG_OSABI_MODESTO_ALT	"Modesto"
#define	MSG_OSABI_MODESTO_ALT_SIZE	7

#define	MSG_OSABI_OPENBSD	"ELFOSABI_OPENBSD"
#define	MSG_OSABI_OPENBSD_SIZE	16

#define	MSG_OSABI_OPENBSD_ALT	"OpenBSD"
#define	MSG_OSABI_OPENBSD_ALT_SIZE	7

#define	MSG_OSABI_OPENVMS	"ELFOSABI_OPENVMS"
#define	MSG_OSABI_OPENVMS_SIZE	16

#define	MSG_OSABI_OPENVMS_ALT	"OpenVMS"
#define	MSG_OSABI_OPENVMS_ALT_SIZE	7

#define	MSG_OSABI_NSK	"ELFOSABI_NSK"
#define	MSG_OSABI_NSK_SIZE	12

#define	MSG_OSABI_NSK_ALT	"NSK"
#define	MSG_OSABI_NSK_ALT_SIZE	3

#define	MSG_OSABI_AROS	"ELFOSABI_AROS"
#define	MSG_OSABI_AROS_SIZE	13

#define	MSG_OSABI_AROS_ALT	"Amiga Research OS"
#define	MSG_OSABI_AROS_ALT_SIZE	17

#define	MSG_OSABI_ARM	"ELFOSABI_ARM"
#define	MSG_OSABI_ARM_SIZE	12

#define	MSG_OSABI_ARM_ALT	"ARM"
#define	MSG_OSABI_ARM_ALT_SIZE	3

#define	MSG_OSABI_STANDALONE	"ELFOSABI_STANDALONE"
#define	MSG_OSABI_STANDALONE_SIZE	19

#define	MSG_OSABI_STANDALONE_ALT	"Standalone"
#define	MSG_OSABI_STANDALONE_ALT_SIZE	10

#define	MSG_GBL_ZERO	"0"
#define	MSG_GBL_ZERO_SIZE	1

#define	MSG_STR_EMPTY	""
#define	MSG_STR_EMPTY_SIZE	0

#endif	/* __lint */

#endif
