// *****************************
// Filesystem debug
// *****************************
#define PRINT_FSAOPEN       0 // Print FSAOpenFile and FSAOpenDirectory calls

#define PRINT_FSAREADWRITE  0 // More intense FSA prints
#define PRINT_FSASEEKFILE   0
// pids out of these bounds won't have their FSA r/w or seekfile printed
// print all = 0, 21. print ios = 0,13. print cos = 14,21
#define FSAPRINT_MIN_PID    0
#define FSAPRINT_MAX_PID    13

#define PRINT_MLC_READ      0 // Print MLC read calls
#define PRINT_MLC_WRITE     0 // Print MLC write calls
#define PRINT_USB_RW        0 // Print USB read/write calls


// PAD
#define PRINT_PAD 0