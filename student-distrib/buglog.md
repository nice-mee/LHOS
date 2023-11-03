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
> **PS**
>   This bug seems to be related to other issues. In theory `RTC_init()` should be already called with interrupt disabled. In the current version of code we removed `cli()` and `sti()` and it works fine. We are not sure why this happens. But anyway it's fixed somehow.

> [!Bug 4] 
> **Description** 
> 	After I add `cli_save_flags(flags)` and `restore_flags(flags)` in `vt_putc()`, the keyboard interrupt is not working.
> **Analysis** 
> 	It took me quite a while to debug through the entire code. I followed the control flow step by step inside `printf()` and in the end I found because I did early return for `\n`, `\r` and `\b`, the interrupt flag is not restored for these two cases.
> **Solution** 
> 	The solution is to add `restore_flags(flags)` before `return` in `vt_putc()`.

>[!Bug 5]
> **Description**
> When testing the RTC, we write a test case that will print a "1" when an interrupt occurs to test the frequency. We first ran this test case on the debug machine and it works right. But when we ran it on the nondebug machine, the frequency was obviously lower than expected. 
> **Analysis**
> We first excluded problems in codes, because if there was any, then the program wouldn't work right in the debug machine. Then > we thought it was because `printf` may works slower on the nondebug machine, so we replaced it with `puts` but the problem was > still there. Finally we found the problem lay in the `target` of the nondebug machine. We added "-d int" to  this machine, and this made the machine record every interrupt, which made qemu simulation very slow.
> **Solution**
> We removed "-d int" from the nondebug machine and the test case can be ran correctly.

>[!Bug 6]
> **Description** 
> 	The output of test of read file always print content out of the target file. And there is a wired sign after the file name of verylargetextwithverylongname.txt
> **Analysis** 
> 	Initially, I thought it was because some sort of overflow. But after I take a deep look at the file and the definition of printf. I find that printf will keep printing until it encounts '\0'. So it need to be changed to vt_write();
> **Solution** 
> 	The solution is to change the function printf() to vt_write() in `tests.c`.

>[!Bug 7]
> **Description** 
> 	The output of syscall_edge_test is always FAIL when it handle "__syscall_read(-1, buf1, 100) != -1".
> **Analysis** 
> 	Initially, I check my code carefully and make sure that every open operation for each specific type checks the validness of input as I thought the fd is directly passed into the open operation. However, I suddenly find that it needs to find corresponding file descriptor first in fd_array before calling the open operation.
> **Solution** 
> 	The solution is to check the validness of fd before calling open operation, while the validness for other input parameter can be left to the open operation.
