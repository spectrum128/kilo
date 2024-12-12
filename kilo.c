/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>


/*** data ***/
// global variable for storing terminal attributes
struct termios orig_termios;


/*** terminal ***/

void die(const char *s){
    perror(s);
    exit(1);
}


void disableRawMode(){
    // STDIN_FILENO comes from unistd.h
    // tcsetattr, TCSAFLUSH is from termios
    // TCSAFLUSH discards any unread input before applying changes to the terminal
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == 1)
        die("tcsetattr");
}


void enableRawMode(){
    // store the original terminal attributes in orig_termios
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    
    // atexit comes from stdlib
    atexit(disableRawMode);

    // make a copy of the original settings
    struct termios raw = orig_termios;

    // Turn off echoing and canonical mode:
    // c_lflag is a field for 'local flags' - 'a dumping ground for other state' / misc flags
    // c_iflag is for input flags, c_oflag is for output flags, c_cflag for control flags
    // ECHO is a bit flag 0000----1000 in binary
    // use the bitwise NOT operator (~) to flip the bits
    // then bitwise AND (&) with the flags field to set the 4th bit to 0
    // ICANON comes from termios also
    // by default ctrl + C sends SIGINT signal which terminates the running process
    // ctrl + Z sends SIGTSTP signal to suspend process
    // we can turn these signals off with ISIG
    // ISIG comes from termios.h
    // disable ctrl + S : stop data transmission to the terminal
    // disable ctrl + Q : resume transmission
    // IXON comes from termios.h. The 'I' stands for input flag and XON comes
    // from the name of the 2 control charcters that ctrl + S and ctrl + Q produce (XOFF and XON)
    // ctrl + S will be read as a 19 byte and ctrl + Q as 17 byte
    // ctrl + V will wait for you to type another character and then send that character literally.
    // we can turn this off with IEXTEN flag which comes from termios also.
    // ctrl + V will now bre read as a 22 byte and ctrl + O as a 15 byte.
    // we can turn off ctrl + M with ICRNL from termios.
    // you would expect ctrl + M to output 13 but the terminal translates any carriage returns (13, '\r')
    // input by the user to new lines (10, '\n')
    // ICRNL is also and Input flag Input Carriage Return New Line
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // we will turn off all output processing with the OPOST flag, this will stop the \n to \r\n conversion (newline to carriage return)
    // OPOST again comes from termios and is an output flag
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsettattr");

}


/*** init ***/

int main(){

    enableRawMode();


    // read and STDIN_FILENO come from unistd.h
    //while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
    while(1){
        
        char c = '\0';
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die ("read");
        // iscntrl comes from ctype.h and printf comes from stdio.h
        // iscntrl tests if the character is a control character - non printable characters
        // ASCII codes 0-31 are control characters and 127 also
        // codes 32 to 126 are all printable
        if(iscntrl(c)){

            // printf can print multiple representations of a byte
            // %d formats the byte as a decimal number - i.e. its ASCII code
            // %c writes the byte out directly as a character
            // all escape sequences start with a single 27 byte, backspace is byte 127
            // enter is byte 10, which is a newline character (also '\n')
            // ctrl + A is 1, ctrl + B is 2, ctrl + C quits lol - ctrl key + A-Z map to codes 1-26
            // ctrl + S stops the program sending output
            // ctrl + Q will resume sending output
            // ctrl + Z or sometimes Y will suspend our program in the background
            // run command fg to bring it back to the foreground
            // as we turned OPOST off we will add carriage returns our printf statements to make sure
            // that the cursor moves back to the start of the line
            printf("%d\r\n", c);
        }
        else{
            printf("%d ('%c')\r\n", c, c);
        }
        if (c == 'q') break;

    }

    return 0;
}
