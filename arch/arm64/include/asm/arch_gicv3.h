/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * arch/arm64/include/asm/arch_gicv3.h
 *
 * Copyright (C) 2015 ARM Ltd.
 */
#ifndef __ASM_ARCH_GICV3_H
#define __ASM_ARCH_GICV3_H

#include <asm/sysreg.h>

#ifndef __ASSEMBLY__

#include <linux/irqchip/arm-gic-common.h>
#include <linux/stringify.h>
#include <asm/barrier.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#define read_gicreg(r)			read_sysreg_s(SYS_ ## r)
#define write_gicreg(v, r)		write_sysreg_s(v, SYS_ ## r)

/*
 * Low-level accessors
 *
 * These system registers are 32 bits, but we make sure that the compiler
 * sets the GP register's most significant bits to 0 with an explicit cast.
 */

static inline void gic_write_eoir(u32 irq)
{
	write_sysreg_s(irq, SYS_ICC_EOIR1_EL1);
	isb();
}

static __always_inline void gic_write_dir(u32 irq)
{
	write_sysreg_s(irq, SYS_ICC_DIR_EL1);
	isb();
}

static inline u64 gic_read_iar_common(void)
{
	u64 irqstat;

	irqstat = read_sysreg_s(SYS_ICC_IAR1_EL1);
	dsb(sy);
	return irqstat;
}

/*
 * Cavium ThunderX erratum 23154
 *
 * The gicv3 of ThunderX requires a modified version for reading the
 * IAR status to ensure data synchronization (access to icc_iar1_el1
 * is not sync'ed before and after).
 */
static inline u64 gic_read_iar_cavium_thunderx(void)
{
	u64 irqstat;

	nops(8);
	irqstat = read_sysreg_s(SYS_ICC_IAR1_EL1);
	nops(4);
	mb();

	return irqstat;
}

static inline void gic_write_ctlr(u32 val)
{
	write_sysreg_s(val, SYS_ICC_CTLR_EL1);
	isb();
}

static inline u32 gic_read_ctlr(void)
{
	return read_sysreg_s(SYS_ICC_CTLR_EL1);
}

static inline void gic_write_grpen1(u32 val)
{
	write_sysreg_s(val, SYS_ICC_IGRPEN1_EL1);
	isb();
}

static inline void gic_write_sgi1r(u64 val)
{
	write_sysreg_s(val, SYS_ICC_SGI1R_EL1);
}

static inline u32 gic_read_sre(void)
{
	return read_sysreg_s(SYS_ICC_SRE_EL1);
}

static inline void gic_write_sre(u32 val)
{
	write_sysreg_s(val, SYS_ICC_SRE_EL1);
	isb();
}

static inline void gic_write_bpr1(u32 val)
{
	write_sysreg_s(val, SYS_ICC_BPR1_EL1);
}

static inline u32 gic_read_pmr(void)
{
	return read_sysreg_s(SYS_ICC_PMR_EL1);
}

static __always_inline void gic_write_pmr(u32 val)
{
	write_sysreg_s(val, SYS_ICC_PMR_EL1);
}

static inline u32 gic_read_rpr(void)
{
	return read_sysreg_s(SYS_ICC_RPR_EL1);
}

/*
 * Even in 32bit systems that use LPAE, there is no guarantee that the I/O
 * interface provides true 64bit atomic accesses, so using strd/ldrd doesn't
 * make much sense.
 * Moreover, 64bit I/O emulation is extremely difficult to implement on
 * AArch32, since the syndrome register doesn't provide any information for
 * them.
 * Consequently, the following IO helpers use 32bit accesses.
 */
static inline void __gic_writeq_nonatomic(u64 val, void __iomem *addr)
{
	writel_relaxed((u32)val, addr);
	writel_relaxed((u32)(val >> 32), addr + 4);
}

static inline u64 __gic_readq_nonatomic(const void __iomem *addr)
{
	u64 val;

	val = readl_relaxed(addr);
	val |= (u64)readl_relaxed(addr + 4) << 32;
	return val;
}

#define gic_read_typer(c)		__gic_readq_nonatomic(c)
#define gic_write_irouter(v, c)		__gic_writeq_nonatomic(v, c)
#define gic_read_lpir(c)		__gic_readq_nonatomic(c)
#define gic_write_lpir(v, c)		__gic_writeq_nonatomic(v, c)

#define gic_flush_dcache_to_poc(a,l)	__flush_dcache_area((a), (l))

#define gits_read_baser(c)		__gic_readq_nonatomic(c)
#define gits_write_baser(v, c)		__gic_writeq_nonatomic(v, c)

#define gits_read_cbaser(c)		__gic_readq_nonatomic(c)
#define gits_write_cbaser(v, c)		__gic_writeq_nonatomic(v, c)

#define gits_write_cwriter(v, c)	__gic_writeq_nonatomic(v, c)

#define gicr_read_propbaser(c)		__gic_readq_nonatomic(c)
#define gicr_write_propbaser(v, c)	__gic_writeq_nonatomic(v, c)

#define gicr_write_pendbaser(v, c)	__gic_writeq_nonatomic(v, c)
#define gicr_read_pendbaser(c)		__gic_readq_nonatomic(c)

#define gicr_write_vpropbaser(v, c)	__gic_writeq_nonatomic(v, c)
#define gicr_read_vpropbaser(c)		__gic_readq_nonatomic(c)

#define gicr_write_vpendbaser(v, c)	__gic_writeq_nonatomic(v, c)
#define gicr_read_vpendbaser(c)		__gic_readq_nonatomic(c)

static inline bool gic_prio_masking_enabled(void)
{
	return system_uses_irq_prio_masking();
}

static inline void gic_pmr_mask_irqs(void)
{
	BUILD_BUG_ON(GICD_INT_DEF_PRI < (__GIC_PRIO_IRQOFF |
					 GIC_PRIO_PSR_I_SET));
	BUILD_BUG_ON(GICD_INT_DEF_PRI >= GIC_PRIO_IRQON);
	/*
	 * Need to make sure IRQON allows IRQs when SCR_EL3.FIQ is cleared
	 * and non-secure PMR accesses are not subject to the shifts that
	 * are applied to IRQ priorities
	 */
	BUILD_BUG_ON((0x80 | (GICD_INT_DEF_PRI >> 1)) >= GIC_PRIO_IRQON);
	/*
	 * Same situation as above, but now we make sure that we can mask
	 * regular interrupts.
	 */
	BUILD_BUG_ON((0x80 | (GICD_INT_DEF_PRI >> 1)) < (__GIC_PRIO_IRQOFF_NS |
							 GIC_PRIO_PSR_I_SET));
	gic_write_pmr(GIC_PRIO_IRQOFF);
}

static inline void gic_arch_enable_irqs(void)
{
	asm volatile ("msr daifclr, #2" : : : "memory");
}

#endif /* __ASSEMBLY__ */
#endif /* __ASM_ARCH_GICV3_H */
