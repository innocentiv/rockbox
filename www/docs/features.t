#define _PAGE_ Firmware Feature Comparison Chart
#include "head.t"

#define NAME    <tr><td class=feature>
#define ENAME   </td>
#define TD <td class=fneutral>
#define ETD </td>
#define EFEAT </tr>

#define YES  <td class=fgood>Yes ETD
#define NO   <td class=fbad>No ETD
#define BADYES  <td class=fbad>Yes ETD
#define GOODNO  <td class=fgood>No ETD
#define UNKNOWN TD ? ETD

<table class=rockbox>

<tr class=header><th>Feature</th><th>Rockbox</th><th>Archos</th></tr>

NAME ID3v1 and ID3v2 support ENAME
YES
UNKNOWN
EFEAT

NAME Background noise ENAME
GOODNO
BADYES
EFEAT

NAME Mid-track resume ENAME
YES
NO
EFEAT

NAME Mid-playlist resume ENAME
YES
NO
EFEAT

NAME Resumed playlist order ENAME
YES
NO
EFEAT

NAME Battery lifetime ENAME
TD Longer ETD
TD  Long ETD
EFEAT

NAME Charges batteries 100% full ENAME
NO
YES
EFEAT

NAME Customizable font (Recorder) ENAME
YES
NO
EFEAT

NAME Customizable screen info when playing songs ENAME
YES
NO
EFEAT

NAME USB attach/detach without reboot ENAME
YES
NO
EFEAT

NAME Can load another firmware without rebooting ENAME
YES
NO
EFEAT

NAME Fast playlist loading ENAME
YES
NO
EFEAT

NAME Max number of songs in a playlist ENAME
TD 10 000 ETD
TD 999 ETD
EFEAT

NAME Open source/development process ENAME
YES
NO
EFEAT

NAME Corrects reported bugs ENAME
YES
NO
EFEAT

NAME Text File Reader ENAME
YES
NO
EFEAT

NAME Games ENAME
TD Tetris, Sokoban, Snake ETD
NO
EFEAT

NAME File Delete & Rename ENAME
NO
YES
EFEAT

NAME Playlist Building ENAME
NO
YES
EFEAT

NAME Recording (Recorder) ENAME
TD In development ETD
YES
EFEAT

</table>

<p>
 Wrong facts? Mail rockbox@cool.haxx.se now!

#include "foot.t"
