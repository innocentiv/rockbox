/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdint.h>

#define BLOCK_SIZE      16

/* All fields are in big-endian BCD */
struct sb_version_t
{
    uint16_t major;
    uint16_t pad0;
    uint16_t minor;
    uint16_t pad1;
    uint16_t revision;
    uint16_t pad2;
};

struct sb_header_t
{
    uint8_t sha1_header[20]; /* SHA-1 of the rest of the header */
    uint8_t signature[4]; /* Signature "STMP" */
    uint8_t major_ver; /* Should be 1 */
    uint8_t minor_ver; /* Should be 1 */
    uint16_t flags;
    uint32_t image_size; /* In blocks (=16bytes) */
    uint32_t first_boot_tag_off; /* Offset in blocks */
    uint32_t first_boot_sec_id; /* First bootable section ID */
    uint16_t nr_keys; /* Number of encryption keys */
    uint16_t key_dict_off; /* Offset to key dictionary (in blocks) */
    uint16_t header_size; /* In blocks */
    uint16_t nr_sections; /* Number of sections */
    uint16_t sec_hdr_size; /* Section header size (in blocks) */
    uint8_t rand_pad0[6]; /* Random padding */
    uint64_t timestamp; /* In microseconds since 2000/1/1 00:00:00 */
    struct sb_version_t product_ver;
    struct sb_version_t component_ver;
    uint16_t drive_tag; /* first tag to boot ? */
    uint8_t rand_pad1[6]; /* Random padding */
} __attribute__((packed));

struct sb_section_header_t
{
    uint32_t identifier;
    uint32_t offset; /* In blocks */
    uint32_t size; /* In blocks */
    uint32_t flags;
} __attribute__((packed));

struct sb_key_dictionary_entry_t
{
    uint8_t hdr_cbc_mac[16]; /* CBC-MAC of the header */
    uint8_t key[16]; /* Actual AES Key (encrypted by the global key) */
} __attribute__((packed));

#define IMAGE_MAJOR_VERSION     1
#define IMAGE_MINOR_VERSION     1

#define SECTION_BOOTABLE        (1 << 0)
#define SECTION_CLEARTEXT       (1 << 1)

#define SB_INST_NOP     0x0
#define SB_INST_TAG     0x1
#define SB_INST_LOAD    0x2
#define SB_INST_FILL    0x3
#define SB_INST_JUMP    0x4
#define SB_INST_CALL    0x5
#define SB_INST_MODE    0x6

/* flags */
#define SB_INST_LAST_TAG    1 /* for TAG */
#define SB_INST_LOAD_DCD    1 /* for LOAD */
#define SB_INST_FILL_BYTE   0 /* for FILL */
#define SB_INST_FILL_HWORD  1 /* for FILL */
#define SB_INST_FILL_WORD   2 /* for FILL */
#define SB_INST_HAB_EXEC    1 /* for JUMP/CALL */

struct sb_instruction_header_t
{
    uint8_t checksum;
    uint8_t opcode;
    uint16_t flags;
} __attribute__((packed));

struct sb_instruction_common_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t len;
    uint32_t data;
} __attribute__((packed));

struct sb_instruction_load_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t len;
    uint32_t crc;
} __attribute__((packed));

struct sb_instruction_fill_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t len;
    uint32_t pattern;
} __attribute__((packed));

struct sb_instruction_mode_t
{
    struct sb_instruction_header_t hdr;
    uint32_t zero1;
    uint32_t zero2;
    uint32_t mode;
} __attribute__((packed));

struct sb_instruction_call_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t zero;
    uint32_t arg;
} __attribute__((packed));

struct sb_instruction_tag_t
{
    struct sb_instruction_header_t hdr;
    uint32_t identifier; /* section identifier */
    uint32_t len; /* length of the section */
    uint32_t flags; /* section flags */
} __attribute__((packed));
