/* Memory regions */
MEMORY {
	ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 128K
	RAM (rwx) : ORIGIN = 0x20000000, LENGTH =  16K
}

/* Library configurations */
GROUP(
	armv7e-m/libgcc.a
	armv7e-m/libc.a
	armv7e-m/libm.a
	armv7e-m/libnosys.a
)

ENTRY(Reset_Handler)

SECTIONS {
	.text : {
		KEEP(*(.isr_vector))
		*(.text*)

		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)

		*(.rodata*)

		KEEP(*(.eh_frame*))
	} > ROM

	.ARM.extab : {
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > ROM

	.ARM.exidx : {
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > ROM

	Data_Start_ROM = .;
		
	.data : AT (Data_Start_ROM) {
		Data_Start_RAM = .;
		*(vtable)
		*(.data*)

		. = ALIGN(4);
		KEEP(*(.preinit_array))

		. = ALIGN(4);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))


		. = ALIGN(4);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))

		*(.verify)

		. = ALIGN(4);
		Data_End_RAM = .;
	} > RAM

	Data_Size = Data_End_RAM - Data_Start_RAM;

	.bss : {
		BSS_Start = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		BSS_End = .;
	} > RAM

	BSS_Size = . - BSS_Start;
	
	.heap : {
		end = .;
		*(.heap*)
		Heap_Limit = .;
	} > RAM

	.stack : {
		*(.stack)
	} > RAM

	/* Stack top to end of RAM */
	Stack_Top = ORIGIN(RAM) + LENGTH(RAM);
	Stack_Limit = Stack_Top - SIZEOF(.stack);

	/* Check for RAM overflow */
	ASSERT(Stack_Limit >= Heap_Limit, "RAM overflowed with stack")
}
