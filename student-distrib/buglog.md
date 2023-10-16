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