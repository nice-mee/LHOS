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
> 	This is a compiling error. Multiple files under the "devices" directory met the error "multiple definition of 'stdin_operation _table' and 'stdout_operation _table.'"
> **Analysis** 
> 	This error occurred because the two tables are defined and initialized in exception_handler.h, which is included in every file under "devices". So these tables will be defined and initialized multiple times when compiling, causing the error. 
> **Solution** 
>   I moved the definition of the two tables into exception_handler.c and use extern in its head file.

/***
 *                                         ,s555SB@@&                          
 *                                      :9H####@@@@@Xi                        
 *                                     1@@@@@@@@@@@@@@8                       
 *                                   ,8@@@@@@@@@B@@@@@@8                      
 *                                  :B@@@@X3hi8Bs;B@@@@@Ah,                   
 *             ,8i                  r@@@B:     1S ,M@@@@@@#8;                 
 *            1AB35.i:               X@@8 .   SGhr ,A@@@@@@@@S                
 *            1@h31MX8                18Hhh3i .i3r ,A@@@@@@@@@5               
 *            ;@&i,58r5                 rGSS:     :B@@@@@@@@@@A               
 *             1#i  . 9i                 hX.  .: .5@@@@@@@@@@@1               
 *              sG1,  ,G53s.              9#Xi;hS5 3B@@@@@@@B1                
 *               .h8h.,A@@@MXSs,           #@H1:    3ssSSX@1                  
 *               s ,@@@@@@@@@@@@Xhi,       r#@@X1s9M8    .GA981               
 *               ,. rS8H#@@@@@@@@@@#HG51;.  .h31i;9@r    .8@@@@BS;i;          
 *                .19AXXXAB@@@@@@@@@@@@@@#MHXG893hrX#XGGXM@@@@@@@@@@MS        
 *                s@@MM@@@hsX#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@&,      
 *              :GB@#3G@@Brs ,1GM@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@B,     
 *            .hM@@@#@@#MX 51  r;iSGAM@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@8     
 *          :3B@@@@@@@@@@@&9@h :Gs   .;sSXH@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@:    
 *      s&HA#@@@@@@@@@@@@@@M89A;.8S.       ,r3@@@@@@@@@@@@@@@@@@@@@@@@@@@r    
 *   ,13B@@@@@@@@@@@@@@@@@@@5 5B3 ;.         ;@@@@@@@@@@@@@@@@@@@@@@@@@@@i    
 *  5#@@#&@@@@@@@@@@@@@@@@@@9  .39:          ;@@@@@@@@@@@@@@@@@@@@@@@@@@@;    
 *  9@@@X:MM@@@@@@@@@@@@@@@#;    ;31.         H@@@@@@@@@@@@@@@@@@@@@@@@@@:    
 *   SH#@B9.rM@@@@@@@@@@@@@B       :.         3@@@@@@@@@@@@@@@@@@@@@@@@@@5    
 *     ,:.   9@@@@@@@@@@@#HB5                 .M@@@@@@@@@@@@@@@@@@@@@@@@@B    
 *           ,ssirhSM@&1;i19911i,.             s@@@@@@@@@@@@@@@@@@@@@@@@@@S   
 *              ,,,rHAri1h1rh&@#353Sh:          8@@@@@@@@@@@@@@@@@@@@@@@@@#:  
 *            .A3hH@#5S553&@@#h   i:i9S          #@@@@@@@@@@@@@@@@@@@@@@@@@A.
 *
 *
 *    
 */
