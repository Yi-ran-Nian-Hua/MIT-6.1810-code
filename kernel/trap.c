#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "proc.h"
#include "defs.h"
#include "fcntl.h"
#include "file.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void) { initlock(&tickslock, "time"); }

// set up to take exceptions and traps while in the kernel.
void trapinithart(void) { w_stvec((uint64) kernelvec); }

int mmapHandler(int virtualAddr, int cause);

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void) {
	int which_dev = 0;

	if ((r_sstatus() & SSTATUS_SPP) != 0)
		panic("usertrap: not from user mode");

	// send interrupts and exceptions to kerneltrap(),
	// since we're now in the kernel.
	w_stvec((uint64) kernelvec);

	struct proc *p = myproc();

	// save user program counter.
	p->trapframe->epc = r_sepc();

	if (r_scause() == 8) {
		// system call

		if (killed(p))
			exit(-1);

		// sepc points to the ecall instruction,
		// but we want to return to the next instruction.
		p->trapframe->epc += 4;

		// an interrupt will change sepc, scause, and sstatus,
		// so enable only now that we're done with those registers.
		intr_on();

		syscall();
	} else if (r_scause() == 13 || r_scause() == 15) {
		// 读取产生页面故障的虚拟地址, 并判断是否位于有效区间
		uint64 faultVirtualAddr = r_stval();
		if (PGROUNDUP(p->trapframe->sp) - 1 < faultVirtualAddr &&
			faultVirtualAddr < p->sz) {
			if (mmapHandler(r_stval(), r_scause()) != 0) {
				// 分配物理页面, 正式建立映射关系
				setkilled(p);
			}
		}else {
			// 页面故障地址非法, 结束进程
			setkilled(p);
		}
	}else if ((which_dev = devintr()) != 0) {
		// ok
	} else {
		printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
		printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
		setkilled(p);
	}

	if (killed(p))
		exit(-1);

	// give up the CPU if this is a timer interrupt.
	if (which_dev == 2)
		yield();

	usertrapret();
}

//
// return to user space
//
void usertrapret(void) {
	struct proc *p = myproc();

	// we're about to switch the destination of traps from
	// kerneltrap() to usertrap(), so turn off interrupts until
	// we're back in user space, where usertrap() is correct.
	intr_off();

	// send syscalls, interrupts, and exceptions to uservec in trampoline.S
	uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
	w_stvec(trampoline_uservec);

	// set up trapframe values that uservec will need when
	// the process next traps into the kernel.
	p->trapframe->kernel_satp = r_satp();		   // kernel page table
	p->trapframe->kernel_sp = p->kstack + PGSIZE;  // process's kernel stack
	p->trapframe->kernel_trap = (uint64) usertrap;
	p->trapframe->kernel_hartid = r_tp();  // hartid for cpuid()

	// set up the registers that trampoline.S's sret will use
	// to get to user space.

	// set S Previous Privilege mode to User.
	unsigned long x = r_sstatus();
	x &= ~SSTATUS_SPP;	// clear SPP to 0 for user mode
	x |= SSTATUS_SPIE;	// enable interrupts in user mode
	w_sstatus(x);

	// set S Exception Program Counter to the saved user pc.
	w_sepc(p->trapframe->epc);

	// tell trampoline.S the user page table to switch to.
	uint64 satp = MAKE_SATP(p->pagetable);

	// jump to userret in trampoline.S at the top of memory, which
	// switches to the user page table, restores user registers,
	// and switches to user mode with sret.
	uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
	((void (*)(uint64)) trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void kerneltrap() {
	int which_dev = 0;
	uint64 sepc = r_sepc();
	uint64 sstatus = r_sstatus();
	uint64 scause = r_scause();

	if ((sstatus & SSTATUS_SPP) == 0)
		panic("kerneltrap: not from supervisor mode");
	if (intr_get() != 0)
		panic("kerneltrap: interrupts enabled");

	if ((which_dev = devintr()) == 0) {
		printf("scause %p\n", scause);
		printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
		panic("kerneltrap");
	}

	// give up the CPU if this is a timer interrupt.
	if (which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
		yield();

	// the yield() may have caused some traps to occur,
	// so restore trap registers for use by kernelvec.S's sepc instruction.
	w_sepc(sepc);
	w_sstatus(sstatus);
}

void clockintr() {
	acquire(&tickslock);
	ticks++;
	wakeup(&ticks);
	release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int devintr() {
	uint64 scause = r_scause();

	if ((scause & 0x8000000000000000L) && (scause & 0xff) == 9) {
		// this is a supervisor external interrupt, via PLIC.

		// irq indicates which device interrupted.
		int irq = plic_claim();

		if (irq == UART0_IRQ) {
			uartintr();
		} else if (irq == VIRTIO0_IRQ) {
			virtio_disk_intr();
		} else if (irq) {
			printf("unexpected interrupt irq=%d\n", irq);
		}

		// the PLIC allows each device to raise at most one
		// interrupt at a time; tell the PLIC the device is
		// now allowed to interrupt again.
		if (irq)
			plic_complete(irq);

		return 1;
	} else if (scause == 0x8000000000000001L) {
		// software interrupt from a machine-mode timer interrupt,
		// forwarded by timervec in kernelvec.S.

		if (cpuid() == 0) {
			clockintr();
		}

		// acknowledge the software interrupt by clearing
		// the SSIP bit in sip.
		w_sip(r_sip() & ~2);

		return 2;
	} else {
		return 0;
	}
}

/**
 * 分配物理页面, 读取文件内容, 添加物理页面到虚拟页面的映射
 * @param virtualAddr 虚拟地址
 * @param cause 造成页面故障的原因
 * @return 0 表示成功, -1 表示失败
 */
int mmapHandler(int virtualAddr, int cause) {
	// 获取当前进程信息
	struct proc* p = myproc();
	int i;
	// 根据虚拟地址查找对应的 vma
	for (i = 0; i < NVMA; i++) {
		if (p->vma[i].used == 1 && p->vma[i].addr <= virtualAddr
			&& virtualAddr <= p->vma[i].addr + p->vma[i].length - 1) {
			break;
		}// 表示查找到了 vma
	}

	if (i == NVMA) {
		return -1;
	}// 没有找到, 返回-1

	// 设置页面的 flag
	int flag = PTE_U; // 表明在用户态使用
	if (p->vma[i].prot & PROT_READ) {
		flag |= PTE_R;
	}
	if (p->vma[i].prot & PROT_WRITE) {
		flag |= PTE_W;
	}
	if (p->vma[i].prot & PROT_EXEC) {
		flag |= PTE_X;
	}

	// 获取文件内容, 读取文件内容
	struct file* file = p->vma[i].file;
	// 读导致的页面错误
	if (cause == 13 && file->readable == 0)
		return -1;
	// 写导致的页面错误
	if (cause == 15 && file->writable == 0)
		return -1;

	void* physicalAddr = kalloc();
	if (physicalAddr == 0) {
		printf("physical address alloc failed\n");
		return -1;
	}

	memset(physicalAddr, 0, PGSIZE);

	// 读取文件内容
	ilock(file->ip);
	int offset = p->vma[i].offset + PGROUNDDOWN(virtualAddr - p->vma[i].addr);
	int readBytes = readi(file->ip, 0, (uint64)physicalAddr, offset, PGSIZE);
	if (readBytes == 0) {
		// 如果什么都没有读到, 返回 -1
		iunlock(file->ip);
		kfree(physicalAddr);
		printf("read fail\n");
		return -1;
	}
	iunlock(file->ip);

	// 添加物理页面与虚拟内存页面的映射
	if (mappages(p->pagetable, PGROUNDDOWN(virtualAddr), PGSIZE,
		(uint64)physicalAddr, flag) != 0) {
		kfree(physicalAddr);
		printf("mappages failed\n");
		return -1;
	}
	return 0;
}