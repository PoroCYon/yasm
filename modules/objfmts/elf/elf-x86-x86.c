/*
 * ELF object format helpers - x86:x86
 *
 *  Copyright (C) 2004  Michael Urman
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <util.h>
/*@unused@*/ RCSID("$Id:$");

#define YASM_LIB_INTERNAL
#define YASM_EXPR_INTERNAL
#include <libyasm.h>
#define YASM_OBJFMT_ELF_INTERNAL
#include "elf.h"
#include "elf-machine.h"

static int
elf_x86_x86_accepts_reloc_size(size_t val)
{
    return (val&(val-1)) ? 0 : ((val & (8|16|32)) != 0);
}

static void
elf_x86_x86_write_symtab_entry(unsigned char *bufp,
                               elf_symtab_entry *entry,
                               yasm_intnum *value_intn,
                               yasm_intnum *size_intn)
{
    YASM_WRITE_32_L(bufp, entry->name ? entry->name->index : 0);
    YASM_WRITE_32I_L(bufp, value_intn);
    YASM_WRITE_32I_L(bufp, size_intn);

    YASM_WRITE_8(bufp, ELF32_ST_INFO(entry->bind, entry->type));
    YASM_WRITE_8(bufp, 0);
    if (entry->sect) {
        if (yasm_section_is_absolute(entry->sect)) {
            YASM_WRITE_16_L(bufp, SHN_ABS);
        } else {
            elf_secthead *shead = yasm_section_get_data(entry->sect,
                &elf_section_data);
            if (!shead)
                yasm_internal_error(
                    N_("symbol references section without data"));
            YASM_WRITE_16_L(bufp, shead->index);
        }
    } else {
        YASM_WRITE_16_L(bufp, entry->index);
    }
}

static void
elf_x86_x86_write_secthead(unsigned char *bufp, elf_secthead *shead)
{
    YASM_WRITE_32_L(bufp, shead->name ? shead->name->index : 0);
    YASM_WRITE_32_L(bufp, shead->type);
    YASM_WRITE_32_L(bufp, shead->flags);
    YASM_WRITE_32_L(bufp, 0); /* vmem address */

    YASM_WRITE_32_L(bufp, shead->offset);
    YASM_WRITE_32I_L(bufp, shead->size);
    YASM_WRITE_32_L(bufp, shead->link);
    YASM_WRITE_32_L(bufp, shead->info);

    if (shead->align)
        YASM_WRITE_32I_L(bufp, shead->align);
    else
        YASM_WRITE_32_L(bufp, 0);
    YASM_WRITE_32_L(bufp, shead->entsize);

}

static void
elf_x86_x86_write_secthead_rel(unsigned char *bufp,
                               elf_secthead *shead,
                               elf_section_index symtab_idx,
                               elf_section_index sindex)
{
    YASM_WRITE_32_L(bufp, shead->rel_name ? shead->rel_name->index : 0);
    YASM_WRITE_32_L(bufp, SHT_REL);
    YASM_WRITE_32_L(bufp, 0);
    YASM_WRITE_32_L(bufp, 0);

    YASM_WRITE_32_L(bufp, shead->rel_offset);
    YASM_WRITE_32_L(bufp, RELOC32_SIZE * shead->nreloc);/* size */
    YASM_WRITE_32_L(bufp, symtab_idx);		/* link: symtab index */
    YASM_WRITE_32_L(bufp, shead->index);	/* info: relocated's index */

    YASM_WRITE_32_L(bufp, RELOC32_ALIGN);	/* align */
    YASM_WRITE_32_L(bufp, RELOC32_SIZE);	/* entity size */
}

static void
elf_x86_x86_handle_reloc_addend(yasm_intnum *intn, elf_reloc_entry *reloc)
{
    return; /* .rel: Leave addend in intn */
}

static unsigned int
elf_x86_x86_map_reloc_info_to_type(elf_reloc_entry *reloc)
{
    if (reloc->rtype_rel) {
        switch (reloc->valsize) {
            case 8: return (unsigned char) R_386_PC8;
            case 16: return (unsigned char) R_386_PC16;
            case 32: return (unsigned char) R_386_PC32;
            default: yasm_internal_error(N_("Unsupported relocation size"));
        }
    } else {
        switch (reloc->valsize) {
            case 8: return (unsigned char) R_386_8;
            case 16: return (unsigned char) R_386_16;
            case 32: return (unsigned char) R_386_32;
            default: yasm_internal_error(N_("Unsupported relocation size"));
        }
    }
    return 0;
}

static void
elf_x86_x86_write_reloc(unsigned char *bufp, elf_reloc_entry *reloc,
                        unsigned int r_type, unsigned int r_sym)
{
    YASM_WRITE_32I_L(bufp, reloc->reloc.addr);
    YASM_WRITE_32_L(bufp, ELF32_R_INFO((unsigned char)r_sym, (unsigned char)r_type));
}

static void
elf_x86_x86_write_proghead(unsigned char **bufpp,
                           elf_offset secthead_addr,
                           unsigned long secthead_count,
                           elf_section_index shstrtab_index)
{
    unsigned char *bufp = *bufpp;
    unsigned char *buf = bufp-4;
    YASM_WRITE_8(bufp, ELFCLASS32);	    /* elf class */
    YASM_WRITE_8(bufp, ELFDATA2LSB);	    /* data encoding :: MSB? */
    YASM_WRITE_8(bufp, EV_CURRENT);	    /* elf version */
    while (bufp-buf < EI_NIDENT)	    /* e_ident padding */
        YASM_WRITE_8(bufp, 0);

    YASM_WRITE_16_L(bufp, ET_REL);	    /* e_type - object file */
    YASM_WRITE_16_L(bufp, EM_386);	    /* e_machine - or others */
    YASM_WRITE_32_L(bufp, EV_CURRENT);	    /* elf version */
    YASM_WRITE_32_L(bufp, 0);		/* e_entry exection startaddr */
    YASM_WRITE_32_L(bufp, 0);		/* e_phoff program header off */
    YASM_WRITE_32_L(bufp, secthead_addr);   /* e_shoff section header off */
    YASM_WRITE_32_L(bufp, 0);		    /* e_flags also by arch */
    YASM_WRITE_16_L(bufp, EHDR32_SIZE);	    /* e_ehsize */
    YASM_WRITE_16_L(bufp, 0);		    /* e_phentsize */
    YASM_WRITE_16_L(bufp, 0);		    /* e_phnum */
    YASM_WRITE_16_L(bufp, SHDR32_SIZE);	    /* e_shentsize */
    YASM_WRITE_16_L(bufp, secthead_count);  /* e_shnum */
    YASM_WRITE_16_L(bufp, shstrtab_index);  /* e_shstrndx */
    *bufpp = bufp;
}

const elf_machine_handler
elf_machine_handler_x86_x86 = {
    "x86", "x86", ".rel",
    SYMTAB32_SIZE, SYMTAB32_ALIGN, RELOC32_SIZE, SHDR32_SIZE, EHDR32_SIZE,
    elf_x86_x86_accepts_reloc_size,
    elf_x86_x86_write_symtab_entry,
    elf_x86_x86_write_secthead,
    elf_x86_x86_write_secthead_rel,
    elf_x86_x86_handle_reloc_addend,
    elf_x86_x86_map_reloc_info_to_type,
    elf_x86_x86_write_reloc,
    elf_x86_x86_write_proghead
};