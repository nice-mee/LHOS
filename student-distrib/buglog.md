> [!Bug 1] 
> **Description** 
> 	All interrupts are non-functional. 
> **Analysis** 
> 	Utilizing gdb, we discovered that the interrupt enable flag was cleared. 
> **Solution** 
> 	Implemented `CLI()` to activate the interrupt enable flag.

> [!Bug 2] 
> **Description** 
> 	Despite setting the interrupt enable flag, the interrupts remained non-functional. 
> **Analysis** 
> 	Post-testing with gdb, it was identified that the initialization of the PIC was incorrect. Initially, all interrupts on the PIC were masked, followed by its initialization, as advised in the PDF. However, this method kept all interrupts unmarked post-initialization. 
> **Solution** 
> 	We reapplied the mask to all interrupts post-initialization, which restored its proper functionality.

> [!Bug 3] 
> **Description** 
> 	During the RTC testing, the characters displayed on the screen were alternating rapidly between different sets. 
> **Analysis** 
> 	Initially, it was presumed to be due to a high base frequency, leading us to test with a significantly reduced frequency. Nonetheless, the alternation frequency on the screen did not diminish, indicating no change. This led to the realization that there was a complication within the RTC's initialization. Our conclusive analysis was that not disabling interrupts during the initialization phase resulted in the frequency being incorrectly established. 
> **Solution** 
> 	We employed `STL()` and `CLI()` at the beginning and conclusion of the `RTC_init()` procedure, successfully normalizing the frequency.

> [!Bug 4] 
> **Description** 
> 	After I add `cli_save_flags(flags)` and `restore_flags(flags)` in `vt_putc()`, the keyboard interrupt is not working.
> **Analysis** 
> 	It took me quite a while to debug through the entire code. I followed the control flow step by step inside `printf()` and in the end I found because I did early return for `\n`, `\r` and `\b`, the interrupt flag is not restored for these two cases.
> **Solution** 
> 	The solution is to add `restore_flags(flags)` before `return` in `vt_putc()`.
