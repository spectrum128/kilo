/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig{
    int screenrows;
    int screencols;

    // global variable for storing terminal attributes
    struct termios orig_termios;
};

struct editorConfig E;

/*** terminal ***/

void die(const char *s){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}


void disableRawMode(){
    // STDIN_FILENO comes from unistd.h
    // tcsetattr, TCSAFLUSH is from termios
    // TCSAFLUSH discards any unread input before applying changes to the terminal
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == 1)
        die("tcsetattr");
}


void enableRawMode(){
    // store the original terminal attributes in orig_termios
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    // atexit comes from stdlib
    atexit(disableRawMode);

    // make a copy of the original settings
    struct termios raw = E.orig_termios;

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
    // ICRNL is also an Input flag Input Carriage Return New Line
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


char editorReadKey(){

    int nread;
    char c;

    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("editor read key");
    }

    return c;
}

int getCursorPosition(int *rows, int *cols){

    char buf[32];
    unsigned int i = 0;


    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while(i < sizeof(buf) -1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if(buf[0] != '\x1b' || buf[1] != '[') return -1;
    if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;    
}

int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** output ***/

void editorDrawRows(){
    int y;
    for(y = 0; y < E.screenrows; y++){
        write(STDOUT_FILENO,"~", 1);

        if(y < E.screenrows -1){
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }
}



void editorRefreshScreen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}


/*** input ***/


void editorProcessKeypress(){
    char c = editorReadKey();

    switch(c){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}


/*** init ***/

void initEditor(){
    if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){

    enableRawMode();
    initEditor();

    // read and STDIN_FILENO come from unistd.h
    //while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
